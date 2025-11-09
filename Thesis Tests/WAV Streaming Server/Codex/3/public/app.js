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
  if (!Number.isFinite(seconds) || seconds < 0) return '0:00';
  const m = Math.floor(seconds / 60);
  const s = Math.floor(seconds % 60)
    .toString()
    .padStart(2, '0');
  return `${m}:${s}`;
}

function setStatus(text, state) {
  statusEl.textContent = text;
  statusEl.classList.remove('playing', 'paused', 'stopped');
  statusEl.classList.add(state);
}

function setSliderProgress(percent) {
  progressBar.value = percent;
  progressBar.style.setProperty('--progress', `${percent}%`);
}

function updateProgress() {
  if (!player.duration || isSeeking) return;
  const percent = (player.currentTime / player.duration) * 100;
  setSliderProgress(percent);
  currentTimeEl.textContent = formatTime(player.currentTime);
  durationEl.textContent = formatTime(player.duration);
}

function progressLoop() {
  requestAnimationFrame(progressLoop);
  updateProgress();
}

playBtn.addEventListener('click', async () => {
  try {
    await player.play();
    setStatus('🎵 Playing', 'playing');
  } catch (err) {
    console.error(err);
    setStatus('⚠️ Unable to play – check console', 'stopped');
  }
});

pauseBtn.addEventListener('click', () => {
  player.pause();
  setStatus('⏸ Paused', 'paused');
});

stopBtn.addEventListener('click', () => {
  player.pause();
  player.currentTime = 0;
  setStatus('⏹ Stopped', 'stopped');
  setSliderProgress(0);
  currentTimeEl.textContent = '0:00';
});

progressBar.addEventListener('input', () => {
  isSeeking = true;
  const percent = parseFloat(progressBar.value);
  setSliderProgress(percent);
  if (player.duration) {
    const preview = (percent / 100) * player.duration;
    currentTimeEl.textContent = formatTime(preview);
  }
});

const commitSeek = () => {
  if (!player.duration) {
    isSeeking = false;
    return;
  }
  const percent = parseFloat(progressBar.value);
  const targetTime = (percent / 100) * player.duration;
  player.currentTime = targetTime;
  isSeeking = false;
};

progressBar.addEventListener('change', commitSeek);
progressBar.addEventListener('mouseup', commitSeek);
progressBar.addEventListener('touchend', commitSeek);
progressBar.addEventListener('mousedown', () => (isSeeking = true));
progressBar.addEventListener('touchstart', () => (isSeeking = true));

player.addEventListener('loadedmetadata', () => {
  durationEl.textContent = formatTime(player.duration);
});

player.addEventListener('timeupdate', updateProgress);

player.addEventListener('ended', () => {
  setStatus('Track finished', 'stopped');
  setSliderProgress(100);
});

player.addEventListener('seeking', () => {
  if (!isSeeking) {
    setStatus('Seeking…', 'paused');
  }
});

player.addEventListener('seeked', () => {
  if (!player.paused) {
    setStatus('🎵 Playing', 'playing');
  }
});

player.addEventListener('error', () => {
  setStatus('⚠️ Error loading audio', 'stopped');
});

progressLoop();
setStatus('Ready to play', 'stopped');
