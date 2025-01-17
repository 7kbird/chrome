// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef MOJO_PUBLIC_APPLICATION_APPLICATION_RUNNER_CHROMIUM_H_
#define MOJO_PUBLIC_APPLICATION_APPLICATION_RUNNER_CHROMIUM_H_

#include "base/memory/scoped_ptr.h"
#include "base/message_loop/message_loop.h"
#include "mojo/public/cpp/system/core.h"

namespace mojo {

class ApplicationDelegate;

// A utility for running a chromium based mojo Application. The typical use
// case is to use when writing your MojoMain:
//
//  MojoResult MojoMain(MojoHandle shell_handle) {
//    mojo::ApplicationRunnerChromium runner(new MyDelegate());
//    return runner.Run(shell_handle);
//  }
//
// ApplicationRunnerChromium takes care of chromium environment initialization
// and shutdown, and starting a RunLoop from which your application can run and
// ultimately Quit().
class ApplicationRunnerChromium {
 public:
  // Takes ownership of |delegate|.
  explicit ApplicationRunnerChromium(ApplicationDelegate* delegate);
  ~ApplicationRunnerChromium();

  void set_message_loop_type(base::MessageLoop::Type type);

  // Once the various parameters have been set above, use Run to initialize an
  // ApplicationImpl wired to the provided delegate, and run a RunLoop until
  // the application exits.
  MojoResult Run(MojoHandle shell_handle);

 private:
  scoped_ptr<ApplicationDelegate> delegate_;

  // MessageLoop type. TYPE_DEFAULT is default.
  base::MessageLoop::Type message_loop_type_;

  MOJO_DISALLOW_COPY_AND_ASSIGN(ApplicationRunnerChromium);
};

}  // namespace mojo

#endif  // MOJO_PUBLIC_APPLICATION_APPLICATION_RUNNER_CHROMIUM_H_
