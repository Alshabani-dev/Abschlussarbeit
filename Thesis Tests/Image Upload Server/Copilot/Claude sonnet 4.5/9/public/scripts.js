document.addEventListener('DOMContentLoaded', function() {
    const uploadForm = document.getElementById('uploadForm');
    const fileInput = document.getElementById('fileInput');
    const fileName = document.getElementById('fileName');
    const statusDiv = document.getElementById('status');
    
    // Update file name display when file is selected
    fileInput.addEventListener('change', function() {
        if (this.files && this.files.length > 0) {
            fileName.textContent = this.files[0].name;
        } else {
            fileName.textContent = 'No file chosen';
        }
    });
    
    // Handle form submission
    uploadForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        if (!fileInput.files || fileInput.files.length === 0) {
            showStatus('Please select a file', 'error');
            return;
        }
        
        const formData = new FormData();
        formData.append('file', fileInput.files[0]);
        
        // Show loading status
        showStatus('Uploading...', 'success');
        
        // Send upload request
        fetch('/', {
            method: 'POST',
            body: formData
        })
        .then(response => response.text())
        .then(data => {
            if (data.startsWith('200')) {
                showStatus('✓ ' + data, 'success');
                // Reset form
                uploadForm.reset();
                fileName.textContent = 'No file chosen';
            } else {
                showStatus('✗ ' + data, 'error');
            }
        })
        .catch(error => {
            showStatus('✗ Upload failed: ' + error.message, 'error');
        });
    });
    
    function showStatus(message, type) {
        statusDiv.textContent = message;
        statusDiv.className = 'status show ' + type;
        
        // Auto-hide after 5 seconds for success messages
        if (type === 'success') {
            setTimeout(() => {
                statusDiv.classList.remove('show');
            }, 5000);
        }
    }
});
