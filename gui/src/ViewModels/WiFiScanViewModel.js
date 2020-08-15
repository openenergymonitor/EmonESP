/* global $, ko */
/* exported WiFiScanViewModel */

function WiFiScanResultViewModel(data)
{
  "use strict";
  var self = this;
  ko.mapping.fromJS(data, {}, self);

  // Return a strength from 0-5 based on rssi, https://eyesaas.com/wi-fi-signal-strength/
  self.strength = ko.computed(() => {
    var rssi = self.rssi();

    if(rssi >= -50) {
      return 5;
    }
    if(rssi >= -60) {
      return 4;
    }
    if(rssi >= -67) {
      return 3;
    }
    if(rssi >= -70) {
      return 2;
    }
    if(rssi >= -80) {
      return 1;
    }

    return 0;
  });
}

function WiFiScanViewModel(baseEndpoint)
{
  "use strict";
  var self = this;
  var endpoint = ko.pureComputed(function () { return baseEndpoint() + "/scan"; });

  const netListMappingSettings = {
    key: function(data) {
      return ko.utils.unwrapObservable(data.bssid);
    },
    create: function (options) {
      return new WiFiScanResultViewModel(options.data);
    }
  };

  self.results = ko.mapping.fromJS([], netListMappingSettings);
  self.filteredResults = ko.mapping.fromJS([], netListMappingSettings);

  // Observable properties
  self.fetching = ko.observable(false);

  self.update = function (after = function () { }) {
    self.fetching(true);
    $.get(endpoint(), function (data) {
      if(data.length > 0) {
        data = data.sort(function (left, right) {
          if(left.ssid === right.ssid) {
            return left.rssi < right.rssi ? 1 : -1;
          }
          return left.ssid < right.ssid ? -1 : 1;
        });

        ko.mapping.fromJS(data, self.results);

        const uniqueArray = data.filter((net, index) => {
          return net.rssi >= -80 && index === data.findIndex(obj => {
            return net.ssid === obj.ssid;
          });
        }).sort(function (left, right) {
          return left.rssi < right.rssi ? 1 : -1;
        });

        ko.mapping.fromJS(uniqueArray, self.filteredResults);
      }
    }, "json").always(function () {
      self.fetching(false);
      after();
    });
  };
}
