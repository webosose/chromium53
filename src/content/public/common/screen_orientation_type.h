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

#ifndef CONTENT_PUBLIC_COMMON_SCREEN_ORIENTATION_TYPE_H_
#define CONTENT_PUBLIC_COMMON_SCREEN_ORIENTATION_TYPE_H_

namespace content {

enum ScreenOrientationType {
  CONTENT_SCREEN_ORIENTATION_UNDEFINED = -1,
  CONTENT_SCREEN_ORIENTATION_0 = 0,
  CONTENT_SCREEN_ORIENTATION_90 = 90,
  CONTENT_SCREEN_ORIENTATION_180 = 180,
  CONTENT_SCREEN_ORIENTATION_270 = 270,
};
}

#endif  // CONTENT_PUBLIC_COMMON_SCREEN_ORIENTATION_TYPE_H_
