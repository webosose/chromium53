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

#ifndef WEBOS_BROWSER_NET_WEBOS_NETWORK_DELEGATE_H_
#define WEBOS_BROWSER_NET_WEBOS_NETWORK_DELEGATE_H_

#include "net/base/completion_callback.h"
#include "net/base/network_delegate_impl.h"

namespace webos {

class WebOSNetworkDelegate : public net::NetworkDelegateImpl {
 public :
  WebOSNetworkDelegate(
      std::vector<std::string>& allowed_target_paths,
      std::vector<std::string>& allowed_trusted_target_paths,
      std::map<std::string, std::string> extra_websocket_headers);

  void append_extra_socket_header(const std::string& key,
                                  const std::string& value) {
    extra_websocket_headers_.insert(std::make_pair(key, value));
  }

 private :
  // net::NetworkDelegate implementation
  bool OnCanAccessFile(const net::URLRequest& request,
                       const base::FilePath& path) const override;
  int OnBeforeURLRequest(net::URLRequest* request,
                         const net::CompletionCallback& callback,
                         GURL* new_url) override;
  std::vector<std::string>& allowed_target_paths_;
  std::vector<std::string>& allowed_trusted_target_paths_;
  std::map<std::string, std::string> extra_websocket_headers_;
  bool allow_all_access_;
};

} // namespace webos

#endif // WEBOS_BROWSER_NET_WEBOS_NETWORK_DELEGATE_H_

