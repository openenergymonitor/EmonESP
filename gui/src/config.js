// Work out the endpoint to use, for dev you can change to point at a remote ESP
// and run the HTML/JS from file, no need to upload to the ESP to test

// development folder
var development = "";

var baseHost = window.location.hostname;
//var baseHost = 'emonesp.local';
//var baseHost = '192.168.4.1';
//var baseHost = '172.16.0.52';
var basePort = window.location.port;
var baseProtocol = window.location.protocol;

var baseEndpoint = "//" + baseHost;
if (80 !== basePort) {
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
    t = "0" + t;
  }
  return t.substr(0, 2) + ":" + t.substr(2, 2);
}

function addhyphen(d) {
  d = new String(d);
  if (d.length != 8) {
    return "0000-00-00";
  }
  return d.substr(0, 4) + "-" + d.substr(4, 2) + "-" + d.substr(6, 2);
}

$(function () {
  // Activates knockout.js
  var emonesp = new EmonEspViewModel(baseHost, basePort, baseProtocol);
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
    document.getElementById("apoff").style.display = "none";
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
  if (e.style.display == "block") {
    e.previousElementSibling.firstChild.textContent = "+";
    e.style.display = "none";
  } else {
    e.previousElementSibling.firstChild.textContent = "-";
    e.style.display = "block";
  }
}
