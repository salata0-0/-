function move(dir) {
    var degrees = parseInt(document.getElementById("angleInput").value);
    var repetitions = parseInt(document.getElementById("valueInput").value);
    if (degrees < 0 || degrees > 180) {
      alert("Please enter a value between 0 and 180 degrees.");
      return;
    }
  
    // Handle direction specific logic
      var url = "/" + dir + "?degrees=" + degrees + "&repetitions=" + repetitions;
      var xhr = new XMLHttpRequest();
      xhr.open("GET", url, true);
      console.log(url);
      xhr.send();
  }

  function center() {
    // Handle direction specific logic
      var url = "/center";
      var xhr = new XMLHttpRequest();
      xhr.open("GET", url, true);
      console.log(url);
      xhr.send();
  }
  
  // if (!!window.EventSource) {
  //   var source = new EventSource('/events');
  
  //   source.addEventListener('open', function (e) {
  //     console.log("Events Connected");
  //   }, false);
  
  //   source.addEventListener('error', function (e) {
  //     if (e.target.readyState != EventSource.OPEN) {
  //       console.log("Events Disconnected");
  //     }
  //   }, false);
  
  //   source.addEventListener('data_readings', function (e) {
  //     //console.log("gyro_readings", e.data);
  //     var obj = JSON.parse(e.data);
  //     data = obj; // Assuming obj is in the expected format
  //     updateGraphs(); // Update the graphs with the newly loaded data
  //   }, false);
  // }

  const angleInput = document.getElementById('angleInput');
const valueInput = document.getElementById('valueInput'); // Added for new input
const upButton = document.getElementById('upButton');
const downButton = document.getElementById('downButton');
const centerButton = document.getElementById('centerButton'); // Added for center button
const leftButton = document.getElementById('leftButton');
const rightButton = document.getElementById('rightButton');
const maxValues = document.getElementById('maxValues');
const currentAngleSpan = document.getElementById('currentAngle'); // Added for displaying current angle
const maxValueUp = document.getElementById('maxValueUp'); // Added for displaying max up value
const maxValueDown = document.getElementById('maxValueDown'); // Added for displaying max down value
const maxValueLeft = document.getElementById('maxValueLeft'); // Added for displaying max left value
const maxValueRight = document.getElementById('maxValueRight'); // Added for displaying max right value

let data = {}; // Stores data for each direction (up, down, left, right)

let maxChartInstance = null;
let allChartInstance = null;

function generateData(direction, value) {
  if (!data[direction]) {
    data[direction] = [];
  }

  const date = new Date();
  const dateOnly = date.toISOString().split('T')[0];

  // Check if the date already exists in the data for the given direction
  const existingEntry = data[direction].find(entry => entry.date === dateOnly);

  if (!existingEntry) {
    // If the date does not exist, add a new entry
    data[direction].push({"date": dateOnly, "data": value});
  } else {
    // If the date exists, update the existing entry
    if (value > existingEntry.data) {
      existingEntry.data = value;
    } else {
      console.log('Value not updated. Existing value is greater.');
    }
  }

  // Update the max value for the direction
  const maxData = data[direction].reduce((max, current) => current.data > max ? current.data : max, 0);

  // Update the max value for the direction
  switch (direction) {
    case 'up':
      maxValueUp.innerText = maxData;
      break;
    case 'down':
      maxValueDown.innerText = maxData;
      break;
    case 'left':
      maxValueLeft.innerText = maxData;
      break;
    case 'right':
      maxValueRight.innerText = maxData;
      break;
  }
}


function updateGraphs() {
  const maxData = {};
  for (const direction in data) {
    data[direction].forEach(point => {
      if (!maxData[direction] || point.data > (maxData[direction]?.data || 0)) {
        maxData[direction] = {date: point.date, data: point.data};
      }
    });
  }

  const maxGraphCanvas = document.getElementById('maxGraph');
  const allGraphCanvas = document.getElementById('allGraph');

  if (!maxGraphCanvas || !allGraphCanvas) {
    console.error('Canvas elements not found.');
    return;
  }

  plotBarGraph(maxGraphCanvas, maxData);
  plotGraph(allGraphCanvas, data);
}


// Button click handlers
upButton.addEventListener('click', () => {
  let angle = parseInt(document.getElementById('angleInput').value);
  move('up')
//   updateGraphs();
});

downButton.addEventListener('click', () => {
  let angle = parseInt(document.getElementById('angleInput').value);
  move('down')
//   updateGraphs();
});

centerButton.addEventListener('click', () => {
  center();
});

leftButton.addEventListener('click', () => {
  let angle = parseInt(document.getElementById('angleInput').value);
  move('left')
//   updateGraphs();
});

rightButton.addEventListener('click', () => {
  let angle = parseInt(document.getElementById('angleInput').value);
  move('right')
//   updateGraphs();
});

// Helper function to generate random color
function getRandomColor() {
  const letters = '0123456789ABCDEF';
  let color = '#';
  for (let i = 0; i < 6; i++) {
    color += letters[Math.floor(Math.random() * 16)];
  }
  return color;
}

// function saveDataToServer() {
//   console.log('Saving data to server:', data)
//   fetch('http://localhost:8000/api/saveData', {
//     method: 'POST',
//     headers: {
//       'Content-Type': 'application/json',
//     },
//     body: JSON.stringify(data),
//   })
//   .then(response => response.json())
//   .then(serverResponse => console.log('Data saved:', serverResponse))
//   .catch(error => console.error('Failed to save data:', error));
// }


function plotBarGraph(container, maxData) {
  const ctx = container.getContext('2d');
  
  // Destroy existing chart if it exists
  if (maxChartInstance) {
    maxChartInstance.destroy();
  }

  const labels = Object.keys(maxData);
  const dataset = {
    labels: labels,
    datasets: [{
      label: 'Max Values',
      data: labels.map(label => maxData[label].data),
      backgroundColor: labels.map(() => getRandomColor()),
    }]
  };

  maxChartInstance = new Chart(ctx, {
    type: 'bar',
    data: dataset,
    options: {
      scales: {
        y: {
          beginAtZero: true,
          title: {
            display: true,
            text: 'Max Value'
          }
        },
        x: {
          title: {
            display: true,
            text: 'Direction'
          }
        }
      }
    }
  });
}

function plotGraph(container, data) {
  const ctx = container.getContext('2d');

  console.log('Checking existing allChartInstance:', allChartInstance);

  if (allChartInstance) {
    console.log('Destroying existing chart instance');
    allChartInstance.destroy();
  } else {
    console.log('No existing chart instance to destroy');
  }

  const datasets = Object.keys(data).map(direction => ({
    label: direction.charAt(0).toUpperCase() + direction.slice(1),
    data: data[direction].map(item => ({
      x: moment(item.date, "YYYY-MM-DD").toDate(),
      y: item.data
    })),
    borderColor: getRandomColor(),
    fill: false
  }));

  allChartInstance = new Chart(ctx, {
    type: 'line',
    data: {
      datasets: datasets
    },
    options: {
      scales: {
        x: {
          type: 'time',
          time: {
            unit: 'day',
            tooltipFormat: 'YYYY-MM-DD',
            parser: 'YYYY-MM-DD' // ensure the parser matches the date format you're using
          },
          title: {
            display: true,
            text: 'Date'
          }
        },
        y: {
          beginAtZero: true,
          title: {
            display: true,
            text: 'Value'
          }
        }
      }
    }
    
  });

  console.log('New allChartInstance created:', allChartInstance);
}

function loadInitialData() {
  fetch('/load', {
      cache: "default"  // This setting uses the browser's caching strategy
  })
  .then(response => {
      if (response.status === 304) {
          console.log('Data not modified. Using cached data.');
      }
      return response.json(); // It will resolve to the cached data if available and valid
  })
  .then(loadedData => {
      data = loadedData; // Assuming loadedData is in the expected format
      updateGraphs(); // Update the graphs with the newly loaded data
  })
  .catch(error => console.log('Failed to load data:', error));
}


// Call this function when the page loads or based on specific actions
document.getElementById('loadButton').addEventListener('click', loadInitialData);


// Example of attaching this function to a button
document.getElementById('saveButton').addEventListener('click', saveDataToServer);
