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

#ifndef WEBOS_SURFACE_GROUP_CLIENT_PROTOCOL_H
#define WEBOS_SURFACE_GROUP_CLIENT_PROTOCOL_H

#ifdef  __cplusplus
extern "C" {
#endif

#include <stdint.h>
#include <stddef.h>
#include "wayland-client.h"

struct wl_client;
struct wl_resource;

struct wl_webos_surface_group_compositor;
struct wl_webos_surface_group;
struct wl_webos_surface_group_layer;

extern const struct wl_interface wl_webos_surface_group_compositor_interface;
extern const struct wl_interface wl_webos_surface_group_interface;
extern const struct wl_interface wl_webos_surface_group_layer_interface;

#define WL_WEBOS_SURFACE_GROUP_COMPOSITOR_CREATE_SURFACE_GROUP	0
#define WL_WEBOS_SURFACE_GROUP_COMPOSITOR_GET_SURFACE_GROUP	1

static inline void
wl_webos_surface_group_compositor_set_user_data(struct wl_webos_surface_group_compositor *wl_webos_surface_group_compositor, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_webos_surface_group_compositor, user_data);
}

static inline void *
wl_webos_surface_group_compositor_get_user_data(struct wl_webos_surface_group_compositor *wl_webos_surface_group_compositor)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_webos_surface_group_compositor);
}

static inline void
wl_webos_surface_group_compositor_destroy(struct wl_webos_surface_group_compositor *wl_webos_surface_group_compositor)
{
	wl_proxy_destroy((struct wl_proxy *) wl_webos_surface_group_compositor);
}

static inline struct wl_webos_surface_group *
wl_webos_surface_group_compositor_create_surface_group(struct wl_webos_surface_group_compositor *wl_webos_surface_group_compositor, struct wl_surface *parent, const char *name)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_webos_surface_group_compositor,
			 WL_WEBOS_SURFACE_GROUP_COMPOSITOR_CREATE_SURFACE_GROUP, &wl_webos_surface_group_interface, NULL, parent, name);

	return (struct wl_webos_surface_group *) id;
}

static inline struct wl_webos_surface_group *
wl_webos_surface_group_compositor_get_surface_group(struct wl_webos_surface_group_compositor *wl_webos_surface_group_compositor, const char *name)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_webos_surface_group_compositor,
			 WL_WEBOS_SURFACE_GROUP_COMPOSITOR_GET_SURFACE_GROUP, &wl_webos_surface_group_interface, NULL, name);

	return (struct wl_webos_surface_group *) id;
}

#ifndef WL_WEBOS_SURFACE_GROUP_Z_HINT_ENUM
#define WL_WEBOS_SURFACE_GROUP_Z_HINT_ENUM
/**
 * wl_webos_surface_group_z_hint - Hint for the z position of the
 *	anonymous surfaces
 * @WL_WEBOS_SURFACE_GROUP_Z_HINT_BELOW: Below the root surface and the
 *	lowest named layer
 * @WL_WEBOS_SURFACE_GROUP_Z_HINT_ABOVE: Above the root surface and above
 *	the highest named layer
 * @WL_WEBOS_SURFACE_GROUP_Z_HINT_TOP: Above all of the anonymous
 *	surfaces
 *
 * The position of anonymous layer is always relative to the root element
 * and the named layers in this group.
 *
 * If there are multiple entries with the same index designator they will
 * get positioned in the order of which they get attached.
 */
enum wl_webos_surface_group_z_hint {
	WL_WEBOS_SURFACE_GROUP_Z_HINT_BELOW = 0,
	WL_WEBOS_SURFACE_GROUP_Z_HINT_ABOVE = 1,
	WL_WEBOS_SURFACE_GROUP_Z_HINT_TOP = 2,
};
#endif /* WL_WEBOS_SURFACE_GROUP_Z_HINT_ENUM */

struct wl_webos_surface_group_listener {
	/**
	 * owner_destroyed - Sent to clients attached to this group
	 *
	 * If the owner crashes or normally destroys this group the
	 * attached client will receive this notification.
	 *
	 * Since compositor does not know what to do with client surface
	 * still attached to this group it will not show them. A well
	 * behaving client will 'detach' from this group and release its
	 * assosicated resource
	 */
	void (*owner_destroyed)(void *data,
				struct wl_webos_surface_group *wl_webos_surface_group);
};

static inline int
wl_webos_surface_group_add_listener(struct wl_webos_surface_group *wl_webos_surface_group,
				    const struct wl_webos_surface_group_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_webos_surface_group,
				     (void (**)(void)) listener, data);
}

#define WL_WEBOS_SURFACE_GROUP_CREATE_LAYER	0
#define WL_WEBOS_SURFACE_GROUP_ATTACH	1
#define WL_WEBOS_SURFACE_GROUP_ATTACH_ANONYMOUS	2
#define WL_WEBOS_SURFACE_GROUP_ALLOW_ANONYMOUS_LAYERS	3
#define WL_WEBOS_SURFACE_GROUP_DETACH	4
#define WL_WEBOS_SURFACE_GROUP_FOCUS_OWNER	5
#define WL_WEBOS_SURFACE_GROUP_FOCUS_LAYER	6
#define WL_WEBOS_SURFACE_GROUP_DESTROY	7

static inline void
wl_webos_surface_group_set_user_data(struct wl_webos_surface_group *wl_webos_surface_group, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_webos_surface_group, user_data);
}

static inline void *
wl_webos_surface_group_get_user_data(struct wl_webos_surface_group *wl_webos_surface_group)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_webos_surface_group);
}

static inline struct wl_webos_surface_group_layer *
wl_webos_surface_group_create_layer(struct wl_webos_surface_group *wl_webos_surface_group, const char *name, int32_t z_index)
{
	struct wl_proxy *id;

	id = wl_proxy_marshal_constructor((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_CREATE_LAYER, &wl_webos_surface_group_layer_interface, NULL, name, z_index);

	return (struct wl_webos_surface_group_layer *) id;
}

static inline void
wl_webos_surface_group_attach(struct wl_webos_surface_group *wl_webos_surface_group, struct wl_surface *surface, const char *layer_name)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_ATTACH, surface, layer_name);
}

static inline void
wl_webos_surface_group_attach_anonymous(struct wl_webos_surface_group *wl_webos_surface_group, struct wl_surface *surface, uint32_t z_hint)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_ATTACH_ANONYMOUS, surface, z_hint);
}

static inline void
wl_webos_surface_group_allow_anonymous_layers(struct wl_webos_surface_group *wl_webos_surface_group, uint32_t allow)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_ALLOW_ANONYMOUS_LAYERS, allow);
}

static inline void
wl_webos_surface_group_detach(struct wl_webos_surface_group *wl_webos_surface_group, struct wl_surface *surface)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_DETACH, surface);
}

static inline void
wl_webos_surface_group_focus_owner(struct wl_webos_surface_group *wl_webos_surface_group)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_FOCUS_OWNER);
}

static inline void
wl_webos_surface_group_focus_layer(struct wl_webos_surface_group *wl_webos_surface_group, const char *layer)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_FOCUS_LAYER, layer);
}

static inline void
wl_webos_surface_group_destroy(struct wl_webos_surface_group *wl_webos_surface_group)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group,
			 WL_WEBOS_SURFACE_GROUP_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) wl_webos_surface_group);
}

/**
 * wl_webos_surface_group_layer - The handle for the group owner to
 *	manipulate the layer
 * @surface_attached: Notify surface-group-owner that surface is attached
 *	to this layer
 * @surface_detached: Notify surface-group-owner that surface is detached
 *	from this layer
 *
 * The handle can be used to change the z order of a particular layer
 */
struct wl_webos_surface_group_layer_listener {
	/**
	 * surface_attached - Notify surface-group-owner that surface is
	 *	attached to this layer
	 *
	 * When any surafce is attached to this layer, the compositor
	 * needs to notify surface-group-owner.
	 */
	void (*surface_attached)(void *data,
				 struct wl_webos_surface_group_layer *wl_webos_surface_group_layer);
	/**
	 * surface_detached - Notify surface-group-owner that surface is
	 *	detached from this layer
	 *
	 * When any surafce is detached from this layer, the compositor
	 * needs to notify surface-group-owner. This will be usefull if
	 * surface attached to this layer is destroyed by associated
	 * surface-group-client.
	 */
	void (*surface_detached)(void *data,
				 struct wl_webos_surface_group_layer *wl_webos_surface_group_layer);
};

static inline int
wl_webos_surface_group_layer_add_listener(struct wl_webos_surface_group_layer *wl_webos_surface_group_layer,
					  const struct wl_webos_surface_group_layer_listener *listener, void *data)
{
	return wl_proxy_add_listener((struct wl_proxy *) wl_webos_surface_group_layer,
				     (void (**)(void)) listener, data);
}

#define WL_WEBOS_SURFACE_GROUP_LAYER_SET_Z_INDEX	0
#define WL_WEBOS_SURFACE_GROUP_LAYER_DESTROY	1

static inline void
wl_webos_surface_group_layer_set_user_data(struct wl_webos_surface_group_layer *wl_webos_surface_group_layer, void *user_data)
{
	wl_proxy_set_user_data((struct wl_proxy *) wl_webos_surface_group_layer, user_data);
}

static inline void *
wl_webos_surface_group_layer_get_user_data(struct wl_webos_surface_group_layer *wl_webos_surface_group_layer)
{
	return wl_proxy_get_user_data((struct wl_proxy *) wl_webos_surface_group_layer);
}

static inline void
wl_webos_surface_group_layer_set_z_index(struct wl_webos_surface_group_layer *wl_webos_surface_group_layer, int32_t z_index)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group_layer,
			 WL_WEBOS_SURFACE_GROUP_LAYER_SET_Z_INDEX, z_index);
}

static inline void
wl_webos_surface_group_layer_destroy(struct wl_webos_surface_group_layer *wl_webos_surface_group_layer)
{
	wl_proxy_marshal((struct wl_proxy *) wl_webos_surface_group_layer,
			 WL_WEBOS_SURFACE_GROUP_LAYER_DESTROY);

	wl_proxy_destroy((struct wl_proxy *) wl_webos_surface_group_layer);
}

#ifdef  __cplusplus
}
#endif

#endif
