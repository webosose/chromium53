// Copyright 2017 LG Electronics, Inc. All rights reserved.

#include "webos/common/webos_resource_delegate.h"

#include "base/base_switches.h"
#include "base/command_line.h"
#include "base/path_service.h"
#include "base/files/file_path.h"
#include "base/files/file_util.h"
#include "content/public/common/content_switches.h"
#include "ui/base/ui_base_switches.h"

namespace webos {

namespace {
const char kPakFileSuffix[] = ".pak";
}

base::FilePath WebosResourceDelegate::GetPathForResourcePack(
    const base::FilePath& pack_path,
    ui::ScaleFactor scale_factor) {
  NOTIMPLEMENTED();
  return pack_path;
}

base::FilePath WebosResourceDelegate::GetPathForLocalePack(
    const base::FilePath& pack_path,
    const std::string& locale) {
  base::FilePath locale_pack_path;
  if (!PathService::Get(base::DIR_CBE_DATA, &locale_pack_path)) {
    LOG(ERROR) << "WebOS Locale path for " << locale << "cannot be resolved";
    return pack_path;
  }

  locale_pack_path = locale_pack_path.Append(FILE_PATH_LITERAL("locales"));
  locale_pack_path = locale_pack_path.AppendASCII(locale + kPakFileSuffix);
  return locale_pack_path;
}

gfx::Image WebosResourceDelegate::GetImageNamed(int resource_id) {
  return gfx::Image();
}

gfx::Image WebosResourceDelegate::GetNativeImageNamed(int resource_id) {
  return gfx::Image();
}

base::RefCountedStaticMemory* WebosResourceDelegate::LoadDataResourceBytes(
    int resource_id,
    ui::ScaleFactor scale_factor) {
  return nullptr;
}

bool WebosResourceDelegate::GetRawDataResource(int resource_id,
                                               ui::ScaleFactor scale_factor,
                                               base::StringPiece* value) {
  return false;
}

bool WebosResourceDelegate::GetLocalizedString(int message_id,
                                               base::string16* value) {
  return false;
}

bool WebosResourceDelegate::LoadBrowserResources() {
  base::FilePath path;
  PathService::Get(base::DIR_CBE_DATA, &path);
  base::FilePath resource_path =
      path.Append(FILE_PATH_LITERAL("webos_resources.pak"));
  ui::ResourceBundle::GetSharedInstance().AddDataPackFromPath(
        resource_path,
        ui::SCALE_FACTOR_100P);
    return true;
}

// static
void WebosResourceDelegate::InitializeResourceBundle() {
  WebosResourceDelegate* resource_delegate = new WebosResourceDelegate();
  base::CommandLine* command_line = base::CommandLine::ForCurrentProcess();
  const std::string locale = command_line->GetSwitchValueASCII(switches::kLang);
  ui::ResourceBundle::InitSharedInstanceWithLocale(
      locale, resource_delegate, ui::ResourceBundle::LOAD_COMMON_RESOURCES);
}

}  // namespace webos
