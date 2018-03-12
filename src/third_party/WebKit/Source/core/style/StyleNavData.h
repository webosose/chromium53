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

#ifndef StyleNavData_h
#define StyleNavData_h

#include <wtf/RefCounted.h>
#include <wtf/PassRefPtr.h>
#include "wtf/text/WTFString.h"

namespace blink {

class StyleNavData : public RefCounted<StyleNavData> {
public:

    enum ENavTarget {
        NAV_TARGET_NONE    = 0,
        NAV_TARGET_CURRENT = 1,
        NAV_TARGET_NAME    = 2,
        NAV_TARGET_ROOT    = 4
    };

    static PassRefPtr<StyleNavData> create() { return adoptRef(new StyleNavData); }
    PassRefPtr<StyleNavData> copy() const { return adoptRef(new StyleNavData(*this)); }

    bool operator==(const StyleNavData& o) const;
    bool operator!=(const StyleNavData& o) const { return !(*this == o); }

    WTF::String id;
    WTF::String target;
    ENavTarget flag;

private:
    StyleNavData();
    StyleNavData(const StyleNavData&);
};

class StyleNavIndex : public RefCounted<StyleNavIndex> {
public:
    static PassRefPtr<StyleNavIndex> create() { return adoptRef(new StyleNavIndex); }
    PassRefPtr<StyleNavIndex> copy() const { return adoptRef(new StyleNavIndex(*this)); }

    bool operator==(const StyleNavIndex& o) const;
    bool operator!=(const StyleNavIndex& o) const { return !(*this == o); }

    int idx;
    bool isAuto;

private:
    StyleNavIndex();
    StyleNavIndex(const StyleNavIndex&);
};

} // namespace blink

#endif // StyleNavData_h
