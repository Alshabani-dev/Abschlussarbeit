// DOM Elements
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

// Format time in M:SS or MM:SS format
function formatTime(seconds) {
    if (isNaN(seconds) || !isFinite(seconds)) {
        return '0:00';
    }
    
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
}

// Update progress bar and current time display
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

// Audio Event Listeners

// When metadata is loaded, set duration
player.addEventListener('loadedmetadata', () => {
    durationEl.textContent = formatTime(player.duration);
    setStatus('Ready to play', '');
});

// Update progress in real-time during playback
player.addEventListener('timeupdate', updateProgress);

// When audio ends
player.addEventListener('ended', () => {
    setStatus('⏹ Playback finished', 'stopped');
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeEl.textContent = '0:00';
});

// Handle errors
player.addEventListener('error', (e) => {
    console.error('Audio error:', e);
    setStatus('❌ Error loading audio', 'stopped');
});

// When seeking starts
player.addEventListener('seeking', () => {
    setStatus('⏩ Seeking...', 'paused');
});

// When seeking completes
player.addEventListener('seeked', () => {
    if (!player.paused) {
        setStatus('🎵 Playing', 'playing');
    } else {
        setStatus('⏸ Paused', 'paused');
    }
});

// Progress Bar Event Listeners

// Track when user starts dragging slider
progressBar.addEventListener('mousedown', () => {
    isSeeking = true;
});

// Apply seek when user releases slider
progressBar.addEventListener('mouseup', () => {
    isSeeking = false;
});

// Update visual progress while dragging
progressBar.addEventListener('input', (e) => {
    const value = e.target.value;
    progressBar.style.setProperty('--progress', `${value}%`);
    
    // Update current time display while dragging
    if (player.duration) {
        const newTime = (value / 100) * player.duration;
        currentTimeEl.textContent = formatTime(newTime);
    }
});

// Seek to new position when slider changes
progressBar.addEventListener('change', (e) => {
    if (player.duration) {
        const newTime = (e.target.value / 100) * player.duration;
        player.currentTime = newTime;
    }
});

// Button Event Listeners

// Play button
playBtn.addEventListener('click', async () => {
    try {
        await player.play();
        setStatus('🎵 Playing', 'playing');
    } catch (error) {
        console.error('Play error:', error);
        setStatus('❌ Failed to play', 'stopped');
    }
});

// Pause button
pauseBtn.addEventListener('click', () => {
    player.pause();
    setStatus('⏸ Paused', 'paused');
});

// Stop button
stopBtn.addEventListener('click', () => {
    player.pause();
    player.currentTime = 0;
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeEl.textContent = '0:00';
    setStatus('⏹ Stopped', 'stopped');
});

// Initialize on load
window.addEventListener('load', () => {
    progressBar.style.setProperty('--progress', '0%');
    currentTimeEl.textContent = '0:00';
    
    // Try to load metadata
    if (player.readyState >= 1) {
        durationEl.textContent = formatTime(player.duration);
    }
});
