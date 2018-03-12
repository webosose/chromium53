// Copyright (c) 2015-2018 LG Electronics, Inc.
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

#undef TRACEPOINT_PROVIDER
#define TRACEPOINT_PROVIDER chromium_tracing

#undef TRACEPOINT_INCLUDE_FILE
#define TRACEPOINT_INCLUDE_FILE base/debug/chromium_lttng_provider.h

#if !defined(BASE_DEBUG_CHROMIUM_LTTNG_PROVIDER_H_) || defined(TRACEPOINT_HEADER_MULTI_READ)
#define BASE_DEBUG_CHROMIUM_LTTNG_PROVIDER_H_

#include <lttng/tracepoint.h>

TRACEPOINT_EVENT(chromium_tracing,
                 scope_entry,
                 TP_ARGS(const char*, text1,
                         const char*, text2),
                 TP_FIELDS(ctf_string(category, text1)
                           ctf_string(scope, text2)
                           )
                 )

TRACEPOINT_EVENT(chromium_tracing,
                 scope_exit,
                 TP_ARGS(const char*, text1,
                         const char*, text2),
                 TP_FIELDS(ctf_string(category, text1)
                           ctf_string(scope, text2)
                           )
                 )

#endif /* BASE_DEBUG_CHROMIUM_LTTNG_PROVIDER_H_ */

#include <lttng/tracepoint-event.h>

