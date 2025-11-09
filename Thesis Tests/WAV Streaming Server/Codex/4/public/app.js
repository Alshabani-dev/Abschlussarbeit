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
  if (!Number.isFinite(seconds)) {
    return "0:00";
  }
  const minutes = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60)
    .toString()
    .padStart(2, "0");
  return `${minutes}:${secs}`;
};

const updateStatus = (text, stateClass) => {
  statusEl.textContent = text;
  statusEl.classList.remove("status-idle", "status-playing", "status-paused", "status-stopped");
  statusEl.classList.add(stateClass);
};

const updateProgressBarFill = () => {
  const value = progressBar.value;
  progressBar.style.background = `linear-gradient(90deg, #ff8a00 ${value}%, rgba(255,255,255,0.2) ${value}%)`;
};

const syncFromPlayer = () => {
  if (isSeeking) return;
  if (!Number.isFinite(player.duration) || player.duration === 0) return;

  const progress = (player.currentTime / player.duration) * 100;
  progressBar.value = progress;
  currentTimeEl.textContent = formatTime(player.currentTime);
  durationEl.textContent = formatTime(player.duration);
  updateProgressBarFill();
};

const rafLoop = () => {
  syncFromPlayer();
  requestAnimationFrame(rafLoop);
};

player.addEventListener("loadedmetadata", () => {
  durationEl.textContent = formatTime(player.duration);
});

player.addEventListener("play", () => {
  updateStatus("🎵 Playing", "status-playing");
});

player.addEventListener("pause", () => {
  if (player.currentTime === 0 || player.ended) return;
  updateStatus("⏸ Paused", "status-paused");
});

player.addEventListener("ended", () => {
  updateStatus("⏹ Stopped", "status-stopped");
  player.currentTime = 0;
  syncFromPlayer();
});

player.addEventListener("timeupdate", syncFromPlayer);

playBtn.addEventListener("click", () => {
  player.play().catch(() => updateStatus("⚠️ Unable to play", "status-stopped"));
});

pauseBtn.addEventListener("click", () => {
  player.pause();
  updateStatus("⏸ Paused", "status-paused");
});

stopBtn.addEventListener("click", () => {
  player.pause();
  player.currentTime = 0;
  updateStatus("⏹ Stopped", "status-stopped");
  syncFromPlayer();
});

progressBar.addEventListener("input", () => {
  isSeeking = true;
  updateProgressBarFill();
  const pct = parseFloat(progressBar.value) / 100;
  const seekTime = player.duration * pct;
  currentTimeEl.textContent = formatTime(seekTime);
});

progressBar.addEventListener("change", () => {
  if (!Number.isFinite(player.duration)) {
    isSeeking = false;
    return;
  }
  const pct = parseFloat(progressBar.value) / 100;
  player.currentTime = player.duration * pct;
  isSeeking = false;
});

progressBar.addEventListener("mouseup", () => (isSeeking = false));
progressBar.addEventListener("touchend", () => (isSeeking = false));

updateStatus("Ready to play", "status-idle");
updateProgressBarFill();
rafLoop();
