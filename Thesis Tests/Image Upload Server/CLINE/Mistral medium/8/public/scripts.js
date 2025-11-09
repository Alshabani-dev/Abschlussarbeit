document.getElementById('uploadForm').addEventListener('submit', async function(e) {
    e.preventDefault();

    const fileInput = document.getElementById('fileInput');
    const messageDiv = document.getElementById('message');

    if (!fileInput.files.length) {
        messageDiv.textContent = 'Please select a file to upload.';
        messageDiv.className = 'error';
        return;
    }

    const file = fileInput.files[0];
    const formData = new FormData();
    formData.append('file', file);

    try {
        const response = await fetch('/', {
            method: 'POST',
            body: formData
        });

        if (response.ok) {
            messageDiv.textContent = 'File uploaded successfully!';
            messageDiv.className = 'success';
        } else {
            messageDiv.textContent = 'Upload failed. Please try again.';
            messageDiv.className = 'error';
        }
    } catch (error) {
        messageDiv.textContent = 'An error occurred during upload.';
        messageDiv.className = 'error';
        console.error('Upload error:', error);
    }
});
