function move(dir) {
  var degrees = parseInt(document.getElementById("degrees").value);
  if (degrees < 0 || degrees > 180) {
    alert("Please enter a value between 0 and 180 degrees.");
    return;
  }

  // Handle direction specific logic
  if (dir === "up" || dir === "down") {
    // Adjust servo position for up/down movement based on degrees (e.g., modify servo angles)
    console.log("Move " + dir + " by " + degrees + " degrees (server-side logic needed)");
  } else {
    var url = "/" + dir + "?degrees=" + degrees;
    var xhr = new XMLHttpRequest();
    xhr.open("GET", url, true);
    console.log(url);
    xhr.send();
  }
}

if (!!window.EventSource) {
  var source = new EventSource('/events');

  source.addEventListener('open', function (e) {
    console.log("Events Connected");
  }, false);

  source.addEventListener('error', function (e) {
    if (e.target.readyState != EventSource.OPEN) {
      console.log("Events Disconnected");
    }
  }, false);

  source.addEventListener('data_readings', function (e) {
    //console.log("gyro_readings", e.data);
    var obj = JSON.parse(e.data);
    document.getElementById("maxDown").innerHTML = obj.down_max;
    document.getElementById("maxUp").innerHTML = obj.up_max;
    document.getElementById("maxLeft").innerHTML = obj.left_max;
    document.getElementById("maxRight").innerHTML = obj.right_max;

  }, false);
}
