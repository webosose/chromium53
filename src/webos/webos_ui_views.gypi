# Copyright 2016 LG Electronics, Inc. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'sources': [
    '<(DEPTH)/webos/ui/views/frame/webos_native_widget_aura.cc',
    '<(DEPTH)/webos/ui/views/frame/webos_native_widget_aura.h',
    '<(DEPTH)/webos/ui/views/frame/webos_root_view.cc',
    '<(DEPTH)/webos/ui/views/frame/webos_root_view.h',
    '<(DEPTH)/webos/ui/views/frame/webos_view.cc',
    '<(DEPTH)/webos/ui/views/frame/webos_view.h',
    '<(DEPTH)/webos/ui/views/frame/webos_widget.cc',
    '<(DEPTH)/webos/ui/views/frame/webos_widget.h',
    '<(DEPTH)/webos/ui/views/frame/webos_widget_view.cc',
    '<(DEPTH)/webos/ui/views/frame/webos_widget_view.h',
  ],
  'dependencies': [
    '<(DEPTH)/base/base.gyp:base',
    '<(DEPTH)/skia/skia.gyp:skia',
    '<(DEPTH)/ui/base/ui_base.gyp:ui_base',
    '<(DEPTH)/ui/events/events.gyp:events',
    '<(DEPTH)/ui/gfx/gfx.gyp:gfx',
    '<(DEPTH)/ui/gfx/gfx.gyp:gfx_geometry',
    '<(DEPTH)/url/url.gyp:url_lib',
  ],
}
