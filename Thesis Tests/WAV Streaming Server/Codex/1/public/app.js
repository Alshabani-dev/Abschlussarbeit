const player = document.getElementById("player");
const playBtn = document.getElementById("play");
const pauseBtn = document.getElementById("pause");
const stopBtn = document.getElementById("stop");
const progressBar = document.getElementById("progressBar");
const currentTimeEl = document.getElementById("currentTime");
const durationEl = document.getElementById("duration");
const statusEl = document.getElementById("status");

let isSeeking = false;

const setStatus = (text, stateClass) => {
  statusEl.textContent = text;
  statusEl.classList.remove("ready", "playing", "paused", "stopped");
  statusEl.classList.add(stateClass);
};

const formatTime = (value) => {
  if (Number.isNaN(value) || !Number.isFinite(value)) {
    return "0:00";
  }
  const minutes = Math.floor(value / 60);
  const seconds = Math.floor(value % 60)
    .toString()
    .padStart(2, "0");
  return `${minutes}:${seconds}`;
};

const updateProgressUI = () => {
  if (isSeeking || !player.duration) {
    return;
  }
  const progress = (player.currentTime / player.duration) * 100;
  progressBar.value = progress;
  currentTimeEl.textContent = formatTime(player.currentTime);
};

playBtn.addEventListener("click", () => {
  player.play();
  setStatus("🎵 Playing", "playing");
});

pauseBtn.addEventListener("click", () => {
  player.pause();
  setStatus("⏸ Paused", "paused");
});

stopBtn.addEventListener("click", () => {
  player.pause();
  player.currentTime = 0;
  progressBar.value = 0;
  setStatus("⏹ Stopped", "stopped");
  currentTimeEl.textContent = "0:00";
});

player.addEventListener("loadedmetadata", () => {
  durationEl.textContent = formatTime(player.duration);
});

player.addEventListener("timeupdate", updateProgressUI);

player.addEventListener("ended", () => {
  progressBar.value = 0;
  setStatus("⏹ Stopped", "stopped");
});

progressBar.addEventListener("input", () => {
  isSeeking = true;
  if (player.duration && Number.isFinite(player.duration)) {
    currentTimeEl.textContent = formatTime(
      (player.duration * progressBar.value) / 100
    );
  }
});

const finalizeSeek = () => {
  if (!isSeeking) return;
  if (!player.duration || Number.isNaN(player.duration)) {
    isSeeking = false;
    progressBar.value = 0;
    return;
  }
  player.currentTime = (player.duration * progressBar.value) / 100;
  isSeeking = false;
};

progressBar.addEventListener("change", finalizeSeek);
progressBar.addEventListener("mouseup", finalizeSeek);
progressBar.addEventListener("touchend", finalizeSeek);

player.addEventListener("error", () => {
  setStatus("⚠️ Unable to load audio", "stopped");
});
