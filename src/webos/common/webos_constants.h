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

#ifndef WEBOS_COMMON_WEBOS_CONSTANTS_H_
#define WEBOS_COMMON_WEBOS_CONSTANTS_H_

namespace webos {

enum InputPanelState { INPUT_PANEL_HIDDEN = 0, INPUT_PANEL_SHOWN = 1 };

enum NativeWindowState {
  NATIVE_WINDOW_DEFAULT = 0,
  NATIVE_WINDOW_MINIMIZED = 1,
  NATIVE_WINDOW_MAXIMIZED = 2,
  NATIVE_WINDOW_FULLSCREEN = 3
};

enum PowerOffState { POWEROFF = 0, SUSPEND = 1, ACTIVESTANDBY = 2 };

enum WebOSKeyMask {
  KEY_MASK_HOME                 = 1,
  KEY_MASK_BACK                 = 1 << 1,
  KEY_MASK_EXIT                 = 1 << 2,
  KEY_MASK_LEFT                 = 1 << 3,
  KEY_MASK_RIGHT                = 1 << 4,
  KEY_MASK_UP                   = 1 << 5,
  KEY_MASK_DOWN                 = 1 << 6,
  KEY_MASK_OK                   = 1 << 7,
  KEY_MASK_NUMERIC              = 1 << 8,
  KEY_MASK_REMOTECOLORRED       = 1 << 9,
  KEY_MASK_REMOTECOLORGREEN     = 1 << 10,
  KEY_MASK_REMOTECOLORYELLOW    = 1 << 11,
  KEY_MASK_REMOTECOLORBLUE      = 1 << 12,
  KEY_MASK_REMOTEPROGRAMMEGROUP = 1 << 13,
  KEY_MASK_REMOTEPLAYBACKGROUP  = 1 << 14,
  KEY_MASK_REMOTETELETEXTGROUP  = 1 << 15,
  KEY_MASK_LOCALLEFT            = 1 << 16,
  KEY_MASK_LOCALRIGHT           = 1 << 17,
  KEY_MASK_LOCALUP              = 1 << 18,
  KEY_MASK_LOCALDOWN            = 1 << 19,
  KEY_MASK_LOCALOK              = 1 << 20,
  KEY_MASK_REMOTEMAGNIFIERGROUP = 1 << 21,
  KEY_MASK_DEFAULT              = 0XFFFFFFF8
};

enum SpecialKeySymbolType { QT_KEY_SYMBOL = 1, NATIVE_KEY_SYMBOL };

enum XInputEventType {
  XINPUT_PRESS_AND_RELEASE = 0,
  XINPUT_PRESS,
  XINPUT_RELEASE
};

enum CustomCursorType {
  CUSTOM_CURSOR_NOT_USE = 0,
  CUSTOM_CURSOR_BLANK = 1,
  CUSTOM_CURSOR_PATH = 2
};

// SSL Certificate Error handling policy
enum SSLCertErrorPolicy {
  // cancel the requests and show error page
  SSL_CERT_ERROR_POLICY_DEFAULT = 0,
  // ignore error and continue requests
  SSL_CERT_ERROR_POLICY_IGNORE = 1,
  // stop all requests and do nothings
  SSL_CERT_ERROR_POLICY_DENY = 2,
};

// WebRTC forced peer connection drop reason
enum DropPeerConnectionReason {
  // Dropped because page got hidden
  DROP_PEER_CONNECTION_REASON_PAGE_HIDDEN = 0,
  DROP_PEER_CONNECTION_REASON_UNKNOWN,
  DROP_PEER_CONNECTION_REASON_LAST = DROP_PEER_CONNECTION_REASON_UNKNOWN,
};

}  // namespace webos

#endif  // WEBOS_COMMON_WEBOS_CONSTANTS_H_
