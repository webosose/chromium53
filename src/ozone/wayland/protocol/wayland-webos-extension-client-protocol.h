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

#ifndef WEBOS_EXTENSION_CLIENT_PROTOCOL_H
#define WEBOS_EXTENSION_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_webos_xinput;
struct wl_webos_xinput_extension;

extern const struct wl_interface wl_webos_xinput_interface;
extern const struct wl_interface wl_webos_xinput_extension_interface;

#ifndef WL_WEBOS_XINPUT_KEYSYM_TYPE_ENUM
#define WL_WEBOS_XINPUT_KEYSYM_TYPE_ENUM
/**
 * wl_webos_xinput_keysym_type - The type for the symbol
 * @WL_WEBOS_XINPUT_KEYSYM_TYPE_QT: (none)
 * @WL_WEBOS_XINPUT_KEYSYM_TYPE_NATIVE: (none)
 *
 * Describes the type of the symbol. The compositor will do the necessary
 * conversion before sending it the the current fullscreen surface
 */
enum wl_webos_xinput_keysym_type {
	WL_WEBOS_XINPUT_KEYSYM_TYPE_QT = 1,
	WL_WEBOS_XINPUT_KEYSYM_TYPE_NATIVE = 2,
};
#endif /* WL_WEBOS_XINPUT_KEYSYM_TYPE_ENUM */

#ifndef WL_WEBOS_XINPUT_EVENT_TYPE_ENUM
#define WL_WEBOS_XINPUT_EVENT_TYPE_ENUM
/**
 * wl_webos_xinput_event_type - The type of the event
 * @WL_WEBOS_XINPUT_EVENT_TYPE_PRESS_AND_RELEASE: (none)
 * @WL_WEBOS_XINPUT_EVENT_TYPE_PRESS: (none)
 * @WL_WEBOS_XINPUT_EVENT_TYPE_RELEASE: (none)
 *
 * Describes the type of the event. "press_and_release" means the
 * compositor will send a released event after a pressed event
 * automatically.
 */
enum wl_webos_xinput_event_type {
	WL_WEBOS_XINPUT_EVENT_TYPE_PRESS_AND_RELEASE = 0,
	WL_WEBOS_XINPUT_EVENT_TYPE_PRESS = 1,
	WL_WEBOS_XINPUT_EVENT_TYPE_RELEASE = 2,
};
#endif /* WL_WEBOS_XINPUT_EVENT_TYPE_ENUM */

/**
 * wl_webos_xinput - interface for adding extended inputs
 * @activation_request: Requests activation
 * @deactivated: (none)
 *
 * An extended input is something other than keyboard, mouse or remote.
 */
struct wl_webos_xinput_listener {
	/**
	 * activation_request - Requests activation
	 * @type: (none)
	 *
	 * Requests that the method matching the argument string is
	 * activated. The method is responsible for replying back with the
	 * activated request. Currently supported methods are: "voice",
	 * "screen_remote", "hot_key_XXXX"
	 *
	 * NOTE: All the registered methods will get the activation request
	 * and thus the client is responsible for performing filtering and
	 * activating itself when it knows that it can handle the type.
	 */
	void (*activation_request)(void *data,
				   struct wl_webos_xinput *wl_webos_xinput,
				   const char *type);
	/**
	 * deactivated - (none)
	 */
	void (*deactivated)(void *data,
			    struct wl_webos_xinput *wl_webos_xinput);
};

static inline int
wl_webos_xinput_add_listener(struct wl_webos_xinput *wl_webos_xinput,
			     const struct wl_webos_xinput_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_webos_xinput,
				     (void (**)(void)) listener, data);
}

#define WL_WEBOS_XINPUT_INVOKE_ACTION	0
#define WL_WEBOS_XINPUT_ACTIVATED	1
#define WL_WEBOS_XINPUT_DEACTIVATED	2

static inline void
wl_webos_xinput_set_user_data(struct wl_webos_xinput *wl_webos_xinput, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_webos_xinput, user_data);
}

static inline void *
wl_webos_xinput_get_user_data(struct wl_webos_xinput *wl_webos_xinput)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_webos_xinput);
}

static inline void
wl_webos_xinput_destroy(struct wl_webos_xinput *wl_webos_xinput)
{
	wl_proxy_destroy((struct wl_proxy *) wl_webos_xinput);
}

static inline void
wl_webos_xinput_invoke_action(struct wl_webos_xinput *wl_webos_xinput, uint32_t keysym, uint32_t symbol_type, uint32_t event_type)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_xinput,
			 WL_WEBOS_XINPUT_INVOKE_ACTION, keysym, symbol_type, event_type);
}

static inline void
wl_webos_xinput_activated(struct wl_webos_xinput *wl_webos_xinput, const char *type)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_xinput,
			 WL_WEBOS_XINPUT_ACTIVATED, type);
}

static inline void
wl_webos_xinput_deactivated(struct wl_webos_xinput *wl_webos_xinput)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_xinput,
			 WL_WEBOS_XINPUT_DEACTIVATED);
}

#define WL_WEBOS_XINPUT_EXTENSION_REGISTER_INPUT	0

static inline void
wl_webos_xinput_extension_set_user_data(struct wl_webos_xinput_extension *wl_webos_xinput_extension, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_webos_xinput_extension, user_data);
}

static inline void *
wl_webos_xinput_extension_get_user_data(struct wl_webos_xinput_extension *wl_webos_xinput_extension)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_webos_xinput_extension);
}

static inline void
wl_webos_xinput_extension_destroy(struct wl_webos_xinput_extension *wl_webos_xinput_extension)
{
	wl_proxy_destroy((struct wl_proxy *) wl_webos_xinput_extension);
}

static inline struct wl_webos_xinput *
wl_webos_xinput_extension_register_input(struct wl_webos_xinput_extension *wl_webos_xinput_extension)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_webos_xinput_extension,
			 WL_WEBOS_XINPUT_EXTENSION_REGISTER_INPUT, &wl_webos_xinput_interface, NULL);

	return (struct wl_webos_xinput *) id;
}

#ifdef  __cplusplus
}
#endif

#endif
