document.addEventListener('DOMContentLoaded', function() {
    const form = document.getElementById('uploadForm');
    const fileInput = document.getElementById('fileInput');
    const fileName = document.getElementById('fileName');
    const preview = document.getElementById('preview');
    
    // Update file name display when file is selected
    fileInput.addEventListener('change', function() {
        if (this.files && this.files.length > 0) {
            fileName.textContent = this.files[0].name;
            
            // Show image preview
            const file = this.files[0];
            if (file.type.startsWith('image/')) {
                const reader = new FileReader();
                reader.onload = function(e) {
                    preview.innerHTML = `<img src="${e.target.result}" alt="Preview">`;
                };
                reader.readAsDataURL(file);
            }
        } else {
            fileName.textContent = 'Choose an image...';
            preview.innerHTML = '';
        }
    });
    
    // Handle form submission
    form.addEventListener('submit', function(e) {
        e.preventDefault();
        
        const file = fileInput.files[0];
        if (!file) {
            alert('❌ Please select a file to upload.');
            return;
        }
        
        // Validate file type
        const validTypes = ['image/jpeg', 'image/jpg', 'image/png', 'image/gif', 'image/bmp'];
        if (!validTypes.includes(file.type.toLowerCase())) {
            alert('❌ Invalid file type. Please upload a JPG, PNG, GIF, or BMP image.');
            return;
        }
        
        // Create FormData
        const formData = new FormData();
        formData.append('file', file);
        
        // Disable form during upload
        const uploadBtn = form.querySelector('.upload-btn');
        const originalText = uploadBtn.querySelector('span').textContent;
        uploadBtn.querySelector('span').textContent = 'Uploading...';
        uploadBtn.disabled = true;
        uploadBtn.classList.add('loading');
        
        // Send upload request
        fetch('/', {
            method: 'POST',
            body: formData
        })
        .then(response => {
            // Check response status
            if (response.ok) {
                return response.text();
            } else {
                return response.text().then(text => {
                    throw new Error(text || `Upload failed with status ${response.status}`);
                });
            }
        })
        .then(data => {
            // Success
            alert('✅ ' + data);
            
            // Reset form
            form.reset();
            fileName.textContent = 'Choose an image...';
            preview.innerHTML = '';
        })
        .catch(error => {
            // Error
            alert('❌ Upload failed: ' + error.message);
        })
        .finally(() => {
            // Re-enable form
            uploadBtn.querySelector('span').textContent = originalText;
            uploadBtn.disabled = false;
            uploadBtn.classList.remove('loading');
        });
    });
});
