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

#include "gl/GrGLInterface.h"
#include "gl/GrGLAssembleInterface.h"
#include "gl/GrGLExtensions.h"
#include "gl/GrGLUtil.h"

#include <EGL/egl.h>
#include <GLES2/gl2.h>
#include <dlfcn.h>

class GLLoader {
 public:
  GLLoader() { fLibrary = dlopen("/usr/lib/libGLESv2.so.2", RTLD_LAZY); }

  ~GLLoader() {
    if (fLibrary) {
      dlclose(fLibrary);
    }
  }

  void* handle() const { return nullptr == fLibrary ? RTLD_DEFAULT : fLibrary; }

 private:
  void* fLibrary;
};

class GLProcGetter {
 public:
  GLProcGetter() {}

  GrGLFuncPtr getProc(const char name[]) const {
    return (GrGLFuncPtr)dlsym(fLoader.handle(), name);
  }

 private:
  GLLoader fLoader;
};

static GrGLFuncPtr webos_get_gl_proc(void* ctx, const char name[]) {
  SkASSERT(ctx);
  const GLProcGetter* getter = (const GLProcGetter*)ctx;
  return getter->getProc(name);
}

const GrGLInterface* GrGLCreateNativeInterface() {
  GLProcGetter getter;
  return GrGLAssembleGLESInterface(&getter, webos_get_gl_proc);
}
