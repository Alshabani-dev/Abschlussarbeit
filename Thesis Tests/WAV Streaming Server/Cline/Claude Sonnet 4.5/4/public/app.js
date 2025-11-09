// Get DOM elements
const player = document.getElementById('player');
const progressBar = document.getElementById('progressBar');
const currentTimeEl = document.getElementById('currentTime');
const durationEl = document.getElementById('duration');
const playBtn = document.getElementById('play');
const pauseBtn = document.getElementById('pause');
const stopBtn = document.getElementById('stop');
const statusEl = document.getElementById('status');

// State
let isSeeking = false;

// Format time from seconds to M:SS or MM:SS
function formatTime(seconds) {
    if (isNaN(seconds) || !isFinite(seconds)) {
        return '0:00';
    }
    
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
}

// Update progress bar and current time
function updateProgress() {
    if (!isSeeking && player.duration) {
        const progress = (player.currentTime / player.duration) * 100;
        progressBar.value = progress;
        progressBar.style.setProperty('--progress', `${progress}%`);
        currentTimeEl.textContent = formatTime(player.currentTime);
    }
}

// Set status message with color class
function setStatus(text, className) {
    statusEl.textContent = text;
    statusEl.className = 'status';
    if (className) {
        statusEl.classList.add(className);
    }
}

// Audio event listeners
player.addEventListener('loadedmetadata', () => {
    durationEl.textContent = formatTime(player.duration);
    setStatus('Ready to play');
});

player.addEventListener('timeupdate', updateProgress);

player.addEventListener('ended', () => {
    setStatus('⏹ Playback finished', 'stopped');
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeEl.textContent = '0:00';
});

player.addEventListener('error', (e) => {
    console.error('Audio error:', e);
    setStatus('❌ Error loading audio', 'stopped');
});

player.addEventListener('seeking', () => {
    setStatus('⏩ Seeking...', 'paused');
});

player.addEventListener('seeked', () => {
    if (!player.paused) {
        setStatus('🎵 Playing', 'playing');
    } else {
        setStatus('⏸ Paused', 'paused');
    }
});

// Progress bar event listeners
progressBar.addEventListener('input', (e) => {
    if (player.duration) {
        const time = (e.target.value / 100) * player.duration;
        currentTimeEl.textContent = formatTime(time);
        progressBar.style.setProperty('--progress', `${e.target.value}%`);
    }
});

progressBar.addEventListener('change', (e) => {
    if (player.duration) {
        const time = (e.target.value / 100) * player.duration;
        player.currentTime = time;
    }
});

progressBar.addEventListener('mousedown', () => {
    isSeeking = true;
});

progressBar.addEventListener('mouseup', () => {
    isSeeking = false;
});

progressBar.addEventListener('touchstart', () => {
    isSeeking = true;
});

progressBar.addEventListener('touchend', () => {
    isSeeking = false;
});

// Control button handlers
playBtn.addEventListener('click', async () => {
    try {
        await player.play();
        setStatus('🎵 Playing', 'playing');
    } catch (error) {
        console.error('Play error:', error);
        setStatus('❌ Cannot play audio', 'stopped');
    }
});

pauseBtn.addEventListener('click', () => {
    player.pause();
    setStatus('⏸ Paused', 'paused');
});

stopBtn.addEventListener('click', () => {
    player.pause();
    player.currentTime = 0;
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeEl.textContent = '0:00';
    setStatus('⏹ Stopped', 'stopped');
});

// Initialize
setStatus('Loading audio...');
