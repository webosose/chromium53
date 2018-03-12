// Copyright (c) 2017 LG Electronics, Inc.

#include "net/base/webos/network_change_notifier_webos.h"

namespace webos {

NetworkChangeNotifierWebos::NetworkChangeNotifierWebos()
    : net::NetworkChangeNotifierLinux(std::unordered_set<std::string>()),
      network_connected_(true) {}

NetworkChangeNotifierWebos::~NetworkChangeNotifierWebos() {}

net::NetworkChangeNotifier::ConnectionType
NetworkChangeNotifierWebos::GetCurrentConnectionType() const {
  if (network_connected_)
    return net::NetworkChangeNotifierLinux::GetCurrentConnectionType();
  return net::NetworkChangeNotifier::CONNECTION_NONE;
}

void NetworkChangeNotifierWebos::OnNetworkStateChanged(bool is_connected) {
  bool was_connected = network_connected_;
  network_connected_ = is_connected;

  double max_bandwidth;
  ConnectionType connection_type;
  if (is_connected) {
    GetCurrentMaxBandwidthAndConnectionType(&max_bandwidth, &connection_type);
  } else {
    // In DNS failure webOS gives is_connected is false. But we get connected
    // as true from NetworkChangeNotifierLinux. So set it to CONNECTION_NONE
    max_bandwidth = 0.0;
    connection_type = net::NetworkChangeNotifier::CONNECTION_NONE;
  }

  if (was_connected != is_connected) {
    net::NetworkChangeNotifier::NotifyObserversOfMaxBandwidthChange(
        max_bandwidth, connection_type);
  }
}

}  // namespace webos
