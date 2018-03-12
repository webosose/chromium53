// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/accessibility/accessibility_tree_formatter.h"

#include "base/logging.h"
#include "base/strings/string_number_conversions.h"
#include "base/strings/string_util.h"
#include "base/strings/stringprintf.h"
#include "base/strings/utf_string_conversions.h"
#include "content/browser/accessibility/webos/browser_accessibility_webos.h"

namespace content {

class AccessibilityTreeFormatterWebos : public AccessibilityTreeFormatter {
 public:
  explicit AccessibilityTreeFormatterWebos();
  ~AccessibilityTreeFormatterWebos() override;

 private:
  const base::FilePath::StringType GetExpectedFileSuffix() override;
  const std::string GetAllowEmptyString() override;
  const std::string GetAllowString() override;
  const std::string GetDenyString() override;
  void AddProperties(const BrowserAccessibility& node,
                     base::DictionaryValue* dict) override;
  base::string16 ToString(const base::DictionaryValue& node) override;
};

// static
AccessibilityTreeFormatter* AccessibilityTreeFormatter::Create() {
  return new AccessibilityTreeFormatterWebos();
}

AccessibilityTreeFormatterWebos::AccessibilityTreeFormatterWebos() {}

AccessibilityTreeFormatterWebos::~AccessibilityTreeFormatterWebos() {}

void AccessibilityTreeFormatterWebos::AddProperties(
    const BrowserAccessibility& node,
    base::DictionaryValue* dict) {
}

base::string16 AccessibilityTreeFormatterWebos::ToString(
    const base::DictionaryValue& node) {
  return base::string16();
}

const base::FilePath::StringType
AccessibilityTreeFormatterWebos::GetExpectedFileSuffix() {
  return FILE_PATH_LITERAL("");
}

const std::string AccessibilityTreeFormatterWebos::GetAllowEmptyString() {
  return "";
}

const std::string AccessibilityTreeFormatterWebos::GetAllowString() {
  return "";
}

const std::string AccessibilityTreeFormatterWebos::GetDenyString() {
  return "";
}
}
