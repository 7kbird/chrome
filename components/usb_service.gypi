# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'targets': [{
    # GN version: //components/usb_service
    'target_name': 'usb_service',
    'type': '<(component)',
    'dependencies': [
      '../base/base.gyp:base',
      '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
      '../content/content.gyp:content_browser',
      '../net/net.gyp:net',
      '../third_party/libusb/libusb.gyp:libusb',
    ],
    'defines': [
      'USB_SERVICE_IMPLEMENTATION',
    ],
    'include_dirs': [
      '..',
    ],
    'sources': [
      # Note: sources list duplicated in GN build.
      'usb_service/usb_context.cc',
      'usb_service/usb_context.h',
      'usb_service/usb_device_impl.cc',
      'usb_service/usb_device_impl.h',
      'usb_service/usb_device.h',
      'usb_service/usb_device_filter.cc',
      'usb_service/usb_device_filter.h',
      'usb_service/usb_device_handle_impl.cc',
      'usb_service/usb_device_handle_impl.h',
      'usb_service/usb_device_handle.h',
      'usb_service/usb_error.cc',
      'usb_service/usb_error.h',
      'usb_service/usb_interface.h',
      'usb_service/usb_interface_impl.cc',
      'usb_service/usb_interface_impl.h',
      'usb_service/usb_service.h',
      'usb_service/usb_service_impl.cc',
    ],
    'conditions': [
      ['use_udev == 1', {
        'dependencies': [
          '../build/linux/system.gyp:udev',
        ],
      }],
      ['chromeos==1', {
        'dependencies': [
          '../chromeos/chromeos.gyp:chromeos',
        ],
      }],
    ]
  }],
}
