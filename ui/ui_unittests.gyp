# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'variables': {
    'chromium_code': 1,
  },
  'targets': [
    {
      'target_name': 'ui_unittests',
      'type': '<(gtest_target_type)',
      'dependencies': [
        '../base/base.gyp:base',
        '../base/base.gyp:test_support_base',
        '../net/net.gyp:net',
        '../skia/skia.gyp:skia',
        '../testing/gmock.gyp:gmock',
        '../testing/gtest.gyp:gtest',
        '../third_party/icu/icu.gyp:icui18n',
        '../third_party/icu/icu.gyp:icuuc',
        '../url/url.gyp:url_lib',
        'base/ui_base.gyp:ui_base',
        'base/ui_base.gyp:ui_base_test_support',
        'events/events.gyp:events_base',
        'gfx/gfx.gyp:gfx_test_support',
        'resources/ui_resources.gyp:ui_resources',
        'resources/ui_resources.gyp:ui_test_pak',
        'strings/ui_strings.gyp:ui_strings',
      ],
      # iOS uses a small subset of ui. common_sources are the only files that
      # are built on iOS.
      'common_sources' : [
        # Note: file list duplicated in GN build.
        'base/layout_unittest.cc',
        'base/l10n/l10n_util_mac_unittest.mm',
        'base/l10n/l10n_util_unittest.cc',
        'base/l10n/l10n_util_win_unittest.cc',
        'base/l10n/time_format_unittest.cc',
        'base/models/tree_node_iterator_unittest.cc',
        'base/resource/data_pack_literal.cc',
        'base/resource/data_pack_unittest.cc',
        'base/resource/resource_bundle_unittest.cc',
        'base/test/run_all_unittests.cc',
      ],
      'all_sources': [
        # Note: file list duplicated in GN build.
        '<@(_common_sources)',
        'base/accelerators/accelerator_manager_unittest.cc',
        'base/accelerators/menu_label_accelerator_util_linux_unittest.cc',
        'base/clipboard/custom_data_helper_unittest.cc',
        'base/cocoa/base_view_unittest.mm',
        'base/cocoa/cocoa_base_utils_unittest.mm',
        'base/cocoa/controls/blue_label_button_unittest.mm',
        'base/cocoa/controls/hover_image_menu_button_unittest.mm',
        'base/cocoa/controls/hyperlink_button_cell_unittest.mm',
        'base/cocoa/focus_tracker_unittest.mm',
        'base/cocoa/fullscreen_window_manager_unittest.mm',
        'base/cocoa/hover_image_button_unittest.mm',
        'base/cocoa/menu_controller_unittest.mm',
        'base/cocoa/nsgraphics_context_additions_unittest.mm',
        'base/cocoa/tracking_area_unittest.mm',
        'base/dragdrop/os_exchange_data_provider_aurax11_unittest.cc',
        'base/ime/candidate_window_unittest.cc',
        'base/ime/chromeos/character_composer_unittest.cc',
        'base/ime/composition_text_util_pango_unittest.cc',
        'base/ime/input_method_base_unittest.cc',
        'base/ime/input_method_chromeos_unittest.cc',
        'base/ime/remote_input_method_win_unittest.cc',
        'base/ime/win/imm32_manager_unittest.cc',
        'base/ime/win/tsf_input_scope_unittest.cc',
        'base/models/list_model_unittest.cc',
        'base/models/list_selection_model_unittest.cc',
        'base/models/tree_node_model_unittest.cc',
        'base/test/data/resource.h',
        'base/text/bytes_formatting_unittest.cc',
        'base/view_prop_unittest.cc',
        'base/webui/web_ui_util_unittest.cc',
        'base/x/selection_requestor_unittest.cc',
      ],
      'include_dirs': [
        '../',
      ],
      'conditions': [
        ['OS!="ios"', {
          'sources' : ['<@(_all_sources)'],
        }, {  # OS=="ios"
          'sources' : [
            '<@(_common_sources)',
          ],
          # The ResourceBundle unittest expects a locale.pak file to exist in
          # the bundle for English-US. Copy it in from where it was generated
          # by ui_resources.gyp:ui_test_pak.
          'mac_bundle_resources': [
            '<(PRODUCT_DIR)/ui/en.lproj/locale.pak',
          ],
          'actions': [
            {
              'action_name': 'copy_test_data',
              'variables': {
                'test_data_files': [
                  'base/test/data',
                ],
                'test_data_prefix' : 'ui',
              },
              'includes': [ '../build/copy_test_data_ios.gypi' ],
            },
          ],
        }],
        ['OS == "win"', {
          'sources': [
            'base/dragdrop/os_exchange_data_win_unittest.cc',
            'base/win/hwnd_subclass_unittest.cc',
            'base/win/open_file_name_win_unittest.cc',
          ],
          'msvs_settings': {
            'VCLinkerTool': {
              'DelayLoadDLLs': [
                'd2d1.dll',
                'd3d10_1.dll',
              ],
              'AdditionalDependencies': [
                'd2d1.lib',
                'd3d10_1.lib',
              ],
            },
          },
          'link_settings': {
            'libraries': [
              '-limm32.lib',
              '-loleacc.lib',
            ],
          },
          # TODO(jschuh): crbug.com/167187 fix size_t to int truncations.
          'msvs_disabled_warnings': [ 4267, ],
        }],
        ['OS == "android"', {
          'dependencies': [
            '../testing/android/native_test.gyp:native_test_native_code',
          ],
        }],
        ['use_pango == 1', {
          'dependencies': [
            '../build/linux/system.gyp:pangocairo',
          ],
          'conditions': [
            ['use_allocator!="none"', {
               'dependencies': [
                 '../base/allocator/allocator.gyp:allocator',
               ],
            }],
          ],
        }],
        ['use_x11==1', {
          'dependencies': [
            '../build/linux/system.gyp:x11',
            '../tools/xdisplaycheck/xdisplaycheck.gyp:xdisplaycheck',
            'events/platform/x11/x11_events_platform.gyp:x11_events_platform',
            'gfx/x/gfx_x11.gyp:gfx_x11',
          ],
        }],
        ['OS!="win" or use_aura==0', {
          'sources!': [
            'base/view_prop_unittest.cc',
          ],
        }],
        ['use_x11==1 and use_aura==1',  {
          'sources': [
            'base/cursor/cursor_loader_x11_unittest.cc',
          ],
        }],
        ['OS=="mac"',  {
          'dependencies': [
            '../third_party/mozilla/mozilla.gyp:mozilla',
            'events/events.gyp:events_test_support',
            'ui_unittests_bundle',
          ],
          'conditions': [
            ['component=="static_library"', {
              # Needed for mozilla.gyp.
              'xcode_settings': {'OTHER_LDFLAGS': ['-Wl,-ObjC']},
            }],
          ],
        }],
        ['use_aura==1 or toolkit_views==1',  {
          'sources': [
            'base/dragdrop/os_exchange_data_unittest.cc',
          ],
          'dependencies': [
            'events/events.gyp:events',
            'events/events.gyp:events_base',
            'events/events.gyp:events_test_support',
            'events/platform/events_platform.gyp:events_platform',
          ],
        }],
        ['chromeos==1', {
          'dependencies': [
            '../chromeos/chromeos.gyp:chromeos',
            'aura/aura.gyp:aura_test_support',
            'chromeos/ui_chromeos.gyp:ui_chromeos',
            'events/events.gyp:gesture_detection',
          ],
          'sources': [
            'chromeos/touch_exploration_controller_unittest.cc'
          ],
          'sources!': [
            'base/dragdrop/os_exchange_data_provider_aurax11_unittest.cc',
            'base/x/selection_requestor_unittest.cc',
          ],
        }],
        ['use_x11==0', {
          'sources!': [
            'base/ime/chromeos/character_composer_unittest.cc',
            'base/ime/input_method_chromeos_unittest.cc',
            'base/ime/composition_text_util_pango_unittest.cc',
          ],
        }],
      ],
      'target_conditions': [
        ['OS == "ios"', {
          'sources/': [
            # Pull in specific Mac files for iOS (which have been filtered out
            # by file name rules).
            ['include', '^base/l10n/l10n_util_mac_unittest\\.mm$'],
          ],
        }],
      ],
    },
  ],
  'conditions': [
    # Mac target to build a test Framework bundle to mock out resource loading.
    ['OS == "mac"', {
      'targets': [
        {
          'target_name': 'ui_unittests_bundle',
          'type': 'shared_library',
          'dependencies': [
            'resources/ui_resources.gyp:ui_test_pak',
          ],
          'includes': [ 'ui_unittests_bundle.gypi' ],
        },
      ],
    }],
    ['OS == "android"', {
      'targets': [
        {
          'target_name': 'ui_unittests_apk',
          'type': 'none',
          'dependencies': [
            'ui_unittests',
          ],
          'variables': {
            'test_suite_name': 'ui_unittests',
          },
          'includes': [ '../build/apk_test.gypi' ],
        },
      ],
    }],
  ],
}
