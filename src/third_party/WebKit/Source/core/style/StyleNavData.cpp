// Copyright (c) 2010-2018 LG Electronics, Inc.
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

#include "StyleNavData.h"

namespace blink {

StyleNavData::StyleNavData()
    : flag(NAV_TARGET_NONE)
{
}

StyleNavData::StyleNavData(const StyleNavData& o)
    : RefCounted<StyleNavData>()
    , id(o.id)
    , target(o.target)
    , flag(o.flag)
{
}

bool StyleNavData::operator==(const StyleNavData& o) const
{
    return flag == o.flag && id == o.id && target == o.target;
}

StyleNavIndex::StyleNavIndex()
    : isAuto(true)
{
}

StyleNavIndex::StyleNavIndex(const StyleNavIndex& o)
    : RefCounted<StyleNavIndex>()
    , idx(o.idx)
    , isAuto(o.isAuto)
{
}

bool StyleNavIndex::operator==(const StyleNavIndex& o) const
{
    return ((isAuto || o.isAuto) ? isAuto == o.isAuto : idx == o.idx);
}

} // namespace blink
