// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "build/build_config.h"
#include "mojo/examples/pepper_container_app/mojo_ppapi_globals.h"
#include "mojo/examples/pepper_container_app/plugin_instance.h"
#include "mojo/examples/pepper_container_app/plugin_module.h"
#include "mojo/examples/pepper_container_app/type_converters.h"
#include "mojo/public/c/system/main.h"
#include "mojo/public/cpp/application/application_delegate.h"
#include "mojo/public/cpp/application/application_impl.h"
#include "mojo/public/cpp/application/application_runner_chromium.h"
#include "mojo/public/cpp/system/core.h"
#include "mojo/services/public/interfaces/gpu/gpu.mojom.h"
#include "mojo/services/public/interfaces/native_viewport/native_viewport.mojom.h"
#include "ppapi/c/pp_rect.h"
#include "ppapi/shared_impl/proxy_lock.h"

namespace mojo {
namespace examples {

class PepperContainerApp: public ApplicationDelegate,
                          public NativeViewportClient,
                          public MojoPpapiGlobals::Delegate {
 public:
  explicit PepperContainerApp()
      : ppapi_globals_(this),
        plugin_module_(new PluginModule) {}

  virtual ~PepperContainerApp() {}

  virtual void Initialize(ApplicationImpl* app) MOJO_OVERRIDE {
    app->ConnectToService("mojo:mojo_native_viewport_service", &viewport_);
    viewport_.set_client(this);

    // TODO(jamesr): Should be mojo:mojo_gpu_service
    app->ConnectToService("mojo:mojo_native_viewport_service", &gpu_service_);

    RectPtr rect(Rect::New());
    rect->x = 10;
    rect->y = 10;
    rect->width = 800;
    rect->height = 600;
    viewport_->Create(rect.Pass());
    viewport_->Show();
  }

  // NativeViewportClient implementation.
  virtual void OnCreated(uint64_t native_viewport_id) OVERRIDE {
    native_viewport_id_ = native_viewport_id;
    ppapi::ProxyAutoLock lock;

    plugin_instance_ = plugin_module_->CreateInstance().Pass();
    if (!plugin_instance_->DidCreate())
      plugin_instance_.reset();
  }

  virtual void OnDestroyed() OVERRIDE {
    ppapi::ProxyAutoLock lock;

    if (plugin_instance_) {
      plugin_instance_->DidDestroy();
      plugin_instance_.reset();
    }

    base::MessageLoop::current()->Quit();
  }

  virtual void OnBoundsChanged(RectPtr bounds) OVERRIDE {
    ppapi::ProxyAutoLock lock;

    if (plugin_instance_)
      plugin_instance_->DidChangeView(bounds.To<PP_Rect>());
  }

  virtual void OnEvent(EventPtr event,
                       const mojo::Callback<void()>& callback) OVERRIDE {
    if (!event->location_data.is_null()) {
      ppapi::ProxyAutoLock lock;

      // TODO(yzshen): Handle events.
    }
    callback.Run();
  }

  // MojoPpapiGlobals::Delegate implementation.
  virtual ScopedMessagePipeHandle CreateGLES2Context() OVERRIDE {
    CommandBufferPtr command_buffer;
    SizePtr size = Size::New();
    size->width = 800;
    size->width = 600;
    gpu_service_->CreateOnscreenGLES2Context(
        native_viewport_id_, size.Pass(), Get(&command_buffer));
    return command_buffer.PassMessagePipe();
  }

 private:
  MojoPpapiGlobals ppapi_globals_;

  uint64_t native_viewport_id_;
  NativeViewportPtr viewport_;
  GpuPtr gpu_service_;
  scoped_refptr<PluginModule> plugin_module_;
  scoped_ptr<PluginInstance> plugin_instance_;

  DISALLOW_COPY_AND_ASSIGN(PepperContainerApp);
};

}  // namespace examples
}  // namespace mojo

MojoResult MojoMain(MojoHandle shell_handle) {
  mojo::ApplicationRunnerChromium runner(
      new mojo::examples::PepperContainerApp);
  return runner.Run(shell_handle);
}

