// Work out the endpoint to use, for dev you can change to point at a remote ESP
// and run the HTML/JS from file, no need to upload to the ESP to test

// development folder
var development = "";

var baseHost = window.location.hostname;
//var baseHost = 'emonesp.local';
//var baseHost = '192.168.4.1';
//var baseHost = '172.16.0.52';
var basePort = window.location.port;

var baseEndpoint = "//" + baseHost;
if(80 !== basePort) {
  baseEndpoint += ":" + basePort;
}
baseEndpoint += development

var statusupdate = false;
var selected_network_ssid = "";
var lastmode = "";
var ipaddress = "";

// Convert string to number, divide by scale, return result
// as a string with specified precision
function scaleString(string, scale, precision) {
  var tmpval = parseInt(string) / scale;
  return tmpval.toFixed(precision);
}

function addcolon(t) {
  t = new String(t);
  if (t.length < 3) {
    return "00:00";
  }
  if (t.length == 3) {
    t = "0"+t;
  }
  return t.substr(0,2)+":"+t.substr(2,4);
}

function BaseViewModel(defaults, remoteUrl, mappings) {
  if(mappings === undefined){
   mappings = {};
  }
  var self = this;
  self.remoteUrl = remoteUrl;

  // Observable properties
  ko.mapping.fromJS(defaults, mappings, self);
  self.fetching = ko.observable(false);
}

BaseViewModel.prototype.update = function (after) {
  if(after === undefined){
   after = function () { };
  }
  var self = this;
  self.fetching(true);
  $.get(self.remoteUrl, function (data) {
    ko.mapping.fromJS(data, self);
  }, 'json').always(function () {
    self.fetching(false);
    after();
  });
};


function StatusViewModel() {
  var self = this;

  BaseViewModel.call(self, {
    "mode": "ERR",
    "networks": [],
    "rssi": [],
    "srssi": "",
    "ipaddress": "",
    "packets_sent": "",
    "packets_success": "",
    "emoncms_connected": "",
    "mqtt_connected": "",
    "free_heap": "",
    "time":"",
    "ctrl_mode":"off",
    "ctrl_state":0
  }, baseEndpoint + '/status');

  // Some devired values
  self.isWifiClient = ko.pureComputed(function () {
    return ("STA" == self.mode()) || ("STA+AP" == self.mode());
  });
  self.isWifiAccessPoint = ko.pureComputed(function () {
    return ("AP" == self.mode()) || ("STA+AP" == self.mode());
  });
  self.fullMode = ko.pureComputed(function () {
    switch (self.mode()) {
      case "AP":
        return "Access Point (AP)";
      case "STA":
        return "Client (STA)";
      case "STA+AP":
        return "Client + Access Point (STA+AP)";
    }

    return "Unknown (" + self.mode() + ")";
  });
}
StatusViewModel.prototype = Object.create(BaseViewModel.prototype);
StatusViewModel.prototype.constructor = StatusViewModel;

function ConfigViewModel() {
  BaseViewModel.call(this, {
    "node_name": "emonESP",
    "node_description":"WiFi Emoncms Link",
    "node_type": "",
    "ssid": "",
    "pass": "",
    "emoncms_server": "data.openevse.com",
    "emoncms_path": "/emoncms",
    "emoncms_apikey": "",
    "emoncms_node": "",
    "emoncms_fingerprint": "",
    "mqtt_server": "",
    "mqtt_port": "",
    "mqtt_topic": "",
    "mqtt_feed_prefix": "",
    "mqtt_user": "",
    "mqtt_pass": "",
    "www_username": "",
    "www_password": "",
    "espflash": "",
    "version": "0.0.0",
    "timer_start1":"",
    "timer_stop1":"",
    "timer_start2":"",
    "timer_stop2":"",
    "voltage_output":"",
    "time_offset":""
  }, baseEndpoint + '/config');
  
  this.f_timer_start1 = ko.pureComputed({
      read: function () {
          return addcolon(this.timer_start1());
      },
      write: function (value) {
          this.timer_start1(value.replace(":",""));
      },
      owner: this
  });
  this.f_timer_stop1 = ko.pureComputed({
      read: function () {
          return addcolon(this.timer_stop1());
      },
      write: function (value) {
          this.timer_stop1(value.replace(":",""));
      },
      owner: this
  });
  this.f_timer_start2 = ko.pureComputed({
      read: function () {
          return addcolon(this.timer_start2());
      },
      write: function (value) {
          this.timer_start2(value.replace(":",""));
      },
      owner: this
  });
  this.f_timer_stop2 = ko.pureComputed({
      read: function () {
          return addcolon(this.timer_stop2());
      },
      write: function (value) {
          this.timer_stop2(value.replace(":",""));
      },
      owner: this
  });
  this.flowT = ko.pureComputed({
      read: function () {
          return (this.voltage_output()*0.0371)+7.14;
      },
      write: function (value) {
          this.voltage_output((value-7.14)/0.0371);
      },
      owner: this
  });
  
}
ConfigViewModel.prototype = Object.create(BaseViewModel.prototype);
ConfigViewModel.prototype.constructor = ConfigViewModel;

function LastValuesViewModel() {
  var self = this;
  self.remoteUrl = baseEndpoint + '/lastvalues';

  // Observable properties
  self.fetching = ko.observable(false);
  self.lastValues = ko.observable(false);
  self.values = ko.mapping.fromJS([]);

  self.update = function (after) {
    if(after === undefined){
     after = function () { };
    }
    self.fetching(true);
    $.get(self.remoteUrl, function (data) {
      // Transform the data into something a bit easier to handle as a binding
      var vals = [];
      if (data!="") {
        var namevaluepairs = data.split(",");
        if (namevaluepairs.length>0) {
          for (var z in namevaluepairs) {
            var namevalue = namevaluepairs[z].split(":");
            var units = "";
            if (namevalue[0].indexOf("CT") === 0) units = "W";
            if (namevalue[0].indexOf("T") === 0) units = String.fromCharCode(176)+"C";
            vals.push({key: namevalue[0], value: namevalue[1]+units});
          }
          self.lastValues(true);
        }
      } else {
        self.lastValues(false);
      }
      ko.mapping.fromJS(vals, self.values);
    }, 'text').always(function () {
      self.fetching(false);
      after();
    });
  };
}

function LogsViewModel() {
  var self = this;
  self.remoteUrl = baseEndpoint + "/lastvalues";

  // Observable properties
  self.fetching = ko.observable(false);
  self.entries = ko.mapping.fromJS([]);

  let oldData = "";

  self.update = function (after) {
    if (after === undefined) {
      after = function () { };
    }
    self.fetching(true);

    $.get(
      self.remoteUrl,
      function (data) {
        if (data !== oldData) {
          var logEntries = self.entries.slice();
          logEntries.push({
            timestamp: new Date().toISOString(),
            log: data
          });

          ko.mapping.fromJS(logEntries, self.entries);
          oldData = data;
        }
      },
      "text"
    ).always(function () {
      self.fetching(false);
      after();
    });
  };
}

function EmonEspViewModel() {
  var self = this;

  self.config = new ConfigViewModel();
  self.status = new StatusViewModel();
  self.last = new LastValuesViewModel();
  self.logs = new LogsViewModel();

  self.initialised = ko.observable(false);
  self.updating = ko.observable(false);
    
  var updateTimer = null;
  var updateTime = 1 * 1000;

  var logUpdateTimer = null;
  var logUpdateTime = 500;
    
  // Upgrade URL
  self.upgradeUrl = ko.observable('about:blank');

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
          logUpdateTimer = setTimeout(self.updateLogs, logUpdateTime);

          self.upgradeUrl(baseEndpoint + '/update');
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

  self.updateLogs = function () {
    if (null !== logUpdateTimer) {
      clearTimeout(logUpdateTimer);
      logUpdateTimer = null;
    }
    self.logs.update(function () {
      logUpdateTimer = setTimeout(self.updateLogs, logUpdateTime);
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

$(function () {
  // Activates knockout.js
  var emonesp = new EmonEspViewModel();
  ko.applyBindings(emonesp);
  emonesp.start();
});

// -----------------------------------------------------------------------
// Event: Turn off Access Point
// -----------------------------------------------------------------------
document.getElementById("apoff").addEventListener("click", function (e) {

  var r = new XMLHttpRequest();
  r.open("POST", "apoff", true);
  r.onreadystatechange = function () {
    if (r.readyState != 4 || r.status != 200)
      return;
    var str = r.responseText;
    console.log(str);
    document.getElementById("apoff").style.display = 'none';
    if (ipaddress !== "")
      window.location = "http://" + ipaddress;
  };
  r.send();
});

// -----------------------------------------------------------------------
// Event: Reset config and reboot
// -----------------------------------------------------------------------
document.getElementById("reset").addEventListener("click", function (e) {

  if (confirm("CAUTION: Do you really want to Factory Reset? All setting and config will be lost.")) {
    var r = new XMLHttpRequest();
    r.open("POST", "reset", true);
    r.onreadystatechange = function () {
      if (r.readyState != 4 || r.status != 200)
        return;
      var str = r.responseText;
      console.log(str);
      if (str !== 0)
        document.getElementById("reset").innerHTML = "Resetting...";
    };
    r.send();
  }
});

// -----------------------------------------------------------------------
// Event: Restart
// -----------------------------------------------------------------------
document.getElementById("restart").addEventListener("click", function (e) {

  if (confirm("Restart emonESP? Current config will be saved, takes approximately 10s.")) {
    var r = new XMLHttpRequest();
    r.open("POST", "restart", true);
    r.onreadystatechange = function () {
      if (r.readyState != 4 || r.status != 200)
        return;
      var str = r.responseText;
      console.log(str);
      if (str !== 0)
        document.getElementById("reset").innerHTML = "Restarting";
    };
    r.send();
  }
});

function toggle(id) {
   var e = document.getElementById(id);
   if(e.style.display == 'block')
      e.style.display = 'none';
   else
      e.style.display = 'block';
}

// -----------------------------------------------------------------------
// Event:Upload Firmware
// -----------------------------------------------------------------------
//document.getElementById("upload").addEventListener("click", function(e) {
//  window.location.href='/upload'
//});
