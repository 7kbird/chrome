# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'target_defaults': {
    'conditions': [
      ['mojo_shell_debug_url != ""', {
        'defines': [
          'MOJO_SHELL_DEBUG=1',
          'MOJO_SHELL_DEBUG_URL="<(mojo_shell_debug_url)"',
         ],
      }],
    ],
  },
  'includes': [
    'mojo_apps.gypi',
    'mojo_examples.gypi',
    'mojo_services.gypi',
    'mojo_variables.gypi',
  ],
  'targets': [
    {
      'target_name': 'mojo',
      'type': 'none',
      'dependencies': [
        'mojo_application_manager',
        'mojo_application_manager_unittests',
        'mojo_apps_js_unittests',
        'mojo_base.gyp:mojo_base',
        'mojo_compositor_app',
        'mojo_content_handler_demo',
        'mojo_echo_client',
        'mojo_echo_service',
        'mojo_example_apptests',
        'mojo_example_service',
        'mojo_geometry_lib',
        'mojo_html_viewer',
        'mojo_js',
        'mojo_launcher',
        'mojo_native_viewport_service_lib',
        'mojo_network_service',
        'mojo_pepper_container_app',
        'mojo_png_viewer',
        'mojo_sample_app',
        'mojo_shell',
        'mojo_shell_lib',
        'mojo_shell_tests',
        'mojo_surfaces_app',
        'mojo_surfaces_app',
        'mojo_surfaces_child_app',
        'mojo_surfaces_child_gl_app',
        'mojo_surfaces_lib',
        'mojo_surfaces_lib_unittests',
        'mojo_surfaces_service',
        'mojo_test_app',
        'mojo_test_request_tracker_app',
        'mojo_view_manager_lib',
        'mojo_view_manager_lib_unittests',
        'mojo_wget',
      ],
      'conditions': [
        ['use_aura==1', {
          'dependencies': [
            'mojo_aura_demo',
            'mojo_aura_demo_init',
            'mojo_browser',
            'mojo_core_window_manager',
            'mojo_core_window_manager_unittests',
            'mojo_demo_launcher',
            'mojo_embedded_app',
            'mojo_keyboard',
            'mojo_media_viewer',
            'mojo_nesting_app',
            'mojo_window_manager',
            'mojo_wm_flow_app',
            'mojo_wm_flow_embedded',
            'mojo_wm_flow_init',
            'mojo_wm_flow_wm',
            'mojo_view_manager',
            'mojo_view_manager_unittests',
          ],
        }],
        ['OS == "linux"', {
          'dependencies': [
            'mojo_dbus_echo',
            'mojo_dbus_echo_service',
          ],
        }],
      ]
    },
    {
      # GN version: //mojo/shell:external_service_bindings
      'target_name': 'mojo_external_service_bindings',
      'type': 'static_library',
      'sources': [
        'shell/external_service.mojom',
      ],
      'includes': [ 'public/tools/bindings/mojom_bindings_generator.gypi' ],
      'export_dependent_settings': [
        'mojo_base.gyp:mojo_cpp_bindings',
      ],
      'dependencies': [
        'mojo_base.gyp:mojo_cpp_bindings',
      ],
    },
    {
      'target_name': 'mojo_spy',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:base_static',
        '../net/net.gyp:http_server',
        '../url/url.gyp:url_lib',
        'mojo_application_manager',
      ],
      'variables': {
        'mojom_base_output_dir': 'mojo',
      },
      'includes': [ 'public/tools/bindings/mojom_bindings_generator.gypi' ],
      'sources': [
        'spy/public/spy.mojom',
        'spy/common.h',
        'spy/spy.cc',
        'spy/spy.h',
        'spy/spy_server_impl.h',
        'spy/spy_server_impl.cc',
        'spy/websocket_server.cc',
        'spy/websocket_server.h',
      ],
    },
    {
      # GN version: //mojo/shell:lib
      'target_name': 'mojo_shell_lib',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:base_static',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../net/net.gyp:net',
        '../url/url.gyp:url_lib',
        'mojo_application_manager',
        'mojo_base.gyp:mojo_application_bindings',
        'mojo_base.gyp:mojo_common_lib',
        'mojo_base.gyp:mojo_gles2_impl',
        'mojo_base.gyp:mojo_system_impl',
        'mojo_base.gyp:mojo_application_chromium',
        'mojo_external_service_bindings',
        'mojo_native_viewport_service_lib',
        'mojo_network_bindings',
        'mojo_spy',
      ],
      'includes': [ 'public/tools/bindings/mojom_bindings_generator.gypi' ],
      'sources': [
        'shell/app_child_process.cc',
        'shell/app_child_process.h',
        'shell/app_child_process.mojom',
        'shell/app_child_process_host.cc',
        'shell/app_child_process_host.h',
        'shell/child_process.cc',
        'shell/child_process.h',
        'shell/child_process_host.cc',
        'shell/child_process_host.h',
        'shell/context.cc',
        'shell/context.h',
        'shell/dbus_application_loader_linux.cc',
        'shell/dbus_application_loader_linux.h',
        'shell/dynamic_application_loader.cc',
        'shell/dynamic_application_loader.h',
        'shell/dynamic_service_runner.h',
        'shell/init.cc',
        'shell/init.h',
        'shell/in_process_dynamic_service_runner.cc',
        'shell/in_process_dynamic_service_runner.h',
        'shell/keep_alive.cc',
        'shell/keep_alive.h',
        'shell/mojo_url_resolver.cc',
        'shell/mojo_url_resolver.h',
        'shell/out_of_process_dynamic_service_runner.cc',
        'shell/out_of_process_dynamic_service_runner.h',
        'shell/run.cc',
        'shell/run.h',
        'shell/switches.cc',
        'shell/switches.h',
        'shell/task_runners.cc',
        'shell/task_runners.h',
        'shell/test_child_process.cc',
        'shell/test_child_process.h',
        'shell/ui_application_loader_android.cc',
        'shell/ui_application_loader_android.h',
        'shell/view_manager_loader.cc',
        'shell/view_manager_loader.h',
      ],
      'conditions': [
        ['OS=="linux"', {
          'dependencies': [
            '../build/linux/system.gyp:dbus',
            '../dbus/dbus.gyp:dbus',
          ],
        }],
        ['OS=="android"', {
          'dependencies': [
            'mojo_network_service_lib',
          ],
          'sources': [
            'shell/network_application_loader.cc',
            'shell/network_application_loader.h',
          ],
        }],
        ['use_aura==1', {
          'dependencies': [
            # These are only necessary as long as we hard code use of ViewManager.
            '../skia/skia.gyp:skia',
            'mojo_view_manager',
            'mojo_view_manager_bindings',
          ],
        }, {  # use_aura==0
          'sources!': [
            'shell/view_manager_loader.cc',
            'shell/view_manager_loader.h',
          ],
        }],
      ],
    },
    {
      # GN version: //mojo/shell:test_support
      'target_name': 'mojo_shell_test_support',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:base_static',
        '../url/url.gyp:url_lib',
        'mojo_application_manager',
        'mojo_base.gyp:mojo_system_impl',
        'mojo_shell_lib',
      ],
      'sources': [
        'shell/shell_test_helper.cc',
        'shell/shell_test_helper.h',
      ],
    },
    {
      # GN version: //mojo/shell
      'target_name': 'mojo_shell',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:base',
        '../ui/gl/gl.gyp:gl',
        '../url/url.gyp:url_lib',
        'mojo_application_manager',
        'mojo_base.gyp:mojo_common_lib',
        'mojo_base.gyp:mojo_environment_chromium',
        'mojo_base.gyp:mojo_system_impl',
        'mojo_shell_lib',
      ],
      'conditions': [
        ['use_ozone==1', {
          'dependencies': [
            '../ui/ozone/ozone.gyp:ozone',
          ],
        }],
      ],
      'sources': [
        'shell/desktop/mojo_main.cc',
      ],
    },
    {
      # GN version: //mojo/shell:mojo_shell_tests
      'target_name': 'mojo_shell_tests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:base_i18n',
        '../base/base.gyp:test_support_base',
        '../testing/gtest.gyp:gtest',
        '../net/net.gyp:net_test_support',
        # TODO(vtl): We don't currently need this, but I imagine we will soon.
        # '../ui/gl/gl.gyp:gl',
        '../url/url.gyp:url_lib',
        'mojo_application_manager',
        'mojo_base.gyp:mojo_common_lib',
        'mojo_base.gyp:mojo_environment_chromium',
        'mojo_base.gyp:mojo_system_impl',
        'mojo_shell_lib',
        'mojo_test_app',
        'mojo_test_request_tracker_app',
        'mojo_test_service_bindings',
      ],
      'sources': [
        'shell/child_process_host_unittest.cc',
        'shell/dynamic_application_loader_unittest.cc',
        'shell/in_process_dynamic_service_runner_unittest.cc',
        'shell/shell_test_base.cc',
        'shell/shell_test_base.h',
        'shell/shell_test_base_unittest.cc',
        'shell/shell_test_main.cc',
      ],
      'conditions': [
        ['OS == "android"', {
          'dependencies': [
            '../testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
      ],
    },
    {
      # GN version: //mojo/application_manager
      'target_name': 'mojo_application_manager',
      'type': '<(component)',
      'defines': [
        'MOJO_APPLICATION_MANAGER_IMPLEMENTATION',
      ],
      'dependencies': [
        '../base/base.gyp:base',
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        '../net/net.gyp:net',
        '../url/url.gyp:url_lib',
        'mojo_content_handler_bindings',
        'mojo_network_bindings',
        'mojo_base.gyp:mojo_application_bindings',
        'mojo_base.gyp:mojo_common_lib',
        'mojo_base.gyp:mojo_environment_chromium',
        '<(mojo_system_for_component)',
      ],
      'sources': [
        'application_manager/application_loader.cc',
        'application_manager/application_loader.h',
        'application_manager/application_manager.cc',
        'application_manager/application_manager.h',
        'application_manager/application_manager_export.h',
        'application_manager/background_shell_application_loader.cc',
        'application_manager/background_shell_application_loader.h',
      ],
      'export_dependent_settings': [
        '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
        'mojo_base.gyp:mojo_application_bindings',
        'mojo_network_bindings',
      ],
    },
    {
      # GN version: //mojo/application_manager:unittests
      'target_name': 'mojo_application_manager_unittests',
      'type': 'executable',
      'dependencies': [
        '../base/base.gyp:base',
        '../testing/gtest.gyp:gtest',
        '../url/url.gyp:url_lib',
        'mojo_application_manager',
        'mojo_base.gyp:mojo_common_lib',
        'mojo_base.gyp:mojo_cpp_bindings',
        'mojo_base.gyp:mojo_environment_chromium',
        'mojo_base.gyp:mojo_run_all_unittests',
        'mojo_base.gyp:mojo_application_chromium',
      ],
      'includes': [ 'public/tools/bindings/mojom_bindings_generator.gypi' ],
      'sources': [
        'application_manager/application_manager_unittest.cc',
        'application_manager/background_shell_application_loader_unittest.cc',
        'application_manager/test.mojom',
      ],
    },
    {
      # GN version: //mojo/cc
      'target_name': 'mojo_cc_support',
      'type': 'static_library',
      'dependencies': [
        '../base/base.gyp:base',
        '../cc/cc.gyp:cc',
        '../skia/skia.gyp:skia',
        '../gpu/gpu.gyp:gles2_implementation',
        '<(mojo_gles2_for_loadable_module)',
      ],
      'sources': [
        'cc/context_provider_mojo.cc',
        'cc/context_provider_mojo.h',
      ],
    },
  ],
  'conditions': [
    ['OS=="android"', {
      'targets': [
        {
          'target_name': 'mojo_native_viewport_java',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_java',
          ],
          'variables': {
            'java_in_dir': '<(DEPTH)/mojo/services/native_viewport/android',
          },
          'includes': [ '../build/java.gypi' ],
        },
        {
          'target_name': 'libmojo_shell',
          'type': 'shared_library',
          'dependencies': [
            '../base/base.gyp:base',
            '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
            '../ui/gfx/gfx.gyp:gfx',
            '../ui/gfx/gfx.gyp:gfx_geometry',
            '../ui/gl/gl.gyp:gl',
            'mojo_base.gyp:mojo_application_bindings',
            'mojo_base.gyp:mojo_common_lib',
            'mojo_base.gyp:mojo_environment_chromium',
            'mojo_base.gyp:mojo_jni_headers',
            'mojo_shell_lib',
          ],
          'sources': [
            'shell/android/library_loader.cc',
            'shell/android/mojo_main.cc',
            'shell/android/mojo_main.h',
          ],
        },
        {
          'target_name': 'mojo_shell_apk',
          'type': 'none',
          'dependencies': [
            '../base/base.gyp:base_java',
            '../net/net.gyp:net_java',
            'mojo_native_viewport_java',
            'libmojo_shell',
          ],
          'variables': {
            'apk_name': 'MojoShell',
            'java_in_dir': '<(DEPTH)/mojo/shell/android/apk',
            'resource_dir': '<(DEPTH)/mojo/shell/android/apk/res',
            'native_lib_target': 'libmojo_shell',
          },
          'includes': [ '../build/java_apk.gypi' ],
        }
      ],
    }],
    ['OS=="linux"', {
      'targets': [
        {
          'target_name': 'mojo_dbus_service',
          'type': 'static_library',
          'dependencies': [
            '../base/base.gyp:base',
            '../build/linux/system.gyp:dbus',
            '../dbus/dbus.gyp:dbus',
            'mojo_base.gyp:mojo_application_chromium',
            'mojo_base.gyp:mojo_common_lib',
            'mojo_base.gyp:mojo_system_impl',
            'mojo_external_service_bindings',
          ],
          'sources': [
            'dbus/dbus_external_service.h',
            'dbus/dbus_external_service.cc',
          ],
        },
      ],
    }],
    ['use_aura==1', {
      'targets': [
        {
          # GN version: //mojo/aura
          'target_name': 'mojo_aura_support',
          'type': 'static_library',
          'dependencies': [
            '../cc/cc.gyp:cc',
            '../ui/aura/aura.gyp:aura',
            '../ui/compositor/compositor.gyp:compositor',
            '../ui/events/events.gyp:events',
            '../ui/events/events.gyp:events_base',
            '../ui/gl/gl.gyp:gl',
            '../webkit/common/gpu/webkit_gpu.gyp:webkit_gpu',
            'mojo_cc_support',
            'mojo_native_viewport_bindings',
            '<(mojo_gles2_for_loadable_module)',
          ],
          'sources': [
            'aura/aura_init.cc',
            'aura/aura_init.h',
            'aura/context_factory_mojo.cc',
            'aura/context_factory_mojo.h',
            'aura/screen_mojo.cc',
            'aura/screen_mojo.h',
            'aura/window_tree_host_mojo.cc',
            'aura/window_tree_host_mojo.h',
            'aura/window_tree_host_mojo_delegate.h',
          ],
        },
        {
          'target_name': 'mojo_views_support',
          'type': 'static_library',
          'dependencies': [
            '../base/base.gyp:base',
            '../base/base.gyp:base_i18n',
            '../skia/skia.gyp:skia',
            '../skia/skia.gyp:skia',
            '../third_party/icu/icu.gyp:icui18n',
            '../third_party/icu/icu.gyp:icuuc',
            '../ui/aura/aura.gyp:aura',
            '../ui/base/ui_base.gyp:ui_base',
            '../ui/views/views.gyp:views',
            '../ui/wm/wm.gyp:wm',
            'mojo_aura_support',
            'mojo_views_support_internal',
          ],
          'sources': [
            'views/native_widget_view_manager.cc',
            'views/native_widget_view_manager.h',
            'views/views_init.cc',
            'views/views_init.h',
          ],
        },
        {
          'target_name': 'mojo_views_support_internal',
          'type': '<(component)',
          'dependencies': [
            '../base/base.gyp:base',
            '../base/base.gyp:base_i18n',
            '../base/base.gyp:base_static',
            '../base/third_party/dynamic_annotations/dynamic_annotations.gyp:dynamic_annotations',
            '../skia/skia.gyp:skia',
            '../skia/skia.gyp:skia',
            '../third_party/icu/icu.gyp:icui18n',
            '../third_party/icu/icu.gyp:icuuc',
            '../ui/base/ui_base.gyp:ui_base',
            '../ui/gfx/gfx.gyp:gfx',
          ],
          'sources': [
            'views/mojo_views_export.h',
            'views/views_init_internal.cc',
            'views/views_init_internal.h',
          ],
          'defines': [
            'MOJO_VIEWS_IMPLEMENTATION',
          ],
        },
      ],
    }],
  ],
}
