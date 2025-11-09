const player = document.getElementById('player');
const progressBar = document.getElementById('progressBar');
const currentTimeEl = document.getElementById('currentTime');
const durationEl = document.getElementById('duration');
const statusEl = document.getElementById('status');
const playBtn = document.getElementById('play');
const pauseBtn = document.getElementById('pause');
const stopBtn = document.getElementById('stop');

let isSeeking = false;
let rafId = null;

const formatTime = (seconds) => {
    if (!Number.isFinite(seconds) || seconds < 0) {
        return '0:00';
    }
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
};

const updateStatus = (text, state) => {
    statusEl.textContent = text;
    statusEl.className = `status ${state}`;
};

const updateProgress = () => {
    if (!isSeeking && Number.isFinite(player.duration) && player.duration > 0) {
        const percentage = (player.currentTime / player.duration) * 100;
        progressBar.value = percentage;
        currentTimeEl.textContent = formatTime(player.currentTime);
    }
    if (!player.paused && !player.ended) {
        rafId = requestAnimationFrame(updateProgress);
    }
};

player.addEventListener('loadedmetadata', () => {
    durationEl.textContent = formatTime(player.duration);
});

player.addEventListener('timeupdate', () => {
    if (!isSeeking) {
        currentTimeEl.textContent = formatTime(player.currentTime);
    }
});

player.addEventListener('play', () => {
    cancelAnimationFrame(rafId);
    updateStatus('🎵 Playing track.wav', 'playing');
    rafId = requestAnimationFrame(updateProgress);
});

player.addEventListener('pause', () => {
    updateStatus('⏸ Paused', 'paused');
    cancelAnimationFrame(rafId);
});

player.addEventListener('ended', () => {
    progressBar.value = 0;
    currentTimeEl.textContent = '0:00';
    updateStatus('⏹ Stopped', 'stopped');
});

progressBar.addEventListener('input', () => {
    isSeeking = true;
    const percentage = parseFloat(progressBar.value);
    if (Number.isFinite(player.duration)) {
        const targetTime = (percentage / 100) * player.duration;
        currentTimeEl.textContent = formatTime(targetTime);
    }
});

const commitSeek = () => {
    if (Number.isFinite(player.duration)) {
        const newTime = (parseFloat(progressBar.value) / 100) * player.duration;
        player.currentTime = newTime;
    }
    isSeeking = false;
    if (!player.paused) {
        rafId = requestAnimationFrame(updateProgress);
    }
};

progressBar.addEventListener('change', commitSeek);
progressBar.addEventListener('mouseup', commitSeek);
progressBar.addEventListener('touchend', commitSeek);

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
    updateStatus('⏹ Stopped', 'stopped');
});

updateStatus('Ready to play', 'ready');
