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
        const progress = (currentTime / duration) * 100;
        progressBar.value = progress;
        progressBar.style.setProperty('--progress', `${progress}%`);

        // Update time display
        currentTimeEl.textContent = formatTime(currentTime);
        durationEl.textContent = formatTime(duration);
    }

    // Update status with color class
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

    player.addEventListener('ended', () => {
        setStatus('Playback finished', 'stopped');
        progressBar.value = 0;
        progressBar.style.setProperty('--progress', '0%');
        currentTimeEl.textContent = '0:00';
    });

    player.addEventListener('error', () => {
        setStatus('Error loading audio', 'stopped');
    });

    // Progress bar events
    progressBar.addEventListener('input', () => {
        isSeeking = true;
        const seekTime = (progressBar.value / 100) * player.duration;
        currentTimeEl.textContent = formatTime(seekTime);
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

    // Control buttons
    playBtn.addEventListener('click', async () => {
        try {
            await player.play();
            setStatus('Playing 🎵', 'playing');
        } catch (error) {
            setStatus('Playback error', 'stopped');
            console.error('Playback error:', error);
        }
    });

    pauseBtn.addEventListener('click', () => {
        player.pause();
        setStatus('Paused ⏸', 'paused');
    });

    stopBtn.addEventListener('click', () => {
        player.pause();
        player.currentTime = 0;
        setStatus('Stopped ⏹', 'stopped');
        progressBar.value = 0;
        progressBar.style.setProperty('--progress', '0%');
        currentTimeEl.textContent = '0:00';
    });

    // Initialize
    setStatus('Ready to play', '');
});
