var statusupdate = false;
var selected_network_ssid = "";
var lastmode = "";
var ipaddress = "";

var r1 = new XMLHttpRequest();
r1.open("GET", "status", false);
r1.onreadystatechange = function () {
  if (r1.readyState != 4 || r1.status != 200) return;
  var status = JSON.parse(r1.responseText);

  document.getElementById("passkey").value = status.pass;
  document.getElementById("emoncms_apikey").value = status.emoncms_apikey;
  document.getElementById("free_heap").innerHTML = status.free_heap;
  document.getElementById("flash_size").innerHTML = status.flash_size;
  document.getElementById("vcc").innerHTML = status.vcc;
  
  if (status.emoncms_connected == "1"){
   document.getElementById("emoncms_connected").innerHTML = "Yes";
  } else {
    document.getElementById("emoncms_connected").innerHTML = "No";
  }
  
  if (status.mqtt_connected == "1"){
   document.getElementById("mqtt_connected").innerHTML = "Yes";
  } else {
    document.getElementById("mqtt_connected").innerHTML = "No";
  }

  if (status.mode=="AP") {
      document.getElementById("mode").innerHTML = "Access Point (AP)";
      document.getElementById("client-view").style.display = 'none';
      document.getElementById("ap-view").style.display = '';

      var out = "";
      for (var z in status.networks) {
        if (status.rssi[z]="undefined") status.rssi[z]="";
        out += "<tr><td><input class='networkcheckbox' name='"+status.networks[z]+"' type='checkbox'></td><td>"+status.networks[z]+"</td><td>"+status.rssi[z]+"</td></tr>";
      }
      document.getElementById("networks").innerHTML = out;
  } else {
      if (status.mode=="STA+AP") {
          document.getElementById("mode").innerHTML = "Client + Access Point (STA+AP)";
          document.getElementById("apoff").style.display = '';
      }
      if (status.mode=="STA") document.getElementById("mode").innerHTML = "Client (STA)";
      document.getElementById("sta-ssid").innerHTML = status.ssid;
      document.getElementById("sta-ip").innerHTML = "<a href='http://"+status.ipaddress+"'>"+status.ipaddress+"</a>";
      document.getElementById("sta-psent").innerHTML = status.packets_sent;
      document.getElementById("sta-psuccess").innerHTML = status.packets_success;
      document.getElementById("ap-view").style.display = 'none';
      document.getElementById("client-view").style.display = '';
      ipaddress = status.ipaddress;
  }
};
r1.send();


update();
setInterval(update,10000);

// -----------------------------------------------------------------------
// Periodic 10s update of last data values and packets sent
// -----------------------------------------------------------------------
function update() {

    var r = new XMLHttpRequest();
    r.open("GET", "lastvalues", true);
    r.onreadystatechange = function () {
	    if (r.readyState != 4 || r.status != 200) return;
	    var str = r.responseText;
	    var namevaluepairs = str.split(",");
	    var out = "";
	    for (var z in namevaluepairs) {
	        var namevalue = namevaluepairs[z].split(":");
	        var units = "";
	        if (namevalue[0].indexOf("CT")==0) units = "W";
	        if (namevalue[0].indexOf("T")==0) units = "&deg;C";
	        out += "<tr><td>"+namevalue[0]+"</td><td>"+namevalue[1]+units+"</td></tr>";
	    }
	    document.getElementById("datavalues").innerHTML = out;
    };
    r.send();

    var r2 = new XMLHttpRequest();
    r2.open("GET", "status", false);
    r2.onreadystatechange = function () {
    if (r2.readyState != 4 || r2.status != 200) return;
      var status = JSON.parse(r2.responseText);
      document.getElementById("sta-psent").innerHTML = status.packets_sent;
      document.getElementById("sta-psuccess").innerHTML = status.packets_success;
      document.getElementById("free_heap").innerHTML = status.free_heap;
      document.getElementById("flash_size").innerHTML = status.flash_size;
      document.getElementById("vcc").innerHTML = status.vcc;
      
      
      if (status.emoncms_connected == "1"){
       document.getElementById("emoncms_connected").innerHTML = "Yes";
      } else {
        document.getElementById("emoncms_connected").innerHTML = "No";
      }
  
      if (status.mqtt_connected == "1"){
       document.getElementById("mqtt_connected").innerHTML = "Yes";
      } else {
       document.getElementById("mqtt_connected").innerHTML = "No";
      }
    };
    r2.send();
}

function updateStatus() {
  var r1 = new XMLHttpRequest();
  r1.open("GET", "status", true);
  r1.timeout = 2000;
  r1.onreadystatechange = function () {
    if (r1.readyState != 4 || r1.status != 200) return;
    var status = JSON.parse(r1.responseText);

    document.getElementById("emoncms_apikey").value = emoncms_apikey;

    if (status.mode=="STA+AP" || status.mode=="STA") {
        // Hide waiting message
        document.getElementById("wait-view").style.display = 'none';
        // Display mode
        if (status.mode=="STA+AP") {
            document.getElementById("mode").innerHTML = "Client + Access Point (STA+AP)";
            document.getElementById("apoff").style.display = '';
        }
        if (status.mode=="STA") document.getElementById("mode").innerHTML = "Client (STA)";
        document.getElementById("sta-ssid").innerHTML = status.ssid;
        document.getElementById("sta-ip").innerHTML = "<a href='http://"+status.ipaddress+"'>"+status.ipaddress+"</a>";
        document.getElementById("sta-psent").innerHTML = status.packets_sent;
        document.getElementById("sta-psuccess").innerHTML = status.packets_success;

        // View display
        document.getElementById("ap-view").style.display = 'none';
        document.getElementById("client-view").style.display = '';
    }
    lastmode = status.mode;
  };
  r1.send();
}

// -----------------------------------------------------------------------
// Event: Connect
// -----------------------------------------------------------------------
document.getElementById("connect").addEventListener("click", function(e) {
    var passkey = document.getElementById("passkey").value;
    if (selected_network_ssid=="") {
        alert("Please select network");
    } else {
        document.getElementById("ap-view").style.display = 'none';
        document.getElementById("wait-view").style.display = '';

        var r = new XMLHttpRequest();
        r.open("POST", "savenetwork", false);
        r.setRequestHeader("Content-type","application/x-www-form-urlencoded");
        r.onreadystatechange = function () {
	        if (r.readyState != 4 || r.status != 200) return;
	        var str = r.responseText;
	        console.log(str);
	        document.getElementById("connect").innerHTML = "Connecting...please wait 10s";

	        statusupdate = setInterval(updateStatus,5000);
        };
        r.send("ssid="+selected_network_ssid+"&pass="+passkey);
    }
});

// -----------------------------------------------------------------------
// Event: Emoncms save
// -----------------------------------------------------------------------
document.getElementById("save-emoncms").addEventListener("click", function(e) {
    var emoncms = {
      server: document.getElementById("emoncms_server").value,
      apikey: document.getElementById("emoncms_apikey").value,
      node: document.getElementById("emoncms_node").value
    }
    if (emoncms.server=="" || emoncms.node==""){
        alert("Please enter Emoncms server and node");
      } else if (emoncms.apikey.length!=32) {
          alert("Please enter valid Emoncms apikey");
        } else {
          document.getElementById("save-emoncms").innerHTML = "Saving...";
          var r = new XMLHttpRequest();
          r.open("POST", "saveemoncms", true);
          r.setRequestHeader("Content-type","application/x-www-form-urlencoded");
          r.onreadystatechange = function () {
            if (r.readyState != 4 || r.status != 200) return;
            r.send("&server="+emoncms.server+"&apikey="+emoncms.apikey+"&node="+emoncms.node);
            var str = r.responseText;
      	    console.log(str);
      	    if (str!=0) document.getElementById("save-emoncms").innerHTML = str;
          };
        }
    
});

// -----------------------------------------------------------------------
// Event: MQTT save
// -----------------------------------------------------------------------
document.getElementById("save-mqtt").addEventListener("click", function(e) {
    var mqtt = {
      server: document.getElementById("mqtt_server").value,
      topic: document.getElementById("mqtt_topic").value,
      user: document.getElementById("mqtt_user").value,
      pass: document.getElementById("mqtt_pass").value
    }
    if (mqtt.server=="") {
      alert("Please enter MQTT server");
    } else {
      document.getElementById("save-mqtt").innerHTML = "Connecting...";
      var r = new XMLHttpRequest();
      r.open("POST", "savemqtt", true);
      r.setRequestHeader("Content-type","application/x-www-form-urlencoded");
      r.onreadystatechange = function () {
        if (r.readyState != 4 || r.status != 200) return;
        r.send("&server="+mqtt.server+"&topic="+mqtt.topic+"&user="+mqtt.user+"&pass="+mqtt.pass);
        console.log(mqtt);
        var str = r.responseText;
  	    console.log(str);
  	    if (str!=0) document.getElementById("save-mqtt").innerHTML = str;
      };
    }
});

// -----------------------------------------------------------------------
// Event: Turn off Access POint
// -----------------------------------------------------------------------
document.getElementById("apoff").addEventListener("click", function(e) {
    var r = new XMLHttpRequest();
    r.open("POST", "apoff", true);
    r.onreadystatechange = function () {
        if (r.readyState != 4 || r.status != 200) return;
        var str = r.responseText;
        console.log(str);
        document.getElementById("apoff").style.display = 'none';
        if (ipaddress!="") window.location = "http://"+ipaddress;

	  };
    r.send();
});
// -----------------------------------------------------------------------
// Event: Reset config and reboot
// -----------------------------------------------------------------------
document.getElementById("reset").addEventListener("click", function(e) {
    var r = new XMLHttpRequest();
    r.open("POST", "reset", true);
    r.onreadystatechange = function () {
        if (r.readyState != 4 || r.status != 200) return;
        var str = r.responseText;
        console.log(str);
	  };
    r.send();
});

// -----------------------------------------------------------------------
// UI: Network select
// -----------------------------------------------------------------------
var networkcheckboxes = document.getElementsByClassName("networkcheckbox");

var networkSelect = function() {
    selected_network_ssid = this.getAttribute("name");

    for (var i = 0; i < networkcheckboxes.length; i++) {
        if (networkcheckboxes[i].getAttribute("name")!=selected_network_ssid)
            networkcheckboxes[i].checked = 0;
    }
};

for (var i = 0; i < networkcheckboxes.length; i++) {
    networkcheckboxes[i].addEventListener('click', networkSelect, false);
}

// -----------------------------------------------------------------------
// Event:Check for updates & display current / latest
// URL /firmware
// -----------------------------------------------------------------------
document.getElementById("updatecheck").addEventListener("click", function(e) {
    document.getElementById("firmware-version").innerHTML = "<tr><td>-</td><td>Connecting...</td></tr>";
    var r = new XMLHttpRequest();
    r.open("POST", "firmware", true);
    r.onreadystatechange = function () {
        if (r.readyState != 4 || r.status != 200) return;
        var str = r.responseText;
        console.log(str);
        var firmware = JSON.parse(r.responseText);
        document.getElementById("firmware").style.display = '';
        document.getElementById("update").style.display = '';
        document.getElementById("firmware-version").innerHTML = "<tr><td>"+firmware.current+"</td><td>"+firmware.latest+"</td></tr>";
	  };
    r.send();
});


// -----------------------------------------------------------------------
// Event:Update Firmware
// -----------------------------------------------------------------------
document.getElementById("update").addEventListener("click", function(e) {
    document.getElementById("update-info").innerHTML = "UPDATING..."
    var r1 = new XMLHttpRequest();
    r1.open("POST", "update", true);
    r1.onreadystatechange = function () {
        if (r1.readyState != 4 || r1.status != 200) return;
        var str1 = r1.responseText;
        document.getElementById("update-info").innerHTML = str1
        console.log(str1);
	  };
    r1.send();
});

// -----------------------------------------------------------------------
// Event:Upload Firmware
// -----------------------------------------------------------------------
document.getElementById("upload").addEventListener("click", function(e) {
  window.location.href='/upload'
});
