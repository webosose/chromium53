// Copyright (c) 2016-2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "media_info_loader.h"

#include "base/callback_helpers.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "third_party/WebKit/public/platform/WebURLLoader.h"
#include "third_party/WebKit/public/platform/WebURLRequest.h"
#include "third_party/WebKit/public/platform/WebURLResponse.h"
#include "third_party/WebKit/public/web/WebLocalFrame.h"

namespace media {

static const int kHttpOK = 200;
static const int kHttpPartialContentOK = 206;

MediaInfoLoader::MediaInfoLoader(const GURL& url, const ReadyCB& ready_cb)
    : url_(url), ready_cb_(ready_cb) {}

MediaInfoLoader::~MediaInfoLoader() {}

void MediaInfoLoader::Start(blink::WebLocalFrame* frame) {
  blink::WebURLRequest request(url_);
  request.setRequestContext(blink::WebURLRequest::RequestContextVideo);
  frame->setReferrerForRequest(request, blink::WebURL());

  // To avoid downloading the data use two byte range
  request.addHTTPHeaderField("Range", "bytes=0-1");

  blink::WebURLLoaderOptions options;
  options.allowCredentials = true;
  options.crossOriginRequestPolicy =
      blink::WebURLLoaderOptions::CrossOriginRequestPolicyAllow;

  std::unique_ptr<blink::WebURLLoader> loader(
      frame->createAssociatedURLLoader(options));
  loader->loadAsynchronously(request, this);
  active_loader_.reset(new media::ActiveLoader(std::move(loader)));
}

/////////////////////////////////////////////////////////////////////////////
// blink::WebURLLoaderClient implementation.
void MediaInfoLoader::willFollowRedirect(
    blink::WebURLLoader* loader,
    blink::WebURLRequest& newRequest,
    const blink::WebURLResponse& redirectResponse) {
  if (ready_cb_.is_null()) {
    newRequest.setURL(blink::WebURL());
    return;
  }

  url_ = newRequest.url();
}

void MediaInfoLoader::didReceiveResponse(
    blink::WebURLLoader* loader,
    const blink::WebURLResponse& response) {
  if (!url_.SchemeIs(url::kHttpScheme) && !url_.SchemeIs(url::kHttpsScheme)) {
    DidBecomeReady(true);
    return;
  }
  if (response.httpStatusCode() == kHttpOK ||
      response.httpStatusCode() == kHttpPartialContentOK) {
    DidBecomeReady(true);
    return;
  }
  DidBecomeReady(false);
}

void MediaInfoLoader::didFinishLoading(blink::WebURLLoader* loader,
                                       double finishTime,
                                       int64_t total_encoded_data_length) {
  DidBecomeReady(true);
}

void MediaInfoLoader::didFail(blink::WebURLLoader* loader,
                              const blink::WebURLError& error) {
  DidBecomeReady(false);
}

void MediaInfoLoader::DidBecomeReady(bool ok) {
  active_loader_.reset();
  if (!ready_cb_.is_null())
    base::ResetAndReturn(&ready_cb_).Run(ok, url_);
}

}  // namespace media
