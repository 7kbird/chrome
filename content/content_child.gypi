# Copyright 2013 The Chromium Authors. All rights reserved.
# Use of this source code is governed by a BSD-style license that can be
# found in the LICENSE file.

{
  'dependencies': [
    '../base/base.gyp:base',
    '../components/tracing.gyp:tracing',
    '../mojo/mojo_base.gyp:mojo_application_bindings',
    '../mojo/mojo_base.gyp:mojo_environment_chromium',
    '../skia/skia.gyp:skia',
    '../ui/base/ui_base.gyp:ui_base',
    '../ui/gfx/gfx.gyp:gfx',
    '../ui/gfx/gfx.gyp:gfx_geometry',
    '../url/url.gyp:url_lib',
  ],
  'include_dirs': [
    '..',
  ],
  'export_dependent_settings': [
    '../base/base.gyp:base',
  ],
  'variables': {
    'public_child_sources': [
      'public/child/image_decoder_utils.h',
      'public/child/request_peer.h',
      'public/child/resource_dispatcher_delegate.h',
    ],
    'private_child_sources': [
      'child/appcache/appcache_backend_proxy.cc',
      'child/appcache/appcache_backend_proxy.h',
      'child/appcache/appcache_dispatcher.cc',
      'child/appcache/appcache_dispatcher.h',
      'child/appcache/appcache_frontend_impl.cc',
      'child/appcache/appcache_frontend_impl.h',
      'child/appcache/web_application_cache_host_impl.cc',
      'child/appcache/web_application_cache_host_impl.h',
      'child/assert_matching_enums.cc',
      'child/blink_platform_impl.cc',
      'child/blink_platform_impl.h',
      'child/browser_font_resource_trusted.cc',
      'child/browser_font_resource_trusted.h',
      'child/child_histogram_message_filter.cc',
      'child/child_histogram_message_filter.h',
      'child/child_message_filter.cc',
      'child/child_message_filter.h',
      'child/child_process.cc',
      'child/child_process.h',
      'child/child_resource_message_filter.cc',
      'child/child_resource_message_filter.h',
      'child/child_shared_bitmap_manager.cc',
      'child/child_shared_bitmap_manager.h',
      'child/child_thread.cc',
      'child/child_thread.h',
      'child/content_child_helpers.cc',
      'child/content_child_helpers.h',
      'child/database_util.cc',
      'child/database_util.h',
      'child/db_message_filter.cc',
      'child/db_message_filter.h',
      'child/file_info_util.cc',
      'child/file_info_util.h',
      'child/fileapi/file_system_dispatcher.cc',
      'child/fileapi/file_system_dispatcher.h',
      'child/fileapi/webfilesystem_impl.cc',
      'child/fileapi/webfilesystem_impl.h',
      'child/fileapi/webfilewriter_base.cc',
      'child/fileapi/webfilewriter_base.h',
      'child/fileapi/webfilewriter_impl.cc',
      'child/fileapi/webfilewriter_impl.h',
      'child/fling_animator_impl_android.cc',
      'child/fling_animator_impl_android.h',
      'child/fling_curve_configuration.cc',
      'child/fling_curve_configuration.h',
      'child/ftp_directory_listing_response_delegate.cc',
      'child/ftp_directory_listing_response_delegate.h',
      'child/image_decoder.cc',
      'child/image_decoder.h',
      'child/indexed_db/indexed_db_dispatcher.cc',
      'child/indexed_db/indexed_db_dispatcher.h',
      'child/indexed_db/indexed_db_key_builders.cc',
      'child/indexed_db/indexed_db_key_builders.h',
      'child/indexed_db/indexed_db_message_filter.cc',
      'child/indexed_db/indexed_db_message_filter.h',
      'child/indexed_db/webidbcursor_impl.cc',
      'child/indexed_db/webidbcursor_impl.h',
      'child/indexed_db/webidbdatabase_impl.cc',
      'child/indexed_db/webidbdatabase_impl.h',
      'child/indexed_db/webidbfactory_impl.cc',
      'child/indexed_db/webidbfactory_impl.h',
      'child/mojo/mojo_application.cc',
      'child/mojo/mojo_application.h',
      'child/multipart_response_delegate.cc',
      'child/multipart_response_delegate.h',
      'child/npapi/np_channel_base.cc',
      'child/npapi/np_channel_base.h',
      'child/npapi/npobject_base.h',
      'child/npapi/npobject_proxy.cc',
      'child/npapi/npobject_proxy.h',
      'child/npapi/npobject_stub.cc',
      'child/npapi/npobject_stub.h',
      'child/npapi/npobject_util.cc',
      'child/npapi/npobject_util.h',
      'child/npapi/npruntime_util.cc',
      'child/npapi/npruntime_util.h',
      'child/npapi/plugin_host.cc',
      'child/npapi/plugin_host.h',
      'child/npapi/plugin_instance.cc',
      'child/npapi/plugin_instance.h',
      'child/npapi/plugin_instance_mac.mm',
      'child/npapi/plugin_lib.cc',
      'child/npapi/plugin_lib.h',
      'child/npapi/plugin_stream.cc',
      'child/npapi/plugin_stream.h',
      'child/npapi/plugin_stream_posix.cc',
      'child/npapi/plugin_stream_url.cc',
      'child/npapi/plugin_stream_url.h',
      'child/npapi/plugin_stream_win.cc',
      'child/npapi/plugin_string_stream.cc',
      'child/npapi/plugin_string_stream.h',
      'child/npapi/plugin_url_fetcher.cc',
      'child/npapi/plugin_url_fetcher.h',
      'child/npapi/plugin_web_event_converter_mac.h',
      'child/npapi/plugin_web_event_converter_mac.mm',
      'child/npapi/webplugin.h',
      'child/npapi/webplugin_accelerated_surface_mac.h',
      'child/npapi/webplugin_delegate.h',
      'child/npapi/webplugin_delegate_impl.cc',
      'child/npapi/webplugin_delegate_impl.h',
      'child/npapi/webplugin_delegate_impl_android.cc',
      'child/npapi/webplugin_delegate_impl_aura.cc',
      'child/npapi/webplugin_delegate_impl_mac.mm',
      'child/npapi/webplugin_delegate_impl_win.cc',
      'child/npapi/webplugin_ime_win.cc',
      'child/npapi/webplugin_ime_win.h',
      'child/npapi/webplugin_resource_client.h',
      'child/plugin_message_generator.cc',
      'child/plugin_message_generator.h',
      'child/plugin_messages.h',
      'child/plugin_param_traits.cc',
      'child/plugin_param_traits.h',
      'child/power_monitor_broadcast_source.cc',
      'child/power_monitor_broadcast_source.h',
      'child/quota_dispatcher.cc',
      'child/quota_dispatcher.h',
      'child/quota_message_filter.cc',
      'child/quota_message_filter.h',
      'child/request_extra_data.cc',
      'child/request_extra_data.h',
      'child/request_info.cc',
      'child/request_info.h',
      'child/resource_dispatcher.cc',
      'child/resource_dispatcher.h',
      'child/runtime_features.cc',
      'child/runtime_features.h',
      'child/scoped_child_process_reference.cc',
      'child/scoped_child_process_reference.h',
      'child/service_worker/service_worker_dispatcher.cc',
      'child/service_worker/service_worker_dispatcher.h',
      'child/service_worker/service_worker_handle_reference.cc',
      'child/service_worker/service_worker_handle_reference.h',
      'child/service_worker/service_worker_message_filter.cc',
      'child/service_worker/service_worker_message_filter.h',
      'child/service_worker/service_worker_network_provider.cc',
      'child/service_worker/service_worker_network_provider.h',
      'child/service_worker/service_worker_provider_context.cc',
      'child/service_worker/service_worker_provider_context.h',
      'child/service_worker/service_worker_registration_handle_reference.cc',
      'child/service_worker/service_worker_registration_handle_reference.h',
      'child/service_worker/web_service_worker_impl.cc',
      'child/service_worker/web_service_worker_impl.h',
      'child/service_worker/web_service_worker_provider_impl.cc',
      'child/service_worker/web_service_worker_provider_impl.h',
      'child/service_worker/web_service_worker_registration_impl.cc',
      'child/service_worker/web_service_worker_registration_impl.h',
      'child/shared_worker_devtools_agent.cc',
      'child/shared_worker_devtools_agent.h',
      'child/simple_webmimeregistry_impl.cc',
      'child/simple_webmimeregistry_impl.h',
      'child/site_isolation_policy.cc',
      'child/site_isolation_policy.h',
      'child/socket_stream_dispatcher.cc',
      'child/socket_stream_dispatcher.h',
      'child/sync_load_response.cc',
      'child/sync_load_response.h',
      'child/thread_safe_sender.cc',
      'child/thread_safe_sender.h',
      'child/threaded_data_provider.cc',
      'child/threaded_data_provider.h',
      'child/touch_fling_gesture_curve.cc',
      'child/touch_fling_gesture_curve.h',
      'child/web_database_observer_impl.cc',
      'child/web_database_observer_impl.h',
      'child/web_discardable_memory_impl.cc',
      'child/web_discardable_memory_impl.h',
      'child/web_socket_stream_handle_bridge.h',
      'child/web_socket_stream_handle_delegate.h',
      'child/web_socket_stream_handle_impl.cc',
      'child/web_socket_stream_handle_impl.h',
      'child/web_url_loader_impl.cc',
      'child/web_url_loader_impl.h',
      'child/web_url_request_util.cc',
      'child/web_url_request_util.h',
      'child/webblobregistry_impl.cc',
      'child/webblobregistry_impl.h',
      'child/webcrypto/algorithm_dispatch.cc',
      'child/webcrypto/algorithm_dispatch.h',
      'child/webcrypto/algorithm_implementation.cc',
      'child/webcrypto/algorithm_implementation.h',
      'child/webcrypto/algorithm_registry.cc',
      'child/webcrypto/algorithm_registry.h',
      'child/webcrypto/crypto_data.cc',
      'child/webcrypto/crypto_data.h',
      'child/webcrypto/jwk.cc',
      'child/webcrypto/jwk.h',
      'child/webcrypto/platform_crypto.h',
      'child/webcrypto/status.cc',
      'child/webcrypto/status.h',
      'child/webcrypto/structured_clone.cc',
      'child/webcrypto/structured_clone.h',
      'child/webcrypto/webcrypto_impl.cc',
      'child/webcrypto/webcrypto_impl.h',
      'child/webcrypto/webcrypto_util.cc',
      'child/webcrypto/webcrypto_util.h',
      'child/webfallbackthemeengine_impl.cc',
      'child/webfallbackthemeengine_impl.h',
      'child/webfileutilities_impl.cc',
      'child/webfileutilities_impl.h',
      'child/webmessageportchannel_impl.cc',
      'child/webmessageportchannel_impl.h',
      'child/websocket_bridge.cc',
      'child/websocket_bridge.h',
      'child/websocket_dispatcher.cc',
      'child/websocket_dispatcher.h',
      'child/webthemeengine_impl_android.cc',
      'child/webthemeengine_impl_android.h',
      'child/webthemeengine_impl_default.cc',
      'child/webthemeengine_impl_default.h',
      'child/webthemeengine_impl_mac.cc',
      'child/webthemeengine_impl_mac.h',
      'child/webthread_impl.cc',
      'child/webthread_impl.h',
      'child/weburlresponse_extradata_impl.cc',
      'child/weburlresponse_extradata_impl.h',
      'child/worker_task_runner.cc',
      'child/worker_task_runner.h',
      'child/worker_thread_task_runner.cc',
      'child/worker_thread_task_runner.h',
    ],
    'webcrypto_nss_sources': [
      'child/webcrypto/nss/aes_cbc_nss.cc',
      'child/webcrypto/nss/aes_gcm_nss.cc',
      'child/webcrypto/nss/aes_key_nss.cc',
      'child/webcrypto/nss/aes_key_nss.h',
      'child/webcrypto/nss/aes_kw_nss.cc',
      'child/webcrypto/nss/hmac_nss.cc',
      'child/webcrypto/nss/key_nss.cc',
      'child/webcrypto/nss/key_nss.h',
      'child/webcrypto/nss/rsa_key_nss.cc',
      'child/webcrypto/nss/rsa_key_nss.h',
      'child/webcrypto/nss/rsa_oaep_nss.cc',
      'child/webcrypto/nss/rsa_ssa_nss.cc',
      'child/webcrypto/nss/sha_nss.cc',
      'child/webcrypto/nss/sym_key_nss.cc',
      'child/webcrypto/nss/sym_key_nss.h',
      'child/webcrypto/nss/util_nss.cc',
      'child/webcrypto/nss/util_nss.h',
    ],
    'webcrypto_openssl_sources': [
      'child/webcrypto/openssl/aes_cbc_openssl.cc',
      'child/webcrypto/openssl/aes_gcm_openssl.cc',
      'child/webcrypto/openssl/aes_key_openssl.cc',
      'child/webcrypto/openssl/aes_key_openssl.h',
      'child/webcrypto/openssl/aes_kw_openssl.cc',
      'child/webcrypto/openssl/hmac_openssl.cc',
      'child/webcrypto/openssl/key_openssl.cc',
      'child/webcrypto/openssl/key_openssl.h',
      'child/webcrypto/openssl/rsa_key_openssl.cc',
      'child/webcrypto/openssl/rsa_key_openssl.h',
      'child/webcrypto/openssl/rsa_oaep_openssl.cc',
      'child/webcrypto/openssl/rsa_ssa_openssl.cc',
      'child/webcrypto/openssl/sha_openssl.cc',
      'child/webcrypto/openssl/sym_key_openssl.cc',
      'child/webcrypto/openssl/sym_key_openssl.h',
      'child/webcrypto/openssl/util_openssl.cc',
      'child/webcrypto/openssl/util_openssl.h',
    ],
  },
  'sources': [
    '<@(public_child_sources)',
    '<@(private_child_sources)',
  ],
  'conditions': [
    ['use_default_render_theme==0',
      {
        'sources/': [
          ['exclude', 'child/webthemeengine_impl_default.cc'],
          ['exclude', 'child/webthemeengine_impl_default.h'],
        ],
      }
    ],
    ['OS=="android"', {
      'includes': [
        '../build/android/cpufeatures.gypi',
      ],
    }],
    ['enable_plugins==0', {
      'sources!': [
        'child/browser_font_resource_trusted.cc',
      ],
      'sources/': [
        ['exclude', '^child/npapi/plugin_'],
        ['exclude', '^child/npapi/webplugin_'],
      ],
    }],
    ['OS=="ios"', {
      'sources/': [
        # iOS only needs a small portion of content; exclude all the
        # implementation, and re-include what is used.
        ['exclude', '\\.(cc|mm)$'],
      ],
    }, {  # OS!="ios"
      'dependencies': [
        'app/resources/content_resources.gyp:content_resources',
        'app/strings/content_strings.gyp:content_strings',
        '../third_party/WebKit/public/blink.gyp:blink',
        '../third_party/WebKit/public/blink_resources.gyp:blink_resources',
        '../third_party/npapi/npapi.gyp:npapi',
        '../webkit/child/webkit_child.gyp:webkit_child',
        '../webkit/common/webkit_common.gyp:webkit_common',
      ],
    }],
    ['use_aura==1', {
      'sources!': [
        'child/npapi/webplugin_delegate_impl_mac.mm',
      ],
    }],
    ['OS=="win"', {
      'sources!': [
        'child/npapi/webplugin_delegate_impl_aura.cc',
      ],
    }],
    ['use_openssl==1', {
      'sources': [
        '<@(webcrypto_openssl_sources)',
      ],
      'dependencies': [
        '../third_party/boringssl/boringssl.gyp:boringssl',
      ],
    }, {
      'sources': [
        '<@(webcrypto_nss_sources)',
      ],
      'conditions': [
        ['os_posix == 1 and OS != "mac" and OS != "ios" and OS != "android"', {
          'dependencies': [
            '../build/linux/system.gyp:ssl',
          ],
        }, {
          'dependencies': [
            '../third_party/nss/nss.gyp:nspr',
            '../third_party/nss/nss.gyp:nss',
          ],
        }],
      ],
    }],
  ],
}