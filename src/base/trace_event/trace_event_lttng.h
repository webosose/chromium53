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

#ifndef BASE_TRACE_EVENT_TRACE_EVENT_LTTNG_H_
#define BASE_TRACE_EVENT_TRACE_EVENT_LTTNG_H_

#if defined(ENABLE_LTTNG) && !defined(OS_NACL) && defined(LTTNG_TARGET)

#include "base/debug/chromium_lttng_provider.h"

#define LTTNG_TRACE_EVENT0(category_group, name) \
  LTTNG_TRACE_SCOPE(category_group, name);

#define LTTNG_TRACE_EVENT_BEGIN0(category_group, name) \
  LTTNG_TRACE_SCOPE_ENTRY(category_group, name);
#define LTTNG_TRACE_EVENT_END0(category_group, name) \
  LTTNG_TRACE_SCOPE_EXIT(category_group, name);

#define LTTNG_TRACE_SCOPE_ENTRY(category_group, name) \
  tracepoint(chromium_tracing, scope_entry, category_group, name)
#define LTTNG_TRACE_SCOPE_EXIT(category_group, name) \
  tracepoint(chromium_tracing, scope_exit, category_group, name)
#define LTTNG_TRACE_SCOPE(category_group, name) \
  trace_event_lttng::LttngTraceScope(category_group, name)

namespace trace_event_lttng {

class LttngTraceScope {
 public:
  LttngTraceScope(const char* category_group, const char* name)
    : category_group_(category_group)
    , name_(name)
  {
    LTTNG_TRACE_SCOPE_ENTRY(category_group, name);
  }

  ~LttngTraceScope()
  {
    LTTNG_TRACE_SCOPE_EXIT(category_group_, name_);
  }
 private:
  const char* category_group_;
  const char* name_;

  void operator delete(void*);
  void* operator new(size_t);
  LttngTraceScope(const LttngTraceScope&);
  LttngTraceScope& operator=(const LttngTraceScope&);
};

} // namespace trace_event_lttng

#else // defined(ENABLE_LTTNG)

#define LTTNG_TRACE_EVENT0(category_group, name)
#define LTTNG_TRACE_EVENT_BEGIN0(category_group, name)
#define LTTNG_TRACE_EVENT_END0(category_group, name)

#endif

#endif  // BASE_DEBUG_TRACE_EVENT_LTTNG_H_
