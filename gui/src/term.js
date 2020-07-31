/* global $, Terminal */

(function () {
  "use strict";

  var socket = false;
  var reconnectInterval = false;

  var validTerminals = ["debug", "emontx"];

  var type = "debug";
  if ("" !== window.location.search) {
    var val = window.location.search.substr(1);
    if (validTerminals.includes(val)) {
      type = val;
    }
  }

  var url = new URL(type, window.location);

  var ws = "ws://" + url.hostname;
  if ("https:" === url.protocol) {
    ws = "wss://" + url.hostname;
  }
  if (url.port && 80 !== url.port) {
    ws += ":" + url.port;
  }
  ws += url.pathname + "/console";

  
  function addText(text, clear = false) {
    var term = $("#term");
    var scroll = ($(document).height() - ($(document).scrollTop() + window.innerHeight)) < 10;
    text = text.replace(/(\r\n|\n|\r)/gm, "\n");
    if(clear) {
      term.text(text);
    } else {
      term.append(text);
    }
    if(scroll) {
      $(document).scrollTop($(document).height());
    }
  }

  function connect() {
    socket = new WebSocket(ws);
    socket.onclose = () => {
      reconnect();
    };
    socket.onmessage = (msg) => {
      addText(msg.data);
    };
    socket.onerror = (ev) => {
      console.log(ev);
      socket.close();
      reconnect();
    };
  }

  function reconnect() {
    if (false === reconnectInterval) {
      reconnectInterval = setTimeout(() => {
        reconnectInterval = false;
        connect();
      }, 500);
    }
  }

  $(() => {
    $.get(url.href, (data) => {
      addText(data, true);
      connect();
    }, "text");
  });
})();
