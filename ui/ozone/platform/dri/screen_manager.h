// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef UI_OZONE_PLATFORM_DRI_SCREEN_MANAGER_H_
#define UI_OZONE_PLATFORM_DRI_SCREEN_MANAGER_H_

#include <map>

#include "base/macros.h"
#include "base/memory/weak_ptr.h"
#include "ui/gfx/native_widget_types.h"
#include "ui/ozone/platform/dri/hardware_display_controller.h"

typedef struct _drmModeModeInfo drmModeModeInfo;

namespace gfx {
class Point;
class Rect;
class Size;
}  // namespace gfx

namespace ui {

class DriWrapper;
class ScanoutBufferGenerator;

// Responsible for keeping track of active displays and configuring them.
class ScreenManager {
 public:
  ScreenManager(DriWrapper* dri, ScanoutBufferGenerator* surface_generator);
  virtual ~ScreenManager();

  // Remove a display controller from the list of active controllers. The
  // controller is removed since it was disconnected.
  void RemoveDisplayController(uint32_t crtc);

  // Configure (and add if not present) a display controller. The display
  // controller is identified by (|crtc|, |connector|) and the controller is
  // modeset using |mode|.
  bool ConfigureDisplayController(uint32_t crtc,
                                  uint32_t connector,
                                  const gfx::Point& origin,
                                  const drmModeModeInfo& mode);

  // Disable the display controller identified by |crtc|. Note, the controller
  // may still be connected, so this does not remove the controller.
  bool DisableDisplayController(uint32_t crtc);

  // Returns a reference to the display controller associated with |widget|.
  // This returns a weak reference since the display controller may be destroyed
  // at any point in time, but the changes are propagated to the compositor much
  // later (Compositor owns SurfaceOzone*, which is responsible for updating the
  // display surface).
  base::WeakPtr<HardwareDisplayController> GetDisplayController(
      gfx::AcceleratedWidget widget);

  // Returns a reference to the display controller configured to display within
  // |bounds|.
  // This returns a weak reference since the display controller may be destroyed
  // at any point in time, but the changes are propagated to the compositor much
  // later (Compositor owns SurfaceOzone*, which is responsible for updating the
  // display surface).
  base::WeakPtr<HardwareDisplayController> GetDisplayController(
      const gfx::Rect& bounds);

 private:
  typedef std::map<gfx::AcceleratedWidget, HardwareDisplayController*>
      HardwareDisplayControllerMap;

  // Returns an iterator into |controllers_| for the controller identified by
  // (|crtc|, |connector|).
  HardwareDisplayControllerMap::iterator FindDisplayController(
      uint32_t crtc);

  // Returns an iterator into |controllers_| for the controller located at
  // |origin|.
  HardwareDisplayControllerMap::iterator FindDisplayControllerByOrigin(
      const gfx::Point& origin);

  // Returns an iterator into |controllers_| for the controller located within
  // |bounds|.
  HardwareDisplayControllerMap::iterator FindActiveDisplayControllerByLocation(
      const gfx::Rect& bounds);

  // Perform modesetting in |controller| using |origin| and |mode|.
  bool ModesetDisplayController(HardwareDisplayController* controller,
                                const gfx::Point& origin,
                                const drmModeModeInfo& mode);

  // Tries to set the controller identified by (|crtc|, |connector|) to mirror
  // those in |mirror|. |original| is an iterator to the HDC where the
  // controller is currently present.
  bool HandleMirrorMode(
      HardwareDisplayControllerMap::iterator original,
      HardwareDisplayControllerMap::iterator mirror,
      uint32_t crtc,
      uint32_t connector);

  // On non CrOS builds there is no display configurator to look-up available
  // displays and initialize the HDCs. In such cases this is called internally
  // to initialize a display.
  virtual void ForceInitializationOfPrimaryDisplay();

  DriWrapper* dri_;  // Not owned.
  ScanoutBufferGenerator* buffer_generator_;  // Not owned.
  // Mapping between an accelerated widget and an active display.
  HardwareDisplayControllerMap controllers_;
  gfx::AcceleratedWidget last_added_widget_;

  DISALLOW_COPY_AND_ASSIGN(ScreenManager);
};

}  // namespace ui

#endif  // UI_OZONE_PLATFORM_DRI_SCREEN_MANAGER_H_
