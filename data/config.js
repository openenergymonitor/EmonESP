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
  document.getElementById("apikey").value = status.apikey;
  
  if (status.mode=="AP") {
      document.getElementById("mode").innerHTML = "Access Point (AP)";
      document.getElementById("client-view").style.display = 'none';
      document.getElementById("ap-view").style.display = '';
      
      var out = "";
      for (var z in status.networks) {
          out += "<tr><td><input class='networkcheckbox' name='"+status.networks[z]+"' type='checkbox'></td><td>"+status.networks[z]+"</td></tr>";
      }
      document.getElementById("networks").innerHTML = out;
  } else {
      document.getElementById("mode").innerHTML = "Client (STA)";
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
// Periodic 10s update of last data values
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
}

function updateStatus() {
  var r1 = new XMLHttpRequest(); 
  r1.open("GET", "status", true);
  r1.timeout = 2000;
  r1.onreadystatechange = function () {
    if (r1.readyState != 4 || r1.status != 200) return;
    var status = JSON.parse(r1.responseText);
    
    document.getElementById("apikey").value = status.apikey;
    
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
	        
	        statusupdate = setInterval(updateStatus,5000);
        };
        r.send("ssid="+selected_network_ssid+"&pass="+passkey);
    }
});

// -----------------------------------------------------------------------
// Event: Apikey save
// -----------------------------------------------------------------------
document.getElementById("save-apikey").addEventListener("click", function(e) {
    var apikey = document.getElementById("apikey").value;
    if (apikey=="") alert("Please enter apikey");
    
    var r = new XMLHttpRequest(); 
    r.open("POST", "saveapikey", true);
    r.setRequestHeader("Content-type","application/x-www-form-urlencoded");
    r.onreadystatechange = function () {};
    r.send("&apikey="+apikey);
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
        if (ipaddress!="") window.location = ipaddress;
        
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