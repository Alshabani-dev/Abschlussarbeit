document.addEventListener('DOMContentLoaded', function() {
    const uploadForm = document.getElementById('uploadForm');
    const fileInput = document.getElementById('fileInput');
    const fileLabel = document.querySelector('.file-label');
    const fileLabelText = document.getElementById('fileLabel');
    const uploadBtn = document.querySelector('.upload-btn');
    const messageDiv = document.getElementById('message');

    // Update label when file is selected
    fileInput.addEventListener('change', function(e) {
        if (e.target.files && e.target.files.length > 0) {
            const fileName = e.target.files[0].name;
            fileLabelText.textContent = fileName;
            fileLabel.classList.add('file-selected');
        } else {
            fileLabelText.textContent = 'Choose Image File';
            fileLabel.classList.remove('file-selected');
        }
    });

    // Handle form submission
    uploadForm.addEventListener('submit', async function(e) {
        e.preventDefault();

        // Hide previous messages
        messageDiv.classList.add('hidden');
        messageDiv.classList.remove('success', 'error');

        const file = fileInput.files[0];
        if (!file) {
            showMessage('Please select a file', 'error');
            return;
        }

        // Validate file type
        const validExtensions = ['jpg', 'jpeg', 'png', 'gif', 'bmp'];
        const fileExtension = file.name.split('.').pop().toLowerCase();
        
        if (!validExtensions.includes(fileExtension)) {
            showMessage('Invalid file type. Please upload JPG, PNG, GIF, or BMP files only.', 'error');
            return;
        }

        // Show loading state
        uploadBtn.disabled = true;
        uploadBtn.classList.add('loading');
        const originalText = uploadBtn.innerHTML;
        uploadBtn.innerHTML = '<span>Uploading...</span>';

        try {
            const formData = new FormData();
            formData.append('file', file);

            const response = await fetch('/', {
                method: 'POST',
                body: formData
            });

            const responseText = await response.text();

            if (response.ok && response.status === 200) {
                showMessage('✅ File uploaded successfully!', 'success');
                alert('✅ File uploaded successfully!');
                
                // Reset form
                uploadForm.reset();
                fileLabelText.textContent = 'Choose Image File';
                fileLabel.classList.remove('file-selected');
            } else {
                showMessage('❌ Upload failed: ' + responseText, 'error');
                alert('❌ Upload failed: ' + responseText);
            }
        } catch (error) {
            showMessage('❌ Network error: ' + error.message, 'error');
            alert('❌ Network error: ' + error.message);
        } finally {
            // Restore button state
            uploadBtn.disabled = false;
            uploadBtn.classList.remove('loading');
            uploadBtn.innerHTML = originalText;
        }
    });

    function showMessage(text, type) {
        messageDiv.textContent = text;
        messageDiv.classList.remove('hidden');
        messageDiv.classList.add(type);

        // Auto-hide after 5 seconds
        setTimeout(() => {
            messageDiv.classList.add('hidden');
        }, 5000);
    }
});
