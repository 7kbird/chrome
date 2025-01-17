// Copyright (c) 2012 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef PPAPI_SIMPLE_PS_H_
#define PPAPI_SIMPLE_PS_H_

#include "ppapi/c/pp_instance.h"
#include "sdk_util/macros.h"

EXTERN_C_BEGIN

/**
 * The ppapi_simple library simplifies the use of the Pepper interfaces by
 * providing a more traditional 'C' or 'C++' style framework.  The library
 * creates an PSInstance derived object based on the ppapi_cpp library and
 * initializes the nacl_io library to provide a POSIX friendly I/O environment.
 *
 * In order to provide a standard blocking environment, the library will hide
 * the actual "Pepper Thread" which is the thread that standard events
 * such as window resize, mouse keyboard, or other inputs arrive.  To prevent
 * blocking, instead we enqueue these events onto a thread safe linked list
 * and expect them to be processed on a new thread.  In addition, the library
 * will automatically start a new thread on which can be used effectively
 * as a "main" entry point.
 *
 * For C style development, the PPAPI_SIMPLE_REGISTER_MAIN(XX) macros provide a
 * mechanism to register the entry an point for "main".  All events are pushed
 * onto an event queue which can then be pulled from this new thread.
 * NOTE: The link will still need libstdc++ and libppapi_cpp since the library
 * is still creating a C++ object which does the initialization work and
 * forwards the events.
 *
 * For C++ style development, use the ppapi_simple_instance.h,
 * ppapi_simple_instance_2d.h, and ppapi_simple_instance_3d.h headers as
 * a base class, and overload the appropriate virtual functions such as
 * Main, ChangeContext, or Render.
 */

/**
 * PSGetInstanceId
 *
 * Return the PP_Instance id of this instance of the module.  This is required
 * by most of the Pepper resource creation routines.
 */
PP_Instance PSGetInstanceId(void);


/**
 * PSGetInterface
 *
 * Return the Pepper instance referred to by 'name'.  Will return a pointer
 * to the interface, or NULL if not found or not available.
 */
const void* PSGetInterface(const char *name);


/**
 * PSUserCreateInstance
 *
 * Prototype for the user provided function which creates and configures
 * the instance object.  This function is defined by one of the macros below,
 * or by the equivalent macro in one of the other headers.  For 'C'
 * development, one of the basic instances which support C callback are used.
 * For C++, this function should instantiate the user defined instance.  See
 * the two macros below.
 */
extern void* PSUserCreateInstance(PP_Instance inst);


/**
 * PPAPI_SIMPLE_USE_MAIN
 *
 * For use with C projects, this macro calls the provided factory with
 * configuration information.
 */
#define PPAPI_SIMPLE_USE_MAIN(factory, func)   \
void* PSUserCreateInstance(PP_Instance inst) { \
  return factory(inst, func);                  \
}


EXTERN_C_END


#endif  // PPAPI_SIMPLE_PS_H_
