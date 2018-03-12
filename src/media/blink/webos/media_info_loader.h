// Copyright (c) 2016-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

#ifndef MEDIA_BLINK_WEBOS_MEDIAINFOLOADER_H_
#define MEDIA_BLINK_WEBOS_MEDIAINFOLOADER_H_

#include "base/callback.h"
#include "media/blink/active_loader.h"
#include "third_party/WebKit/public/platform/WebURLLoaderClient.h"
#include "url/gurl.h"

namespace blink {
class WebURLError;
class WebLocalFrame;
class WebURLLoader;
class WebURLResponse;
}

namespace media {

// This class is used to check if URL passes WebOS whitelisting
// filter implemented by WebOS webview. Similar to Android version
// but simplified approach.
class MediaInfoLoader : private blink::WebURLLoaderClient {
 public:
  typedef base::Callback<void(bool, const GURL&)> ReadyCB;

  MediaInfoLoader(const GURL& url, const ReadyCB& ready_cb);
  ~MediaInfoLoader() override;

  void Start(blink::WebLocalFrame* frame);

 private:
  // From blink::WebURLLoaderClient
  void willFollowRedirect(
      blink::WebURLLoader* loader,
      blink::WebURLRequest& newRequest,
      const blink::WebURLResponse& redirectResponse) override;
  void didReceiveResponse(blink::WebURLLoader* loader,
                          const blink::WebURLResponse& response) override;
  void didFinishLoading(blink::WebURLLoader* loader,
                        double finishTime,
                        int64_t total_encoded_data_length) override;
  void didFail(blink::WebURLLoader* loader, const blink::WebURLError&) override;

  void DidBecomeReady(bool ok);

  // Keeps track of an active WebURLLoader and associated state.
  std::unique_ptr<media::ActiveLoader> active_loader_;

  GURL url_;
  ReadyCB ready_cb_;
};

}  // namespace media

#endif  // MEDIA_BLINK_WEBOS_MEDIAINFOLOADER_H_
