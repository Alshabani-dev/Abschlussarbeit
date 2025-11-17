document.addEventListener('DOMContentLoaded', () => {
    const player = document.getElementById('player');
    const progressBar = document.getElementById('progressBar');
    const currentTimeEl = document.getElementById('currentTime');
    const durationEl = document.getElementById('duration');
    const playBtn = document.getElementById('play');
    const pauseBtn = document.getElementById('pause');
    const stopBtn = document.getElementById('stop');
    const statusEl = document.getElementById('status');

    let isSeeking = false;

    // Format time from seconds to MM:SS
    function formatTime(seconds) {
        if (isNaN(seconds) || !isFinite(seconds)) {
            return "0:00";
        }

        const minutes = Math.floor(seconds / 60);
        const remainingSeconds = Math.floor(seconds % 60);
        return `${minutes}:${remainingSeconds < 10 ? '0' : ''}${remainingSeconds}`;
    }

    // Update progress bar and time display
    function updateProgress() {
        if (isSeeking) return;

        const progress = (player.currentTime / player.duration) * 100;
        progressBar.value = progress;
        progressBar.style.setProperty('--progress', `${progress}%`);
        currentTimeEl.textContent = formatTime(player.currentTime);
    }

    // Set status with appropriate class
    function setStatus(text, className) {
        statusEl.textContent = text;
        statusEl.className = 'status ' + className;
    }

    // Event listeners
    player.addEventListener('loadedmetadata', () => {
        durationEl.textContent = formatTime(player.duration);
        setStatus('Ready to play', '');
    });

    player.addEventListener('timeupdate', updateProgress);

    player.addEventListener('play', () => {
        setStatus('🎵 Playing', 'playing');
    });

    player.addEventListener('pause', () => {
        setStatus('⏸ Paused', 'paused');
    });

    player.addEventListener('ended', () => {
        setStatus('⏹ Stopped', 'stopped');
        progressBar.value = 0;
        progressBar.style.setProperty('--progress', '0%');
        currentTimeEl.textContent = '0:00';
    });

    player.addEventListener('error', () => {
        setStatus('⚠ Error loading audio', 'stopped');
    });

    // Progress bar events
    progressBar.addEventListener('input', () => {
        isSeeking = true;
        const progress = progressBar.value;
        progressBar.style.setProperty('--progress', `${progress}%`);
    });

    progressBar.addEventListener('change', () => {
        const seekTime = (progressBar.value / 100) * player.duration;
        player.currentTime = seekTime;
        isSeeking = false;
    });

    progressBar.addEventListener('mousedown', () => {
        isSeeking = true;
    });

    progressBar.addEventListener('mouseup', () => {
        isSeeking = false;
    });

    // Button events
    playBtn.addEventListener('click', async () => {
        try {
            await player.play();
        } catch (error) {
            console.error('Playback failed:', error);
            setStatus('⚠ Playback error', 'stopped');
        }
    });

    pauseBtn.addEventListener('click', () => {
        player.pause();
    });

    stopBtn.addEventListener('click', () => {
        player.pause();
        player.currentTime = 0;
        setStatus('⏹ Stopped', 'stopped');
    });

    // Initialize UI
    setStatus('Loading...', '');
});
