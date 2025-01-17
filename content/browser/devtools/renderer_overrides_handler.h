// Copyright (c) 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_DEVTOOLS_RENDERER_OVERRIDES_HANDLER_H_
#define CONTENT_BROWSER_DEVTOOLS_RENDERER_OVERRIDES_HANDLER_H_

#include "base/basictypes.h"
#include "base/compiler_specific.h"
#include "base/memory/ref_counted_memory.h"
#include "base/memory/scoped_ptr.h"
#include "base/memory/weak_ptr.h"
#include "base/time/time.h"
#include "cc/output/compositor_frame_metadata.h"
#include "content/browser/devtools/devtools_protocol.h"
#include "content/common/content_export.h"
#include "content/public/browser/render_widget_host.h"
#include "third_party/skia/include/core/SkBitmap.h"

class SkBitmap;

namespace IPC {
class Message;
}

namespace blink {
class WebMouseEvent;
}

namespace content {

class DevToolsTracingHandler;
class RenderViewHostImpl;

// Overrides Inspector commands before they are sent to the renderer.
// May override the implementation completely, ignore it, or handle
// additional browser process implementation details.
class CONTENT_EXPORT RendererOverridesHandler
    : public DevToolsProtocol::Handler {
 public:
  RendererOverridesHandler();
  virtual ~RendererOverridesHandler();

  void OnClientDetached();
  void OnSwapCompositorFrame(const cc::CompositorFrameMetadata& frame_metadata);
  void OnVisibilityChanged(bool visible);
  void SetRenderViewHost(RenderViewHostImpl* host);
  void ClearRenderViewHost();

 private:
  void InnerSwapCompositorFrame();

  // DOM domain.
  scoped_refptr<DevToolsProtocol::Response>
      GrantPermissionsForSetFileInputFiles(
          scoped_refptr<DevToolsProtocol::Command> command);

  // Network domain.
  scoped_refptr<DevToolsProtocol::Response> CanEmulateNetworkConditions(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> ClearBrowserCache(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> ClearBrowserCookies(
      scoped_refptr<DevToolsProtocol::Command> command);

  // Page domain.
  scoped_refptr<DevToolsProtocol::Response> PageDisable(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageHandleJavaScriptDialog(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageNavigate(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageReload(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageGetNavigationHistory(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageNavigateToHistoryEntry(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageSetTouchEmulationEnabled(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageCaptureScreenshot(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageCanScreencast(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageStartScreencast(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageStopScreencast(
      scoped_refptr<DevToolsProtocol::Command> command);
  scoped_refptr<DevToolsProtocol::Response> PageQueryUsageAndQuota(
      scoped_refptr<DevToolsProtocol::Command>);
  scoped_refptr<DevToolsProtocol::Response> PageSetColorPickerEnabled(
      scoped_refptr<DevToolsProtocol::Command>);

  void ScreenshotCaptured(
      scoped_refptr<DevToolsProtocol::Command> command,
      const unsigned char* png_data,
      size_t png_size);

  void ScreencastFrameCaptured(
      const std::string& format,
      int quality,
      const cc::CompositorFrameMetadata& metadata,
      bool success,
      const SkBitmap& bitmap);

  void PageQueryUsageAndQuotaCompleted(
     scoped_refptr<DevToolsProtocol::Command>,
     scoped_ptr<base::DictionaryValue> response_data);

  void NotifyScreencastVisibility(bool visible);
  void SetColorPickerEnabled(bool enabled);
  void UpdateColorPickerFrame();
  void ResetColorPickerFrame();
  void ColorPickerFrameUpdated(bool succeeded, const SkBitmap& bitmap);
  bool HandleMouseEvent(const blink::WebMouseEvent& event);
  void UpdateColorPickerCursor();

  // Input domain.
  scoped_refptr<DevToolsProtocol::Response> InputEmulateTouchFromMouseEvent(
      scoped_refptr<DevToolsProtocol::Command> command);

  void UpdateTouchEventEmulationState();

  RenderViewHostImpl* host_;
  scoped_refptr<DevToolsProtocol::Command> screencast_command_;
  bool has_last_compositor_frame_metadata_;
  cc::CompositorFrameMetadata last_compositor_frame_metadata_;
  base::TimeTicks last_frame_time_;
  int capture_retry_count_;
  bool touch_emulation_enabled_;
  bool color_picker_enabled_;
  SkBitmap color_picker_frame_;
  int last_cursor_x_;
  int last_cursor_y_;
  RenderWidgetHost::MouseEventCallback mouse_event_callback_;
  base::WeakPtrFactory<RendererOverridesHandler> weak_factory_;
  DISALLOW_COPY_AND_ASSIGN(RendererOverridesHandler);
};

}  // namespace content

#endif  // CONTENT_BROWSER_DEVTOOLS_RENDERER_OVERRIDES_HANDLER_H_
