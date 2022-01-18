static const char CONTENT_CONFIG_JS[] PROGMEM = 
  "\"use strict\";function BaseViewModel(e,t,n){void 0===n&&(n={});var s=this;s.remoteUrl=t,ko.mapping.fromJS(e,n,s),s.fetching=ko.observable(!1)}function StatusViewModel(){var e=this;BaseViewModel.call(e,{mode:\"ERR\",wifi_client_connected:0,net_connected:0,srssi:0,ipaddress:\"\",packets_sent:0,packets_success:0,emoncms_connected:0,mqtt_connected:0,free_heap:0,time:\"\",ctrl_mode:\"off\",ctrl_state:0,ota_update:!1},baseEndpoint+\"/status\"),e.isWiFiError=ko.pureComputed(function(){return\"ERR\"===e.mode()}),e.isWifiClient=ko.pureComputed(function(){return\"STA\"==e.mode()||\"STA+AP\"==e.mode()}),e.isWifiAccessPoint=ko.pureComputed(function(){return\"AP\"==e.mode()||\"STA+AP\"==e.mode()}),e.isWired=ko.pureComputed(function(){return\"Wired\"===e.mode()}),e.fullMode=ko.pureComputed(function(){switch(e.mode()){case\"AP\":return\"Access Point (AP)\";case\"STA\":return\"Client (STA)\";case\"STA+AP\":return\"Client + Access Point (STA+AP)\";case\"Wired\":return\"Wired Ethernet\"}return\"Unknown (\"+e.mode()+\")\"})}function ConfigViewModel(){BaseViewModel.call(this,{node_name:\"emonESP\",node_description:\"WiFi Emoncms Link\",node_type:\"\",ssid:\"\",pass:\"\",emoncms_enabled:!1,emoncms_server:\"data.openevse.com\",emoncms_port:\"\",emoncms_path:\"/emoncms\",emoncms_apikey:\"\",emoncms_node:\"\",emoncms_fingerprint:\"\",mqtt_enabled:!1,mqtt_server:\"\",mqtt_port:\"\",mqtt_topic:\"\",mqtt_feed_prefix:\"\",mqtt_user:\"\",mqtt_pass:\"\",www_username:\"\",www_password:\"\",espflash:\"\",version:\"0.0.0\",timer_start1:\"\",timer_stop1:\"\",timer_start2:\"\",timer_stop2:\"\",voltage_output:\"\",time_offset:\"\"},baseEndpoint+\"/config\"),this.f_timer_start1=ko.pureComputed({read:function(){return addcolon(this.timer_start1())},write:function(e){this.timer_start1(e.replace(\":\",\"\"))},owner:this}),this.f_timer_stop1=ko.pureComputed({read:function(){return addcolon(this.timer_stop1())},write:function(e){this.timer_stop1(e.replace(\":\",\"\"))},owner:this}),this.f_timer_start2=ko.pureComputed({read:function(){return addcolon(this.timer_start2())},write:function(e){this.timer_start2(e.replace(\":\",\"\"))},owner:this}),this.f_timer_stop2=ko.pureComputed({read:function(){return addcolon(this.timer_stop2())},write:function(e){this.timer_stop2(e.replace(\":\",\"\"))},owner:this}),this.flowT=ko.pureComputed({read:function(){return.0371*this.voltage_output()+7.14},write:function(e){this.voltage_output((e-7.14)/.0371)},owner:this})}function WiFiConfigViewModel(e,t,n,s){var o=this;o.baseEndpoint=e,o.config=t,o.status=n,o.scan=s,o.scanUpdating=ko.observable(!1),o.selectedNet=ko.observable(!1),o.bssid=ko.pureComputed({read:function(){return!1===o.selectedNet()?\"\":o.selectedNet().bssid()},write:function(e){for(var t=0;t<o.scan.results().length;t++){var n=o.scan.results()[t];if(e===n.bssid())return void o.selectedNet(n)}}}),o.select=function(e){o.selectedNet(e)},o.setSsid=function(e){if(!1===o.selectedNet()||e!==o.selectedNet().ssid()){for(var t=0;t<o.scan.filteredResults().length;t++)if(e===(n=o.scan.filteredResults()[t]).ssid())return void o.selectedNet(n);for(var n,t=0;t<o.scan.results().length;t++)if(e===(n=o.scan.results()[t]).ssid())return void o.selectedNet(n);o.selectedNet(!1)}};var i=null,a=!1;o.startScan=function(){o.scanUpdating()||(a=!0,o.scanUpdating(!0),null!==i&&(clearTimeout(i),i=null),o.scan.update(function(){if(a&&(i=setTimeout(o.startScan,3e3)),\"\"===o.bssid())for(var e=o.config.ssid(),t=0;t<o.scan.results().length;t++){var n=o.scan.results()[t];if(e===n.ssid()){o.bssid(n.bssid());break}}o.scanUpdating(!1)}))},o.stopScan=function(){a=!1,o.scanUpdating()||null!==i&&(clearTimeout(i),i=null)},o.enableScan=function(e){e?o.startScan():o.stopScan()},o.forceConfig=ko.observable(!1),o.canConfigure=ko.pureComputed(function(){return!(o.status.isWiFiError()||o.wifiConnecting()||o.status.isWired())&&(!o.status.isWifiClient()||o.forceConfig())}),o.wifiConnecting=ko.observable(!1),o.canConfigure.subscribe(function(e){o.enableScan(e)}),o.status.wifi_client_connected.subscribe(function(e){e&&o.wifiConnecting(!1)}),o.enableScan(o.canConfigure()),o.saveNetworkFetching=ko.observable(!1),o.saveNetworkSuccess=ko.observable(!1),o.saveNetwork=function(){\"\"===o.config.ssid()?alert(\"Please select network\"):(o.saveNetworkFetching(!0),o.saveNetworkSuccess(!1),$.post(o.baseEndpoint()+\"/savenetwork\",{ssid:o.config.ssid(),pass:o.config.pass()},function(){o.status.wifi_client_connected(!1),o.forceConfig(!1),o.wifiConnecting(!0),o.saveNetworkSuccess(!0)}).fail(function(){alert(\"Failed to save WiFi config\")}).always(function(){o.saveNetworkFetching(!1)}))},o.turnOffAccessPointFetching=ko.observable(!1),o.turnOffAccessPointSuccess=ko.observable(!1),o.turnOffAccessPoint=function(){o.turnOffAccessPointFetching(!0),o.turnOffAccessPointSuccess(!1),$.post(o.baseEndpoint()+\"/apoff\",{},function(e){console.log(e),\"\"!==o.status.ipaddress()?setTimeout(function(){window.location=\"//\"+o.status.ipaddress(),o.turnOffAccessPointSuccess(!0)},3e3):o.turnOffAccessPointSuccess(!0)}).fail(function(){alert(\"Failed to turn off Access Point\")}).always(function(){o.turnOffAccessPointFetching(!1)})}}function WiFiScanResultViewModel(e){var t=this;ko.mapping.fromJS(e,{},t),t.strength=ko.computed(function(){var e=t.rssi();return-50<=e?5:-60<=e?4:-67<=e?3:-70<=e?2:-80<=e?1:0})}function WiFiScanViewModel(e){var t=this,n=ko.pureComputed(function(){return e()+\"/scan\"}),s={key:function(e){return ko.utils.unwrapObservable(e.bssid)},create:function(e){return new WiFiScanResultViewModel(e.data)}};t.results=ko.mapping.fromJS([],s),t.filteredResults=ko.mapping.fromJS([],s),t.fetching=ko.observable(!1),t.update=function(){var e=0<arguments.length&&void 0!==arguments[0]?arguments[0]:function(){};t.fetching(!0),$.get(n(),function(n){var e;0<n.length&&(n=n.sort(function(e,t){return e.ssid===t.ssid?e.rssi<t.rssi?1:-1:e.ssid<t.ssid?-1:1}),ko.mapping.fromJS(n,t.results),e=n.filter(function(t,e){return-80<=t.rssi&&e===n.findIndex(function(e){return t.ssid===e.ssid})}).sort(function(e,t){return e.rssi<t.rssi?1:-1}),ko.mapping.fromJS(e,t.filteredResults))},\"json\").always(function(){t.fetching(!1),e()})}}function LastValuesViewModel(){var a=this;a.remoteUrl=baseEndpoint+\"/lastvalues\",a.fetching=ko.observable(!1),a.lastValues=ko.observable(!1),a.values=ko.mapping.fromJS([]),a.entries=ko.mapping.fromJS([]);var r=\"\";a.update=function(e){void 0===e&&(e=function(){}),a.fetching(!0),$.get(a.remoteUrl,function(e){var t=[];if(\"\"!=e&&e!==r){r=e,a.entries.push({timestamp:(new Date).toISOString(),log:e});try{var n,s=JSON.parse(e);for(n in s){var o=s[n],i=\"\";n.startsWith(\"CT\")&&(i=\" W\"),n.startsWith(\"P\")&&(i=\" W\"),n.startsWith(\"E\")&&(i=\" Wh\"),n.startsWith(\"V\")&&(i=\" V\"),n.startsWith(\"T\")&&(i=\" \"+String.fromCharCode(176)+\"C\"),t.push({key:n,value:o+i})}a.lastValues(!0),ko.mapping.fromJS(t,a.values)}catch(e){console.error(e),a.lastValues(!1)}}else a.lastValues(\"\"!=e)},\"text\").always(function(){a.fetching(!1),e()})}}function PasswordViewModel(t){var e=this;e.show=ko.observable(!1),e.value=ko.computed({read:function(){return e.show()&&e.isDummy()?\"\":t()},write:function(e){t(e)}}),e.isDummy=ko.computed(function(){return[\"___DUMMY_PASSWORD___\",\"_DUMMY_PASSWORD\"].includes(t())})}function EmonEspViewModel(e,t,n){var s=this;s.baseHost=ko.observable(\"\"!==e?e:\"openevse.local\"),s.basePort=ko.observable(t),s.baseProtocol=ko.observable(n),s.baseEndpoint=ko.pureComputed(function(){var e=\"//\"+s.baseHost();return 80!==s.basePort()&&(e+=\":\"+s.basePort()),e}),s.wsEndpoint=ko.pureComputed(function(){var e=\"ws://\"+s.baseHost();return\"https:\"===s.baseProtocol()&&(e=\"wss://\"+s.baseHost()),80!==s.basePort()&&(e+=\":\"+s.basePort()),e+=\"/ws\"}),s.config=new ConfigViewModel,s.status=new StatusViewModel,s.last=new LastValuesViewModel,s.scan=new WiFiScanViewModel(s.baseEndpoint),s.wifi=new WiFiConfigViewModel(s.baseEndpoint,s.config,s.status,s.scan),s.wifiPassword=new PasswordViewModel(s.config.pass),s.emoncmsApiKey=new PasswordViewModel(s.config.emoncms_apikey),s.mqttPassword=new PasswordViewModel(s.config.mqtt_pass),s.wwwPassword=new PasswordViewModel(s.config.www_password),s.initialised=ko.observable(!1),s.updating=ko.observable(!1),s.wifi.selectedNet.subscribe(function(e){!1!==e&&s.config.ssid(e.ssid())}),s.config.ssid.subscribe(function(e){s.wifi.setSsid(e)});var o=null;s.upgradeUrl=ko.observable(\"about:blank\"),s.start=function(){s.updating(!0),s.config.update(function(){s.status.update(function(){s.last.update(function(){s.initialised(!0),o=setTimeout(s.update,1e3),s.upgradeUrl(baseEndpoint+\"/update\"),s.updating(!1)})})})},s.update=function(){s.updating()||(s.updating(!0),null!==o&&(clearTimeout(o),o=null),s.status.update(function(){s.last.update(function(){o=setTimeout(s.update,1e3),s.updating(!1)})}))},s.wifiConnecting=ko.observable(!1),s.status.mode.subscribe(function(e){\"STA+AP\"!==e&&\"STA\"!==e||s.wifiConnecting(!1)}),s.saveNetworkFetching=ko.observable(!1),s.saveNetworkSuccess=ko.observable(!1),s.saveNetwork=function(){\"\"===s.config.ssid()?alert(\"Please select network\"):(s.saveNetworkFetching(!0),s.saveNetworkSuccess(!1),$.post(baseEndpoint+\"/savenetwork\",{ssid:s.config.ssid(),pass:s.config.pass()},function(e){s.saveNetworkSuccess(!0),s.wifiConnecting(!0)}).fail(function(){alert(\"Failed to save WiFi config\")}).always(function(){s.saveNetworkFetching(!1)}))},s.saveAdminFetching=ko.observable(!1),s.saveAdminSuccess=ko.observable(!1),s.saveAdmin=function(){s.saveAdminFetching(!0),s.saveAdminSuccess(!1),$.post(baseEndpoint+\"/saveadmin\",{user:s.config.www_username(),pass:s.config.www_password()},function(e){s.saveAdminSuccess(!0)}).fail(function(){alert(\"Failed to save Admin config\")}).always(function(){s.saveAdminFetching(!1)})},s.saveTimerFetching=ko.observable(!1),s.saveTimerSuccess=ko.observable(!1),s.saveTimer=function(){s.saveTimerFetching(!0),s.saveTimerSuccess(!1),$.post(baseEndpoint+\"/savetimer\",{timer_start1:s.config.timer_start1(),timer_stop1:s.config.timer_stop1(),timer_start2:s.config.timer_start2(),timer_stop2:s.config.timer_stop2(),voltage_output:s.config.voltage_output(),time_offset:s.config.time_offset()},function(e){s.saveTimerSuccess(!0),setTimeout(function(){s.saveTimerSuccess(!1)},5e3)}).fail(function(){alert(\"Failed to save Timer config\")}).always(function(){s.saveTimerFetching(!1)})},s.ctrlMode=function(e){var t=s.status.ctrl_mode();s.status.ctrl_mode(e),$.post(baseEndpoint+\"/ctrlmode?mode=\"+e,{},function(e){}).fail(function(){s.status.ctrl_mode(t),alert(\"Failed to switch \"+e)})},s.saveEmonCmsFetching=ko.observable(!1),s.saveEmonCmsSuccess=ko.observable(!1),s.saveEmonCms=function(){var e={enable:s.config.emoncms_enabled(),server:s.config.emoncms_server(),port:s.config.emoncms_port(),path:s.config.emoncms_path(),apikey:s.config.emoncms_apikey(),node:s.config.emoncms_node(),fingerprint:s.config.emoncms_fingerprint()};\"\"===e.server||\"\"===e.node?alert(\"Please enter Emoncms server and node\"):Number(e.port)%1!=0?alert(\"Please enter port number or leave blank\"):32==e.apikey.length||s.emoncmsApiKey.isDummy()?\"\"!==e.fingerprint&&59!=e.fingerprint.length?alert(\"Please enter valid SSL SHA-1 fingerprint\"):(s.saveEmonCmsFetching(!0),s.saveEmonCmsSuccess(!1),$.post(baseEndpoint+\"/saveemoncms\",e,function(e){s.saveEmonCmsSuccess(!0)}).fail(function(){alert(\"Failed to save Admin config\")}).always(function(){s.saveEmonCmsFetching(!1)})):alert(\"Please enter valid Emoncms apikey\")},s.saveMqttFetching=ko.observable(!1),s.saveMqttSuccess=ko.observable(!1),s.saveMqtt=function(){var e={enable:s.config.mqtt_enabled(),server:s.config.mqtt_server(),port:s.config.mqtt_port(),topic:s.config.mqtt_topic(),prefix:s.config.mqtt_feed_prefix(),user:s.config.mqtt_user(),pass:s.config.mqtt_pass()};\"\"===e.server?alert(\"Please enter MQTT server\"):(s.saveMqttFetching(!0),s.saveMqttSuccess(!1),$.post(baseEndpoint+\"/savemqtt\",e,function(e){s.saveMqttSuccess(!0)}).fail(function(){alert(\"Failed to save MQTT config\")}).always(function(){s.saveMqttFetching(!1)}))}}BaseViewModel.prototype.update=function(e){void 0===e&&(e=function(){});var t=this;t.fetching(!0),$.get(t.remoteUrl,function(e){ko.mapping.fromJS(e,t)},\"json\").always(function(){t.fetching(!1),e()})},StatusViewModel.prototype=Object.create(BaseViewModel.prototype),StatusViewModel.prototype.constructor=StatusViewModel,ConfigViewModel.prototype=Object.create(BaseViewModel.prototype),ConfigViewModel.prototype.constructor=ConfigViewModel,self.updateFetching=ko.observable(!1),self.updateComplete=ko.observable(!1),self.updateError=ko.observable(\"\"),self.updateFilename=ko.observable(\"\"),self.updateLoaded=ko.observable(0),self.updateTotal=ko.observable(1),self.updateProgress=ko.pureComputed(function(){return self.updateLoaded()/self.updateTotal()*100}),self.otaUpdate=function(){var e;\"\"!==self.updateFilename()?(self.updateFetching(!0),self.updateError(\"\"),e=$(\"#upload_form\")[0],e=new FormData(e),$.ajax({url:\"/upload\",type:\"POST\",data:e,contentType:!1,processData:!1,xhr:function(){var e;return(e=new window.XMLHttpRequest).upload.addEventListener(\"progress\",function(e){e.lengthComputable&&(self.updateLoaded(e.loaded),self.updateTotal(e.total))},!1),e}}).done(function(e){console.log(e),\"OK\"==e?(self.updateComplete(!0),setTimeout(function(){location.reload()},5e3)):self.updateError(e)}).fail(function(){self.updateError(\"HTTP Update failed\")}).always(function(){self.updateFetching(!1)})):self.updateError(\"Filename not set\")};var development=\"\",baseHost=window.location.hostname,basePort=window.location.port,baseProtocol=window.location.protocol,baseEndpoint=\"//\"+baseHost;80!==basePort&&(baseEndpoint+=\":\"+basePort),baseEndpoint+=development;var statusupdate=!1,selected_network_ssid=\"\",lastmode=\"\",ipaddress=\"\";function scaleString(e,t,n){return(parseInt(e)/t).toFixed(n)}function addcolon(e){return(e=new String(e)).length<3?\"00:00\":(e=3==e.length?\"0\"+e:e).substr(0,2)+\":\"+e.substr(2,2)}function toggle(e){e=document.getElementById(e);\"block\"==e.style.display?(e.previousElementSibling.firstChild.textContent=\"+\",e.style.display=\"none\"):(e.previousElementSibling.firstChild.textContent=\"-\",e.style.display=\"block\")}$(function(){var e=new EmonEspViewModel(baseHost,basePort,baseProtocol);ko.applyBindings(e),e.start()}),document.getElementById(\"apoff\").addEventListener(\"click\",function(e){var t=new XMLHttpRequest;t.open(\"POST\",\"apoff\",!0),t.onreadystatechange=function(){var e;4==t.readyState&&200==t.status&&(e=t.responseText,console.log(e),document.getElementById(\"apoff\").style.display=\"none\",\"\"!==ipaddress&&(window.location=\"http://\"+ipaddress))},t.send()}),document.getElementById(\"reset\").addEventListener(\"click\",function(e){var t;confirm(\"CAUTION: Do you really want to Factory Reset? All setting and config will be lost.\")&&((t=new XMLHttpRequest).open(\"POST\",\"reset\",!0),t.onreadystatechange=function(){var e;4==t.readyState&&200==t.status&&(e=t.responseText,console.log(e),0!==e&&(document.getElementById(\"reset\").innerHTML=\"Resetting...\"))},t.send())}),document.getElementById(\"restart\").addEventListener(\"click\",function(e){var t;confirm(\"Restart emonESP? Current config will be saved, takes approximately 10s.\")&&((t=new XMLHttpRequest).open(\"POST\",\"restart\",!0),t.onreadystatechange=function(){var e;4==t.readyState&&200==t.status&&(e=t.responseText,console.log(e),0!==e&&(document.getElementById(\"reset\").innerHTML=\"Restarting\"))},t.send())});\n"
  "//# sourceMappingURL=config.js.map\n";
