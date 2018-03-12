// Copyright (c) 2017 LG Electronics, Inc.

#include "net/base/webos/network_change_notifier_factory_webos.h"

#include "net/base/webos/network_change_notifier_webos.h"

namespace webos {

namespace {

NetworkChangeNotifierWebos* g_network_change_notifier_webos = NULL;

}  // namespace

net::NetworkChangeNotifier*
NetworkChangeNotifierFactoryWebos::CreateInstance() {
  if (!g_network_change_notifier_webos)
    g_network_change_notifier_webos = new NetworkChangeNotifierWebos();
  return g_network_change_notifier_webos;
}

// static
NetworkChangeNotifierWebos* NetworkChangeNotifierFactoryWebos::GetInstance() {
  return g_network_change_notifier_webos;
}

}  // namespace webos
