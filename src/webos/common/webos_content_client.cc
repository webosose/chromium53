// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file

#include "webos/common/webos_content_client.h"

#include "base/files/file_util.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "content/public/common/pepper_plugin_info.h"
#include "content/public/common/user_agent.h"
#include "ppapi/shared_impl/ppapi_permissions.h"
#include "ui/base/resource/resource_bundle.h"

#if defined(WIDEVINE_CDM_AVAILABLE)
#include "third_party/widevine/cdm/webos/widevine_cdm_version.h"
#endif

#if defined(ENABLE_PLAYREADY_CDM)
#include "third_party/playready/cdm/playready_cdm_version.h"
#endif

namespace webos {

WebOSContentClient::~WebOSContentClient() {
}

base::StringPiece WebOSContentClient::GetDataResource(
    int resource_id, ui::ScaleFactor scale_factor) const {
  return ui::ResourceBundle::GetSharedInstance().GetRawDataResourceForScale(
      resource_id, scale_factor);
}

base::RefCountedMemory* WebOSContentClient::GetDataResourceBytes(
    int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().LoadDataResourceBytes(
      resource_id);
}

gfx::Image& WebOSContentClient::GetNativeImageNamed(int resource_id) const {
  return ui::ResourceBundle::GetSharedInstance().GetNativeImageNamed(
      resource_id);
}

std::string WebOSContentClient::GetUserAgent() const {
  return content::BuildUserAgentFromProduct("Chrome/" CHROMIUM_VERSION);
}

void WebOSContentClient::AddPepperPlugins(
    std::vector<content::PepperPluginInfo>* plugins) {
  base::FilePath path;
#if defined(WIDEVINE_CDM_AVAILABLE) && defined(ENABLE_PEPPER_CDMS)
  static bool skip_widevine_cdm_file_check = false;
  std::string widevine_lib_path(getenv("CDM_LIB_PATH"));
  std::string widevine_lib_file("/libwidevinecdmadapter.so");
  path = base::FilePath(widevine_lib_path + widevine_lib_file);

  if (skip_widevine_cdm_file_check || base::PathExists(path)) {
    content::PepperPluginInfo widevine_cdm;
    widevine_cdm.is_out_of_process = true;
    widevine_cdm.path = path;
    widevine_cdm.name = kWidevineCdmDisplayName;
    widevine_cdm.description = kWidevineCdmDescription +
        std::string(" (version: ") + WIDEVINE_CDM_VERSION_STRING + ")";
    widevine_cdm.version = WIDEVINE_CDM_VERSION_STRING;
    content::WebPluginMimeType widevine_cdm_mime_type(
        kWidevineCdmPluginMimeType,
        "",
        kWidevineCdmPluginMimeTypeDescription);

    // Add the supported codecs as if they came from the component manifest.
    std::vector<std::string> codecs;
    codecs.push_back(kCdmSupportedCodecVorbis);
    codecs.push_back(kCdmSupportedCodecVp8);
    codecs.push_back(kCdmSupportedCodecVp9);
#if defined(USE_PROPRIETARY_CODECS)
    codecs.push_back(kCdmSupportedCodecAac);
    codecs.push_back(kCdmSupportedCodecAvc1);
#endif

    const char widevine_codecs_delimiter[] = { kCdmSupportedCodecsValueDelimiter };
    std::string codec_string =
        base::JoinString(codecs, widevine_codecs_delimiter);
    widevine_cdm_mime_type.additional_param_names.push_back(
        base::ASCIIToUTF16(kCdmSupportedCodecsParamName));
    widevine_cdm_mime_type.additional_param_values.push_back(
        base::ASCIIToUTF16(codec_string));

    widevine_cdm.mime_types.push_back(widevine_cdm_mime_type);
    widevine_cdm.permissions = ppapi::PERMISSION_DEV |
                               ppapi::PERMISSION_PRIVATE;

    plugins->push_back(widevine_cdm);

    skip_widevine_cdm_file_check = true;
  }
#endif

#if defined(ENABLE_PLAYREADY_CDM) && defined(ENABLE_PEPPER_CDMS)
  static bool skip_playready_cdm_file_check = false;
  std::string playready_lib_path(getenv("CDM_LIB_PATH"));
  std::string playready_lib_file("/libplayreadycdmadapter.so");
  path = base::FilePath(playready_lib_path + playready_lib_file);
  if (skip_playready_cdm_file_check || base::PathExists(path)) {
    content::PepperPluginInfo playready_cdm;
    playready_cdm.is_out_of_process = true;
    playready_cdm.path = path;
    playready_cdm.name = kPlayReadyCdmDisplayName;
    playready_cdm.description =
        kPlayReadyCdmDescription +
        std::string(" (version: ") +
        PLAYREADY_CDM_VERSION_STRING +
        ")";
    playready_cdm.version = PLAYREADY_CDM_VERSION_STRING;
    content::WebPluginMimeType playready_cdm_mime_type(
        kPlayReadyCdmPluginMimeType,
        "",
        kPlayReadyCdmDisplayName);

    std::vector<std::string> codecs;
    codecs.push_back(kCdmSupportedCodecVorbis);
    codecs.push_back(kCdmSupportedCodecVp8);
    codecs.push_back(kCdmSupportedCodecVp9);
#if defined(USE_PROPRIETARY_CODECS)
    codecs.push_back(kCdmSupportedCodecAac);
    codecs.push_back(kCdmSupportedCodecAvc1);
#endif

    const char playready_codecs_delimiter[] = { kCdmSupportedCodecsValueDelimiter };
    std::string codec_string =
        base::JoinString(codecs, playready_codecs_delimiter);
    playready_cdm_mime_type.additional_param_names.push_back(
        base::ASCIIToUTF16(kCdmSupportedCodecsParamName));
    playready_cdm_mime_type.additional_param_values.push_back(
        base::ASCIIToUTF16(codec_string));

    playready_cdm.mime_types.push_back(playready_cdm_mime_type);
    playready_cdm.permissions = ppapi::PERMISSION_DEV |
                                ppapi::PERMISSION_PRIVATE;

    plugins->push_back(playready_cdm);

    skip_playready_cdm_file_check = true;
  }
#endif
}

} // namespace webos
