// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "athena/test/athena_test_helper.h"

#include "athena/extensions/public/extensions_delegate.h"
#include "athena/main/athena_launcher.h"
#include "athena/screen/public/screen_manager.h"
#include "athena/test/sample_activity_factory.h"
#include "athena/test/test_app_model_builder.h"
#include "athena/test/test_screen_manager_delegate.h"
#include "base/command_line.h"
#include "base/message_loop/message_loop.h"
#include "base/run_loop.h"
#include "base/threading/thread.h"
#include "chromeos/dbus/dbus_thread_manager.h"
#include "ui/app_list/app_list_switches.h"
#include "ui/aura/client/aura_constants.h"
#include "ui/aura/env.h"
#include "ui/aura/input_state_lookup.h"
#include "ui/aura/test/env_test_helper.h"
#include "ui/aura/test/test_focus_client.h"
#include "ui/aura/test/test_screen.h"
#include "ui/aura/window_event_dispatcher.h"
#include "ui/base/ime/input_method_initializer.h"
#include "ui/compositor/scoped_animation_duration_scale_mode.h"
#include "ui/gfx/screen.h"
#include "ui/wm/core/focus_controller.h"
#include "ui/wm/core/input_method_event_filter.h"

#if defined(USE_X11)
#include "ui/base/x/x11_util.h"
#endif

namespace athena {
namespace test {

AthenaTestHelper::AthenaTestHelper(base::MessageLoopForUI* message_loop)
    : setup_called_(false), teardown_called_(false) {
  DCHECK(message_loop);
  message_loop_ = message_loop;
  // Disable animations during tests.
  zero_duration_mode_.reset(new ui::ScopedAnimationDurationScaleMode(
      ui::ScopedAnimationDurationScaleMode::ZERO_DURATION));
}

AthenaTestHelper::~AthenaTestHelper() {
  CHECK(setup_called_) << "AthenaTestHelper::SetUp() never called.";
  CHECK(teardown_called_) << "AthenaTestHelper::TearDown() never called.";
}

void AthenaTestHelper::SetUp(ui::ContextFactory* context_factory) {
  setup_called_ = true;
  file_thread_.reset(new base::Thread("FileThread"));
  base::Thread::Options options(base::MessageLoop::TYPE_IO, 0);
  file_thread_->StartWithOptions(options);

  // Force showing in the experimental app-list view.
  base::CommandLine::ForCurrentProcess()->AppendSwitch(
      app_list::switches::kEnableExperimentalAppList);

  chromeos::DBusThreadManager::InitializeWithStub();

  aura::Env::CreateInstance(true);
  aura::Env::GetInstance()->set_context_factory(context_factory);

  // Unit tests generally don't want to query the system, rather use the state
  // from RootWindow.
  aura::test::EnvTestHelper(aura::Env::GetInstance())
      .SetInputStateLookup(scoped_ptr<aura::InputStateLookup>());

  ui::InitializeInputMethodForTesting();

  const gfx::Size host_size(800, 600);
  test_screen_.reset(aura::TestScreen::Create(host_size));
  gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE, test_screen_.get());
  host_.reset(test_screen_->CreateHostForPrimaryDisplay());
  screen_manager_delegate_.reset(
      new TestScreenManagerDelegate(test_screen_.get()));

  input_method_filter_.reset(new ::wm::InputMethodEventFilter(
      root_window()->GetHost()->GetAcceleratedWidget()));
  input_method_filter_->SetInputMethodPropertyInRootWindow(
      root_window());

  wm::FocusController* focus_controller =
      new wm::FocusController(ScreenManager::CreateFocusRules());
  aura::client::SetFocusClient(root_window(), focus_controller);
  root_window()->AddPreTargetHandler(focus_controller);
  aura::client::SetActivationClient(root_window(), focus_controller);
  focus_client_.reset(focus_controller);

  root_window()->Show();
  // Ensure width != height so tests won't confuse them.
  host()->SetBounds(gfx::Rect(host_size));

  athena::StartAthenaEnv(root_window(), screen_manager_delegate_.get(),
                         file_thread_->message_loop_proxy());
  athena::ExtensionsDelegate::CreateExtensionsDelegateForTest();
  athena::StartAthenaSession(new SampleActivityFactory(),
                             new TestAppModelBuilder());
}

void AthenaTestHelper::TearDown() {
  teardown_called_ = true;

  athena::ShutdownAthena();

  aura::client::SetFocusClient(root_window(), NULL);
  focus_client_.reset();
  input_method_filter_.reset();

  host_.reset();
  ui::GestureRecognizer::Reset();
  test_screen_.reset();
  gfx::Screen::SetScreenInstance(gfx::SCREEN_TYPE_NATIVE, NULL);

#if defined(USE_X11)
  ui::test::ResetXCursorCache();
#endif

  ui::ShutdownInputMethodForTesting();

  chromeos::DBusThreadManager::Shutdown();

  aura::Env::DeleteInstance();
}

void AthenaTestHelper::RunAllPendingInMessageLoop() {
  // TODO(jbates) crbug.com/134753 Find quitters of this RunLoop and have them
  //              use run_loop.QuitClosure().
  base::RunLoop run_loop;
  run_loop.RunUntilIdle();
}

}  // namespace test
}  // namespace athena
