document.addEventListener('DOMContentLoaded', () => {
    const player = document.getElementById('player');
    const progressBar = document.getElementById('progressBar');
    const currentTimeDisplay = document.getElementById('currentTime');
    const durationDisplay = document.getElementById('duration');
    const playBtn = document.getElementById('play');
    const pauseBtn = document.getElementById('pause');
    const stopBtn = document.getElementById('stop');
    const statusElement = document.getElementById('status');

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

        const currentTime = player.currentTime;
        const duration = player.duration;

        // Update progress bar
        const progress = (currentTime / duration) * 100 || 0;
        progressBar.value = progress;
        progressBar.parentElement.style.setProperty('--progress', `${progress}%`);

        // Update time displays
        currentTimeDisplay.textContent = formatTime(currentTime);
        durationDisplay.textContent = formatTime(duration);
    }

    // Set status with appropriate class
    function setStatus(text, className) {
        statusElement.textContent = text;
        statusElement.className = `status ${className}`;
    }

    // Event listeners
    player.addEventListener('loadedmetadata', () => {
        durationDisplay.textContent = formatTime(player.duration);
        setStatus('🎵 Ready to play', 'playing');
    });

    player.addEventListener('timeupdate', updateProgress);

    player.addEventListener('ended', () => {
        setStatus('⏹ Playback finished', 'stopped');
        progressBar.value = 100;
        progressBar.parentElement.style.setProperty('--progress', '100%');
    });

    player.addEventListener('error', () => {
        setStatus('⚠ Error loading audio', 'stopped');
    });

    // Progress bar interactions
    progressBar.addEventListener('input', () => {
        isSeeking = true;
        const progress = progressBar.value;
        progressBar.parentElement.style.setProperty('--progress', `${progress}%`);
    });

    progressBar.addEventListener('change', () => {
        const progress = progressBar.value / 100;
        player.currentTime = progress * player.duration;
        isSeeking = false;
    });

    progressBar.addEventListener('mousedown', () => {
        isSeeking = true;
    });

    progressBar.addEventListener('mouseup', () => {
        isSeeking = false;
    });

    // Control buttons
    playBtn.addEventListener('click', async () => {
        try {
            await player.play();
            setStatus('🎵 Playing', 'playing');
        } catch (error) {
            setStatus('⚠ Playback error', 'stopped');
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
        updateProgress();
    });

    // Initialize UI
    setStatus('Ready to play', '');
});
