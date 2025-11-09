const player = document.getElementById("player");
const progressBar = document.getElementById("progressBar");
const currentTimeEl = document.getElementById("currentTime");
const durationEl = document.getElementById("duration");
const statusEl = document.getElementById("status");
const playBtn = document.getElementById("play");
const pauseBtn = document.getElementById("pause");
const stopBtn = document.getElementById("stop");

let isSeeking = false;

const setStatus = (text, state) => {
  statusEl.textContent = text;
  statusEl.className = `status ${state}`;
};

const setProgressVisual = (value) => {
  progressBar.style.setProperty("--progress", value.toFixed(2));
};

const formatTime = (seconds) => {
  if (!Number.isFinite(seconds) || seconds < 0) {
    return "0:00";
  }
  const rounded = Math.floor(seconds);
  const mins = Math.floor(rounded / 60);
  const secs = rounded % 60;
  return `${mins}:${secs.toString().padStart(2, "0")}`;
};

const updateProgress = () => {
  if (!player.duration || isSeeking) {
    return;
  }
  const progress = (player.currentTime / player.duration) * 100;
  progressBar.value = progress;
  setProgressVisual(progress);
  currentTimeEl.textContent = formatTime(player.currentTime);
};

const applySeek = () => {
  if (!player.duration) {
    return;
  }
  const value = parseFloat(progressBar.value);
  const seekTime = (value / 100) * player.duration;
  player.currentTime = seekTime;
  currentTimeEl.textContent = formatTime(seekTime);
  setProgressVisual(value);
};

player.addEventListener("loadedmetadata", () => {
  durationEl.textContent = formatTime(player.duration);
  updateProgress();
});

player.addEventListener("timeupdate", updateProgress);

player.addEventListener("play", () => {
  setStatus("🎵 Now playing", "playing");
});

player.addEventListener("pause", () => {
  if (player.currentTime === 0 || player.currentTime === player.duration) {
    return;
  }
  setStatus("⏸ Paused", "paused");
});

player.addEventListener("ended", () => {
  setStatus("⏹ Track finished", "stopped");
  progressBar.value = 100;
  setProgressVisual(100);
  currentTimeEl.textContent = durationEl.textContent;
});

player.addEventListener("error", () => {
  setStatus("⚠️ Unable to load audio", "stopped");
});

progressBar.addEventListener("input", (event) => {
  isSeeking = true;
  const value = parseFloat(event.target.value);
  setProgressVisual(value);
  if (player.duration) {
    currentTimeEl.textContent = formatTime((value / 100) * player.duration);
  }
});

const finishSeek = () => {
  if (!isSeeking) return;
  isSeeking = false;
  applySeek();
};

progressBar.addEventListener("change", finishSeek);
progressBar.addEventListener("mousedown", () => {
  isSeeking = true;
});
progressBar.addEventListener("mouseup", finishSeek);
progressBar.addEventListener("touchstart", () => {
  isSeeking = true;
});
progressBar.addEventListener("touchend", finishSeek);

playBtn.addEventListener("click", async () => {
  try {
    await player.play();
  } catch (error) {
    console.error(error);
    setStatus("⚠️ Tap the page to allow audio playback", "stopped");
  }
});

pauseBtn.addEventListener("click", () => {
  player.pause();
  setStatus("⏸ Paused", "paused");
});

stopBtn.addEventListener("click", () => {
  player.pause();
  player.currentTime = 0;
  progressBar.value = 0;
  setProgressVisual(0);
  currentTimeEl.textContent = "0:00";
  setStatus("⏹ Stopped", "stopped");
});
