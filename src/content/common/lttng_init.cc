// Copyright 2015 LG Corporation. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/lttng_init.h"

#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/native_library.h"
#include "base/path_service.h"

namespace content {

void LttngInit (void)
{
#if defined(ENABLE_LTTNG)
  base::FilePath module_dir;
#if defined(OS_WEBOS) && defined(USE_CBE)
  CHECK(PathService::Get(base::DIR_CBE_DATA, &module_dir));
#else
  CHECK(PathService::Get(base::DIR_LTTNG_PROVIDERS, &module_dir));
#endif
  base::FilePath kLttngModulePath(FILE_PATH_LITERAL("liblttng_provider.so"));
  base::FilePath provider_path(module_dir.Append(kLttngModulePath).value());

  if (!base::PathExists(provider_path)) {
    VLOG(1) << "Lttng provider library does not exist";
    return;
  }

  base::NativeLibraryLoadError error;
  base::NativeLibrary library = base::LoadNativeLibrary(provider_path,
                                                        &error);
  VLOG_IF(1, !library) << "Unable to load lttng_provider"
                       << error.ToString();

  (void)library;  // Prevent release-mode warning.
#endif
}

}  // content
