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
        if (isNaN(seconds) || !isFinite(seconds)) {
            return "0:00";
        }

        const minutes = Math.floor(seconds / 60);
        const remainingSeconds = Math.floor(seconds % 60);
        return `${minutes}:${remainingSeconds < 10 ? '0' : ''}${remainingSeconds}`;
    }

    // Update status display with color class
    function setStatus(text, className) {
        statusDisplay.textContent = text;
        statusDisplay.className = 'status ' + className;
    }

    // Update progress bar and time display
    function updateProgress() {
        if (isSeeking) return;

        const currentTime = player.currentTime;
        const duration = player.duration;

        // Update time display
        currentTimeDisplay.textContent = formatTime(currentTime);
        durationDisplay.textContent = formatTime(duration);

        // Update progress bar
        if (duration > 0) {
            const progress = (currentTime / duration) * 100;
            progressBar.value = progress;
            progressBar.style.setProperty('--progress', `${progress}%`);
        }
    }

    // Play button handler
    playBtn.addEventListener('click', async () => {
        try {
            await player.play();
            setStatus('🎵 Playing', 'playing');
        } catch (error) {
            console.error('Playback failed:', error);
            setStatus('⚠ Error playing', 'stopped');
        }
    });

    // Pause button handler
    pauseBtn.addEventListener('click', () => {
        player.pause();
        setStatus('⏸ Paused', 'paused');
    });

    // Stop button handler
    stopBtn.addEventListener('click', () => {
        player.pause();
        player.currentTime = 0;
        setStatus('⏹ Stopped', 'stopped');
    });

    // Progress bar input (dragging)
    progressBar.addEventListener('input', () => {
        isSeeking = true;
        const seekTime = (progressBar.value / 100) * player.duration;
        progressBar.style.setProperty('--progress', `${progressBar.value}%`);
    });

    // Progress bar change (seek completed)
    progressBar.addEventListener('change', () => {
        const seekTime = (progressBar.value / 100) * player.duration;
        player.currentTime = seekTime;
        isSeeking = false;
    });

    // Progress bar mouse events for better UX
    progressBar.addEventListener('mousedown', () => {
        isSeeking = true;
    });

    progressBar.addEventListener('mouseup', () => {
        isSeeking = false;
        const seekTime = (progressBar.value / 100) * player.duration;
        player.currentTime = seekTime;
    });

    // Audio events
    player.addEventListener('loadedmetadata', () => {
        durationDisplay.textContent = formatTime(player.duration);
        setStatus('🎵 Ready to play', 'playing');
    });

    player.addEventListener('timeupdate', updateProgress);

    player.addEventListener('play', () => {
        setStatus('🎵 Playing', 'playing');
    });

    player.addEventListener('pause', () => {
        setStatus('⏸ Paused', 'paused');
    });

    player.addEventListener('ended', () => {
        progressBar.value = 100;
        progressBar.style.setProperty('--progress', '100%');
        setStatus('⏹ Playback finished', 'stopped');
    });

    player.addEventListener('error', () => {
        setStatus('⚠ Error loading audio', 'stopped');
        console.error('Audio error:', player.error);
    });

    player.addEventListener('seeking', () => {
        isSeeking = true;
    });

    player.addEventListener('seeked', () => {
        isSeeking = false;
    });

    // Initialize UI
    setStatus('Ready to play', '');
});
