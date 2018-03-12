// Copyright (c) 2014-2018 LG Electronics, Inc.
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

#include <stdlib.h>
#include <stdint.h>
#include "wayland-util.h"

extern const struct wl_interface wl_surface_interface;
extern const struct wl_interface wl_webos_surface_group_interface;
extern const struct wl_interface wl_webos_surface_group_layer_interface;

static const struct wl_interface *types[] = {
	NULL,
	&wl_webos_surface_group_interface,
	&wl_surface_interface,
	NULL,
	&wl_webos_surface_group_interface,
	NULL,
	&wl_webos_surface_group_layer_interface,
	NULL,
	NULL,
	&wl_surface_interface,
	NULL,
	&wl_surface_interface,
	NULL,
	&wl_surface_interface,
};

static const struct wl_message wl_webos_surface_group_compositor_requests[] = {
	{ "create_surface_group", "nos", types + 1 },
	{ "get_surface_group", "ns", types + 4 },
};

WL_EXPORT const struct wl_interface wl_webos_surface_group_compositor_interface = {
	"wl_webos_surface_group_compositor", 1,
	2, wl_webos_surface_group_compositor_requests,
	0, NULL,
};

static const struct wl_message wl_webos_surface_group_requests[] = {
	{ "create_layer", "nsi", types + 6 },
	{ "attach", "os", types + 9 },
	{ "attach_anonymous", "ou", types + 11 },
	{ "allow_anonymous_layers", "u", types + 0 },
	{ "detach", "o", types + 13 },
	{ "focus_owner", "", types + 0 },
	{ "focus_layer", "s", types + 0 },
	{ "destroy", "", types + 0 },
};

static const struct wl_message wl_webos_surface_group_events[] = {
	{ "owner_destroyed", "", types + 0 },
};

WL_EXPORT const struct wl_interface wl_webos_surface_group_interface = {
	"wl_webos_surface_group", 1,
	8, wl_webos_surface_group_requests,
	1, wl_webos_surface_group_events,
};

static const struct wl_message wl_webos_surface_group_layer_requests[] = {
	{ "set_z_index", "i", types + 0 },
	{ "destroy", "", types + 0 },
};

static const struct wl_message wl_webos_surface_group_layer_events[] = {
	{ "surface_attached", "", types + 0 },
	{ "surface_detached", "", types + 0 },
};

WL_EXPORT const struct wl_interface wl_webos_surface_group_layer_interface = {
	"wl_webos_surface_group_layer", 1,
	2, wl_webos_surface_group_layer_requests,
	2, wl_webos_surface_group_layer_events,
};

