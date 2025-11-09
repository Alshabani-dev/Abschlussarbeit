'use strict';

const player = document.getElementById('player');
const progressBar = document.getElementById('progressBar');
const playBtn = document.getElementById('play');
const pauseBtn = document.getElementById('pause');
const stopBtn = document.getElementById('stop');
const currentTimeEl = document.getElementById('currentTime');
const durationEl = document.getElementById('duration');
const statusEl = document.getElementById('status');

let isSeeking = false;
let rafId = null;

const formatTime = (seconds) => {
    if (!Number.isFinite(seconds)) {
        return '0:00';
    }
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60)
        .toString()
        .padStart(2, '0');
    return `${mins}:${secs}`;
};

const setStatus = (text, stateClass) => {
    statusEl.textContent = text;
    statusEl.className = `status ${stateClass}`;
};

const updateDuration = () => {
    if (Number.isFinite(player.duration)) {
        durationEl.textContent = formatTime(player.duration);
    }
};

const syncProgress = () => {
    if (!Number.isFinite(player.duration) || isSeeking) {
        return;
    }
    const percent = (player.currentTime / player.duration) * 100;
    progressBar.value = percent;
    currentTimeEl.textContent = formatTime(player.currentTime);
};

const tick = () => {
    syncProgress();
    rafId = requestAnimationFrame(tick);
};

const startTicker = () => {
    if (rafId === null) {
        rafId = requestAnimationFrame(tick);
    }
};

const stopTicker = () => {
    if (rafId !== null) {
        cancelAnimationFrame(rafId);
        rafId = null;
    }
};

const commitSeek = () => {
    if (!isSeeking || !Number.isFinite(player.duration)) return;
    player.currentTime = (progressBar.value / 100) * player.duration;
    isSeeking = false;
};

playBtn.addEventListener('click', () => {
    player.play().catch(() => {
        setStatus('⚠️ Unable to start playback', 'stopped');
    });
});

pauseBtn.addEventListener('click', () => {
    player.pause();
});

stopBtn.addEventListener('click', () => {
    player.pause();
    player.currentTime = 0;
    progressBar.value = 0;
    currentTimeEl.textContent = '0:00';
    setStatus('⏹ Stopped', 'stopped');
});

player.addEventListener('loadedmetadata', updateDuration);

player.addEventListener('play', () => {
    setStatus('🎵 Playing', 'playing');
    startTicker();
});

player.addEventListener('pause', () => {
    stopTicker();
    if (!player.ended) {
        setStatus('⏸ Paused', 'paused');
    }
});

player.addEventListener('waiting', () => {
    setStatus('⌛ Buffering…', 'paused');
});

player.addEventListener('timeupdate', syncProgress);

player.addEventListener('ended', () => {
    stopTicker();
    progressBar.value = 100;
    currentTimeEl.textContent = durationEl.textContent;
    setStatus('⏹ Finished', 'stopped');
});

player.addEventListener('error', () => {
    stopTicker();
    setStatus('⚠️ Audio failed to load', 'stopped');
});

progressBar.addEventListener('input', () => {
    if (!Number.isFinite(player.duration)) return;
    isSeeking = true;
    const preview = (progressBar.value / 100) * player.duration;
    currentTimeEl.textContent = formatTime(preview);
});

progressBar.addEventListener('change', commitSeek);
progressBar.addEventListener('mouseup', commitSeek);
progressBar.addEventListener('touchend', commitSeek);
progressBar.addEventListener('mousedown', () => (isSeeking = true));
progressBar.addEventListener('touchstart', () => (isSeeking = true));

document.addEventListener('visibilitychange', () => {
    if (document.hidden) {
        stopTicker();
    } else if (!player.paused) {
        startTicker();
    }
});

setStatus('Ready to play', 'ready');
