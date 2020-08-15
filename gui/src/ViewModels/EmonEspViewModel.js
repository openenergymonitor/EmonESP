
function EmonEspViewModel() {
  var self = this;

  self.config = new ConfigViewModel();
  self.status = new StatusViewModel();
  self.last = new LastValuesViewModel();

  self.initialised = ko.observable(false);
  self.updating = ko.observable(false);
    
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
      server: self.config.emoncms_server(),
      path: self.config.emoncms_path(),
      apikey: self.config.emoncms_apikey(),
      node: self.config.emoncms_node(),
      fingerprint: self.config.emoncms_fingerprint()
    };

    if (emoncms.server === "" || emoncms.node === "") {
      alert("Please enter Emoncms server and node");
    } else if (emoncms.apikey.length != 32) {
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
