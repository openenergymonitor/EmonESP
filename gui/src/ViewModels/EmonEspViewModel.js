
function EmonEspViewModel(baseHost, basePort, baseProtocol) {
  var self = this;

  self.baseHost = ko.observable("" !== baseHost ? baseHost : "openevse.local");
  self.basePort = ko.observable(basePort);
  self.baseProtocol = ko.observable(baseProtocol);

  self.baseEndpoint = ko.pureComputed(function () {
    var endpoint = "//" + self.baseHost();
    if(80 !== self.basePort()) {
      endpoint += ":"+self.basePort();
    }
    return endpoint;
  });

  self.wsEndpoint = ko.pureComputed(function () {
    var endpoint = "ws://" + self.baseHost();
    if("https:" === self.baseProtocol()){
      endpoint = "wss://" + self.baseHost();
    }
    if(80 !== self.basePort()) {
      endpoint += ":"+self.basePort();
    }
    endpoint += "/ws";
    return endpoint;
  });

  self.config = new ConfigViewModel();
  self.status = new StatusViewModel();
  self.last = new LastValuesViewModel();
  self.scan = new WiFiScanViewModel(self.baseEndpoint);
  self.wifi = new WiFiConfigViewModel(self.baseEndpoint, self.config, self.status, self.scan);

  // Show/hide password state
  self.wifiPassword = new PasswordViewModel(self.config.pass);
  self.emoncmsApiKey = new PasswordViewModel(self.config.emoncms_apikey);
  self.mqttPassword = new PasswordViewModel(self.config.mqtt_pass);
  self.wwwPassword = new PasswordViewModel(self.config.www_password);

  self.initialised = ko.observable(false);
  self.updating = ko.observable(false);

  self.wifi.selectedNet.subscribe((net) => {
    if(false !== net) {
      self.config.ssid(net.ssid());
    }
  });

  self.config.ssid.subscribe((ssid) => {
    self.wifi.setSsid(ssid);
  });
    
  var updateTimer = null;
  var updateTime = 1 * 1000;

  var logUpdateTimer = null;
  var logUpdateTime = 500;
    
  // Upgrade URL
  self.upgradeUrl = ko.observable("about:blank");

  // -----------------------------------------------------------------------
  // Initialise the app
  // -----------------------------------------------------------------------
  self.start = function () {
    self.updating(true);
    self.config.update(function () {
      self.status.update(function () {
        self.last.update(function () {
          self.initialised(true);

          updateTimer = setTimeout(self.update, updateTime);

          self.upgradeUrl(baseEndpoint + "/update");
          self.updating(false);
        });
      });
    });
  };

  // -----------------------------------------------------------------------
  // Get the updated state from the ESP
  // -----------------------------------------------------------------------
  self.update = function () {
    if (self.updating()) {
      return;
    }
    self.updating(true);    
    if (null !== updateTimer) {
      clearTimeout(updateTimer);
      updateTimer = null;
    }
    self.status.update(function () {
      self.last.update(function () {
        updateTimer = setTimeout(self.update, updateTime);
        self.updating(false);
      });
    });
  };

  self.wifiConnecting = ko.observable(false);
  self.status.mode.subscribe(function (newValue) {
    if(newValue === "STA+AP" || newValue === "STA") {
      self.wifiConnecting(false);
    }
  });

  // -----------------------------------------------------------------------
  // Event: WiFi Connect
  // -----------------------------------------------------------------------
  self.saveNetworkFetching = ko.observable(false);
  self.saveNetworkSuccess = ko.observable(false);
  self.saveNetwork = function () {
    if (self.config.ssid() === "") {
      alert("Please select network");
    } else {
      self.saveNetworkFetching(true);
      self.saveNetworkSuccess(false);
      $.post(baseEndpoint + "/savenetwork", { ssid: self.config.ssid(), pass: self.config.pass() }, function (data) {
          self.saveNetworkSuccess(true);
          self.wifiConnecting(true);
        }).fail(function () {
          alert("Failed to save WiFi config");
        }).always(function () {
          self.saveNetworkFetching(false);
        });
    }
  };

  // -----------------------------------------------------------------------
  // Event: Admin save
  // -----------------------------------------------------------------------
  self.saveAdminFetching = ko.observable(false);
  self.saveAdminSuccess = ko.observable(false);
  self.saveAdmin = function () {
    self.saveAdminFetching(true);
    self.saveAdminSuccess(false);
    $.post(baseEndpoint + "/saveadmin", { user: self.config.www_username(), pass: self.config.www_password() }, function (data) {
      self.saveAdminSuccess(true);
    }).fail(function () {
      alert("Failed to save Admin config");
    }).always(function () {
      self.saveAdminFetching(false);
    });
  };
  
  // -----------------------------------------------------------------------
  // Event: Timer save
  // -----------------------------------------------------------------------
  self.saveTimerFetching = ko.observable(false);
  self.saveTimerSuccess = ko.observable(false);
  self.saveTimer = function () {
    self.saveTimerFetching(true);
    self.saveTimerSuccess(false);
    $.post(baseEndpoint + "/savetimer", { 
      timer_start1: self.config.timer_start1(), 
      timer_stop1: self.config.timer_stop1(), 
      timer_start2: self.config.timer_start2(), 
      timer_stop2: self.config.timer_stop2(),
      voltage_output: self.config.voltage_output(),
      time_offset: self.config.time_offset()
    }, function (data) {
      self.saveTimerSuccess(true);
      setTimeout(function(){
          self.saveTimerSuccess(false);
      },5000);
    }).fail(function () {
      alert("Failed to save Timer config");
    }).always(function () {
      self.saveTimerFetching(false);
    });
  };
  
  // -----------------------------------------------------------------------
  // Event: Switch On, Off, Timer
  // -----------------------------------------------------------------------  
  //self.btn_off = ko.observable(false);
  //self.btn_timer = ko.observable(false);
  
  self.ctrlMode = function (mode) {
    var last = self.status.ctrl_mode();
    self.status.ctrl_mode(mode);
    $.post(baseEndpoint + "/ctrlmode?mode="+mode,{}, function (data) {
      // success
    }).fail(function () {
      self.status.ctrl_mode(last);
      alert("Failed to switch "+mode);
    });
  };

  // -----------------------------------------------------------------------
  // Event: Emoncms save
  // -----------------------------------------------------------------------
  self.saveEmonCmsFetching = ko.observable(false);
  self.saveEmonCmsSuccess = ko.observable(false);
  self.saveEmonCms = function () {
    var emoncms = {
      enable: self.config.emoncms_enabled(),
      server: self.config.emoncms_server(),
      port: self.config.emoncms_port(),
      path: self.config.emoncms_path(),
      apikey: self.config.emoncms_apikey(),
      node: self.config.emoncms_node(),
      fingerprint: self.config.emoncms_fingerprint()
    };

    if (emoncms.server === "" || emoncms.node === "") {
      alert("Please enter Emoncms server and node");
    } else if (Number(emoncms.port) % 1 !== 0) {
      // accepts whitespace or integers
      alert("Please enter port number or leave blank");
    } else if (emoncms.apikey.length != 32 && !self.emoncmsApiKey.isDummy()) {
      alert("Please enter valid Emoncms apikey");
    } else if (emoncms.fingerprint !== "" && emoncms.fingerprint.length != 59) {
      alert("Please enter valid SSL SHA-1 fingerprint");
    } else {
      self.saveEmonCmsFetching(true);
      self.saveEmonCmsSuccess(false);
      $.post(baseEndpoint + "/saveemoncms", emoncms, function (data) {
        self.saveEmonCmsSuccess(true);
      }).fail(function () {
        alert("Failed to save Admin config");
      }).always(function () {
        self.saveEmonCmsFetching(false);
      });
    }
  };

  // -----------------------------------------------------------------------
  // Event: MQTT save
  // -----------------------------------------------------------------------
  self.saveMqttFetching = ko.observable(false);
  self.saveMqttSuccess = ko.observable(false);
  self.saveMqtt = function () {
    var mqtt = {
      enable: self.config.mqtt_enabled(),
      server: self.config.mqtt_server(),
      port: self.config.mqtt_port(),
      topic: self.config.mqtt_topic(),
      prefix: self.config.mqtt_feed_prefix(),
      user: self.config.mqtt_user(),
      pass: self.config.mqtt_pass()
    };

    if (mqtt.server === "") {
      alert("Please enter MQTT server");
    } else {
      self.saveMqttFetching(true);
      self.saveMqttSuccess(false);
      $.post(baseEndpoint + "/savemqtt", mqtt, function (data) {
        self.saveMqttSuccess(true);
      }).fail(function () {
        alert("Failed to save MQTT config");
      }).always(function () {
        self.saveMqttFetching(false);
      });
    }
  };
}

  // -----------------------------------------------------------------------
  // Event: Update
  // -----------------------------------------------------------------------

  // Support for OTA update of the OpenEVSE
  self.updateFetching = ko.observable(false);
  self.updateComplete = ko.observable(false);
  self.updateError = ko.observable("");
  self.updateFilename = ko.observable("");
  self.updateLoaded = ko.observable(0);
  self.updateTotal = ko.observable(1);
  self.updateProgress = ko.pureComputed(function () {
    return (self.updateLoaded() / self.updateTotal()) * 100;
  });

  self.otaUpdate = function() {
    if("" === self.updateFilename()) {
      self.updateError("Filename not set");
      return;
    }

    self.updateFetching(true);
    self.updateError("");

    var form = $("#upload_form")[0];
    var data = new FormData(form);

    $.ajax({
      url: "/upload",
      type: "POST",
      data: data,
      contentType: false,
      processData:false,
      xhr: function() {
        var xhr = new window.XMLHttpRequest();
        xhr.upload.addEventListener("progress", function(evt) {
          if (evt.lengthComputable) {
            self.updateLoaded(evt.loaded);
            self.updateTotal(evt.total);
          }
        }, false);
        return xhr;
      }
    }).done(function(msg) {
      console.log(msg);
      if("OK" == msg) {
        self.updateComplete(true);
        setTimeout(() => {
          location.reload();
        }, 5000);
      } else {
        self.updateError(msg);
      }
    }).fail(function () {
      self.updateError("HTTP Update failed");
    }).always(function () {
      self.updateFetching(false);
    });
  };
