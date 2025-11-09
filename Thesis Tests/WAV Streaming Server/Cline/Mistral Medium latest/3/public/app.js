document.addEventListener('DOMContentLoaded', () => {
    const player = document.getElementById('player');
    const progressBar = document.getElementById('progressBar');
    const currentTimeDisplay = document.getElementById('currentTime');
    const durationDisplay = document.getElementById('duration');
    const playBtn = document.getElementById('play');
    const pauseBtn = document.getElementById('pause');
    const stopBtn = document.getElementById('stop');
    const statusDisplay = document.getElementById('status');

    let isSeeking = false;

    // Format time from seconds to M:SS
    function formatTime(seconds) {
        if (isNaN(seconds) || !isFinite(seconds)) return "0:00";

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
        const progress = duration > 0 ? (currentTime / duration) * 100 : 0;
        progressBar.value = progress;
        progressBar.style.setProperty('--progress', `${progress}%`);

        // Update time displays
        currentTimeDisplay.textContent = formatTime(currentTime);
        durationDisplay.textContent = formatTime(duration);
    }

    // Set status with appropriate class
    function setStatus(text, className) {
        statusDisplay.textContent = text;
        statusDisplay.className = `status ${className}`;
    }

    // Event listeners
    player.addEventListener('loadedmetadata', () => {
        durationDisplay.textContent = formatTime(player.duration);
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
        progressBar.value = 100;
    });

    player.addEventListener('error', () => {
        setStatus('⚠ Error loading audio', 'stopped');
        console.error('Audio error:', player.error);
    });

    // Progress bar interactions
    progressBar.addEventListener('input', () => {
        isSeeking = true;
        const seekTime = (progressBar.value / 100) * player.duration;
        currentTimeDisplay.textContent = formatTime(seekTime);
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
        updateProgress();
    });

    // Control buttons
    playBtn.addEventListener('click', async () => {
        try {
            await player.play();
        } catch (error) {
            console.error('Playback failed:', error);
            setStatus('⚠ Playback failed', 'stopped');
        }
    });

    pauseBtn.addEventListener('click', () => {
        player.pause();
    });

    stopBtn.addEventListener('click', () => {
        player.pause();
        player.currentTime = 0;
        setStatus('⏹ Stopped', 'stopped');
        updateProgress();
    });

    // Initialize UI
    updateProgress();
    setStatus('Ready to play', '');
});
