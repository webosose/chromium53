// Copyright (c) 2014-2018 LG Electronics, Inc.
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

// Multiply-included message file, hence no include guard

#include "base/strings/string16.h"
#include "ipc/ipc_message.h"
#include "ipc/ipc_message_macros.h"

#undef IPC_MESSAGE_EXPORT
#define IPC_MESSAGE_EXPORT CONTENT_EXPORT

#ifdef IPC_MESSAGE_START
#error IPC_MESSAGE_START
#endif

#define IPC_MESSAGE_START InjectionMsgStart

// Browser -> Renderer Messages ////////////////////////////////////////////////
// These messages are sent from the browser to the renderer. Each one has a
// corresponding reply message.
////////////////////////////////////////////////////////////////////////////////

IPC_MESSAGE_ROUTED1(InjectionMsg_LoadExtension,
                    std::string /*extension*/)

IPC_MESSAGE_ROUTED(InjectionMsg_ClearExtensions)

////////////////////////////////////////////////////////////////////////////////

// Renderer -> Browser Replies /////////////////////////////////////////////////
// These messages are sent in reply to the above messages.
////////////////////////////////////////////////////////////////////////////////

// ...
