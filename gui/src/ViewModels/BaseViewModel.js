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
  }, "json").always(function () {
    self.fetching(false);
    after();
  });
};
