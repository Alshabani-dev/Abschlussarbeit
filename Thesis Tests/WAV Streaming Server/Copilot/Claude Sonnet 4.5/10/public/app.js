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
        if (isNaN(seconds) || !isFinite(seconds)) return "0:00";

        const minutes = Math.floor(seconds / 60);
        const secs = Math.floor(seconds % 60);
        return `${minutes}:${secs < 10 ? '0' : ''}${secs}`;
    }

    // Update progress bar and time display
    function updateProgress() {
        if (isSeeking) return;

        const currentTime = player.currentTime;
        const duration = player.duration;

        // Update progress bar
        const progress = (currentTime / duration) * 100 || 0;
        progressBar.value = progress;
        progressBar.style.setProperty('--progress', `${progress}%`);

        // Update time display
        currentTimeEl.textContent = formatTime(currentTime);
        durationEl.textContent = formatTime(duration);
    }

    // Set status with appropriate class
    function setStatus(text, className) {
        statusEl.textContent = text;
        statusEl.className = `status ${className}`;
    }

    // Event listeners
    player.addEventListener('loadedmetadata', () => {
        durationEl.textContent = formatTime(player.duration);
        setStatus('🎵 Ready to play', 'playing');
    });

    player.addEventListener('timeupdate', updateProgress);

    player.addEventListener('ended', () => {
        setStatus('🎵 Playback finished', 'stopped');
        progressBar.value = 100;
    });

    player.addEventListener('error', () => {
        setStatus('⚠️ Error loading audio', 'stopped');
    });

    player.addEventListener('play', () => {
        setStatus('🎵 Playing', 'playing');
    });

    player.addEventListener('pause', () => {
        setStatus('⏸ Paused', 'paused');
    });

    // Progress bar events
    progressBar.addEventListener('input', () => {
        isSeeking = true;
        const progress = progressBar.value;
        progressBar.style.setProperty('--progress', `${progress}%`);
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
        updateProgress();
    });

    // Button events
    playBtn.addEventListener('click', async () => {
        try {
            await player.play();
        } catch (err) {
            console.error('Playback failed:', err);
            setStatus('⚠️ Playback error', 'stopped');
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

    // Initialize
    updateProgress();
});
