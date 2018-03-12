// Copyright (c) 2011 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef WEBOS_APP_NET_WEBOS_LOCALIZED_ERROR_H_
#define WEBOS_APP_NET_WEBOS_LOCALIZED_ERROR_H_

#include <string>

#include "base/strings/string16.h"
#include "url/gurl.h"

namespace base {
class DictionaryValue;
class ListValue;
}

namespace blink {
struct WebURLError;
}

namespace error_page {

struct ErrorPageParams;

class WebOsLocalizedError {
 public:
  // Fills |error_strings| with values to be used to build an error page used
  // on HTTP errors, like 404 or connection reset.
  static void GetStrings(int error_code,
                         const std::string& error_domain,
                         const GURL& failed_url,
                         bool is_post,
                         bool show_stale_load_button,
                         const std::string& locale,
                         const std::string& accept_languages,
                         std::unique_ptr<ErrorPageParams> params,
                         base::DictionaryValue* strings);

  // Returns a description of the encountered error.
  static base::string16 GetErrorDetails(const std::string& error_domain,
                                        int error_code,
                                        bool is_post);

  // Returns true if an error page exists for the specified parameters.
  static bool HasStrings(const std::string& error_domain, int error_code);

  static const char kHttpErrorDomain[];

 private:
  DISALLOW_IMPLICIT_CONSTRUCTORS(WebOsLocalizedError);
};

}  // namespace error_page

#endif  // WEBOS_APP_NET_WEBOS_LOCALIZED_ERROR_H_
