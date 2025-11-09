const player = document.getElementById("player");
const playBtn = document.getElementById("play");
const pauseBtn = document.getElementById("pause");
const stopBtn = document.getElementById("stop");
const progressBar = document.getElementById("progressBar");
const currentTimeEl = document.getElementById("currentTime");
const durationEl = document.getElementById("duration");
const statusEl = document.getElementById("status");

let isSeeking = false;

const formatTime = (seconds) => {
  if (!Number.isFinite(seconds)) return "0:00";
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60);
  return `${mins}:${secs.toString().padStart(2, "0")}`;
};

const updateDuration = () => {
  if (Number.isFinite(player.duration)) {
    durationEl.textContent = formatTime(player.duration);
    progressBar.max = player.duration;
  }
};

const updateStatus = (text, state) => {
  statusEl.textContent = text;
  statusEl.className = `status ${state}`;
};

playBtn.addEventListener("click", () => {
  player.play();
});

pauseBtn.addEventListener("click", () => {
  player.pause();
  updateStatus("⏸ Paused", "paused");
});

stopBtn.addEventListener("click", () => {
  player.pause();
  player.currentTime = 0;
  updateStatus("⏹ Stopped", "stopped");
  if (!isSeeking) {
    progressBar.value = 0;
    currentTimeEl.textContent = "0:00";
  }
});

player.addEventListener("loadedmetadata", updateDuration);

player.addEventListener("play", () => updateStatus("🎵 Playing", "playing"));
player.addEventListener("pause", () => {
  if (player.currentTime === 0 || player.currentTime === player.duration) return;
  updateStatus("⏸ Paused", "paused");
});
player.addEventListener("ended", () => {
  updateStatus("⏹ Finished", "stopped");
  progressBar.value = 0;
  currentTimeEl.textContent = "0:00";
});

player.addEventListener("timeupdate", () => {
  if (isSeeking) return;
  currentTimeEl.textContent = formatTime(player.currentTime);
  progressBar.value = player.currentTime;
});

progressBar.addEventListener("input", () => {
  isSeeking = true;
  currentTimeEl.textContent = formatTime(progressBar.value);
});

progressBar.addEventListener("change", () => {
  player.currentTime = Number(progressBar.value);
  isSeeking = false;
});

progressBar.addEventListener("mouseup", () => (isSeeking = false));
progressBar.addEventListener("touchend", () => (isSeeking = false));

// Animated background canvas
const canvas = document.getElementById("backdrop");
const ctx = canvas.getContext("2d");

let width = 0;
let height = 0;
let hue = 200;

const resize = () => {
  width = canvas.width = window.innerWidth;
  height = canvas.height = window.innerHeight;
};

window.addEventListener("resize", resize);
resize();

const drawBackdrop = () => {
  hue = (hue + 0.2) % 360;
  const gradient = ctx.createLinearGradient(0, 0, width, height);
  gradient.addColorStop(0, `hsla(${hue}, 80%, 55%, 0.6)`);
  gradient.addColorStop(1, `hsla(${(hue + 90) % 360}, 70%, 45%, 0.5)`);
  ctx.fillStyle = gradient;
  ctx.fillRect(0, 0, width, height);
  requestAnimationFrame(drawBackdrop);
};

drawBackdrop();
