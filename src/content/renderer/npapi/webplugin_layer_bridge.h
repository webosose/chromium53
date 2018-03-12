// Copyright 2015 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_RENDERER_NPAPI_WEBPLUGIN_LAYER_BRIDGE_H_
#define CONTENT_RENDERER_NPAPI_WEBPLUGIN_LAYER_BRIDGE_H_

#include <map>

#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "cc/layers/texture_layer_client.h"
#include "gpu/command_buffer/client/gles2_interface.h"
#include "gpu/command_buffer/common/mailbox.h"
#include "gpu/command_buffer/common/sync_token.h"
#include "ui/gfx/gpu_memory_buffer.h"

namespace content {
class ContextProviderCommandBuffer;
class WebGraphicsContext3DCommandBufferImpl;
class WebPlugin;

// Manages a rendering target (CHROMIUM_image) for a web plugin.
// Can publish its rendering results to a TextureLayer for compositing.
class WebPluginLayerBridge : public base::RefCounted<WebPluginLayerBridge>,
                             public cc::TextureLayerClient {
 public:
  static scoped_refptr<WebPluginLayerBridge> Create(WebPlugin* client);

  // Destruction will be completed after all mailboxes are released.
  void BeginDestruction();

  void AllocateBuffer(uint32_t width,
                      uint32_t height,
                      uint32_t internalformat,
                      uint32_t usage,
                      uint32_t format,
                      int32_t* id,
                      gfx::GpuMemoryBufferHandle* handle);
  void ReleaseBuffer(int32_t id);
  void SwapBuffer(int32_t id);

  // cc::TextureLayerClient implementation:
  virtual bool PrepareTextureMailbox(
      cc::TextureMailbox* out_mailbox,
      std::unique_ptr<cc::SingleReleaseCallback>* release_callback,
      bool use_shared_memory) override;

  void MailboxReleased(int32_t image_id,
                       const gpu::SyncToken& sync_token,
                       bool lost_resource = false);

 private:
  friend class base::RefCounted<WebPluginLayerBridge>;

  WebPluginLayerBridge(WebPlugin* client);
  virtual ~WebPluginLayerBridge();

  bool Initialize();

  // Returns a GPU memory buffer handle to the buffer
  // that can be sent via IPC.
  gfx::GpuMemoryBufferHandle ShareGpuMemoryBuffer(ClientBuffer buffer);

  struct MailboxInfo : public base::RefCounted<MailboxInfo> {
    MailboxInfo();
    ~MailboxInfo();

    gpu::Mailbox mailbox;
    GLuint texture_id;
    int32_t image_id;
    gpu::SyncToken sync_token;
    bool exported;
    bool marked_for_deletion;

    // This keeps the parent layer bridge alive as long as the compositor
    // is referring to one of the produced mailboxes.
    // The parent layer bridge is cleared when the compositor
    // returns the mailbox. See MailboxReleased().
    scoped_refptr<WebPluginLayerBridge> parent_layer_bridge;
  };

  bool CreateMailbox(int32_t image_id);
  void DeleteMailbox(int32_t image_id);

  WebPlugin* client_;

  scoped_refptr<ContextProviderCommandBuffer> context_provider_;

  gpu::gles2::GLES2Interface* context_;
  // Local cache of gpu memory buffers with associated mailboxes.
  typedef std::map<int32_t, scoped_refptr<MailboxInfo>> BufferMailboxMap;
  BufferMailboxMap buffer_mailboxes_;

  int32_t swap_image_id_;
  bool destruction_in_progress_;

  base::WeakPtrFactory<WebPluginLayerBridge> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(WebPluginLayerBridge);
};

}  // namespace content

#endif  // CONTENT_RENDERER_NPAPI_WEBPLUGIN_LAYER_BRIDGE_H_
