// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef NET_BASE_WEBOS_NETWORK_CHANGE_NOTIFIER_FACTORY_WEBOS_H_
#define NET_BASE_WEBOS_NETWORK_CHANGE_NOTIFIER_FACTORY_WEBOS_H_

#include "base/compiler_specific.h"
#include "base/macros.h"
#include "net/base/net_export.h"
#include "net/base/network_change_notifier_factory.h"

namespace webos {

class NetworkChangeNotifierWebos;

class NET_EXPORT_PRIVATE NetworkChangeNotifierFactoryWebos
    : public net::NetworkChangeNotifierFactory {
 public:
  NetworkChangeNotifierFactoryWebos() {}

  // net::NetworkChangeNotifierFactory overrides.
  net::NetworkChangeNotifier* CreateInstance() override;

  static NetworkChangeNotifierWebos* GetInstance();

 private:
  DISALLOW_COPY_AND_ASSIGN(NetworkChangeNotifierFactoryWebos);
};

}  // namespace webos

#endif  // NET_BASE_WEBOS_NETWORK_CHANGE_NOTIFIER_FACTORY_WEBOS_H_
