// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stddef.h>

#include "media/filters/source_buffer_platform.h"

namespace media {

// 2MB: approximately 1 minute of 256Kbps content.
// 30MB: approximately 1 minute of 4Mbps content.
const size_t kSourceBufferAudioMemoryLimit = 2 * 1024 * 1024;
#if defined(OS_WEBOS) && defined(USE_UMEDIASERVER)
#if defined(PLATFORM_APOLLO)
const size_t kSourceBufferVideoMemoryLimit = 15 * 1024 * 1024;
#else
const size_t kSourceBufferVideoMemoryLimit = 30 * 1024 * 1024;
#endif
#endif

}  // namespace media
