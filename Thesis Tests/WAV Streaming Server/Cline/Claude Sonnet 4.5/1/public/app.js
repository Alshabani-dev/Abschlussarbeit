// DOM Elements
const player = document.getElementById('player');
const progressBar = document.getElementById('progressBar');
const currentTimeDisplay = document.getElementById('currentTime');
const durationDisplay = document.getElementById('duration');
const playButton = document.getElementById('play');
const pauseButton = document.getElementById('pause');
const stopButton = document.getElementById('stop');
const statusDisplay = document.getElementById('status');

// State
let isSeeking = false;

// Utility Functions
function formatTime(seconds) {
    if (isNaN(seconds) || !isFinite(seconds)) {
        return '0:00';
    }
    
    const mins = Math.floor(seconds / 60);
    const secs = Math.floor(seconds % 60);
    return `${mins}:${secs.toString().padStart(2, '0')}`;
}

function updateProgress() {
    if (!isSeeking && player.duration) {
        const progress = (player.currentTime / player.duration) * 100;
        progressBar.value = progress;
        progressBar.style.setProperty('--progress', `${progress}%`);
        currentTimeDisplay.textContent = formatTime(player.currentTime);
    }
}

function setStatus(text, className) {
    statusDisplay.textContent = text;
    statusDisplay.className = 'status';
    if (className) {
        statusDisplay.classList.add(className);
    }
}

// Audio Event Listeners
player.addEventListener('loadedmetadata', () => {
    durationDisplay.textContent = formatTime(player.duration);
    setStatus('Ready to play');
});

player.addEventListener('timeupdate', updateProgress);

player.addEventListener('ended', () => {
    setStatus('⏹ Playback finished', 'stopped');
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeDisplay.textContent = '0:00';
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

// Progress Bar Event Listeners
progressBar.addEventListener('input', (e) => {
    if (player.duration) {
        const seekTime = (e.target.value / 100) * player.duration;
        currentTimeDisplay.textContent = formatTime(seekTime);
        progressBar.style.setProperty('--progress', `${e.target.value}%`);
    }
});

progressBar.addEventListener('change', (e) => {
    if (player.duration) {
        const seekTime = (e.target.value / 100) * player.duration;
        player.currentTime = seekTime;
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

// Control Button Event Listeners
playButton.addEventListener('click', async () => {
    try {
        await player.play();
        setStatus('🎵 Playing', 'playing');
    } catch (error) {
        console.error('Play error:', error);
        setStatus('❌ Cannot play audio', 'stopped');
    }
});

pauseButton.addEventListener('click', () => {
    player.pause();
    setStatus('⏸ Paused', 'paused');
});

stopButton.addEventListener('click', () => {
    player.pause();
    player.currentTime = 0;
    setStatus('⏹ Stopped', 'stopped');
    progressBar.value = 0;
    progressBar.style.setProperty('--progress', '0%');
    currentTimeDisplay.textContent = '0:00';
});

// Initialize
window.addEventListener('load', () => {
    progressBar.style.setProperty('--progress', '0%');
    console.log('🎵 WAV Stream Player initialized');
});
