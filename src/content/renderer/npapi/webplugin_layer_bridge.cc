// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/renderer/npapi/webplugin_layer_bridge.h"

#include "cc/resources/texture_mailbox.h"
#include "content/child/npapi/webplugin.h"
#include "content/common/gpu/client/context_provider_command_buffer.h"
#include "content/renderer/render_thread_impl.h"
#include "gpu/command_buffer/client/gles2_implementation.h"
#include "gpu/command_buffer/client/gpu_memory_buffer_manager.h"
#include "gpu/command_buffer/common/gpu_memory_buffer_support.h"
#include "gpu/command_buffer/service/image_factory.h"
#include "gpu/ipc/client/gpu_channel_host.h"
#include "gpu/GLES2/gl2extchromium.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace content {

WebPluginLayerBridge::MailboxInfo::MailboxInfo()
    : texture_id(0), image_id(0), exported(false), marked_for_deletion(false) {
  sync_token = gpu::SyncToken();
}

WebPluginLayerBridge::MailboxInfo::~MailboxInfo() {}

// static
scoped_refptr<WebPluginLayerBridge> WebPluginLayerBridge::Create(
    WebPlugin* client) {
  scoped_refptr<WebPluginLayerBridge> layer_bridge =
      new WebPluginLayerBridge(client);
  if (!layer_bridge->Initialize())
    return scoped_refptr<WebPluginLayerBridge>();
  return layer_bridge;
}

WebPluginLayerBridge::WebPluginLayerBridge(WebPlugin* client)
    : client_(client),
      context_(NULL),
      swap_image_id_(0),
      destruction_in_progress_(false),
      weak_factory_(this) {}

WebPluginLayerBridge::~WebPluginLayerBridge() {}

bool WebPluginLayerBridge::Initialize() {
  context_provider_ =
      RenderThreadImpl::current()->SharedMainThreadPluginContextProvider();
  if (!context_provider_.get())
    return false;

  context_ = context_provider_->ContextGL();
  if (!context_)
    return false;

  return true;
}

void WebPluginLayerBridge::BeginDestruction() {
  DCHECK(!destruction_in_progress_);
  destruction_in_progress_ = true;

  client_ = NULL;

  swap_image_id_ = 0;

  for (BufferMailboxMap::iterator it = buffer_mailboxes_.begin();
       it != buffer_mailboxes_.end(); ++it) {
    it->second->marked_for_deletion = true;
    if (!it->second->exported)
      DeleteMailbox(it->first);
  }
}

gfx::GpuMemoryBufferHandle WebPluginLayerBridge::ShareGpuMemoryBuffer(
    ClientBuffer buffer) {
  gpu::GpuChannelHost* gpu_channel_host =
      RenderThreadImpl::current()->GetGpuChannel();
  if (!gpu_channel_host)
    return gfx::GpuMemoryBufferHandle();

  gpu::GpuMemoryBufferManager* gpu_memory_buffer_manager =
      RenderThreadImpl::current()->GetGpuMemoryBufferManager();
  if (!gpu_memory_buffer_manager)
    return gfx::GpuMemoryBufferHandle();

  gfx::GpuMemoryBuffer* gpu_memory_buffer =
      gpu_memory_buffer_manager->GpuMemoryBufferFromClientBuffer(buffer);

  DCHECK(gpu_memory_buffer);
  // This handle is owned by the GPU process and must be passed to it or it
  // will leak. In otherwords, do not early out on error between here and the
  // sending of the CreateImage IPC below.
  bool requires_sync_point = false;
  gfx::GpuMemoryBufferHandle handle =
      gpu_channel_host->ShareGpuMemoryBufferToGpuProcess(
          gpu_memory_buffer->GetHandle(), &requires_sync_point);
  return handle;
}

void WebPluginLayerBridge::AllocateBuffer(uint32_t width,
                                          uint32_t height,
                                          uint32_t internalformat,
                                          uint32_t usage,
                                          uint32_t format,
                                          int32_t* id,
                                          gfx::GpuMemoryBufferHandle* handle) {
  std::unique_ptr<gfx::GpuMemoryBuffer> buffer(
      RenderThreadImpl::current()
          ->GetGpuMemoryBufferManager()
          ->AllocateGpuMemoryBuffer(
              gfx::Size(width, height), gfx::BufferFormat(format),
              gfx::BufferUsage(usage), gpu::kNullSurfaceHandle));
  if (!buffer)
    return;

  unsigned int image_id = context_->CreateImageCHROMIUM(
      buffer->AsClientBuffer(), width, height, internalformat);
  if (!image_id)
    return;

  if (!CreateMailbox(image_id)) {
    context_->DestroyImageCHROMIUM(image_id);
    return;
  }
  gfx::GpuMemoryBufferHandle buffer_handle =
      ShareGpuMemoryBuffer(buffer->AsClientBuffer());
  if (buffer_handle.is_null()) {
    DeleteMailbox(image_id);
    return;
  }

  *id = image_id;
  *handle = buffer_handle;
}

void WebPluginLayerBridge::ReleaseBuffer(int32_t id) {
  if (swap_image_id_ == id)
    swap_image_id_ = 0;

  BufferMailboxMap::iterator it = buffer_mailboxes_.find(id);
  if (it != buffer_mailboxes_.end()) {
    it->second->marked_for_deletion = true;
    if (!it->second->exported)
      DeleteMailbox(id);
  }
}

void WebPluginLayerBridge::SwapBuffer(int32_t id) {
  DCHECK_EQ(swap_image_id_, 0);
  swap_image_id_ = id;
}

bool WebPluginLayerBridge::PrepareTextureMailbox(
    cc::TextureMailbox* out_mailbox,
    std::unique_ptr<cc::SingleReleaseCallback>* release_callback,
    bool use_shared_memory) {
  if (destruction_in_progress_)
    return false;

  if (use_shared_memory)
    return false;

  // Nothing to export for drawing.
  if (!swap_image_id_)
    return false;

  DCHECK(buffer_mailboxes_.find(swap_image_id_) != buffer_mailboxes_.end());
  scoped_refptr<MailboxInfo> mailbox =
      buffer_mailboxes_.find(swap_image_id_)->second;
  if (!mailbox.get())
    return false;

  // Bind image for sampling
  context_->BindTexture(GL_TEXTURE_2D, mailbox->texture_id);
  context_->BindTexImage2DCHROMIUM(GL_TEXTURE_2D, mailbox->image_id);

  context_->GenMailboxCHROMIUM(mailbox->mailbox.name);
  context_->ProduceTextureDirectCHROMIUM(mailbox->texture_id, GL_TEXTURE_2D,
                                         mailbox->mailbox.name);
  const GLuint64 fence_sync = context_->InsertFenceSyncCHROMIUM();
  context_->Flush();
  mailbox->exported = true;
  gpu::SyncToken sync_token;
  context_->GenSyncTokenCHROMIUM(fence_sync,
                                 sync_token.GetData());  // may be not need
  mailbox->sync_token = sync_token;

  *out_mailbox =
      cc::TextureMailbox(mailbox->mailbox, sync_token, GL_TEXTURE_2D);

  if (out_mailbox->IsValid()) {
    *release_callback = cc::SingleReleaseCallback::Create(
        base::Bind(&WebPluginLayerBridge::MailboxReleased,
                   make_scoped_refptr(this), mailbox->image_id));
  }
  if (client_)
    client_->AcceleratedPluginDidSwapBuffer(swap_image_id_);

  swap_image_id_ = 0;

  return true;
}

void WebPluginLayerBridge::MailboxReleased(int32_t image_id,
                                           const gpu::SyncToken& sync_token,
                                           bool lost_resource) {
  BufferMailboxMap::iterator it = buffer_mailboxes_.find(image_id);
  if (it == buffer_mailboxes_.end())
    return;

  it->second->sync_token = sync_token;
  it->second->exported = false;

  if (destruction_in_progress_ || lost_resource) {
    DeleteMailbox(image_id);
    return;
  }

  if (it->second->marked_for_deletion) {
    DeleteMailbox(image_id);
    return;
  }

  if (client_)
    client_->AcceleratedPluginDidSwapBufferComplete(image_id);
}

bool WebPluginLayerBridge::CreateMailbox(int32_t image_id) {
  GLuint texture;
  context_->GenTextures(1, &texture);
  if (!texture)
    return false;

  context_->ActiveTexture(GL_TEXTURE0);
  context_->BindTexture(GL_TEXTURE_2D, texture);
  context_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  context_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  context_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  context_->TexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

  context_->BindTexImage2DCHROMIUM(GL_TEXTURE_2D, image_id);
  context_->Flush();

  scoped_refptr<MailboxInfo> mailbox = new MailboxInfo();

  context_->GenMailboxCHROMIUM(mailbox->mailbox.name);

  mailbox->texture_id = texture;
  mailbox->image_id = image_id;
  mailbox->parent_layer_bridge = this;

  DCHECK(buffer_mailboxes_.find(image_id) == buffer_mailboxes_.end());
  std::pair<BufferMailboxMap::iterator, bool> result =
      buffer_mailboxes_.insert(std::make_pair(image_id, mailbox));
  return result.second;
}

void WebPluginLayerBridge::DeleteMailbox(int32_t image_id) {
  BufferMailboxMap::iterator it = buffer_mailboxes_.find(image_id);
  if (it == buffer_mailboxes_.end())
    return;

  // Ensure not to call the parent layer bridge's destructor
  // until mailbox resource releasing is completed.
  scoped_refptr<MailboxInfo> mailbox = it->second;
  DCHECK(!mailbox->exported);

  context_->DestroyImageCHROMIUM(mailbox->image_id);
  if (mailbox->texture_id) {
    context_->DeleteTextures(1, &mailbox->texture_id);
    mailbox->texture_id = 0;
  }

  context_->Flush();
  buffer_mailboxes_.erase(image_id);
}

}  // namespace content
