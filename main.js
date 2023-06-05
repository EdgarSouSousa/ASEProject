function fetchData() {
  fetch("./data.json")
    .then((response) => response.json())
    .then((data) => displayData(data))
    .catch((error) => console.error("Error fetching data:", error));
}

function displayData(data) {
  document.querySelector("#value1").textContent = data.value1[0];
  document.querySelector("#value2").textContent = data.value2[0];
  document.querySelector("#value3").textContent = data.temperature[0];
  document.querySelector("#value4").textContent = data.humidity[0];
}

// Fetch data every 3 seconds (3000 milliseconds)
setInterval(fetchData, 10000);