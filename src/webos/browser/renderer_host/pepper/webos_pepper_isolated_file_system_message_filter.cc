// Copyright (c) 2016-2017 LG Electronics, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos_pepper_isolated_file_system_message_filter.h"

#include "content/public/browser/browser_ppapi_host.h"
#include "content/public/browser/browser_thread.h"
#include "content/public/browser/child_process_security_policy.h"
#include "content/public/browser/render_view_host.h"

#include "ppapi/c/pp_errors.h"
#include "ppapi/host/dispatch_host_message.h"
#include "ppapi/host/host_message_context.h"
#include "ppapi/host/ppapi_host.h"
#include "ppapi/proxy/ppapi_messages.h"
#include "ppapi/shared_impl/file_system_util.h"
#include "storage/browser/fileapi/isolated_context.h"

namespace webos {

// static
WebOSPepperIsolatedFileSystemMessageFilter*
WebOSPepperIsolatedFileSystemMessageFilter::Create(
    PP_Instance instance,
    content::BrowserPpapiHost* host) {
  int render_process_id;
  int unused_render_frame_id;
  if (!host->GetRenderFrameIDsForInstance(instance, &render_process_id,
                                          &unused_render_frame_id)) {
    return NULL;
  }
  return new WebOSPepperIsolatedFileSystemMessageFilter(render_process_id,
                                                        host->GetPpapiHost());
}

WebOSPepperIsolatedFileSystemMessageFilter::
    WebOSPepperIsolatedFileSystemMessageFilter(
        int render_process_id,
        ppapi::host::PpapiHost* ppapi_host)
    : render_process_id_(render_process_id), ppapi_host_(ppapi_host) {}

WebOSPepperIsolatedFileSystemMessageFilter::
    ~WebOSPepperIsolatedFileSystemMessageFilter() {}

scoped_refptr<base::TaskRunner>
WebOSPepperIsolatedFileSystemMessageFilter::OverrideTaskRunnerForMessage(
    const IPC::Message& msg) {
  return content::BrowserThread::GetMessageLoopProxyForThread(
      content::BrowserThread::UI);
}

int32_t WebOSPepperIsolatedFileSystemMessageFilter::OnResourceMessageReceived(
    const IPC::Message& msg,
    ppapi::host::HostMessageContext* context) {
  PPAPI_BEGIN_MESSAGE_MAP(WebOSPepperIsolatedFileSystemMessageFilter, msg)
    PPAPI_DISPATCH_HOST_RESOURCE_CALL(
        PpapiHostMsg_IsolatedFileSystem_BrowserOpen, OnOpenFileSystem)
  PPAPI_END_MESSAGE_MAP()
  return PP_ERROR_FAILED;
}

int32_t WebOSPepperIsolatedFileSystemMessageFilter::OnOpenFileSystem(
    ppapi::host::HostMessageContext* context,
    PP_IsolatedFileSystemType_Private type) {
  switch (type) {
    case PP_ISOLATEDFILESYSTEMTYPE_PRIVATE_PLUGINPRIVATE:
      return OpenPluginPrivateFileSystem(context);
  }

  context->reply_msg =
      PpapiPluginMsg_IsolatedFileSystem_BrowserOpenReply(std::string());
  return PP_ERROR_FAILED;
}

int32_t WebOSPepperIsolatedFileSystemMessageFilter::OpenPluginPrivateFileSystem(
    ppapi::host::HostMessageContext* context) {
  if (!ppapi_host_->permissions().HasPermission(ppapi::PERMISSION_PRIVATE))
    return PP_ERROR_NOACCESS;

  const std::string& root_name = ppapi::IsolatedFileSystemTypeToRootName(
      PP_ISOLATEDFILESYSTEMTYPE_PRIVATE_PLUGINPRIVATE);
  const std::string& fsid =
      storage::IsolatedContext::GetInstance()->RegisterFileSystemForVirtualPath(
          storage::kFileSystemTypePluginPrivate, root_name, base::FilePath());

  content::ChildProcessSecurityPolicy* policy =
      content::ChildProcessSecurityPolicy::GetInstance();
  policy->GrantCreateReadWriteFileSystem(render_process_id_, fsid);

  context->reply_msg = PpapiPluginMsg_IsolatedFileSystem_BrowserOpenReply(fsid);
  return PP_OK;
}

}  // namespace webos
