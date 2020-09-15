function LastValuesViewModel() {
  var self = this;
  self.remoteUrl = baseEndpoint + "/lastvalues";

  // Observable properties
  self.fetching = ko.observable(false);
  self.lastValues = ko.observable(false);
  self.values = ko.mapping.fromJS([]);
  self.entries = ko.mapping.fromJS([]);

  let oldData = "";

  self.update = function (after) {
    if(after === undefined){
      after = function () { };
    }
    self.fetching(true);
    $.get(self.remoteUrl, (data) =>
    {
      // Transform the data into something a bit easier to handle as a binding
      var vals = [];
      if (data != "" && data !== oldData)
      {
        oldData = data;
        self.entries.push({
          timestamp: new Date().toISOString(),
          log: data
        });

        try
        {

          var parsed = JSON.parse(data);
          for (var key in parsed) {
            let value = parsed[key];
            var units = "";

            if (key.startsWith("CT")) units = " W";
            if (key.startsWith("P")) units = " W";
            if (key.startsWith("E")) units = " Wh";
            if (key.startsWith("V")) units = " V";
            if (key.startsWith("T")) units = " "+String.fromCharCode(176)+"C";

            vals.push({key: key, value: value+units});
          }
          self.lastValues(true);
          ko.mapping.fromJS(vals, self.values);
        }
        catch(e) {
          console.error(e);
          self.lastValues(false);
        }
      } else {
        self.lastValues(data!="");
      }
    }, "text").always(function () {
      self.fetching(false);
      after();
    });
  };
}
