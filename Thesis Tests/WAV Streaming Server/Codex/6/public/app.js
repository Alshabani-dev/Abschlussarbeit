const player = document.getElementById('player');
const progressBar = document.getElementById('progressBar');
const currentTimeEl = document.getElementById('currentTime');
const durationEl = document.getElementById('duration');
const statusEl = document.getElementById('status');
const playBtn = document.getElementById('play');
const pauseBtn = document.getElementById('pause');
const stopBtn = document.getElementById('stop');

let isSeeking = false;

function formatTime(seconds) {
  if (!Number.isFinite(seconds) || seconds < 0) {
    return '0:00';
  }
  const mins = Math.floor(seconds / 60);
  const secs = Math.floor(seconds % 60).toString().padStart(2, '0');
  return `${mins}:${secs}`;
}

function setStatus(text, className) {
  statusEl.textContent = text;
  statusEl.classList.remove('playing', 'paused', 'stopped');
  statusEl.classList.add(className);
}

function updateProgress() {
  if (isSeeking || !Number.isFinite(player.duration)) {
    return;
  }
  const progress = (player.currentTime / player.duration) * 100;
  progressBar.value = progress;
  progressBar.style.setProperty('--progress', progress);
  currentTimeEl.textContent = formatTime(player.currentTime);
  durationEl.textContent = formatTime(player.duration);
}

progressBar.addEventListener('input', () => {
  isSeeking = true;
  const value = Number(progressBar.value);
  progressBar.style.setProperty('--progress', value);
  currentTimeEl.textContent = formatTime((value / 100) * player.duration);
});

progressBar.addEventListener('change', () => {
  const value = Number(progressBar.value);
  if (Number.isFinite(player.duration)) {
    player.currentTime = (value / 100) * player.duration;
  }
  isSeeking = false;
});

['mousedown', 'touchstart'].forEach(evt => {
  progressBar.addEventListener(evt, () => {
    isSeeking = true;
  });
});

['mouseup', 'touchend', 'touchcancel'].forEach(evt => {
  progressBar.addEventListener(evt, () => {
    isSeeking = false;
  });
});

player.addEventListener('loadedmetadata', () => {
  durationEl.textContent = formatTime(player.duration);
  progressBar.value = 0;
  progressBar.style.setProperty('--progress', 0);
});

player.addEventListener('timeupdate', updateProgress);
player.addEventListener('seeking', () => {
  setStatus('Seeking…', 'paused');
});

player.addEventListener('seeked', () => {
  setStatus('Playing 🎵', player.paused ? 'paused' : 'playing');
});

player.addEventListener('ended', () => {
  setStatus('Playback complete ⏹', 'stopped');
  progressBar.value = 100;
  progressBar.style.setProperty('--progress', 100);
});

player.addEventListener('error', () => {
  setStatus('Playback error ⚠️', 'stopped');
});

playBtn.addEventListener('click', async () => {
  try {
    await player.play();
    setStatus('Playing 🎵', 'playing');
  } catch (err) {
    setStatus('Unable to start playback', 'stopped');
    console.error(err);
  }
});

pauseBtn.addEventListener('click', () => {
  player.pause();
  setStatus('Paused ⏸', 'paused');
});

stopBtn.addEventListener('click', () => {
  player.pause();
  player.currentTime = 0;
  progressBar.value = 0;
  progressBar.style.setProperty('--progress', 0);
  currentTimeEl.textContent = '0:00';
  setStatus('Stopped ⏹', 'stopped');
});
