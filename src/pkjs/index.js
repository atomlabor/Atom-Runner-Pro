var Clay = require('pebble-clay');
var clayConfig = require('./config.json');

function clayCustomFn() {
  var clayConfig = this;
  function calculateZones() {
    var ageVal = parseInt(clayConfig.getItemByMessageKey('CONFIG_AGE').get());
    if (!ageVal || isNaN(ageVal)) return; 
    var maxHR = Math.round(208 - (0.7 * ageVal));
    clayConfig.getItemByMessageKey('CONFIG_ZONE_GREEN').set(Math.round(maxHR * 0.60));
    clayConfig.getItemByMessageKey('CONFIG_ZONE_ORANGE').set(Math.round(maxHR * 0.80));
    clayConfig.getItemByMessageKey('CONFIG_ZONE_RED').set(Math.round(maxHR * 0.90));
    clayConfig.getItemByMessageKey('DUMMY_EXPLAIN').set("Tanaka-Analyse:\nMax HF: " + maxHR + " bpm");
  }
  clayConfig.on(clayConfig.EVENTS.AFTER_BUILD, function() {
    clayConfig.getItemByMessageKey('CONFIG_AGE').on('change', calculateZones);
  });
}

var clay = new Clay(clayConfig, clayCustomFn, { autoHandleEvents: false });

Pebble.addEventListener('showConfiguration', function(e) {
  Pebble.openURL(clay.generateUrl());
});

Pebble.addEventListener('webviewclosed', function(e) {
  if (e && e.response) {
    var settings = clay.getSettings(e.response);
    Pebble.sendAppMessage(settings);
  }
});