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

#ifndef WAYLAND_WEBOS_SURFACE_GROUP_
#define WAYLAND_WEBOS_SURFACE_GROUP_

#include <string>
#include "ozone/wayland/protocol/wayland_webos_surface_group_client_protocol.h"

namespace ozonewayland {
class wl_webos_surface_group_compositor {
 public:
  wl_webos_surface_group_compositor(struct ::wl_registry *registry, int id);
  wl_webos_surface_group_compositor(
      struct ::wl_webos_surface_group_compositor *object);
  wl_webos_surface_group_compositor();

  virtual ~wl_webos_surface_group_compositor();

  void init(struct ::wl_registry *registry, int id);
  void init(struct ::wl_webos_surface_group_compositor *object);

  struct ::wl_webos_surface_group_compositor *object();
  const struct ::wl_webos_surface_group_compositor *object() const;

  bool isInitialized() const;

  struct ::wl_webos_surface_group *create_surface_group(
      struct ::wl_surface *parent, const std::string &name);
  struct ::wl_webos_surface_group *get_surface_group(const std::string &name);

 private:
  struct ::wl_webos_surface_group_compositor *wl_webos_surface_group_compositor_;
};

class wl_webos_surface_group {
 public:
  wl_webos_surface_group(struct ::wl_registry *registry, int id);
  wl_webos_surface_group(struct ::wl_webos_surface_group *object);
  wl_webos_surface_group();

  virtual ~wl_webos_surface_group();

  void init(struct ::wl_registry *registry, int id);
  void init(struct ::wl_webos_surface_group *object);

  struct ::wl_webos_surface_group *object();
  const struct ::wl_webos_surface_group *object() const;

  bool isInitialized() const;

  enum z_hint {
   z_hint_below = 0, // Below the root surface and the lowest named layer
   z_hint_above = 1, // Above the root surface and above the highest named layer
   z_hint_top = 2 // Above all of the anonymous surfaces
  };

  struct ::wl_webos_surface_group_layer *create_layer(
      const std::string &name, int32_t z_index);
  void attach(struct ::wl_surface *surface, const std::string &layer_name);
  void attach_anonymous(struct ::wl_surface *surface, uint32_t z_hint);
  void allow_anonymous_layers(uint32_t allow);
  void detach(struct ::wl_surface *surface);
  void focus_owner();
  void focus_layer(const std::string &layer);
  void destroy();

 protected:
  virtual void webos_surface_group_owner_destroyed();

 private:
  void init_listener();
  static const struct wl_webos_surface_group_listener wl_webos_surface_group_listener_;
  static void handle_owner_destroyed(
      void *data,
      struct ::wl_webos_surface_group *object);
      struct ::wl_webos_surface_group *wl_webos_surface_group_;
};

class wl_webos_surface_group_layer {
 public:
  wl_webos_surface_group_layer(struct ::wl_registry *registry, int id);
  wl_webos_surface_group_layer(struct ::wl_webos_surface_group_layer *object);
  wl_webos_surface_group_layer();

  virtual ~wl_webos_surface_group_layer();

  void init(struct ::wl_registry *registry, int id);
  void init(struct ::wl_webos_surface_group_layer *object);

  struct ::wl_webos_surface_group_layer *object();
  const struct ::wl_webos_surface_group_layer *object() const;

  bool isInitialized() const;

  void set_z_index(int32_t z_index);
  void destroy();

 protected:
  virtual void webos_surface_group_layer_surface_attached();
  virtual void webos_surface_group_layer_surface_detached();

 private:
  void init_listener();
  static const struct wl_webos_surface_group_layer_listener wl_webos_surface_group_layer_listener_;
  static void handle_surface_attached(
      void *data,
      struct ::wl_webos_surface_group_layer *object);
  static void handle_surface_detached(
      void *data,
      struct ::wl_webos_surface_group_layer *object);
      struct ::wl_webos_surface_group_layer *wl_webos_surface_group_layer_;
};

}

#endif
