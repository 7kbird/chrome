// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

var getCharacteristic = chrome.bluetoothLowEnergy.getCharacteristic;
var charId = 'char_id0';

getCharacteristic(charId, function (result) {
  if (chrome.runtime.lastError) {
    chrome.test.fail(chrome.runtime.lastError.message);
  }

  chrome.test.assertEq(charId, result.instanceId);

  chrome.test.sendMessage('ready', function (message) {
    getCharacteristic(charId, function (result) {
      if (result || !chrome.runtime.lastError) {
        chrome.test.fail('Call to getCharacteristic should have failed');
      }

      chrome.test.sendMessage('ready', function (message) {
        chrome.test.succeed();
      });
    });
  });
});