// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Event management for WebViewInternal.

var DeclarativeWebRequestSchema =
    requireNative('schema_registry').GetSchema('declarativeWebRequest');
var EventBindings = require('event_bindings');
var IdGenerator = requireNative('id_generator');
var MessagingNatives = requireNative('messaging_natives');
var WebRequestEvent = require('webRequestInternal').WebRequestEvent;
var WebRequestSchema =
    requireNative('schema_registry').GetSchema('webRequest');
var WebView = require('webViewInternal').WebView;

var CreateEvent = function(name) {
  var eventOpts = {supportsListeners: true, supportsFilters: true};
  return new EventBindings.Event(name, undefined, eventOpts);
};

var FrameNameChangedEvent = CreateEvent('webViewInternal.onFrameNameChanged');
var WebRequestMessageEvent = CreateEvent('webViewInternal.onMessage');

// WEB_VIEW_EVENTS is a map of stable <webview> DOM event names to their
//     associated extension event descriptor objects.
// An event listener will be attached to the extension event |evt| specified in
//     the descriptor.
// |fields| specifies the public-facing fields in the DOM event that are
//     accessible to <webview> developers.
// |customHandler| allows a handler function to be called each time an extension
//     event is caught by its event listener. The DOM event should be dispatched
//     within this handler function. With no handler function, the DOM event
//     will be dispatched by default each time the extension event is caught.
// |cancelable| (default: false) specifies whether the event's default
//     behavior can be canceled. If the default action associated with the event
//     is prevented, then its dispatch function will return false in its event
//     handler. The event must have a custom handler for this to be meaningful.
var WEB_VIEW_EVENTS = {
  'close': {
    evt: CreateEvent('webViewInternal.onClose'),
    fields: []
  },
  'consolemessage': {
    evt: CreateEvent('webViewInternal.onConsoleMessage'),
    fields: ['level', 'message', 'line', 'sourceId']
  },
  'contentload': {
    evt: CreateEvent('webViewInternal.onContentLoad'),
    fields: []
  },
  'contextmenu': {
    evt: CreateEvent('webViewInternal.contextmenu'),
    cancelable: true,
    customHandler: function(handler, event, webViewEvent) {
      handler.handleContextMenu(event, webViewEvent);
    },
    fields: ['items']
  },
  'dialog': {
    cancelable: true,
    customHandler: function(handler, event, webViewEvent) {
      handler.handleDialogEvent(event, webViewEvent);
    },
    evt: CreateEvent('webViewInternal.onDialog'),
    fields: ['defaultPromptText', 'messageText', 'messageType', 'url']
  },
  'exit': {
     evt: CreateEvent('webViewInternal.onExit'),
     fields: ['processId', 'reason']
  },
  'findupdate': {
    evt: CreateEvent('webViewInternal.onFindReply'),
    fields: [
      'searchText',
      'numberOfMatches',
      'activeMatchOrdinal',
      'selectionRect',
      'canceled',
      'finalUpdate'
    ]
  },
  'loadabort': {
    cancelable: true,
    customHandler: function(handler, event, webViewEvent) {
      handler.handleLoadAbortEvent(event, webViewEvent);
    },
    evt: CreateEvent('webViewInternal.onLoadAbort'),
    fields: ['url', 'isTopLevel', 'reason']
  },
  'loadcommit': {
    customHandler: function(handler, event, webViewEvent) {
      handler.handleLoadCommitEvent(event, webViewEvent);
    },
    evt: CreateEvent('webViewInternal.onLoadCommit'),
    fields: ['url', 'isTopLevel']
  },
  'loadprogress': {
    evt: CreateEvent('webViewInternal.onLoadProgress'),
    fields: ['url', 'progress']
  },
  'loadredirect': {
    evt: CreateEvent('webViewInternal.onLoadRedirect'),
    fields: ['isTopLevel', 'oldUrl', 'newUrl']
  },
  'loadstart': {
    evt: CreateEvent('webViewInternal.onLoadStart'),
    fields: ['url', 'isTopLevel']
  },
  'loadstop': {
    evt: CreateEvent('webViewInternal.onLoadStop'),
    fields: []
  },
  'newwindow': {
    cancelable: true,
    customHandler: function(handler, event, webViewEvent) {
      handler.handleNewWindowEvent(event, webViewEvent);
    },
    evt: CreateEvent('webViewInternal.onNewWindow'),
    fields: [
      'initialHeight',
      'initialWidth',
      'targetUrl',
      'windowOpenDisposition',
      'name'
    ]
  },
  'permissionrequest': {
    cancelable: true,
    customHandler: function(handler, event, webViewEvent) {
      handler.handlePermissionEvent(event, webViewEvent);
    },
    evt: CreateEvent('webViewInternal.onPermissionRequest'),
    fields: [
      'identifier',
      'lastUnlockedBySelf',
      'name',
      'permission',
      'requestMethod',
      'url',
      'userGesture'
    ]
  },
  'responsive': {
    evt: CreateEvent('webViewInternal.onResponsive'),
    fields: ['processId']
  },
  'sizechanged': {
    evt: CreateEvent('webViewInternal.onSizeChanged'),
    customHandler: function(handler, event, webViewEvent) {
      handler.handleSizeChangedEvent(event, webViewEvent);
    },
    fields: ['oldHeight', 'oldWidth', 'newHeight', 'newWidth']
  },
  'unresponsive': {
    evt: CreateEvent('webViewInternal.onUnresponsive'),
    fields: ['processId']
  },
  'zoomchange': {
    evt: CreateEvent('webViewInternal.onZoomChange'),
    fields: ['oldZoomFactor', 'newZoomFactor']
  }
};

function DeclarativeWebRequestEvent(opt_eventName,
                                    opt_argSchemas,
                                    opt_eventOptions,
                                    opt_webViewInstanceId) {
  var subEventName = opt_eventName + '/' + IdGenerator.GetNextId();
  EventBindings.Event.call(this, subEventName, opt_argSchemas, opt_eventOptions,
      opt_webViewInstanceId);

  var self = this;
  // TODO(lazyboy): When do we dispose this listener?
  WebRequestMessageEvent.addListener(function() {
    // Re-dispatch to subEvent's listeners.
    $Function.apply(self.dispatch, self, $Array.slice(arguments));
  }, {instanceId: opt_webViewInstanceId || 0});
}

DeclarativeWebRequestEvent.prototype = {
  __proto__: EventBindings.Event.prototype
};

// Constructor.
function WebViewEvents(webViewInternal, viewInstanceId) {
  this.webViewInternal = webViewInternal;
  this.viewInstanceId = viewInstanceId;
  this.setup();
}

// Sets up events.
WebViewEvents.prototype.setup = function() {
  this.setupFrameNameChangedEvent();
  this.setupWebRequestEvents();
  this.webViewInternal.setupExperimentalContextMenus();

  var events = this.getEvents();
  for (var eventName in events) {
    this.setupEvent(eventName, events[eventName]);
  }
};

WebViewEvents.prototype.setupFrameNameChangedEvent = function() {
  var self = this;
  FrameNameChangedEvent.addListener(function(e) {
    self.webViewInternal.onFrameNameChanged(e.name);
  }, {instanceId: self.viewInstanceId});
};

WebViewEvents.prototype.setupWebRequestEvents = function() {
  var self = this;
  var request = {};
  var createWebRequestEvent = function(webRequestEvent) {
    return function() {
      if (!self[webRequestEvent.name]) {
        self[webRequestEvent.name] =
            new WebRequestEvent(
                'webViewInternal.' + webRequestEvent.name,
                webRequestEvent.parameters,
                webRequestEvent.extraParameters, webRequestEvent.options,
                self.viewInstanceId);
      }
      return self[webRequestEvent.name];
    };
  };

  var createDeclarativeWebRequestEvent = function(webRequestEvent) {
    return function() {
      if (!self[webRequestEvent.name]) {
        // The onMessage event gets a special event type because we want
        // the listener to fire only for messages targeted for this particular
        // <webview>.
        var EventClass = webRequestEvent.name === 'onMessage' ?
            DeclarativeWebRequestEvent : EventBindings.Event;
        self[webRequestEvent.name] =
            new EventClass(
                'webViewInternal.' + webRequestEvent.name,
                webRequestEvent.parameters,
                webRequestEvent.options,
                self.viewInstanceId);
      }
      return self[webRequestEvent.name];
    };
  };

  for (var i = 0; i < DeclarativeWebRequestSchema.events.length; ++i) {
    var eventSchema = DeclarativeWebRequestSchema.events[i];
    var webRequestEvent = createDeclarativeWebRequestEvent(eventSchema);
    Object.defineProperty(
        request,
        eventSchema.name,
        {
          get: webRequestEvent,
          enumerable: true
        }
    );
  }

  // Populate the WebRequest events from the API definition.
  for (var i = 0; i < WebRequestSchema.events.length; ++i) {
    var webRequestEvent = createWebRequestEvent(WebRequestSchema.events[i]);
    Object.defineProperty(
        request,
        WebRequestSchema.events[i].name,
        {
          get: webRequestEvent,
          enumerable: true
        }
    );
  }

  this.webViewInternal.setRequestPropertyOnWebViewNode(request);
};

WebViewEvents.prototype.getEvents = function() {
  var experimentalEvents = this.webViewInternal.maybeGetExperimentalEvents();
  for (var eventName in experimentalEvents) {
    WEB_VIEW_EVENTS[eventName] = experimentalEvents[eventName];
  }
  return WEB_VIEW_EVENTS;
};

WebViewEvents.prototype.setupEvent = function(name, info) {
  var self = this;
  info.evt.addListener(function(e) {
    var details = {bubbles:true};
    if (info.cancelable)
      details.cancelable = true;
    var webViewEvent = new Event(name, details);
    $Array.forEach(info.fields, function(field) {
      if (e[field] !== undefined) {
        webViewEvent[field] = e[field];
      }
    });
    if (info.customHandler) {
      info.customHandler(self, e, webViewEvent);
      return;
    }
    self.webViewInternal.dispatchEvent(webViewEvent);
  }, {instanceId: self.viewInstanceId});

  this.webViewInternal.setupEventProperty(name);
};


// Event handlers.
WebViewEvents.prototype.handleContextMenu = function(e, webViewEvent) {
  this.webViewInternal.maybeHandleContextMenu(e, webViewEvent);
};

WebViewEvents.prototype.handleDialogEvent = function(event, webViewEvent) {
  var showWarningMessage = function(dialogType) {
    var VOWELS = ['a', 'e', 'i', 'o', 'u'];
    var WARNING_MSG_DIALOG_BLOCKED = '<webview>: %1 %2 dialog was blocked.';
    var article = (VOWELS.indexOf(dialogType.charAt(0)) >= 0) ? 'An' : 'A';
    var output = WARNING_MSG_DIALOG_BLOCKED.replace('%1', article);
    output = output.replace('%2', dialogType);
    window.console.warn(output);
  };

  var self = this;
  var requestId = event.requestId;
  var actionTaken = false;

  var validateCall = function() {
    var ERROR_MSG_DIALOG_ACTION_ALREADY_TAKEN = '<webview>: ' +
        'An action has already been taken for this "dialog" event.';

    if (actionTaken) {
      throw new Error(ERROR_MSG_DIALOG_ACTION_ALREADY_TAKEN);
    }
    actionTaken = true;
  };

  var getGuestInstanceId = function() {
    return self.webViewInternal.getGuestInstanceId();
  };

  var dialog = {
    ok: function(user_input) {
      validateCall();
      user_input = user_input || '';
      WebView.setPermission(getGuestInstanceId(), requestId, 'allow',
                            user_input);
    },
    cancel: function() {
      validateCall();
      WebView.setPermission(getGuestInstanceId(), requestId, 'deny');
    }
  };
  webViewEvent.dialog = dialog;

  var defaultPrevented = !self.webViewInternal.dispatchEvent(webViewEvent);
  if (actionTaken) {
    return;
  }

  if (defaultPrevented) {
    // Tell the JavaScript garbage collector to track lifetime of |dialog| and
    // call back when the dialog object has been collected.
    MessagingNatives.BindToGC(dialog, function() {
      // Avoid showing a warning message if the decision has already been made.
      if (actionTaken) {
        return;
      }
      WebView.setPermission(
          getGuestInstanceId(), requestId, 'default', '', function(allowed) {
        if (allowed) {
          return;
        }
        showWarningMessage(event.messageType);
      });
    });
  } else {
    actionTaken = true;
    // The default action is equivalent to canceling the dialog.
    WebView.setPermission(
        getGuestInstanceId(), requestId, 'default', '', function(allowed) {
      if (allowed) {
        return;
      }
      showWarningMessage(event.messageType);
    });
  }
};

WebViewEvents.prototype.handleLoadAbortEvent = function(event, webViewEvent) {
  var showWarningMessage = function(reason) {
    var WARNING_MSG_LOAD_ABORTED = '<webview>: ' +
        'The load has aborted with reason "%1".';
    window.console.warn(WARNING_MSG_LOAD_ABORTED.replace('%1', reason));
  };
  if (this.webViewInternal.dispatchEvent(webViewEvent)) {
    showWarningMessage(event.reason);
  }
};

WebViewEvents.prototype.handleLoadCommitEvent = function(event, webViewEvent) {
  this.webViewInternal.onLoadCommit(event.currentEntryIndex, event.entryCount,
                                    event.processId, event.url,
                                    event.isTopLevel);
  this.webViewInternal.dispatchEvent(webViewEvent);
};

WebViewEvents.prototype.handleNewWindowEvent = function(event, webViewEvent) {
  var ERROR_MSG_NEWWINDOW_ACTION_ALREADY_TAKEN = '<webview>: ' +
      'An action has already been taken for this "newwindow" event.';

  var ERROR_MSG_NEWWINDOW_UNABLE_TO_ATTACH = '<webview>: ' +
      'Unable to attach the new window to the provided webViewInternal.';

  var ERROR_MSG_WEBVIEW_EXPECTED = '<webview> element expected.';

  var showWarningMessage = function() {
    var WARNING_MSG_NEWWINDOW_BLOCKED = '<webview>: A new window was blocked.';
    window.console.warn(WARNING_MSG_NEWWINDOW_BLOCKED);
  };

  var requestId = event.requestId;
  var actionTaken = false;
  var self = this;
  var getGuestInstanceId = function() {
    return self.webViewInternal.getGuestInstanceId();
  };

  var validateCall = function () {
    if (actionTaken) {
      throw new Error(ERROR_MSG_NEWWINDOW_ACTION_ALREADY_TAKEN);
    }
    actionTaken = true;
  };

  var windowObj = {
    attach: function(webview) {
      validateCall();
      if (!webview || !webview.tagName || webview.tagName != 'WEBVIEW')
        throw new Error(ERROR_MSG_WEBVIEW_EXPECTED);
      // Attach happens asynchronously to give the tagWatcher an opportunity
      // to pick up the new webview before attach operates on it, if it hasn't
      // been attached to the DOM already.
      // Note: Any subsequent errors cannot be exceptions because they happen
      // asynchronously.
      setTimeout(function() {
        var webViewInternal = privates(webview).internal;
        // Update the partition.
        if (event.storagePartitionId) {
          webViewInternal.onAttach(event.storagePartitionId);
        }

        var attached = webViewInternal.attachWindow(event.windowId, true);

        if (!attached) {
          window.console.error(ERROR_MSG_NEWWINDOW_UNABLE_TO_ATTACH);
        }
        // If the object being passed into attach is not a valid <webview>
        // then we will fail and it will be treated as if the new window
        // was rejected. The permission API plumbing is used here to clean
        // up the state created for the new window if attaching fails.
        WebView.setPermission(
            getGuestInstanceId(), requestId, attached ? 'allow' : 'deny');
      }, 0);
    },
    discard: function() {
      validateCall();
      WebView.setPermission(getGuestInstanceId(), requestId, 'deny');
    }
  };
  webViewEvent.window = windowObj;

  var defaultPrevented = !self.webViewInternal.dispatchEvent(webViewEvent);
  if (actionTaken) {
    return;
  }

  if (defaultPrevented) {
    // Make browser plugin track lifetime of |windowObj|.
    MessagingNatives.BindToGC(windowObj, function() {
      // Avoid showing a warning message if the decision has already been made.
      if (actionTaken) {
        return;
      }
      WebView.setPermission(
          getGuestInstanceId(), requestId, 'default', '', function(allowed) {
        if (allowed) {
          return;
        }
        showWarningMessage();
      });
    });
  } else {
    actionTaken = true;
    // The default action is to discard the window.
    WebView.setPermission(
        getGuestInstanceId(), requestId, 'default', '', function(allowed) {
      if (allowed) {
        return;
      }
      showWarningMessage();
    });
  }
};

WebViewEvents.prototype.getPermissionTypes = function() {
  var permissions =
      ['media',
      'geolocation',
      'pointerLock',
      'download',
      'loadplugin',
      'filesystem'];
  return permissions.concat(
      this.webViewInternal.maybeGetExperimentalPermissions());
};

WebViewEvents.prototype.handlePermissionEvent =
    function(event, webViewEvent) {
  var ERROR_MSG_PERMISSION_ALREADY_DECIDED = '<webview>: ' +
      'Permission has already been decided for this "permissionrequest" event.';

  var showWarningMessage = function(permission) {
    var WARNING_MSG_PERMISSION_DENIED = '<webview>: ' +
        'The permission request for "%1" has been denied.';
    window.console.warn(
        WARNING_MSG_PERMISSION_DENIED.replace('%1', permission));
  };

  var requestId = event.requestId;
  var self = this;
  var getGuestInstanceId = function() {
    return self.webViewInternal.getGuestInstanceId();
  };

  if (this.getPermissionTypes().indexOf(event.permission) < 0) {
    // The permission type is not allowed. Trigger the default response.
    WebView.setPermission(
        getGuestInstanceId(), requestId, 'default', '', function(allowed) {
      if (allowed) {
        return;
      }
      showWarningMessage(event.permission);
    });
    return;
  }

  var decisionMade = false;
  var validateCall = function() {
    if (decisionMade) {
      throw new Error(ERROR_MSG_PERMISSION_ALREADY_DECIDED);
    }
    decisionMade = true;
  };

  // Construct the event.request object.
  var request = {
    allow: function() {
      validateCall();
      WebView.setPermission(getGuestInstanceId(), requestId, 'allow');
    },
    deny: function() {
      validateCall();
      WebView.setPermission(getGuestInstanceId(), requestId, 'deny');
    }
  };
  webViewEvent.request = request;

  var defaultPrevented = !self.webViewInternal.dispatchEvent(webViewEvent);
  if (decisionMade) {
    return;
  }

  if (defaultPrevented) {
    // Make browser plugin track lifetime of |request|.
    MessagingNatives.BindToGC(request, function() {
      // Avoid showing a warning message if the decision has already been made.
      if (decisionMade) {
        return;
      }
      WebView.setPermission(
          getGuestInstanceId(), requestId, 'default', '', function(allowed) {
        if (allowed) {
          return;
        }
        showWarningMessage(event.permission);
      });
    });
  } else {
    decisionMade = true;
    WebView.setPermission(
        getGuestInstanceId(), requestId, 'default', '',
        function(allowed) {
          if (allowed) {
            return;
          }
          showWarningMessage(event.permission);
        });
  }
};

WebViewEvents.prototype.handleSizeChangedEvent = function(
    event, webViewEvent) {
  this.webViewInternal.onSizeChanged(webViewEvent);
};

exports.WebViewEvents = WebViewEvents;
exports.CreateEvent = CreateEvent;
