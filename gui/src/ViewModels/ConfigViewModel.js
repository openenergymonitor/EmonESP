function ConfigViewModel() {
  BaseViewModel.call(this, {
    "node_name": "emonESP",
    "node_description": "WiFi Emoncms Link",
    "node_type": "",
    "ssid": "",
    "pass": "",
    "emoncms_enabled": false,
    "emoncms_server": "data.openevse.com",
    "emoncms_path": "/emoncms",
    "emoncms_apikey": "",
    "emoncms_node": "",
    "emoncms_fingerprint": "",
    "mqtt_enabled": false,
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
    "timer_start1": "",
    "timer_stop1": "",
    "timer_start2": "",
    "timer_stop2": "",
    "standby_start": "",
    "standby_stop": "",
    "rotation": false,
    "voltage_output": "",
    "time_offset": ""
  }, baseEndpoint + "/config");

  this.f_timer_start1 = ko.pureComputed({
    read: function () {
      return addcolon(this.timer_start1());
    },
    write: function (value) {
      this.timer_start1(value.replace(":", ""));
    },
    owner: this
  });
  this.f_timer_stop1 = ko.pureComputed({
    read: function () {
      return addcolon(this.timer_stop1());
    },
    write: function (value) {
      this.timer_stop1(value.replace(":", ""));
    },
    owner: this
  });
  this.f_timer_start2 = ko.pureComputed({
    read: function () {
      return addcolon(this.timer_start2());
    },
    write: function (value) {
      this.timer_start2(value.replace(":", ""));
    },
    owner: this
  });
  this.f_timer_stop2 = ko.pureComputed({
    read: function () {
      return addcolon(this.timer_stop2());
    },
    write: function (value) {
      this.timer_stop2(value.replace(":", ""));
    },
    owner: this
  });
  this.flowT = ko.pureComputed({
    read: function () {
      return (this.voltage_output() * 0.0371) + 7.14;
    },
    write: function (value) {
      this.voltage_output((value - 7.14) / 0.0371);
    },
    owner: this
  });
  this.f_standby_start = ko.pureComputed({
    read: function () {
      return addhyphen(this.standby_start());
    },
    write: function (value) {
      this.standby_start(value.replaceAll("-", ""));
    },
    owner: this
  });
  this.f_standby_stop = ko.pureComputed({
    read: function () {
      return addhyphen(this.standby_stop());
    },
    write: function (value) {
      this.standby_stop(value.replaceAll("-", ""));
    },
    owner: this
  });

}
ConfigViewModel.prototype = Object.create(BaseViewModel.prototype);
ConfigViewModel.prototype.constructor = ConfigViewModel;
