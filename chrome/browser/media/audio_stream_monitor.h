// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CHROME_BROWSER_MEDIA_AUDIO_STREAM_MONITOR_H_
#define CHROME_BROWSER_MEDIA_AUDIO_STREAM_MONITOR_H_

#include <map>

#include "base/callback_forward.h"
#include "base/threading/thread_checker.h"
#include "base/time/default_tick_clock.h"
#include "base/time/time.h"
#include "base/timer/timer.h"
#include "content/public/browser/web_contents_user_data.h"

namespace base {
class TickClock;
}

// Repeatedly polls audio streams for their power levels, and "debounces" the
// information into a simple, binary "was recently audible" result for the audio
// indicators in the tab UI.  The debouncing logic is to: 1) Turn on immediately
// when sound is audible; and 2) Hold on for X amount of time after sound has
// gone silent, then turn off.  Said another way, we don't want tab indicators
// to turn on/off repeatedly and annoy the user.  AudioStreamMonitor sends UI
// update notifications only when needed, but may be queried at any time.
//
// There are zero or one instances of AudioStreamMonitor per
// content::WebContents instance (referred to as "the tab" in comments below).
// AudioStreamMonitor is created on-demand, and automatically destroyed when its
// associated WebContents is destroyed.  See content::WebContentsUserData for
// usage.
class AudioStreamMonitor
    : public content::WebContentsUserData<AudioStreamMonitor> {
 public:
  // Returns true if audio has recently been audible from the tab.  This is
  // usually called whenever the tab data model is refreshed; but there are
  // other use cases as well (e.g., the OOM killer uses this to de-prioritize
  // the killing of tabs making sounds).
  bool WasRecentlyAudible() const;

  // Starts polling the stream for audio stream power levels using |callback|.
  typedef base::Callback<std::pair<float, bool>()> ReadPowerAndClipCallback;
  void StartMonitoringStream(int stream_id,
                             const ReadPowerAndClipCallback& callback);

  // Stops polling the stream, discarding the internal copy of the |callback|
  // provided in the call to StartMonitoringStream().
  void StopMonitoringStream(int stream_id);

 private:
  friend class content::WebContentsUserData<AudioStreamMonitor>;
  friend class AudioStreamMonitorTest;

  enum {
    // Desired polling frequency.  Note: If this is set too low, short-duration
    // "blip" sounds won't be detected.  http://crbug.com/339133#c4
    kPowerMeasurementsPerSecond = 15,

    // Amount of time to hold a tab indicator on after its last blurt.
    kHoldOnMilliseconds = 2000
  };

  // Invoked by content::WebContentsUserData only.
  explicit AudioStreamMonitor(content::WebContents* contents);
  virtual ~AudioStreamMonitor();

  // Called by |poll_timer_| to sample the power levels from each of the streams
  // playing in the tab.
  void Poll();

  // Compares last known indicator state with what it should be, and triggers UI
  // updates through |web_contents_| if needed.  When the indicator is turned
  // on, |off_timer_| is started to re-invoke this method in the future.
  void MaybeToggle();

  // The WebContents instance instance to receive indicator toggle
  // notifications.  This pointer should be valid for the lifetime of
  // AudioStreamMonitor.
  content::WebContents* const web_contents_;

  // Note: |clock_| is always |&default_tick_clock_|, except during unit
  // testing.
  base::DefaultTickClock default_tick_clock_;
  base::TickClock* const clock_;

  // Confirms single-threaded access in debug builds.
  base::ThreadChecker thread_checker_;

  // The callbacks to read power levels for each stream.  Only playing (i.e.,
  // not paused) streams will have an entry in this map.
  typedef std::map<int, ReadPowerAndClipCallback> StreamPollCallbackMap;
  StreamPollCallbackMap poll_callbacks_;

  // Records the last time at which sound was audible from any stream.
  base::TimeTicks last_blurt_time_;

  // Set to true if the last call to MaybeToggle() determined the indicator
  // should be turned on.
  bool was_recently_audible_;

  // Calls Poll() at regular intervals while |poll_callbacks_| is non-empty.
  base::RepeatingTimer<AudioStreamMonitor> poll_timer_;

  // Started only when an indicator is toggled on, to turn it off again in the
  // future.
  base::OneShotTimer<AudioStreamMonitor> off_timer_;

  DISALLOW_COPY_AND_ASSIGN(AudioStreamMonitor);
};

#endif  // CHROME_BROWSER_MEDIA_AUDIO_STREAM_MONITOR_H_