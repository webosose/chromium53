// Copyright 2017 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "webos/renderer/net/template_builder.h"

#include "base/json/json_file_value_serializer.h"
#include "base/json/json_string_value_serializer.h"
#include "base/logging.h"
#include "base/strings/string_util.h"
#include "grit/webos_inspector_resources.h"
#include "ui/base/layout.h"
#include "ui/base/resource/resource_bundle.h"
#include "ui/base/template_expressions.h"

using namespace ui;

namespace webos {
// Appends the source for i18n Templates in a script tag.
void AppendI18nTemplateSourceHtml(std::string* output) {
  base::StringPiece err_page_template(
      ResourceBundle::GetSharedInstance().GetRawDataResource(
          IDR_WEBOS_NETWORK_ERROR_PAGE_TEMPLATE_JS));

  // if (err_page_template.empty()) {
  //   NOTREACHED() << "Unable to get i18n template src";
  //   return;
  // }

  output->append("<script>");
  err_page_template.AppendToString(output);
  output->append("</script>");
}

void AppendJsonJS(const base::DictionaryValue* json, std::string* output) {
  // Convert the template data to a json string.
  DCHECK(json) << "must include json data structure";

  std::string jstext;
  JSONStringValueSerializer serializer(&jstext);
  serializer.Serialize(*json);
  output->append("error_info = ");
  output->append(jstext);
  output->append(";");
}

void AppendJsonHtml(const base::DictionaryValue* json, std::string* output) {
  std::string javascript_string;
  AppendJsonJS(json, &javascript_string);

  // </ confuses the HTML parser because it could be a </script> tag.  So we
  // replace </ with <\/.  The extra \ will be ignored by the JS engine.
  base::ReplaceSubstringsAfterOffset(&javascript_string, 0, "</", "<\\/");

  output->append("<script>");
  output->append(javascript_string);
  output->append("</script>");
}

std::string GetTemplatesHtml(const base::StringPiece& html_template,
                             const base::DictionaryValue* json,
                             int err_code,
                             const base::StringPiece& template_id,
                             int viewport_width,
                             int viewport_height) {
  std::string output(html_template.data(), html_template.size());

  output.append("<script>");
  output.append("var errorCode = ");
  output.append(std::to_string(err_code));
  output.append(";");

  output.append("var appResolutionWidth = ");
  output.append(std::to_string(viewport_width));
  output.append(";");

  output.append("var appResolutionHeight = ");
  output.append(std::to_string(viewport_height));
  output.append(";");
  output.append("</script>");

  AppendJsonHtml(json, &output);
  AppendI18nTemplateSourceHtml(&output);

  return output;
}

}  // namespace webos
