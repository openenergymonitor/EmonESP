
function StatusViewModel() {
  var self = this;

  BaseViewModel.call(self, {
    "mode": "ERR",
    "wifi_client_connected": 0,
    "net_connected": 0,
    "srssi": 0,
    "ipaddress": "",
    "packets_sent": 0,
    "packets_success": 0,
    "emoncms_connected": 0,
    "mqtt_connected": 0,
    "free_heap": 0,
    "time":"",
    "ctrl_mode":"off",
    "ctrl_state":0,
    "ota_update": false
  }, baseEndpoint + "/status");

  // Some devired values
  self.isWiFiError = ko.pureComputed(function () {
    return ("ERR" === self.mode());
  });
  self.isWifiClient = ko.pureComputed(function () {
    return ("STA" == self.mode()) || ("STA+AP" == self.mode());
  });
  self.isWifiAccessPoint = ko.pureComputed(function () {
    return ("AP" == self.mode()) || ("STA+AP" == self.mode());
  });
  self.isWired = ko.pureComputed(() => {
    return ("Wired" === self.mode());
  });
  self.fullMode = ko.pureComputed(function () {
    switch (self.mode()) {
      case "AP":
        return "Access Point (AP)";
      case "STA":
        return "Client (STA)";
      case "STA+AP":
        return "Client + Access Point (STA+AP)";
      case "Wired":
        return "Wired Ethernet";
    }

    return "Unknown (" + self.mode() + ")";
  });
}
StatusViewModel.prototype = Object.create(BaseViewModel.prototype);
StatusViewModel.prototype.constructor = StatusViewModel;
