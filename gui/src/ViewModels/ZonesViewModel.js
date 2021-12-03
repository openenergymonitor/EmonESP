/* global ko, BaseViewModel */

function TimeZoneViewModel(data)
{
  "use strict";
  var self = this;

  self.name = ko.observable(data.name);
  self.value = ko.observable(data.name+"|"+data.tz);
}

function ZonesViewModel(baseEndpoint) {
  "use strict";
  var self = this;
  var endpoint = ko.pureComputed(function () { return baseEndpoint() + "/zones.json"; });

  // Observable properties
  self.fetching = ko.observable(false);
  self.zones = ko.mapping.fromJS([], {
    key: function(data) {
      return ko.utils.unwrapObservable(data.name);
    },
    create: function (options) {
      return new TimeZoneViewModel(options.data);
    }
  });

  self.initialValue = function (val) {
    var parts = val.split("|", 2);
    if(2 == parts.length)
    {
      self.zones.push(new TimeZoneViewModel({
        name: parts[0],
        tz: parts[1]
      }));
    }
    else
    {
      self.zones.push(new TimeZoneViewModel({
        name: "Default",
        tz: parts[1]
      }));
    }
  };

  self.update = function (after = function () { }) {
    self.fetching(true);
    $.get(endpoint(), function (data) {
      var zones = [];
      for (const name in data) {
        if (data.hasOwnProperty(name)) {
          zones.push({name:name, tz:data[name]});
        }
      }

      ko.mapping.fromJS(zones, self.zones);
    }, "json").always(function () {
      self.fetching(false);
      after();
    });
  };
}
