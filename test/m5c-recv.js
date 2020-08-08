

// (function () {
  "use strict";
  var connection = null;
  // var clientID = 0;
  var msgpack = msgpack5();
  
  // document.addEventListener("DOMContentLoaded", function () {
    function createCharts(num_charts, chartLabels) {
      var timeseries = [];
      var charts = [];
      var chartContainer = document.getElementById("charts");

      for (var i = 0; i < num_charts; i++) {
        var width = "100%";

        var label = document.createElement("span");
        label.classList.add("canvas-label");
        label.textContent = chartLabels[i];
        label.style.width = width;
        chartContainer.appendChild(label);

        var canvas = document.createElement("canvas");
        canvas.id = "channel_" + i;
        canvas.style.width = width;
        canvas.height = 20;
        chartContainer.appendChild(canvas);

        var ts = new TimeSeries();

        var chart = new SmoothieChart({
          responsive: true,
          interpolation: 'linear',
          millisPerLine: 100
        });

        chart.streamTo(canvas, 1000);
        chart.addTimeSeries(ts);
        // chartLabels.push(chart);
        chart.addTimeSeries(ts, {
            strokeStyle:'rgb(255, 255, 255)',
            lineWidth: 4
        });

        timeseries.push(ts);
        charts.push(chart)
      }
      return [timeseries, charts];
      // return timeseries;

    } // end function createCharts

  // }); // end document.addEventListener("DOMContentLoaded", ...)


  function start_stream_func() {
    var serverUrl;
    var scheme = "ws";
  
    // If this is an HTTPS connection, we have to use a secure WebSocket
    // connection too, so add another "s" to the scheme.
    if (document.location.protocol === "https:") {
      scheme += "s";
    }
  
    // serverUrl = scheme + "://" + document.location.hostname + ":6502";
    serverUrl = scheme + "://" + "m5c-imu" + ":42000";
  
    connection = new WebSocket(serverUrl);
    console.log("***CREATED WEBSOCKET");
  
    connection.onopen = function(evt) {
      console.log("***ONOPEN");
    };
    console.log("***CREATED ONOPEN");
  
    // initialize label_keys and plots (i.e. timeseries)
    var label_keys = ["ax", "ay", "az", "gx", "gy", "gz"];
    const _tmp = createCharts(label_keys.length, label_keys);
    var timeseries = _tmp[0];
    var charts = _tmp[1];
    var g_time = 0.0;
  
    connection.onmessage = function(evt) {
      console.log("***ONMESSAGE");
      console.log(charts.length)
      // var f = document.getElementById("some_html_identifier").contentDocument;
  
      console.log(typeof evt.data);
      console.log(evt.data);
  
      var obj;
      // process websocket buffer and msgpack object
      evt.data.arrayBuffer().then(function(data) {
        // decode MsgPack object from arrayBuffer in to JSON object
        obj = msgpack.decode(data);
        console.log(obj);
  
        // loop through label_keys and plots
        // console.log(label_keys)
        var yoyoyo = new Date().getTime();
        // console.log(yoyoyo);
        label_keys.forEach(function(label_key, label_key_ix) {
          var loc_time = g_time;
          // console.log(label_key)
          // console.log(obj[label_key])
          // console.log(label_key_ix)
          obj[label_key].forEach(function(val, val_ix) {
            // update local time and convert micros to millis
            loc_time += (obj["micros"][val_ix] / 1e3);
            // console.log(loc_time)
            // console.log(val)
            timeseries[label_key_ix].append(Math.round(loc_time) , val);
          }); // end foreach plot/graph
  
          // if last label, then update
          if(label_key_ix == label_keys.length - 1) {
            console.log(g_time);
            // console.log(Math.round(g_time));
            g_time = loc_time;
          }
  
        }); // end loop through label_keys and plots 
  
      }); // end process websocket buffer and msgpack object
  
    }; // end connection.onmessage
    console.log("***CREATED ONMESSAGE");
  
  } // end start_stream_func

  // function handleKey(evt) {
  //   if (evt.keyCode === 13 || evt.keyCode === 14) {
  //     if (!document.getElementById("send").disabled) {
  //       send();
  //     }
  //   }
  // }

// }) // end (function() ... )


