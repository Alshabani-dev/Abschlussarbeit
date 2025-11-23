// File input handling
const fileInput = document.getElementById('file');
const fileLabel = document.getElementById('fileLabel');
const uploadForm = document.getElementById('uploadForm');
const preview = document.getElementById('preview');

// Update label when file is selected
fileInput.addEventListener('change', function() {
    if (this.files && this.files[0]) {
        const fileName = this.files[0].name;
        fileLabel.textContent = fileName;
        
        // Show preview for images
        const reader = new FileReader();
        reader.onload = function(e) {
            preview.innerHTML = `<img src="${e.target.result}" alt="Preview">`;
        };
        reader.readAsDataURL(this.files[0]);
    } else {
        fileLabel.textContent = 'Choose an image file';
        preview.innerHTML = '';
    }
});

// Form submission
uploadForm.addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const file = fileInput.files[0];
    if (!file) {
        alert('⚠️ Please select a file to upload');
        return;
    }
    
    // Client-side validation
    const validExtensions = ['jpg', 'jpeg', 'png', 'gif', 'bmp'];
    const fileName = file.name.toLowerCase();
    const fileExtension = fileName.split('.').pop();
    
    if (!validExtensions.includes(fileExtension)) {
        alert('❌ Invalid file type!\n\nOnly JPG, JPEG, PNG, GIF, and BMP files are allowed.');
        return;
    }
    
    // Prepare form data
    const formData = new FormData();
    formData.append('file', file);
    
    // Disable button during upload
    const uploadBtn = this.querySelector('.upload-btn');
    const originalText = uploadBtn.textContent;
    uploadBtn.disabled = true;
    uploadBtn.textContent = 'Uploading...';
    
    try {
        const response = await fetch('/', {
            method: 'POST',
            body: formData
        });
        
        if (response.ok) {
            alert('✅ Success!\n\nYour image has been uploaded successfully.');
            // Reset form and preview
            uploadForm.reset();
            fileLabel.textContent = 'Choose an image file';
            preview.innerHTML = '';
        } else {
            const errorText = await response.text();
            alert(`❌ Upload Failed!\n\n${errorText}`);
        }
    } catch (error) {
        alert('❌ Network Error!\n\nCould not connect to the server. Please try again.');
        console.error('Upload error:', error);
    } finally {
        // Re-enable button
        uploadBtn.disabled = false;
        uploadBtn.textContent = originalText;
    }
});

// Drag and drop support
const fileWrapper = document.querySelector('.file-label');

fileWrapper.addEventListener('dragover', function(e) {
    e.preventDefault();
    this.style.borderColor = '#764ba2';
    this.style.background = '#e8ebff';
});

fileWrapper.addEventListener('dragleave', function(e) {
    e.preventDefault();
    this.style.borderColor = '#667eea';
    this.style.background = '#f8f9ff';
});

fileWrapper.addEventListener('drop', function(e) {
    e.preventDefault();
    this.style.borderColor = '#667eea';
    this.style.background = '#f8f9ff';
    
    const files = e.dataTransfer.files;
    if (files.length > 0) {
        fileInput.files = files;
        // Trigger change event
        const event = new Event('change', { bubbles: true });
        fileInput.dispatchEvent(event);
    }
});
