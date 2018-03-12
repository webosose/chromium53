// Copyright (c) 2016-2018 LG Electronics, Inc.
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

extern const struct wl_interface wl_webos_xinput_interface;

static const struct wl_interface *types[] = {
	NULL,
	NULL,
	NULL,
	&wl_webos_xinput_interface,
};

static const struct wl_message wl_webos_xinput_requests[] = {
	{ "invoke_action", "uuu", types + 0 },
	{ "activated", "s", types + 0 },
	{ "deactivated", "", types + 0 },
};

static const struct wl_message wl_webos_xinput_events[] = {
	{ "activation_request", "s", types + 0 },
	{ "deactivated", "", types + 0 },
};

WL_EXPORT const struct wl_interface wl_webos_xinput_interface = {
	"wl_webos_xinput", 1,
	3, wl_webos_xinput_requests,
	2, wl_webos_xinput_events,
};

static const struct wl_message wl_webos_xinput_extension_requests[] = {
	{ "register_input", "n", types + 3 },
};

WL_EXPORT const struct wl_interface wl_webos_xinput_extension_interface = {
	"wl_webos_xinput_extension", 1,
	1, wl_webos_xinput_extension_requests,
	0, NULL,
};

