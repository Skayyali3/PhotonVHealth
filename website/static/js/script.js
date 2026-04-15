async function fetchData() {
  const res = await fetch('/api/latest');
  const data = await res.json();

  document.getElementById("power").innerText = data.power ?? "--";
  document.getElementById("temp").innerText = data.temp ?? "--";
  document.getElementById("light").innerText = data.light ?? "--";
  document.getElementById("eff").innerText = data.efficiency ?? "--";
  document.getElementById("health").innerText = data.health ?? "--";
}

document.addEventListener('DOMContentLoaded', () => {
  setInterval(fetchData, 2000);
});