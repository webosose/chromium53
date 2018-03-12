// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Reference to develop Source Code in this file is taken from browser
// Localized error code.

#include "webos/common/webos_localized_error.h"

#include "base/i18n/rtl.h"
#include "base/logging.h"
#include "base/strings/string16.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/utf_string_conversions.h"
#include "base/values.h"
#include "components/error_page/common/error_page_params.h"
#include "components/error_page/common/net_error_info.h"
#include "components/url_formatter/url_formatter.h"
#include "grit/webos_inspector_resources.h"
#include "webos/network_error_resources/grit/webos_network_error_resources.h"
#include "net/base/escape.h"
#include "net/base/net_errors.h"
#include "third_party/WebKit/public/platform/WebURLError.h"
#include "ui/base/l10n/l10n_util.h"
#include "ui/base/webui/web_ui_util.h"

using blink::WebURLError;

namespace error_page {

// Some error pages have no details.
const unsigned int kErrorPagesNoDetails = 0;

namespace {

struct WebOsLocalizedErrorMap {
  int error_code;
  unsigned int title_resource_id;
  unsigned int details_resource_id;
  // Detailed summary used when the error is in the main frame.
  unsigned int error_guide_resource_id;
  // Short one sentence description shown on mouse over when the error is in
  // a frame.
  unsigned int error_info_resource_id;
  //int suggestions;  // Bitmap of SUGGEST_* values.
};

//Currently we are handling below error cases
  // '-6' : FILE_NOT_FOUND
  // '-7' : TIMED_OUT
  // '-15': SOCKET_NOT_CONNECTED
  // '-21': NETWORK_CHANGED
  // '-101': CONNECTION_RESET
  // '-102': CONNECTION_REFUSED
  // '-105': NAME_NOT_RESOLVED
  // '-106': INTERNET_DISCONNECTED
  // '-109': ADDRESS_UNREACHABLE
  // '-118': CONNECTION_TIMED_OUT
  // '-137': NAME_RESOLUTION_FAILED
  // '-200': CERT_COMMON_NAME_INVALID
  // '-201': CERT_DATE_INVALID
  // '-202': CERT_AUTHORITY_INVALID
  // '-324': EMPTY_RESPONSE
  // '-501': INSECURE_RESPONSE

const WebOsLocalizedErrorMap net_error_options[] = {
    {
        net::ERR_FILE_NOT_FOUND,  //-6
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_FILE_OR_DIRECTORY_NOT_FOUND,
        IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION,
    },
    {
        net::ERR_TIMED_OUT,  //-7
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_OPERATION_TIMED_OUT,
        IDS_NET_ERROR_CHECK_NETWORK_STATUS_AND_TRY, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION,
    },
    {
        net::ERR_SOCKET_NOT_CONNECTED,  //-15
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION, IDS_NET_ERROR_NETWORK_UNSTABLE,
        IDS_NET_ERROR_CHECK_NETWORK_STATUS_AND_TRY, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION,
    },
    {
        net::ERR_NETWORK_CHANGED,  //-21
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION, IDS_NET_ERROR_NETWORK_UNSTABLE,
        IDS_NET_ERROR_CHECK_NETWORK_STATUS_AND_TRY, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION,
    },
    {
        net::ERR_CONNECTION_TIMED_OUT,  //-118
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_CONNECTION_ATTEMP_TIMED_OUT,
        IDS_NET_ERROR_CHECK_NETWORK_STATUS_AND_TRY, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION,
    },
    {
        net::ERR_CONNECTION_CLOSED,  //-100
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION, IDS_NET_ERROR_TEMPORARY_ISSUE,
        IDS_NET_ERROR_TRY_AGAIN_LATER, IDS_NET_ERROR_TEMPORARY_ISSUE,
        // SUGGEST_RELOAD,
    },
    {
        net::ERR_CONNECTION_RESET,  //-101
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION, IDS_NET_ERROR_NETWORK_UNSTABLE,
        IDS_NET_ERROR_CHECK_NETWORK_STATUS_AND_TRY, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION,
    },
    {
        net::ERR_CONNECTION_REFUSED,  //-102
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_CONNECTION_TO_SERVER_UNSTABLE,
        IDS_NET_ERROR_CHECK_NETWORK_STATUS_AND_TRY, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION,
    },
    {
        net::ERR_CONNECTION_FAILED, IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_TEMPORARY_ISSUE, IDS_NET_ERROR_TRY_AGAIN_LATER,
        IDS_NET_ERROR_TEMPORARY_ISSUE,
        // SUGGEST_RELOAD,
    },
    {
        net::ERR_NAME_NOT_RESOLVED,  //-105
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_ADDRESS_CANNOT_BE_FOUND,
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION_STATUS,
        IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD | SUGGEST_CHECK_CONNECTION | SUGGEST_DNS_CONFIG,
    },
    {
        net::ERR_INTERNET_DISCONNECTED,  //-106
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_NETWORK_NOT_CONNECTED,
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION_STATUS,
        IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_NONE,
    },
    {
        net::ERR_ADDRESS_UNREACHABLE,  //-109
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_ADDRESS_CANNOT_BE_FOUND,
        IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD,
    },
    {
        net::ERR_NAME_RESOLUTION_FAILED,  // -137
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_ADDRESS_CANNOT_BE_FOUND,
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION_STATUS, IDS_NET_ERROR_HOSTNAME,
        // SUGGEST_RELOAD,
    },
    {
        net::ERR_CERT_COMMON_NAME_INVALID,  // -200
        IDS_NET_ERROR_UNABLE_TO_LOAD,
        IDS_NET_ERROR_HOSTNAME_DOES_NOT_MATCH_CERTIFICATE,
        IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_RELOAD,
    },
    {
        net::ERR_CERT_DATE_INVALID,  // -201
        IDS_NET_ERROR_UNABLE_TO_LOAD, IDS_NET_ERROR_CERTIFICATE_HAS_EXPIRED_TV,
        IDS_NET_ERROR_CHECK_TIME_SETTINGS, IDS_NET_ERROR_CURRENT_TIME_SETTINGS,
        // SUGGEST_FIREWALL_CONFIG,
    },
    {
        net::ERR_CERT_AUTHORITY_INVALID,  //-202
        IDS_NET_ERROR_UNABLE_TO_LOAD,
        IDS_NET_ERROR_CERTIFICATE_CANNOT_BE_TRUSTED,
        IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_NONE,
    },
    {
        net::ERR_EMPTY_RESPONSE,  //-324
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION, IDS_NET_ERROR_NETWORK_UNSTABLE,
        IDS_NET_ERROR_CHECK_NETWORK_STATUS_AND_TRY, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_NONE,
    },
    {
        net::ERR_INSECURE_RESPONSE,  //-501
        IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
        IDS_NET_ERROR_CERTIFICATE_CANNOT_BE_TRUSTED,
        IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER, IDS_NET_ERROR_EMPTY_STRING,
        // SUGGEST_NONE,
    },
};

// Special error page to be used in the case of navigating back to a page
// generated by a POST.  LocalizedError::HasStrings expects this net error code
// to also appear in the array above.
const WebOsLocalizedErrorMap repost_error = {
  net::ERR_CACHE_MISS,
  IDS_NET_ERROR_ERRORPAGES_TITLE_NOT_AVAILABLE,
  IDS_NET_ERROR_TEMPORARY_ISSUE,
  IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
  IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER,
  //SUGGEST_RELOAD,
};

const WebOsLocalizedErrorMap http_error_options[] = {
  {403,
   IDS_NET_ERROR_TEMPORARY_ISSUE,
   IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
   IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER,
   IDS_NET_ERROR_TEMPORARY_ISSUE,
   //SUGGEST_NONE,
  },
};

const WebOsLocalizedErrorMap dns_probe_error_options[] = {
  {error_page::DNS_PROBE_POSSIBLE,
   IDS_NET_ERROR_TEMPORARY_ISSUE,
   IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
   IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER,
   IDS_NET_ERROR_TEMPORARY_ISSUE,
   //SUGGEST_RELOAD,
  },
};

const WebOsLocalizedErrorMap* FindErrorMapInArray(
    const WebOsLocalizedErrorMap* maps, size_t num_maps, int error_code) {
  for (size_t i = 0; i < num_maps; ++i) {
    if (maps[i].error_code == error_code)
      return &maps[i];
  }
  return NULL;
}

const WebOsLocalizedErrorMap* LookupErrorMap(const std::string& error_domain,
                                               int error_code, bool is_post) {
  return FindErrorMapInArray(net_error_options,
                                arraysize(net_error_options),
                                error_code);
}

bool LocaleIsRTL() {
  return base::i18n::IsRTL();
}

}  // namespace

const char WebOsLocalizedError::kHttpErrorDomain[] = "http";

void WebOsLocalizedError::GetStrings(int error_code,
                                       const std::string& error_domain,
                                       const GURL& failed_url,
                                       bool is_post,
                                       bool show_stale_load_button,
                                       const std::string& locale,
                                       const std::string& accept_languages,
                                       std::unique_ptr<ErrorPageParams> params,
                                       base::DictionaryValue* error_strings) {
  bool rtl = LocaleIsRTL();
  error_strings->SetString("textdirection", rtl ? "rtl" : "ltr");

  // // Grab the strings and settings that depend on the error type.  Init
  // // options with default values.
   WebOsLocalizedErrorMap options = {
     0,
     IDS_NET_ERROR_CHECK_NETWORK_CONNECTION,
     IDS_NET_ERROR_TEMPORARY_ISSUE,
     IDS_NET_ERROR_CONTACT_CONTENT_PROVIDER,
     IDS_NET_ERROR_TRY_AGAIN_LATER,
  //   SUGGEST_NONE,
   };

   WebOsLocalizedErrorMap atsc_legay_box_options = {
     -201,
     IDS_NET_ERROR_UNABLE_TO_LOAD,
     IDS_NET_ERROR_CERTIFICATE_HAS_EXPIRED_UHD,
     IDS_NET_ERROR_CHECK_TIME_SETTINGS,
     IDS_NET_ERROR_CURRENT_TIME_SETTINGS,
  //   SUGGEST_NONE,
   };

   const WebOsLocalizedErrorMap* error_map =
       LookupErrorMap(error_domain, error_code, is_post);
   if (error_map)
     options = *error_map;

   base::string16 failed_url_string(url_formatter::FormatUrl(
       failed_url, url_formatter::kFormatUrlOmitNothing,
       net::UnescapeRule::NORMAL, nullptr, nullptr, nullptr));

   //TBD:
   //1. Append error code to title
   //2. Place failed url string in error_info
   error_strings->SetString("error_title",
       l10n_util::GetStringUTF16(options.title_resource_id));
   error_strings->SetString("error_details",
       l10n_util::GetStringUTF16(options.details_resource_id));
   error_strings->SetString("error_guide",
       l10n_util::GetStringUTF16(options.error_guide_resource_id));
   error_strings->SetString("url_to_reload", failed_url_string);

   failed_url_string = base::UTF8ToUTF16(failed_url.host());

   if (net::ERR_NAME_RESOLUTION_FAILED == error_code ||
       net::ERR_NAME_NOT_RESOLVED == error_code)
     error_strings->SetString("error_info", failed_url_string);
   else
      error_strings->SetString("error_info",
        l10n_util::GetStringFUTF16(options.error_info_resource_id,
                                      failed_url_string));

    //We need to change some strings in case of ATSC30LegacyBoxSupport
    if (net::ERR_CERT_DATE_INVALID == error_code) {
      error_strings->SetString("error_title_atsc_legacy_box",
          l10n_util::GetStringUTF16(atsc_legay_box_options.title_resource_id));
      error_strings->SetString("error_details_atsc_legacy_box",
          l10n_util::GetStringUTF16(atsc_legay_box_options.details_resource_id));
      error_strings->SetString("error_guide_atsc_legacy_box",
          l10n_util::GetStringUTF16(atsc_legay_box_options.error_guide_resource_id));
      error_strings->SetString(
          "error_info_atsc_legacy_box",
          l10n_util::GetStringUTF16(
              atsc_legay_box_options.error_info_resource_id));
    }

    // We want localized Buttons on page
    // Error page uses Buttons
    // 1. Exit App
    // 2. Nwtwork Settings
    // 3. Retry
    // 4. Settings
    error_strings->SetString(
        "exit_app_button_text",
        l10n_util::GetStringUTF16(IDS_NET_ERROR_EXIT_APP_BUTTON_TEXT));
    error_strings->SetString(
        "network_settings_button_text",
        l10n_util::GetStringUTF16(IDS_NET_ERROR_NETWORK_SETTINGS_BUTTON_TEXT));
    error_strings->SetString(
        "retry_button_text",
        l10n_util::GetStringUTF16(IDS_NET_ERROR_RETRY_BUTTON_TEXT));
    error_strings->SetString(
        "settings_button_text",
        l10n_util::GetStringUTF16(IDS_NET_ERROR_SETTINGS_UPPER_CASE));
}

base::string16 WebOsLocalizedError::GetErrorDetails(
    const std::string& error_domain,
    int error_code,
    bool is_post) {
  const WebOsLocalizedErrorMap* error_map =
      LookupErrorMap(error_domain, error_code, is_post);
  if (error_map)
    return l10n_util::GetStringUTF16(error_map->details_resource_id);
  else
    return l10n_util::GetStringUTF16(IDS_NET_ERROR_ERRORPAGES_HEADING_NOT_AVAILABLE);
}

bool WebOsLocalizedError::HasStrings(const std::string& error_domain,
                                       int error_code) {
  // Whether or not the there are strings for an error does not depend on
  // whether or not the page was be generated by a POST, so just claim it was
  // not.
  return LookupErrorMap(error_domain, error_code, /*is_post=*/false) != NULL;
}

}  // namespace error_page
