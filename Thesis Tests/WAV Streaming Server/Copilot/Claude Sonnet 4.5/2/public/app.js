// Get DOM elements
const player = document.getElementById('player');
const progressBar = document.getElementById('progressBar');
const currentTimeEl = document.getElementById('currentTime');
const durationEl = document.getElementById('duration');
const playBtn = document.getElementById('play');
const pauseBtn = document.getElementById('pause');
const stopBtn = document.getElementById('stop');
const statusEl = document.getElementById('status');

// State management
let isSeeking = false;

/**
 * Format time in seconds to M:SS format
 * @param {number} seconds - Time in seconds
 * @returns {string} Formatted time string
 */
function formatTime(seconds) {
    if (isNaN(seconds) || !isFinite(seconds)) {
        return '0:00';
    }
    
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
}

/**
 * Update status display with color class
 * @param {string} text - Status text to display
 * @param {string} className - CSS class for styling (playing/paused/stopped)
 */
function setStatus(text, className) {
    statusEl.textContent = text;
    statusEl.className = 'status';
    if (className) {
        statusEl.classList.add(className);
    }
}

/**
 * Update progress bar and current time display
 */
function updateProgress() {
    if (!isSeeking && player.duration) {
        const progress = (player.currentTime / player.duration) * 100;
        progressBar.value = progress;
        progressBar.style.setProperty('--progress', `${progress}%`);
        currentTimeEl.textContent = formatTime(player.currentTime);
    }
}

// Audio event listeners

player.addEventListener('loadedmetadata', () => {
    durationEl.textContent = formatTime(player.duration);
    console.log('Audio metadata loaded, duration:', player.duration);
});

player.addEventListener('timeupdate', updateProgress);

player.addEventListener('ended', () => {
    setStatus('⏹ Track finished', 'stopped');
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeEl.textContent = '0:00';
});

player.addEventListener('error', (e) => {
    console.error('Audio error:', e);
    setStatus('❌ Error loading audio', 'stopped');
});

player.addEventListener('seeking', () => {
    console.log('Seeking to:', player.currentTime);
});

player.addEventListener('seeked', () => {
    console.log('Seeked to:', player.currentTime);
    setStatus('🎵 Playing', 'playing');
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
        console.log('Seeked to:', time);
    }
});

progressBar.addEventListener('mousedown', () => {
    isSeeking = true;
});

progressBar.addEventListener('mouseup', () => {
    isSeeking = false;
});

// Touch support for mobile
progressBar.addEventListener('touchstart', () => {
    isSeeking = true;
});

progressBar.addEventListener('touchend', () => {
    isSeeking = false;
});

// Control button event listeners

playBtn.addEventListener('click', async () => {
    try {
        await player.play();
        setStatus('🎵 Playing', 'playing');
        console.log('Playback started');
    } catch (error) {
        console.error('Play error:', error);
        setStatus('❌ Failed to play', 'stopped');
    }
});

pauseBtn.addEventListener('click', () => {
    player.pause();
    setStatus('⏸ Paused', 'paused');
    console.log('Playback paused');
});

stopBtn.addEventListener('click', () => {
    player.pause();
    player.currentTime = 0;
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeEl.textContent = '0:00';
    setStatus('⏹ Stopped', 'stopped');
    console.log('Playback stopped');
});

// Initialize
console.log('WAV Stream Player initialized');
setStatus('Ready to play', '');
