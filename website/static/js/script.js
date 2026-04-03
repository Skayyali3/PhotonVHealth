async function fetchData() {
  const res = await fetch('/api/latest');
  const data = await res.json();

  console.log(data);
}

document.addEventListener('DOMContentLoaded', () => {
  setInterval(fetchData, 2000);
});
