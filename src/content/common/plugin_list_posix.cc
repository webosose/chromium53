// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/common/plugin_list.h"

#if defined(WEBOS_PLUGIN_SUPPORT)
#include <algorithm>
#include <dlfcn.h>
#include "base/files/file_enumerator.h"
#include "base/path_service.h"
#include "base/strings/utf_string_conversions.h"
#include "base/strings/string_split.h"
#include "base/strings/string_util.h"
#include "base/native_library.h"
#include "third_party/npapi/bindings/nphostapi.h"
#endif

namespace content {

#if defined(WEBOS_PLUGIN_SUPPORT)

// Copied from nsplugindefs.h instead of including the file since it has a bunch
// of dependencies.
enum nsPluginVariable {
  nsPluginVariable_NameString        = 1,
  nsPluginVariable_DescriptionString = 2
};

static void ExtractVersionString(const std::string& desc,
                                      WebPluginInfo* info) {
  // This matching works by extracting a version substring, along the lines of:
  // No postfix: second match in .*<prefix>.*$
  // With postfix: second match .*<prefix>.*<postfix>
  static const struct {
    const char* kPrefix;
    const char* kPostfix;
  } kPrePostFixes[] = {
    { "Shockwave Flash ", 0 },
    { "Java(TM) Plug-in ", 0 },
    { "(using IcedTea-Web ", " " },
    { 0, 0 }
  };
  std::string version;
  for (size_t i = 0; kPrePostFixes[i].kPrefix; ++i) {
    size_t pos;
    if ((pos = desc.find(kPrePostFixes[i].kPrefix)) != std::string::npos) {
      version = desc.substr(pos + strlen(kPrePostFixes[i].kPrefix));
      pos = std::string::npos;
      if (kPrePostFixes[i].kPostfix)
        pos = version.find(kPrePostFixes[i].kPostfix);
      if (pos != std::string::npos)
        version = version.substr(0, pos);
      break;
    }
  }
  if (!version.empty()) {
    info->version = base::UTF8ToUTF16(version);
  }
}

static void ParseMIMEDescription(const std::string& description,
    std::vector<WebPluginMimeType>* mime_types) {
  // We parse the description here into WebPluginMimeType structures.
  // Naively from the NPAPI docs you'd think you could use
  // string-splitting, but the Firefox parser turns out to do something
  // different: find the first colon, then the second, then a semi.
  //
  // See ParsePluginMimeDescription near
  // http://mxr.mozilla.org/firefox/source/modules/plugin/base/src/nsPluginsDirUtils.h#53

  std::string::size_type ofs = 0;
  for (;;) {
    WebPluginMimeType mime_type;

    std::string::size_type end = description.find(':', ofs);
    if (end == std::string::npos)
      break;
    mime_type.mime_type = base::ToLowerASCII(description.substr(ofs, end - ofs));
    ofs = end + 1;

    end = description.find(':', ofs);
    if (end == std::string::npos)
      break;
    const std::string extensions = description.substr(ofs, end - ofs);
    mime_type.file_extensions = base::SplitString(
        extensions, ",", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);
    ofs = end + 1;

    end = description.find(';', ofs);
    // It's ok for end to run off the string here.  If there's no
    // trailing semicolon we consume the remainder of the string.
    if (end != std::string::npos) {
      mime_type.description = base::UTF8ToUTF16(description.substr(ofs, end - ofs));
    } else {
      mime_type.description = base::UTF8ToUTF16(description.substr(ofs));
    }
    mime_types->push_back(mime_type);
    if (end == std::string::npos)
      break;
    ofs = end + 1;
  }
}
#endif

bool PluginList::ReadWebPluginInfo(const base::FilePath& filename,
                                   WebPluginInfo* info) {
#if defined(WEBOS_PLUGIN_SUPPORT)
  base::NativeLibraryLoadError error;
  void* dl = base::LoadNativeLibrary(filename, &error);
  if (!dl) {
    LOG_IF(ERROR, PluginList::DebugPluginLoading())
        << "While reading plugin info, unable to load library "
        << filename.value() << " (" << error.ToString() << "), skipping.";
    return false;
  }

  info->path = filename;

  // See comments in plugin_lib_mac regarding this symbol.
  typedef const char* (*NP_GetMimeDescriptionType)();
  NP_GetMimeDescriptionType NP_GetMIMEDescription =
      reinterpret_cast<NP_GetMimeDescriptionType>(
          dlsym(dl, "NP_GetMIMEDescription"));
  const char* mime_description = NULL;
  if (!NP_GetMIMEDescription) {
    LOG_IF(ERROR, PluginList::DebugPluginLoading())
        << "Plugin " << filename.value() << " doesn't have a "
        << "NP_GetMIMEDescription symbol";
    return false;
  }
  mime_description = NP_GetMIMEDescription();

  if (!mime_description) {
    LOG_IF(ERROR, PluginList::DebugPluginLoading())
        << "MIME description for " << filename.value() << " is empty";
    return false;
  }
  ParseMIMEDescription(mime_description, &info->mime_types);

  // The plugin name and description live behind NP_GetValue calls.
  typedef NPError (*NP_GetValueType)(void* unused,
                                     nsPluginVariable variable,
                                     void* value_out);
  NP_GetValueType NP_GetValue =
      reinterpret_cast<NP_GetValueType>(dlsym(dl, "NP_GetValue"));
  if (NP_GetValue) {
    const char* name = NULL;
    NP_GetValue(NULL, nsPluginVariable_NameString, &name);
    if (name) {
      info->name = base::UTF8ToUTF16(name);
      ExtractVersionString(name, info);
    }

    const char* description = NULL;
    NP_GetValue(NULL, nsPluginVariable_DescriptionString, &description);
    if (description) {
      info->desc = base::UTF8ToUTF16(description);
      if (info->version.empty())
        ExtractVersionString(description, info);
    }

    LOG_IF(ERROR, PluginList::DebugPluginLoading())
        << "Got info for plugin " << filename.value()
        << " Name = \"" << UTF16ToUTF8(info->name)
        << "\", Description = \"" << UTF16ToUTF8(info->desc)
        << "\", Version = \"" << UTF16ToUTF8(info->version)
        << "\".";
  } else {
    LOG_IF(ERROR, PluginList::DebugPluginLoading())
        << "Plugin " << filename.value()
        << " has no GetValue() and probably won't work.";
  }

  // Intentionally not unloading the plugin here, it can lead to crashes.

  return true;
#endif
  return false;
}

void PluginList::GetPluginDirectories(
    std::vector<base::FilePath>* plugin_dirs) {
#if defined(WEBOS_PLUGIN_SUPPORT)
#if defined(USE_NPAPI_PLUGIN_EXTERNAL_PATH)
  std::vector<std::string> dirs;

  const char* npapi_plugin_path = getenv("NPAPI_PLUGIN_PATH");
  if (npapi_plugin_path)
    dirs = base::SplitString(
        npapi_plugin_path, ":", base::TRIM_WHITESPACE, base::SPLIT_WANT_ALL);

  for (std::vector<std::string>::const_iterator it = dirs.begin(); it != dirs.end(); ++it)
    if (!(*it).empty())
      plugin_dirs->push_back(base::FilePath(*it));
#else
  plugin_dirs->push_back(base::FilePath("/usr/lib/BrowserPlugins"));
#endif
#endif
}

void PluginList::GetPluginsInDir(const base::FilePath& dir_path,
                                 std::vector<base::FilePath>* plugins) {
#if defined(WEBOS_PLUGIN_SUPPORT)
  base::FileEnumerator enumerator(dir_path,
                                  false,  // not recursive
                                  base::FileEnumerator::FILES);
  for (base::FilePath path = enumerator.Next(); !path.value().empty();
       path = enumerator.Next()) {

    if (std::find(plugins->begin(), plugins->end(), path) != plugins->end()) {
      LOG_IF(ERROR, PluginList::DebugPluginLoading())
          << "Skipping duplicate instance of " << path.value();
      continue;
    }

    plugins->push_back(path);
  }
#endif
}

bool PluginList::ShouldLoadPluginUsingPluginList(
    const WebPluginInfo& info,
    std::vector<WebPluginInfo>* plugins) {
  LOG_IF(ERROR, PluginList::DebugPluginLoading())
      << "Considering " << info.path.value() << " (" << info.name << ")";

  if (info.type == WebPluginInfo::PLUGIN_TYPE_NPAPI) {
#if defined(WEBOS_PLUGIN_SUPPORT)
    return true;
#else
    NOTREACHED() << "NPAPI plugins are not supported";
    return false;
#endif
  }

  VLOG_IF(1, PluginList::DebugPluginLoading()) << "Using " << info.path.value();
  return true;
}

}  // namespace content
