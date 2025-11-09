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

        // Update time displays
        currentTimeDisplay.textContent = formatTime(currentTime);
        durationDisplay.textContent = formatTime(duration);

        // Update progress bar
        if (duration > 0) {
            const progress = (currentTime / duration) * 100;
            progressBar.value = progress;
            progressBar.style.setProperty('--progress', `${progress}%`);
        }
    }

    // Event listeners
    player.addEventListener('loadedmetadata', () => {
        durationDisplay.textContent = formatTime(player.duration);
        setStatus('Ready to play', '');
    });

    player.addEventListener('timeupdate', updateProgress);

    player.addEventListener('ended', () => {
        setStatus('Playback finished', 'stopped');
        progressBar.value = 100;
    });

    player.addEventListener('error', () => {
        setStatus('Error loading audio', 'stopped');
    });

    player.addEventListener('seeking', () => {
        isSeeking = true;
    });

    player.addEventListener('seeked', () => {
        isSeeking = false;
        updateProgress();
    });

    // Progress bar events
    progressBar.addEventListener('input', () => {
        const seekTime = (progressBar.value / 100) * player.duration;
        currentTimeDisplay.textContent = formatTime(seekTime);
    });

    progressBar.addEventListener('change', () => {
        const seekTime = (progressBar.value / 100) * player.duration;
        player.currentTime = seekTime;
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
            setStatus('Playing...', 'playing');
        } catch (error) {
            setStatus('Playback error', 'stopped');
        }
    });

    pauseBtn.addEventListener('click', () => {
        player.pause();
        setStatus('Paused', 'paused');
    });

    stopBtn.addEventListener('click', () => {
        player.pause();
        player.currentTime = 0;
        setStatus('Stopped', 'stopped');
        progressBar.value = 0;
        currentTimeDisplay.textContent = '0:00';
    });

    // Initialize UI
    setStatus('Ready to play', '');
});
