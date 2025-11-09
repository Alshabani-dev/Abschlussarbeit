const player = document.getElementById('player');
const progressBar = document.getElementById('progressBar');
const currentTimeEl = document.getElementById('currentTime');
const durationEl = document.getElementById('duration');
const statusEl = document.getElementById('status');
const playBtn = document.getElementById('play');
const pauseBtn = document.getElementById('pause');
const stopBtn = document.getElementById('stop');

let isSeeking = false;

const formatTime = (value) => {
  if (!value || Number.isNaN(value) || !Number.isFinite(value)) {
    return '0:00';
  }
  const minutes = Math.floor(value / 60);
  const seconds = Math.floor(value % 60)
    .toString()
    .padStart(2, '0');
  return `${minutes}:${seconds}`;
};

const setStatus = (state, message) => {
  statusEl.className = `status ${state}`;
  statusEl.textContent = message;
};

const updateLoop = () => {
  if (!isSeeking && player.duration) {
    const progress = (player.currentTime / player.duration) * 100;
    progressBar.value = progress;
  }
  currentTimeEl.textContent = formatTime(player.currentTime);
  requestAnimationFrame(updateLoop);
};

player.addEventListener('loadedmetadata', () => {
  durationEl.textContent = formatTime(player.duration);
});

player.addEventListener('play', () => {
  setStatus('playing', '🎵 Now playing');
});

player.addEventListener('pause', () => {
  if (player.currentTime === 0 || player.ended) {
    setStatus('stopped', '⏹ Stopped');
  } else {
    setStatus('paused', '⏸ Paused');
  }
});

player.addEventListener('ended', () => {
  setStatus('stopped', '⏹ Playback finished');
  progressBar.value = 0;
  currentTimeEl.textContent = '0:00';
});

progressBar.addEventListener('input', (event) => {
  isSeeking = true;
  const value = Number(event.target.value);
  if (player.duration) {
    const preview = (value / 100) * player.duration;
    currentTimeEl.textContent = formatTime(preview);
  }
});

progressBar.addEventListener('change', (event) => {
  if (player.duration) {
    const value = Number(event.target.value);
    player.currentTime = (value / 100) * player.duration;
  }
  isSeeking = false;
});

playBtn.addEventListener('click', () => {
  player.play();
});

pauseBtn.addEventListener('click', () => {
  player.pause();
});

stopBtn.addEventListener('click', () => {
  player.pause();
  player.currentTime = 0;
  progressBar.value = 0;
  currentTimeEl.textContent = '0:00';
  setStatus('stopped', '⏹ Stopped');
});

requestAnimationFrame(updateLoop);
