// Copyright 2013 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include <stdio.h>
#include <string>

#include "base/bind.h"
#include "mojo/aura/context_factory_mojo.h"
#include "mojo/aura/screen_mojo.h"
#include "mojo/aura/window_tree_host_mojo.h"
#include "mojo/aura/window_tree_host_mojo_delegate.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_connection.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_runner_chromium.h"
#include "mojo/public/cpp/system/core.h"
#include "mojo/services/public/cpp/view_manager/view.h"
#include "mojo/services/public/cpp/view_manager/view_manager.h"
#include "mojo/services/public/cpp/view_manager/view_manager_client_factory.h"
#include "mojo/services/public/cpp/view_manager/view_manager_delegate.h"
#include "mojo/services/public/interfaces/native_viewport/native_viewport.mojom.h"
#include "ui/aura/client/default_capture_client.h"
#include "ui/aura/client/window_tree_client.h"
#include "ui/aura/env.h"
#include "ui/aura/window.h"
#include "ui/aura/window_delegate.h"
#include "ui/base/hit_test.h"
#include "ui/gfx/canvas.h"
#include "ui/gfx/codec/png_codec.h"

namespace mojo {
namespace examples {

// Trivial WindowDelegate implementation that draws a colored background.
class DemoWindowDelegate : public aura::WindowDelegate {
 public:
  explicit DemoWindowDelegate(SkColor color) : color_(color) {}

  // Overridden from WindowDelegate:
  virtual gfx::Size GetMinimumSize() const OVERRIDE {
    return gfx::Size();
  }

  virtual gfx::Size GetMaximumSize() const OVERRIDE {
    return gfx::Size();
  }

  virtual void OnBoundsChanged(const gfx::Rect& old_bounds,
                               const gfx::Rect& new_bounds) OVERRIDE {}
  virtual gfx::NativeCursor GetCursor(const gfx::Point& point) OVERRIDE {
    return gfx::kNullCursor;
  }
  virtual int GetNonClientComponent(const gfx::Point& point) const OVERRIDE {
    return HTCAPTION;
  }
  virtual bool ShouldDescendIntoChildForEventHandling(
      aura::Window* child,
      const gfx::Point& location) OVERRIDE {
    return true;
  }
  virtual bool CanFocus() OVERRIDE { return true; }
  virtual void OnCaptureLost() OVERRIDE {}
  virtual void OnPaint(gfx::Canvas* canvas) OVERRIDE {
    canvas->DrawColor(color_, SkXfermode::kSrc_Mode);
  }
  virtual void OnDeviceScaleFactorChanged(float device_scale_factor) OVERRIDE {}
  virtual void OnWindowDestroying(aura::Window* window) OVERRIDE {}
  virtual void OnWindowDestroyed(aura::Window* window) OVERRIDE {}
  virtual void OnWindowTargetVisibilityChanged(bool visible) OVERRIDE {}
  virtual bool HasHitTestMask() const OVERRIDE { return false; }
  virtual void GetHitTestMask(gfx::Path* mask) const OVERRIDE {}

 private:
  const SkColor color_;

  DISALLOW_COPY_AND_ASSIGN(DemoWindowDelegate);
};

class DemoWindowTreeClient : public aura::client::WindowTreeClient {
 public:
  explicit DemoWindowTreeClient(aura::Window* window) : window_(window) {
    aura::client::SetWindowTreeClient(window_, this);
  }

  virtual ~DemoWindowTreeClient() {
    aura::client::SetWindowTreeClient(window_, NULL);
  }

  // Overridden from aura::client::WindowTreeClient:
  virtual aura::Window* GetDefaultParent(aura::Window* context,
                                         aura::Window* window,
                                         const gfx::Rect& bounds) OVERRIDE {
    if (!capture_client_) {
      capture_client_.reset(
          new aura::client::DefaultCaptureClient(window_->GetRootWindow()));
    }
    return window_;
  }

 private:
  aura::Window* window_;
  scoped_ptr<aura::client::DefaultCaptureClient> capture_client_;

  DISALLOW_COPY_AND_ASSIGN(DemoWindowTreeClient);
};

class AuraDemo : public ApplicationDelegate,
                 public WindowTreeHostMojoDelegate,
                 public ViewManagerDelegate {
 public:
  AuraDemo()
      : window1_(NULL),
        window2_(NULL),
        window21_(NULL),
        view_manager_client_factory_(this) {}
  virtual ~AuraDemo() {}

 private:
  // Overridden from ViewManagerDelegate:
  virtual void OnEmbed(ViewManager* view_manager,
                       View* root,
                       ServiceProviderImpl* exported_services,
                       scoped_ptr<ServiceProvider> imported_services) OVERRIDE {
    // TODO(beng): this function could be called multiple times!
    root_ = root;

    window_tree_host_.reset(new WindowTreeHostMojo(root, this));
    window_tree_host_->InitHost();

    window_tree_client_.reset(
        new DemoWindowTreeClient(window_tree_host_->window()));

    delegate1_.reset(new DemoWindowDelegate(SK_ColorBLUE));
    window1_ = new aura::Window(delegate1_.get());
    window1_->Init(aura::WINDOW_LAYER_TEXTURED);
    window1_->SetBounds(gfx::Rect(100, 100, 400, 400));
    window1_->Show();
    window_tree_host_->window()->AddChild(window1_);

    delegate2_.reset(new DemoWindowDelegate(SK_ColorRED));
    window2_ = new aura::Window(delegate2_.get());
    window2_->Init(aura::WINDOW_LAYER_TEXTURED);
    window2_->SetBounds(gfx::Rect(200, 200, 350, 350));
    window2_->Show();
    window_tree_host_->window()->AddChild(window2_);

    delegate21_.reset(new DemoWindowDelegate(SK_ColorGREEN));
    window21_ = new aura::Window(delegate21_.get());
    window21_->Init(aura::WINDOW_LAYER_TEXTURED);
    window21_->SetBounds(gfx::Rect(10, 10, 50, 50));
    window21_->Show();
    window2_->AddChild(window21_);

    window_tree_host_->Show();
  }
  virtual void OnViewManagerDisconnected(
      ViewManager* view_manager) OVERRIDE {
    base::MessageLoop::current()->Quit();
  }

  // WindowTreeHostMojoDelegate:
  virtual void CompositorContentsChanged(const SkBitmap& bitmap) OVERRIDE {
    root_->SetContents(bitmap);
  }

  virtual void Initialize(ApplicationImpl* app) MOJO_OVERRIDE {
    aura::Env::CreateInstance(true);
    context_factory_.reset(new ContextFactoryMojo);
    aura::Env::GetInstance()->set_context_factory(context_factory_.get());
    screen_.reset(ScreenMojo::Create());
    gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE, screen_.get());
  }

  virtual bool ConfigureIncomingConnection(ApplicationConnection* connection)
      MOJO_OVERRIDE {
    connection->AddService(&view_manager_client_factory_);
    return true;
  }

  scoped_ptr<DemoWindowTreeClient> window_tree_client_;

  scoped_ptr<ui::ContextFactory> context_factory_;

  scoped_ptr<ScreenMojo> screen_;

  scoped_ptr<DemoWindowDelegate> delegate1_;
  scoped_ptr<DemoWindowDelegate> delegate2_;
  scoped_ptr<DemoWindowDelegate> delegate21_;

  aura::Window* window1_;
  aura::Window* window2_;
  aura::Window* window21_;

  View* root_;

  ViewManagerClientFactory view_manager_client_factory_;

  scoped_ptr<aura::WindowTreeHost> window_tree_host_;

  DISALLOW_COPY_AND_ASSIGN(AuraDemo);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(new mojo::examples::AuraDemo);
  return runner.Run(shell_handle);
}
