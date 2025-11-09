document.addEventListener('DOMContentLoaded', () => {
    const player = document.getElementById('player');
    const progressBar = document.getElementById('progressBar');
    const currentTime = document.getElementById('currentTime');
    const duration = document.getElementById('duration');
    const playBtn = document.getElementById('play');
    const pauseBtn = document.getElementById('pause');
    const stopBtn = document.getElementById('stop');
    const status = document.getElementById('status');

    let isSeeking = false;

    // Format time from seconds to M:SS
    function formatTime(seconds) {
        if (isNaN(seconds) || !isFinite(seconds)) return "0:00";

        const minutes = Math.floor(seconds / 60);
        const remainingSeconds = Math.floor(seconds % 60);
        return `${minutes}:${remainingSeconds < 10 ? '0' : ''}${remainingSeconds}`;
    }

    // Update status with color class
    function setStatus(text, className) {
        status.textContent = text;
        status.className = 'status ' + className;
    }

    // Update progress bar and time display
    function updateProgress() {
        if (isSeeking) return;

        const current = player.currentTime;
        const total = player.duration;

        // Update time display
        currentTime.textContent = formatTime(current);
        duration.textContent = formatTime(total);

        // Update progress bar
        if (total > 0) {
            const progress = (current / total) * 100;
            progressBar.value = progress;
            progressBar.style.setProperty('--progress', `${progress}%`);
        }
    }

    // Event listeners
    player.addEventListener('loadedmetadata', () => {
        updateProgress();
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
    });

    player.addEventListener('error', () => {
        setStatus('⚠ Error loading audio', 'stopped');
    });

    // Progress bar interactions
    progressBar.addEventListener('input', () => {
        isSeeking = true;
        const seekTime = (progressBar.value / 100) * player.duration;
        player.currentTime = seekTime;
    });

    progressBar.addEventListener('change', () => {
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
        } catch (error) {
            setStatus('⚠ Playback failed', 'stopped');
            console.error('Playback error:', error);
        }
    });

    pauseBtn.addEventListener('click', () => {
        player.pause();
    });

    stopBtn.addEventListener('click', () => {
        player.pause();
        player.currentTime = 0;
        setStatus('⏹ Stopped', 'stopped');
        progressBar.value = 0;
    });

    // Initialize UI
    setStatus('Ready to play', '');
});
