// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_CHILD_NPAPI_WEBPLUGIN_H_
#define CONTENT_CHILD_NPAPI_WEBPLUGIN_H_

#include <stdint.h>

#include <string>
#include <vector>

#include "build/build_config.h"
#include "content/public/common/content_client.h"
#include "ui/gfx/geometry/rect.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/gl/gpu_preference.h"

// TODO(port): this typedef is obviously incorrect on non-Windows
// platforms, but now a lot of code now accidentally depends on them
// existing.  #ifdef out these declarations and fix all the users.
typedef void* HANDLE;

class GURL;
struct NPObject;

#if defined(OS_WEBOS)
namespace gfx {
struct GpuMemoryBufferHandle;
}
#endif

namespace content {

class WebPluginResourceClient;
#if defined(OS_MACOSX) || defined(OS_WEBOS)
class WebPluginAcceleratedSurface;
#endif

// The WebKit side of a plugin implementation.  It provides wrappers around
// operations that need to interact with the frame and other WebCore objects.
class WebPlugin {
 public:
  virtual ~WebPlugin() {}

  // Cancels a pending request.
  virtual void CancelResource(unsigned long id) = 0;
  virtual void Invalidate() = 0;
  virtual void InvalidateRect(const gfx::Rect& rect) = 0;

  // Returns the NPObject for the browser's window object. Does not
  // take a reference.
  virtual NPObject* GetWindowScriptNPObject() = 0;

  // Returns the DOM element that loaded the plugin. Does not take a
  // reference.
  virtual NPObject* GetPluginElement() = 0;

  // Resolves the proxies for the url, returns true on success.
  virtual bool FindProxyForUrl(const GURL& url, std::string* proxy_list) = 0;

  // Cookies
  virtual void SetCookie(const GURL& url,
                         const GURL& first_party_for_cookies,
                         const std::string& cookie) = 0;
  virtual std::string GetCookies(const GURL& url,
                                 const GURL& first_party_for_cookies) = 0;

  // Handles GetURL/GetURLNotify/PostURL/PostURLNotify requests initiated
  // by plugins.  If the plugin wants notification of the result, notify_id will
  // be non-zero.
  virtual void HandleURLRequest(const char* url,
                                const char* method,
                                const char* target,
                                const char* buf,
                                unsigned int len,
                                int notify_id,
                                bool popups_allowed,
                                bool notify_redirects) = 0;

  // Cancels document load.
  virtual void CancelDocumentLoad() = 0;

  // Initiates a HTTP range request for an existing stream.
  virtual void InitiateHTTPRangeRequest(const char* url,
                                        const char* range_info,
                                        int range_request_id) = 0;

  virtual void DidStartLoading() = 0;
  virtual void DidStopLoading() = 0;

  // Returns true iff in incognito mode.
  virtual bool IsOffTheRecord() = 0;

  // Called when the WebPluginResourceClient instance is deleted.
  virtual void ResourceClientDeleted(
      WebPluginResourceClient* resource_client) {}

  // Defers the loading of the resource identified by resource_id. This is
  // controlled by the defer parameter.
  virtual void SetDeferResourceLoading(unsigned long resource_id,
                                       bool defer) = 0;

  // Handles NPN_URLRedirectResponse calls issued by plugins in response to
  // HTTP URL redirect notifications.
  virtual void URLRedirectResponse(bool allow, int resource_id) = 0;

  virtual std::string GetUserAgent() {
    return content::GetContentClient()->GetUserAgent();
  }

#if defined(OS_MACOSX)
  // Called to inform the WebPlugin that the plugin has gained or lost focus.
  virtual void FocusChanged(bool focused) {}

  // Starts plugin IME.
  virtual void StartIme() {}

  // Returns the accelerated surface abstraction for accelerated plugins.
  virtual WebPluginAcceleratedSurface* GetAcceleratedSurface(
      gfx::GpuPreference gpu_preference) = 0;

  // Core Animation plugin support. CA plugins always render through
  // the compositor.
  virtual void AcceleratedPluginEnabledRendering() = 0;
  virtual void AcceleratedPluginAllocatedIOSurface(int32_t width,
                                                   int32_t height,
                                                   uint32_t surface_id) = 0;
  virtual void AcceleratedPluginSwappedIOSurface() = 0;
#endif

#if defined(OS_WEBOS)
  // Called by the plugin delegate to let the WebPlugin know if the plugin is
  // will be renderable.
  virtual void PluginEnabledRendering() = 0;

  // Returns the accelerated surface abstraction for accelerated plugins.
  virtual WebPluginAcceleratedSurface* GetAcceleratedSurface(
      gl::GpuPreference gpu_preference) = 0;

  // Accelerated rendering support (through the compositor).
  virtual void AcceleratedPluginEnabledRendering(bool paintless) = 0;
  virtual void AcceleratedPluginAllocateBuffer(
      uint32_t width,
      uint32_t height,
      uint32_t internalformat,
      uint32_t usage,
      uint32_t format,
      int32_t* id,
      gfx::GpuMemoryBufferHandle* handle) = 0;
  virtual void AcceleratedPluginReleaseBuffer(int32_t id) = 0;
  virtual void AcceleratedPluginSwapBuffer(int32_t id) = 0;
  virtual void AcceleratedPluginDidSwapBuffer(int32_t id) = 0;
  virtual void AcceleratedPluginDidSwapBufferComplete(int32_t id) = 0;

  // NPAPI punch hole for video support. Called by the plugin instance to know
  // that video hole will be drawn or not.
  virtual void SetUsesPunchHoleForVideo(bool uses_punch_hole_for_video) = 0;
#endif
};

}  // namespace content

#endif  // CONTENT_CHILD_NPAPI_WEBPLUGIN_H_
