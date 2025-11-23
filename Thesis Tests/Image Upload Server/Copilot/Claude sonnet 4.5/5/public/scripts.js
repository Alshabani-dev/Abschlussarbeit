document.addEventListener('DOMContentLoaded', function() {
    const uploadForm = document.getElementById('uploadForm');
    const fileInput = document.getElementById('fileInput');
    const fileName = document.getElementById('fileName');
    const previewContainer = document.getElementById('preview-container');
    const imagePreview = document.getElementById('imagePreview');
    
    // Update file name display when file is selected
    fileInput.addEventListener('change', function() {
        if (fileInput.files.length > 0) {
            const file = fileInput.files[0];
            fileName.textContent = file.name;
            
            // Show preview
            const reader = new FileReader();
            reader.onload = function(e) {
                imagePreview.src = e.target.result;
                previewContainer.classList.remove('hidden');
            };
            reader.readAsDataURL(file);
        } else {
            fileName.textContent = 'Choose an image file...';
            previewContainer.classList.add('hidden');
        }
    });
    
    // Handle form submission
    uploadForm.addEventListener('submit', function(e) {
        e.preventDefault();
        
        if (fileInput.files.length === 0) {
            alert('Please select a file to upload.');
            return;
        }
        
        const file = fileInput.files[0];
        const formData = new FormData();
        formData.append('file', file);
        
        // Disable submit button during upload
        const uploadBtn = uploadForm.querySelector('.upload-btn');
        const originalText = uploadBtn.textContent;
        uploadBtn.textContent = 'Uploading...';
        uploadBtn.disabled = true;
        
        // Send upload request
        fetch('/', {
            method: 'POST',
            body: formData
        })
        .then(response => {
            if (response.ok) {
                alert('✓ Upload successful! File saved to Data/ directory.');
                // Reset form
                uploadForm.reset();
                fileName.textContent = 'Choose an image file...';
                previewContainer.classList.add('hidden');
            } else {
                return response.text().then(text => {
                    throw new Error(text || 'Upload failed');
                });
            }
        })
        .catch(error => {
            alert('✗ Upload failed: ' + error.message);
        })
        .finally(() => {
            // Re-enable submit button
            uploadBtn.textContent = originalText;
            uploadBtn.disabled = false;
        });
    });
});
