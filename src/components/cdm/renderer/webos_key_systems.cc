// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "components/cdm/renderer/webos_key_systems.h"

#if defined(ENABLE_PEPPER_CDMS) && defined(WIDEVINE_CDM_AVAILABLE)
#include "components/cdm/renderer/widevine_key_system_properties.h"
#include "media/base/eme_constants.h"
#endif

namespace cdm {

void AddWebOSKeySystems(
    std::vector<std::unique_ptr<media::KeySystemProperties>>*
        key_systems_properties) {
#if defined(ENABLE_PEPPER_CDMS) && defined(WIDEVINE_CDM_AVAILABLE)
  media::SupportedCodecs supported_codecs = media::EME_CODEC_WEBM_ALL;

#if defined(USE_PROPRIETARY_CODECS)
  supported_codecs |= media::EME_CODEC_MP4_AVC1;
  supported_codecs |= media::EME_CODEC_MP4_AAC;
  supported_codecs |= media::EME_CODEC_MP4_VP9;
#endif  // defined(USE_PROPRIETARY_CODECS)

  using Robustness = cdm::WidevineKeySystemProperties::Robustness;
  key_systems_properties->emplace_back(new cdm::WidevineKeySystemProperties(
      supported_codecs,
      Robustness::HW_SECURE_CRYPTO,  // Maximum audio robustness.
      Robustness::HW_SECURE_ALL,     // Maximum video robustness.
      media::EmeSessionTypeSupport::
          SUPPORTED_WITH_IDENTIFIER,  // persistent-license.
      media::EmeSessionTypeSupport::
          NOT_SUPPORTED,                         // persistent-release-message.
      media::EmeFeatureSupport::ALWAYS_ENABLED,  // Persistent state.
      media::EmeFeatureSupport::REQUESTABLE));   // Distinctive identifier.
#endif
}

}  // namespace cdm
