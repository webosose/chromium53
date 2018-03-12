// Copyright 2016 LG Electronics, Inc. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "wayland_webos_surface_group.h"

namespace ozonewayland {
wl_webos_surface_group_compositor::wl_webos_surface_group_compositor(
    struct ::wl_registry *registry, int id) {
  init(registry, id);
}

wl_webos_surface_group_compositor::wl_webos_surface_group_compositor(
    struct ::wl_webos_surface_group_compositor *obj)
    : wl_webos_surface_group_compositor_(obj) {
}

wl_webos_surface_group_compositor::wl_webos_surface_group_compositor()
    : wl_webos_surface_group_compositor_(0) {
}

wl_webos_surface_group_compositor::~wl_webos_surface_group_compositor() {
}

void wl_webos_surface_group_compositor::init(
    struct ::wl_registry *registry, int id) {
  wl_webos_surface_group_compositor_ =
    static_cast<struct ::wl_webos_surface_group_compositor *>(wl_registry_bind(
         registry, id, &wl_webos_surface_group_compositor_interface, 1));
}

void wl_webos_surface_group_compositor::init(
    struct ::wl_webos_surface_group_compositor *obj) {
  wl_webos_surface_group_compositor_ = obj;
}

struct ::wl_webos_surface_group_compositor *
wl_webos_surface_group_compositor::object() {
  return wl_webos_surface_group_compositor_;
}

const struct ::wl_webos_surface_group_compositor *
wl_webos_surface_group_compositor::object() const {
  return wl_webos_surface_group_compositor_;
}

bool wl_webos_surface_group_compositor::isInitialized() const {
  return wl_webos_surface_group_compositor_ != 0;
}

struct ::wl_webos_surface_group *
wl_webos_surface_group_compositor::create_surface_group(
    struct ::wl_surface *parent, const std::string &name) {
  return wl_webos_surface_group_compositor_create_surface_group(
      wl_webos_surface_group_compositor_, parent, name.c_str());
}

struct ::wl_webos_surface_group *
wl_webos_surface_group_compositor::get_surface_group(
    const std::string &name) {
  return wl_webos_surface_group_compositor_get_surface_group(
      wl_webos_surface_group_compositor_, name.c_str());
}

wl_webos_surface_group::wl_webos_surface_group(
    struct ::wl_registry *registry, int id) {
  init(registry, id);
}

wl_webos_surface_group::wl_webos_surface_group(
    struct ::wl_webos_surface_group *obj)
    : wl_webos_surface_group_(obj) {
  init_listener();
}

wl_webos_surface_group::wl_webos_surface_group()
    : wl_webos_surface_group_(0) {
}

wl_webos_surface_group::~wl_webos_surface_group() {
}

void wl_webos_surface_group::init(struct ::wl_registry *registry, int id) {
  wl_webos_surface_group_ =
    static_cast<struct ::wl_webos_surface_group *>(wl_registry_bind(
        registry, id, &wl_webos_surface_group_interface, 1));

  init_listener();
}

void wl_webos_surface_group::init(struct ::wl_webos_surface_group *obj) {
  wl_webos_surface_group_ = obj;
  init_listener();
}

struct ::wl_webos_surface_group* wl_webos_surface_group::object() {
  return wl_webos_surface_group_;
}

const struct ::wl_webos_surface_group* wl_webos_surface_group::object() const {
  return wl_webos_surface_group_;
}

bool wl_webos_surface_group::isInitialized() const {
  return wl_webos_surface_group_ != 0;
}

struct ::wl_webos_surface_group_layer *wl_webos_surface_group::create_layer(
    const std::string &name, int32_t z_index) {
  return wl_webos_surface_group_create_layer(
      wl_webos_surface_group_, name.c_str(), z_index);
}

void wl_webos_surface_group::attach(struct ::wl_surface *surface,
                                    const std::string &layer_name) {
  wl_webos_surface_group_attach(wl_webos_surface_group_, surface,
                                layer_name.c_str());
}

void wl_webos_surface_group::attach_anonymous(struct ::wl_surface *surface,
                                              uint32_t z_hint) {
  wl_webos_surface_group_attach_anonymous(wl_webos_surface_group_,
                                          surface, z_hint);
}

void wl_webos_surface_group::allow_anonymous_layers(uint32_t allow) {
  wl_webos_surface_group_allow_anonymous_layers(wl_webos_surface_group_,
                                                allow);
}

void wl_webos_surface_group::detach(struct ::wl_surface *surface) {
  wl_webos_surface_group_detach(wl_webos_surface_group_, surface);
}

void wl_webos_surface_group::focus_owner() {
  wl_webos_surface_group_focus_owner(wl_webos_surface_group_);
}

void wl_webos_surface_group::focus_layer(const std::string &layer) {
  wl_webos_surface_group_focus_layer(wl_webos_surface_group_, layer.c_str());
}

void wl_webos_surface_group::destroy() {
  wl_webos_surface_group_destroy(wl_webos_surface_group_);
}

void wl_webos_surface_group::webos_surface_group_owner_destroyed() {
}

void wl_webos_surface_group::handle_owner_destroyed(
    void *data, struct ::wl_webos_surface_group *object) {
  static_cast<wl_webos_surface_group *>(data)->webos_surface_group_owner_destroyed();
}

const struct wl_webos_surface_group_listener
wl_webos_surface_group::wl_webos_surface_group_listener_ = {
  wl_webos_surface_group::handle_owner_destroyed
};

void wl_webos_surface_group::init_listener() {
  wl_webos_surface_group_add_listener(
      wl_webos_surface_group_, &wl_webos_surface_group_listener_, this);
}

wl_webos_surface_group_layer::wl_webos_surface_group_layer(
    struct ::wl_registry *registry, int id) {
  init(registry, id);
}

wl_webos_surface_group_layer::wl_webos_surface_group_layer(
    struct ::wl_webos_surface_group_layer *obj)
    : wl_webos_surface_group_layer_(obj)  {
  init_listener();
}

wl_webos_surface_group_layer::wl_webos_surface_group_layer()
    : wl_webos_surface_group_layer_(0) {
}

wl_webos_surface_group_layer::~wl_webos_surface_group_layer() {
}

void wl_webos_surface_group_layer::init(
    struct ::wl_registry *registry, int id) {
  wl_webos_surface_group_layer_ =
     static_cast<struct ::wl_webos_surface_group_layer *>(wl_registry_bind(
       registry, id, &wl_webos_surface_group_layer_interface, 1));

  init_listener();
}

void wl_webos_surface_group_layer::init(
    struct ::wl_webos_surface_group_layer *obj) {
  wl_webos_surface_group_layer_ = obj;
  init_listener();
}

struct ::wl_webos_surface_group_layer* wl_webos_surface_group_layer::object() {
  return wl_webos_surface_group_layer_;
}

const struct ::wl_webos_surface_group_layer *
wl_webos_surface_group_layer::object() const {
  return wl_webos_surface_group_layer_;
}

bool wl_webos_surface_group_layer::isInitialized() const {
  return wl_webos_surface_group_layer_ != 0;
}

void wl_webos_surface_group_layer::set_z_index(int32_t z_index) {
  wl_webos_surface_group_layer_set_z_index(
      wl_webos_surface_group_layer_, z_index);
}

void wl_webos_surface_group_layer::destroy() {
  wl_webos_surface_group_layer_destroy(wl_webos_surface_group_layer_);
}

void wl_webos_surface_group_layer::webos_surface_group_layer_surface_attached() {
}

void wl_webos_surface_group_layer::handle_surface_attached(
    void *data, struct ::wl_webos_surface_group_layer *object) {
  static_cast<wl_webos_surface_group_layer *>(data)->webos_surface_group_layer_surface_attached();
}

void wl_webos_surface_group_layer::webos_surface_group_layer_surface_detached() {
}

void wl_webos_surface_group_layer::handle_surface_detached(
    void *data, struct ::wl_webos_surface_group_layer *object) {
  static_cast<wl_webos_surface_group_layer *>(data)->webos_surface_group_layer_surface_detached();
}

const struct wl_webos_surface_group_layer_listener
wl_webos_surface_group_layer::wl_webos_surface_group_layer_listener_ = {
    wl_webos_surface_group_layer::handle_surface_attached,
    wl_webos_surface_group_layer::handle_surface_detached
};

void wl_webos_surface_group_layer::init_listener() {
  wl_webos_surface_group_layer_add_listener(
      wl_webos_surface_group_layer_, &wl_webos_surface_group_layer_listener_,
      this);
}

}

