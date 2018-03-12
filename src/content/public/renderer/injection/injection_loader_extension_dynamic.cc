// Copyright (c) 2014 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/logging.h"
#include "base/strings/string_split.h"
#include "content/public/renderer/render_thread.h"
#include "content/public/renderer/injection/injection_loader_extension.h"
#include "v8/include/v8.h"

#include <dlfcn.h>
#include <string>
#include <sstream>

namespace {

typedef v8::Extension* (*inject_t)();

const char kInjectionEnvironment[] = "LD_CHROMIUM_INJECTION_PATH";
const char kInjectionPrefix[] = "lib";
const char kInjectionPostfix[] = "_inj.so";

std::string GetInjectionLibraryName(const std::string& name) {
  std::stringstream library_name;
  library_name << kInjectionPrefix << name << kInjectionPostfix;
  return library_name.str();
}

void* LoadInjectionLibrary(const std::string& name) {
  const std::string libname = GetInjectionLibraryName(name);

  typedef std::vector<std::string> Strings;
  Strings locations;

  const char* path = getenv(kInjectionEnvironment);
  if (path)
    base::SplitString(path, ':', &locations);

  if (locations.empty())
    locations.push_back("");

  Strings::const_iterator it = locations.begin();
  for (; it != locations.end(); ++ it) {
    std::stringstream fullname;
    fullname << *it;
    if (!it->empty() && *(it->rbegin()) != '/')
      fullname << "/";
    fullname << libname;

    LOG(INFO) << "Opening " << fullname.str();
    void* handle = dlopen(fullname.str().c_str(), RTLD_NOW);
    if (handle)
      return handle;
  }

  LOG(ERROR) << "Cannot load library " << libname;
  return NULL;
}

} // namespace

namespace extensions_v8 {

v8::Extension* InjectionLoaderExtension::Get(
    const std::string& name) {
  void* handle = LoadInjectionLibrary(name);
  if (handle) {
    inject_t createInjection = (inject_t) dlsym(handle, "createInjection");
    if (createInjection)
      return createInjection();

    // FIXME: Does not unload the library at closing if symbol was loaded
    LOG(ERROR) << "Cannot load symbol 'createInjection': " << dlerror();
    dlclose(handle);
  }
  return NULL;
}

}  // namespace extensions_v8
