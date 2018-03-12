// Copyright (c) 2017 LG Electronics, Inc.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/common/webos_locales_mapping.h"

#include <map>
#include <utility>

#define MAP_SIZE(pair, pair_struct) \
  pair, pair + sizeof(pair) / sizeof(pair_struct)

namespace webos {

namespace {

typedef std::pair<std::string, const std::string> StringPair;
typedef std::map<std::string, const std::string> StringMap;

static StringPair g_language_pair[] = {
    // African
    std::make_pair("af-ZA", "af"),       // Afrikaans
    std::make_pair("am-ET", "am"),       // Amharic
    std::make_pair("ha-Latn-NG", "ha"),  // Hausa
    std::make_pair("ha-NG-Latn", "ha"),
    // Albanian
    std::make_pair("sq-AL", "sq"), std::make_pair("sq-ME", "sq"),
    // Arabic
    std::make_pair("ar-SA", "ar"), std::make_pair("ar-DZ", "ar"),
    std::make_pair("ar-BH", "ar"), std::make_pair("ar-DJ", "ar"),
    std::make_pair("ar-EG", "ar"), std::make_pair("ar-IQ", "ar"),
    std::make_pair("ar-JO", "ar"), std::make_pair("ar-KW", "ar"),
    std::make_pair("ar-LB", "ar"), std::make_pair("ar-LY", "ar"),
    std::make_pair("ar-MR", "ar"), std::make_pair("ar-MA", "ar"),
    std::make_pair("ar-OM", "ar"), std::make_pair("ar-QA", "ar"),
    std::make_pair("ar-SD", "ar"), std::make_pair("ar-SY", "ar"),
    std::make_pair("ar-TN", "ar"), std::make_pair("ar-AE", "ar"),
    std::make_pair("ar-YE", "ar"),
    // Bosnian
    std::make_pair("bs-BA-Latn", "bs"), std::make_pair("bs-Latn-BA", "bs"),
    std::make_pair("bs-Latn-ME", "bs"), std::make_pair("bs-ME-Latn", "bs"),
    // Bulgarian
    std::make_pair("bg-BG", "bg"),
    // Chinese
    std::make_pair("zh-CN-Hans", "zh-CN"),
    std::make_pair("zh-Hans-CN", "zh-CN"),
    std::make_pair("zh-Hant-HK", "zh-HK"),
    std::make_pair("zh-HK-Hant", "zh-HK"),
    std::make_pair("zh-SG-Hans", "zh-CN"),
    std::make_pair("zh-Hans-SG", "zh-CN"),
    std::make_pair("zh-Hant-TW", "zh-TW"),
    std::make_pair("zh-TW-Hant", "zh-TW"),
    std::make_pair("zh-MY-Hans", "zh-CN"),
    std::make_pair("zh-Hans-MY", "zh-CN"),
    // Croation
    std::make_pair("hr-HR", "hr"), std::make_pair("hr-ME", "hr"),
    // Czech
    std::make_pair("cs-CZ", "cs"),
    // Danish
    std::make_pair("da-DK", "da"),
    // Dutch
    std::make_pair("nl-NL", "nl"), std::make_pair("nl-BE", "nl"),
    // English
    std::make_pair("en-US", "en-US"), std::make_pair("en-AM", "en"),
    std::make_pair("en-AU", "en-AU"), std::make_pair("en-AZ", "en"),
    std::make_pair("en-CA", "en-US"), std::make_pair("en-CN", "en-GB"),
    std::make_pair("en-ET", "en"), std::make_pair("en-GM", "en"),
    std::make_pair("en-GE", "en"), std::make_pair("en-GH", "en"),
    std::make_pair("en-HK", "en-GB"), std::make_pair("en-IN", "en"),
    std::make_pair("en-IS", "en"), std::make_pair("en-IE", "en"),
    std::make_pair("en-JP", "en-JP"), std::make_pair("en-KE", "en"),
    std::make_pair("en-LR", "en"), std::make_pair("en-MM", "en"),
    std::make_pair("en-MW", "en"), std::make_pair("en-MX", "en-US"),
    std::make_pair("en-MY", "en"), std::make_pair("en-NZ", "en-NZ"),
    std::make_pair("en-NG", "en"), std::make_pair("en-PK", "en"),
    std::make_pair("en-PH", "en"), std::make_pair("en-PR", "en"),
    std::make_pair("en-RW", "en"), std::make_pair("en-SL", "en"),
    std::make_pair("en-SG", "en"), std::make_pair("en-ZA", "en-ZA"),
    std::make_pair("en-LK", "en"), std::make_pair("en-SD", "en"),
    std::make_pair("en-TW", "en-US"), std::make_pair("en-TZ", "en"),
    std::make_pair("en-GB", "en-GB"), std::make_pair("en-ZM", "en"),
    // Estonian
    std::make_pair("et-EE", "et"),
    // Farsi
    std::make_pair("fa-IR", "fa"), std::make_pair("fa-AF", "fa"),
    // Finnish
    std::make_pair("fi-FI", "fi"),
    // French
    std::make_pair("fr-FR", "fr"), std::make_pair("fr-DZ", "fr"),
    std::make_pair("fr-BE", "fr"), std::make_pair("fr-BJ", "fr"),
    std::make_pair("fr-BF", "fr"), std::make_pair("fr-CM", "fr"),
    std::make_pair("fr-CA", "fr"), std::make_pair("fr-CI", "fr"),
    std::make_pair("fr-DJ", "fr"), std::make_pair("fr-CD", "fr"),
    std::make_pair("fr-GA", "fr"), std::make_pair("fr-GN", "fr"),
    std::make_pair("fr-CQ", "fr"), std::make_pair("fr-LB", "fr"),
    std::make_pair("fr-LU", "fr"), std::make_pair("fr-ML", "fr"),
    std::make_pair("fr-CF", "fr"), std::make_pair("fr-CG", "fr"),
    std::make_pair("fr-RW", "fr"), std::make_pair("fr-SN", "fr"),
    std::make_pair("fr-CH", "fr-CH"), std::make_pair("fr-TG", "fr"),
    // German
    std::make_pair("de-DE", "de"), std::make_pair("de-LU", "de"),
    std::make_pair("de-AT", "de"), std::make_pair("de-CH", "de"),
    // Greek
    std::make_pair("el-GR", "el"), std::make_pair("el-CY", "el"),
    // Hebrew
    std::make_pair("he-IL", "he"),
    // Hungarian
    std::make_pair("hu-HU", "hu"),
    // Indian
    std::make_pair("hi-IN", "hi"),  // Hindi
    std::make_pair("kn-IN", "kn"),  // Kannada
    std::make_pair("te-IN", "te"),  // Telugu
    std::make_pair("ta-IN", "ta"),  // Tamil
    std::make_pair("ml-IN", "ml"),  // Malayalam
    std::make_pair("bn-IN", "bn"),  // Bengali
    std::make_pair("pa-IN", "pa"),  // Punjabi
    std::make_pair("gu-IN", "gu"),  // Gujarati
    std::make_pair("mr-IN", "mr"),  // Marathi
    std::make_pair("as-IN", "as"),  // Assamese
    std::make_pair("ur-IN", "ur"),  // Urdu
    std::make_pair("or-IN", "or"),  // Oriya
    // Indonesian
    std::make_pair("id-ID", "id"),
    // Irish Gaelic
    std::make_pair("ga-IE", "ga"),
    // Italian
    std::make_pair("it-IT", "it"), std::make_pair("it-CH", "it"),
    // Japanese
    std::make_pair("ja-JP", "ja"),
    // Kazakh
    std::make_pair("kk-KZ-Cyrl", "kk"), std::make_pair("kk-Cyrl-KZ", "kk"),
    // Khmer
    std::make_pair("km-KH", "km"),
    // Korean
    std::make_pair("ko-KR", "ko"),
    // Kurdish
    std::make_pair("ku-Arab-IQ", "ku"), std::make_pair("ku-IQ-Arab", "ku"),
    // Latvian
    std::make_pair("lv-LV", "lv"),
    // Lithuanian
    std::make_pair("lt-LT", "lt"),
    // Macedonian
    std::make_pair("mk-MK", "mk"),
    // Malay
    std::make_pair("ms-MY", "ms"), std::make_pair("ms-SG", "ms"),
    // Mongolian
    std::make_pair("mn-Cyrl-MN", "mn"), std::make_pair("mn-MN-Cyrl", "mn"),
    // Norwegian Bokmal
    std::make_pair("nb-NO", "nb"),
    // Polish
    std::make_pair("pl-PL", "pl"),
    // Portuguese
    std::make_pair("pt-PT", "pt-PT"), std::make_pair("pt-BR", "pt-BR"),
    std::make_pair("pt-AO", "pt"), std::make_pair("pt-CV", "pt"),
    std::make_pair("pt-CQ", "pt"),
    // Romanian
    std::make_pair("ro-RO", "ro"),
    // Russian
    std::make_pair("ru-RU", "ru"), std::make_pair("ru-BY", "ru"),
    std::make_pair("ru-GE", "ru"), std::make_pair("ru-KZ", "ru"),
    std::make_pair("ru-KG", "ru"), std::make_pair("ru-UA", "ru"),
    // Serbian
    std::make_pair("sr-RS-Latn", "sr"), std::make_pair("sr-Latn-RS", "sr"),
    std::make_pair("sr-ME-Latn", "sr"), std::make_pair("sr-Latn-ME", "sr"),
    // Slovak
    std::make_pair("sk-SK", "sk"),
    // Slovenian
    std::make_pair("sl-SI", "sl"),
    // Spanish
    std::make_pair("es-AR", "es"), std::make_pair("es-BO", "es"),
    std::make_pair("es-CL", "es"), std::make_pair("es-CO", "es-419"),
    std::make_pair("es-CR", "es"), std::make_pair("es-DO", "es"),
    std::make_pair("es-EC", "es"), std::make_pair("es-ES", "es"),
    std::make_pair("es-GT", "es"), std::make_pair("es-GQ", "es"),
    std::make_pair("es-HN", "es"), std::make_pair("es-MX", "es"),
    std::make_pair("es-NI", "es"), std::make_pair("es-PA", "es"),
    std::make_pair("es-PY", "es"), std::make_pair("es-PE", "es"),
    std::make_pair("es-PH", "es"), std::make_pair("es-PR", "es"),
    std::make_pair("es-SV", "es"), std::make_pair("es-US", "es"),
    std::make_pair("es-VE", "es"),
    // Swedish
    std::make_pair("sv-SE", "sv"), std::make_pair("sv-FI", "sv"),
    // Thai
    std::make_pair("th-TH", "th"),
    // Turkish
    std::make_pair("tr-TR", "tr"), std::make_pair("tr-CY", "tr"),
    std::make_pair("tr-AM", "tr"), std::make_pair("tr-AZ", "tr"),
    // Ukranian
    std::make_pair("uk-UA", "uk"),
    // Uzbek
    std::make_pair("uz-Latn-UZ", "uz"), std::make_pair("uz-UZ-Latn", "uz"),
    std::make_pair("uz-Cyrl-UZ", "uz-CR"),
    std::make_pair("uz-UZ-Cyrl", "uz-CR"),
    // Vietnamese
    std::make_pair("vi-VN", "vi"),
    // End of Map : set to default value
    std::make_pair("ENDOFMAP", "en-GB")};

static StringMap kLanguageMap(MAP_SIZE(g_language_pair, StringPair));

}  // namespace

std::string MapWebOsToChromeLocales(const std::string& language_string) {
  std::string temp_language = language_string;
  StringMap::iterator iter = kLanguageMap.find(language_string);
  if (iter != kLanguageMap.end()) {
    temp_language = iter->second;
  } else if (temp_language.length() > 2) {
    temp_language = language_string.substr(0, 2);
  }

  if (temp_language == "zh" && language_string.length() >= 10)
    temp_language.append(language_string.substr(7, 10));

  return temp_language;
}
}  // content
