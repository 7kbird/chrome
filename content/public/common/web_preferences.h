// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_PUBLIC_COMMON_WEB_PREFERENCES_H_
#define CONTENT_PUBLIC_COMMON_WEB_PREFERENCES_H_

#include <map>
#include <string>
#include <vector>

#include "base/strings/string16.h"
#include "content/common/content_export.h"
#include "net/base/network_change_notifier.h"
#include "url/gurl.h"

namespace blink {
class WebView;
}

namespace content {

// Map of ISO 15924 four-letter script code to font family.  For example,
// "Arab" to "My Arabic Font".
typedef std::map<std::string, base::string16> ScriptFontFamilyMap;

enum EditingBehavior {
  EDITING_BEHAVIOR_MAC,
  EDITING_BEHAVIOR_WIN,
  EDITING_BEHAVIOR_UNIX,
  EDITING_BEHAVIOR_ANDROID,
  EDITING_BEHAVIOR_LAST = EDITING_BEHAVIOR_ANDROID
};

enum V8CacheOptions {
  V8_CACHE_OPTIONS_OFF,
  V8_CACHE_OPTIONS_PARSE,
  V8_CACHE_OPTIONS_CODE,
  V8_CACHE_OPTIONS_LAST = V8_CACHE_OPTIONS_CODE
};

// The ISO 15924 script code for undetermined script aka Common. It's the
// default used on WebKit's side to get/set a font setting when no script is
// specified.
CONTENT_EXPORT extern const char kCommonScript[];

// A struct for managing blink's settings.
//
// Adding new values to this class probably involves updating
// blink::WebSettings, content/common/view_messages.h, browser/tab_contents/
// render_view_host_delegate_helper.cc, and browser/profiles/profile.cc.
struct CONTENT_EXPORT WebPreferences {
  ScriptFontFamilyMap standard_font_family_map;
  ScriptFontFamilyMap fixed_font_family_map;
  ScriptFontFamilyMap serif_font_family_map;
  ScriptFontFamilyMap sans_serif_font_family_map;
  ScriptFontFamilyMap cursive_font_family_map;
  ScriptFontFamilyMap fantasy_font_family_map;
  ScriptFontFamilyMap pictograph_font_family_map;
  int default_font_size;
  int default_fixed_font_size;
  int minimum_font_size;
  int minimum_logical_font_size;
  std::string default_encoding;
  bool javascript_enabled;
  bool web_security_enabled;
  bool javascript_can_open_windows_automatically;
  bool loads_images_automatically;
  bool images_enabled;
  bool plugins_enabled;
  bool dom_paste_enabled;
  bool shrinks_standalone_images_to_fit;
  bool uses_universal_detector;
  bool text_areas_are_resizable;
  bool java_enabled;
  bool allow_scripts_to_close_windows;
  bool remote_fonts_enabled;
  bool javascript_can_access_clipboard;
  bool xslt_enabled;
  bool xss_auditor_enabled;
  // We don't use dns_prefetching_enabled to disable DNS prefetching.  Instead,
  // we disable the feature at a lower layer so that we catch non-WebKit uses
  // of DNS prefetch as well.
  bool dns_prefetching_enabled;
  bool local_storage_enabled;
  bool databases_enabled;
  bool application_cache_enabled;
  bool tabs_to_links;
  bool caret_browsing_enabled;
  bool hyperlink_auditing_enabled;
  bool is_online;
  net::NetworkChangeNotifier::ConnectionType connection_type;
  bool allow_universal_access_from_file_urls;
  bool allow_file_access_from_file_urls;
  bool webaudio_enabled;
  bool experimental_webgl_enabled;
  bool pepper_3d_enabled;
  bool flash_3d_enabled;
  bool flash_stage3d_enabled;
  bool flash_stage3d_baseline_enabled;
  bool gl_multisampling_enabled;
  bool privileged_webgl_extensions_enabled;
  bool webgl_errors_to_console_enabled;
  bool mock_scrollbars_enabled;
  bool layer_squashing_enabled;
  bool asynchronous_spell_checking_enabled;
  bool unified_textchecker_enabled;
  bool accelerated_2d_canvas_enabled;
  int minimum_accelerated_2d_canvas_size;
  bool antialiased_2d_canvas_disabled;
  int accelerated_2d_canvas_msaa_sample_count;
  bool accelerated_filters_enabled;
  bool deferred_filters_enabled;
  bool container_culling_enabled;
  bool allow_displaying_insecure_content;
  bool allow_running_insecure_content;
  bool password_echo_enabled;
  bool should_print_backgrounds;
  bool should_clear_document_background;
  bool enable_scroll_animator;
  bool css_variables_enabled;
  bool region_based_columns_enabled;
  bool touch_enabled;
  bool device_supports_touch;
  bool device_supports_mouse;
  bool touch_adjustment_enabled;
  int pointer_events_max_touch_points;
  bool sync_xhr_in_documents_enabled;
  bool deferred_image_decoding_enabled;
  bool should_respect_image_orientation;
  int number_of_cpu_cores;
  EditingBehavior editing_behavior;
  bool supports_multiple_windows;
  bool viewport_enabled;
  bool viewport_meta_enabled;
  bool use_expanded_heuristics_for_gpu_rasterization;
  bool main_frame_resizes_are_orientation_changes;
  bool initialize_at_minimum_page_scale;
  bool smart_insert_delete_enabled;
  bool spatial_navigation_enabled;
  bool pinch_virtual_viewport_enabled;
  int pinch_overlay_scrollbar_thickness;
  bool use_solid_color_scrollbars;
  bool navigate_on_drag_drop;
  V8CacheOptions v8_cache_options;

  // This flags corresponds to a Page's Settings' setCookieEnabled state. It
  // only controls whether or not the "document.cookie" field is properly
  // connected to the backing store, for instance if you wanted to be able to
  // define custom getters and setters from within a unique security content
  // without raising a DOM security exception.
  bool cookie_enabled;

  // This flag indicates whether H/W accelerated video decode is enabled for
  // pepper plugins. Defaults to false.
  bool pepper_accelerated_video_decode_enabled;

#if defined(OS_ANDROID)
  bool text_autosizing_enabled;
  float font_scale_factor;
  float device_scale_adjustment;
  bool force_enable_zoom;
  bool disallow_fullscreen_for_non_media_elements;
  bool fullscreen_supported;
  bool double_tap_to_zoom_enabled;
  bool user_gesture_required_for_media_playback;
  GURL default_video_poster_url;
  bool support_deprecated_target_density_dpi;
  bool use_legacy_background_size_shorthand_behavior;
  bool wide_viewport_quirk;
  bool use_wide_viewport;
  bool force_zero_layout_height;
  bool viewport_meta_layout_size_quirk;
  bool viewport_meta_merge_content_quirk;
  bool viewport_meta_non_user_scalable_quirk;
  bool viewport_meta_zero_values_quirk;
  bool clobber_user_agent_initial_scale_quirk;
  bool ignore_main_frame_overflow_hidden_quirk;
  bool report_screen_size_in_physical_pixels_quirk;
#endif

  // We try to keep the default values the same as the default values in
  // chrome, except for the cases where it would require lots of extra work for
  // the embedder to use the same default value.
  WebPreferences();
  ~WebPreferences();
};

}  // namespace content

#endif  // CONTENT_PUBLIC_COMMON_WEB_PREFERENCES_H_
