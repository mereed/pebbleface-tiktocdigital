var mConfig = {};

Pebble.addEventListener("ready", function(e) {
  loadLocalData();
  returnConfigToPebble();
});

Pebble.addEventListener("showConfiguration", function(e) {
	Pebble.openURL(mConfig.configureUrl);
});

Pebble.addEventListener("webviewclosed",
  function(e) {
    if (e.response) {
      var config = JSON.parse(e.response);
      saveLocalData(config);
      returnConfigToPebble();
    }
  }
);

function saveLocalData(config) {

  //console.log("loadLocalData() " + JSON.stringify(config));

  localStorage.setItem("bluetoothvibe", parseInt(config.bluetoothvibe)); 
  localStorage.setItem("hourlyvibe", parseInt(config.hourlyvibe)); 
  localStorage.setItem("flip", parseInt(config.flip));  
  localStorage.setItem("colour", parseInt(config.colour));  
  localStorage.setItem("blink", parseInt(config.blink)); 
  
  loadLocalData();

}
function loadLocalData() {
  
	mConfig.bluetoothvibe = parseInt(localStorage.getItem("bluetoothvibe"));
	mConfig.hourlyvibe = parseInt(localStorage.getItem("hourlyvibe"));
	mConfig.flip = parseInt(localStorage.getItem("flip"));
	mConfig.colour = parseInt(localStorage.getItem("colour"));
	mConfig.blink = parseInt(localStorage.getItem("blink"));
	mConfig.configureUrl = "http://www.themapman.com/pebblewatch/tiktok.html";
	

	if(isNaN(mConfig.bluetoothvibe)) {
		mConfig.bluetoothvibe = 0;
	}
	if(isNaN(mConfig.hourlyvibe)) {
		mConfig.hourlyvibe = 0;
	}
	if(isNaN(mConfig.flip)) {
		mConfig.flip = 0;
	} 
	if(isNaN(mConfig.colour)) {
		mConfig.colour = 0;
	}
	if(isNaN(mConfig.blink)) {
		mConfig.blink = 0;
	}

  //console.log("loadLocalData() " + JSON.stringify(mConfig));
}
function returnConfigToPebble() {
  //console.log("Configuration window returned: " + JSON.stringify(mConfig));
  Pebble.sendAppMessage({
    "bluetoothvibe":parseInt(mConfig.bluetoothvibe), 
    "hourlyvibe":parseInt(mConfig.hourlyvibe),
    "flip":parseInt(mConfig.flip),
    "colour":parseInt(mConfig.colour),
	"blink":parseInt(mConfig.blink),
  });    
}