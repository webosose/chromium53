// Copyright (c) 2017-2018 LG Electronics, Inc.
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

#ifndef WEBOS_RENDERER_NET_TEMPLATE_BUILDER_H_
#define WEBOS_RENDERER_NET_TEMPLATE_BUILDER_H_

#include <string>

#include "base/strings/string_piece.h"
#include "ui/base/ui_base_export.h"

namespace base {
class DictionaryValue;
}

namespace webos {

// A helper function that generates a string of HTML to be loaded.  The
// string includes the HTML and the javascript code necessary to generate the
// full page with support for both i18n Templates and JsTemplates.
void AppendJsonHtml(const base::DictionaryValue* json, std::string* output);
std::string GetTemplatesHtml(const base::StringPiece& html_template,
                             const base::DictionaryValue* json,
                             int err_code,
                             const base::StringPiece& template_id,
                             int viewport_width,
                             int viewport_height);

}  // namespace webos

#endif  // WEBOS_RENDERER_NET_TEMPLATE_BUILDER_H_
