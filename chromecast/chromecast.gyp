# Copyright 2014 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
    'chromecast_branding%': 'Chromium',
  },
  'target_defaults': {
    'include_dirs': [
      '..',  # Root of Chromium checkout
    ],
  },
  'targets': [
    {
      'target_name': 'cast_common',
      'type': '<(component)',
      'dependencies': [
        '../base/base.gyp:base',
      ],
      'sources': [
        'common/cast_paths.cc',
        'common/cast_paths.h',
        'common/cast_resource_delegate.cc',
        'common/cast_resource_delegate.h',
        'common/chromecast_config.cc',
        'common/chromecast_config.h',
        'common/pref_names.cc',
        'common/pref_names.h',
      ],
      'conditions': [
        ['chromecast_branding=="Chrome"', {
          'dependencies': [
            'internal/chromecast_internal.gyp:cast_common_internal',
          ],
        }, {
          'sources': [
            'common/chromecast_config_simple.cc',
          ],
        }],
      ],
    },
    {
      'target_name': 'cast_service',
      'type': '<(component)',
      'dependencies': [
        '../skia/skia.gyp:skia',
      ],
      'sources': [
        'service/cast_service.cc',
        'service/cast_service.h',
      ],
      'conditions': [
        ['chromecast_branding=="Chrome"', {
          'dependencies': [
            'internal/chromecast_internal.gyp:cast_service_internal',
          ],
        }, {
          'dependencies': [
            '../base/base.gyp:base',
            '../content/content.gyp:content',
          ],
          'sources': [
            'service/cast_service_simple.cc',
            'service/cast_service_simple.h',
          ],
        }],
      ],
    },
    {
      'target_name': 'cast_shell_resources',
      'type': 'none',
      'variables': {
        'grit_out_dir': '<(SHARED_INTERMEDIATE_DIR)/chromecast',
      },
      'actions': [
        {
          'action_name': 'cast_shell_resources',
          'variables': {
            'grit_grd_file': 'shell/browser/resources/shell_resources.grd',
            'grit_resource_ids': 'shell/browser/resources/resource_ids',
          },
          'includes': [ '../build/grit_action.gypi' ],
        },
      ],
      'includes': [ '../build/grit_target.gypi' ],
    },
    {
      'target_name': 'cast_shell_pak',
      'type': 'none',
      'dependencies': [
        'cast_shell_resources',
        '../content/app/resources/content_resources.gyp:content_resources',
        '../content/app/strings/content_strings.gyp:content_strings',
        '../content/browser/devtools/devtools_resources.gyp:devtools_resources',
        '../content/content_resources.gyp:content_resources',
        '../net/net.gyp:net_resources',
        '../third_party/WebKit/public/blink_resources.gyp:blink_resources',
        '../ui/resources/ui_resources.gyp:ui_resources',
        '../ui/strings/ui_strings.gyp:ui_strings',
      ],
      'actions': [
        {
          'action_name': 'repack_cast_shell_pack',
          'variables': {
            'pak_inputs': [
              '<(SHARED_INTERMEDIATE_DIR)/blink/public/resources/blink_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/chromecast/shell_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/content_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/app/resources/content_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/content/app/strings/content_strings_en-US.pak',
              '<(SHARED_INTERMEDIATE_DIR)/net/net_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/ui_resources_100_percent.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/resources/webui_resources.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/strings/app_locale_settings_en-US.pak',
              '<(SHARED_INTERMEDIATE_DIR)/ui/strings/ui_strings_en-US.pak',
              '<(SHARED_INTERMEDIATE_DIR)/webkit/devtools_resources.pak',
            ],
            'pak_output': '<(PRODUCT_DIR)/assets/cast_shell.pak',
          },
          'includes': [ '../build/repack_action.gypi' ],
        },
      ],
    },
    {
      'target_name': 'cast_shell',
      'type': 'executable',
      'dependencies': [
        'cast_common',
        'cast_service',
        'cast_shell_pak',
        'cast_shell_resources',
        'cast_version_header',
        'chromecast_locales.gyp:chromecast_locales_pak',
        'chromecast_locales.gyp:chromecast_settings',
        '../ui/aura/aura.gyp:aura_test_support',
        '../content/content.gyp:content',
        '../content/content.gyp:content_app_browser',
        '../skia/skia.gyp:skia',
      ],
      'sources': [
        'net/network_change_notifier_cast.cc',
        'net/network_change_notifier_cast.h',
        'net/network_change_notifier_factory_cast.cc',
        'net/network_change_notifier_factory_cast.h',
        'shell/app/cast_main.cc',
        'shell/app/cast_main_delegate.cc',
        'shell/app/cast_main_delegate.h',
        'shell/browser/cast_browser_context.cc',
        'shell/browser/cast_browser_context.h',
        'shell/browser/cast_browser_main_parts.cc',
        'shell/browser/cast_browser_main_parts.h',
        'shell/browser/cast_content_browser_client.cc',
        'shell/browser/cast_content_browser_client.h',
        'shell/browser/cast_http_user_agent_settings.cc',
        'shell/browser/cast_http_user_agent_settings.h',
        'shell/browser/devtools/cast_dev_tools_delegate.cc',
        'shell/browser/devtools/cast_dev_tools_delegate.h',
        'shell/browser/devtools/remote_debugging_server.cc',
        'shell/browser/devtools/remote_debugging_server.h',
        'shell/browser/geolocation/cast_access_token_store.cc',
        'shell/browser/geolocation/cast_access_token_store.h',
        'shell/browser/url_request_context_factory.cc',
        'shell/browser/url_request_context_factory.h',
        'shell/browser/webui/webui_cast.h',
        'shell/common/cast_content_client.cc',
        'shell/common/cast_content_client.h',
        'shell/renderer/cast_content_renderer_client.cc',
        'shell/renderer/cast_content_renderer_client.h',
      ],
      'conditions': [
        ['chromecast_branding=="Chrome"', {
          'dependencies': [
            'internal/chromecast_internal.gyp:cast_gfx_internal',
            'internal/chromecast_internal.gyp:cast_shell_internal',
          ],
        }, {
          'dependencies': [
            '../ui/ozone/ozone.gyp:eglplatform_shim_x11',
          ],
          'sources': [
            'shell/browser/devtools/remote_debugging_server_simple.cc',
            'shell/browser/webui/webui_cast_simple.cc',
          ],
        }],
      ],
    },
    {
      'target_name': 'cast_version_header',
      'type': 'none',
      'direct_dependent_settings': {
        'include_dirs': [
          '<(SHARED_INTERMEDIATE_DIR)',
        ],
      },
      'actions': [
        {
          'action_name': 'version_header',
          'message': 'Generating version header file: <@(_outputs)',
          'inputs': [
            '<(version_path)',
            'common/version.h.in',
          ],
          'outputs': [
            '<(SHARED_INTERMEDIATE_DIR)/chromecast/common/version.h',
          ],
          'action': [
            'python',
            '<(version_py_path)',
            '-e', 'VERSION_FULL="<(version_full)"',
            'common/version.h.in',
            '<@(_outputs)',
          ],
          'includes': [
            '../build/util/version.gypi',
          ],
        },
      ],
    },
  ],  # end of targets
}
