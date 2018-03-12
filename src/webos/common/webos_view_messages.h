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

#ifndef WEBOS_COMMON_WEBOS_VIEW_MESSAGES_H_
#define WEBOS_COMMON_WEBOS_VIEW_MESSAGES_H_

#include "base/memory/memory_pressure_listener.h"
#include "ipc/ipc_message_macros.h"
#include "third_party/skia/include/core/SkColor.h"
#include "third_party/WebKit/public/platform/WebPageVisibilityState.h"

#define IPC_MESSAGE_START WebOSViewMsgStart
IPC_ENUM_TRAITS(blink::WebPageVisibilityState)
IPC_ENUM_TRAITS(base::MemoryPressureListener::MemoryPressureLevel)

IPC_MESSAGE_ROUTED0(WebOSViewMsgHost_DidClearWindowObject)

IPC_MESSAGE_ROUTED1(WebOSViewMsgHost_DidFirstMeaningfulPaint,
                    double /* fmp_detected */)

IPC_MESSAGE_ROUTED0(WebOSViewMsgHost_DidNonFirstMeaningfulPaint)

IPC_MESSAGE_ROUTED1(WebOSViewMsg_SetVisibilityState,
                    blink::WebPageVisibilityState /* visibilityState */)

IPC_MESSAGE_ROUTED1(WebOSViewMsg_SetBackgroundColor, SkColor)

IPC_MESSAGE_ROUTED2(WebOSViewMsg_SetViewportSize,
                    int /* width */,
                    int /* height */)

#endif // WEBOS_COMMON_WEBOS_VIEW_MESSAGES_H_
