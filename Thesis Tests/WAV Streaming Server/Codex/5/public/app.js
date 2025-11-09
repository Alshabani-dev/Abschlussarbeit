const statusMap = {
  ready: { text: 'Ready to play', className: 'ready' },
  playing: { text: '🎵 Playing', className: 'playing' },
  paused: { text: '⏸ Paused', className: 'paused' },
  stopped: { text: '⏹ Stopped', className: 'stopped' }
};

document.addEventListener('DOMContentLoaded', () => {
  const player = document.getElementById('player');
  const playBtn = document.getElementById('play');
  const pauseBtn = document.getElementById('pause');
  const stopBtn = document.getElementById('stop');
  const progressBar = document.getElementById('progressBar');
  const currentTimeEl = document.getElementById('currentTime');
  const durationEl = document.getElementById('duration');
  const statusEl = document.getElementById('status');

  let isSeeking = false;

  const setStatus = (state) => {
    Object.values(statusMap).forEach(({ className }) => statusEl.classList.remove(className));
    statusEl.classList.add(statusMap[state].className);
    statusEl.textContent = statusMap[state].text;
  };

  const formatTime = (time) => {
    if (!Number.isFinite(time) || time < 0) return '0:00';
    const minutes = Math.floor(time / 60);
    const seconds = Math.floor(time % 60).toString().padStart(2, '0');
    return `${minutes}:${seconds}`;
  };

  const updateProgress = () => {
    if (isSeeking) return;
    const duration = player.duration || 0;
    const current = player.currentTime || 0;
    const percent = duration ? (current / duration) * 100 : 0;
    progressBar.value = percent;
    currentTimeEl.textContent = formatTime(current);
    durationEl.textContent = formatTime(duration);
  };

  playBtn.addEventListener('click', () => {
    player.play();
  });

  pauseBtn.addEventListener('click', () => {
    player.pause();
    setStatus('paused');
  });

  stopBtn.addEventListener('click', () => {
    player.pause();
    player.currentTime = 0;
    setStatus('stopped');
    updateProgress();
  });

  progressBar.addEventListener('input', () => {
    isSeeking = true;
    const duration = player.duration || 0;
    const nextTime = (progressBar.value / 100) * duration;
    currentTimeEl.textContent = formatTime(nextTime);
  });

  progressBar.addEventListener('change', () => {
    const duration = player.duration || 0;
    const nextTime = (progressBar.value / 100) * duration;
    player.currentTime = nextTime;
    isSeeking = false;
  });

  player.addEventListener('timeupdate', updateProgress);

  player.addEventListener('play', () => setStatus('playing'));
  player.addEventListener('pause', () => {
    if (player.currentTime === 0 || player.currentTime === player.duration) return;
    setStatus('paused');
  });
  player.addEventListener('ended', () => {
    setStatus('stopped');
    player.currentTime = 0;
    updateProgress();
  });

  player.addEventListener('loadedmetadata', () => {
    durationEl.textContent = formatTime(player.duration);
  });

  player.addEventListener('error', () => {
    statusEl.textContent = '⚠️ Unable to load audio';
    statusEl.className = 'status stopped';
  });

  setStatus('ready');
});
