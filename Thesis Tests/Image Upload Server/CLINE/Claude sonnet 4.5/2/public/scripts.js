// File input handling
const fileInput = document.getElementById('fileInput');
const fileInfo = document.getElementById('fileInfo');
const fileName = document.getElementById('fileName');
const fileSize = document.getElementById('fileSize');
const uploadForm = document.getElementById('uploadForm');
const uploadBtn = document.querySelector('.upload-btn');

// Update file info when file is selected
fileInput.addEventListener('change', function() {
    if (this.files && this.files[0]) {
        const file = this.files[0];
        
        // Show file info
        fileInfo.classList.remove('hidden');
        fileName.textContent = `📄 ${file.name}`;
        
        // Format file size
        const sizeInMB = (file.size / (1024 * 1024)).toFixed(2);
        const sizeInKB = (file.size / 1024).toFixed(2);
        const displaySize = sizeInMB > 1 ? `${sizeInMB} MB` : `${sizeInKB} KB`;
        fileSize.textContent = `Size: ${displaySize}`;
    } else {
        fileInfo.classList.add('hidden');
    }
});

// Handle form submission
uploadForm.addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const file = fileInput.files[0];
    if (!file) {
        alert('❌ Please select a file first!');
        return;
    }
    
    // Validate file type
    const validExtensions = ['jpg', 'jpeg', 'png', 'gif', 'bmp'];
    const fileExtension = file.name.split('.').pop().toLowerCase();
    
    if (!validExtensions.includes(fileExtension)) {
        alert('❌ Invalid file type! Please upload JPG, PNG, GIF, or BMP images only.');
        return;
    }
    
    // Create FormData
    const formData = new FormData();
    formData.append('file', file);
    
    // Show loading state
    uploadBtn.classList.add('loading');
    uploadBtn.textContent = 'Uploading...';
    
    try {
        // Send the file
        const response = await fetch('/', {
            method: 'POST',
            body: formData
        });
        
        // Reset button state
        uploadBtn.classList.remove('loading');
        uploadBtn.innerHTML = '<span class="btn-icon">⬆️</span>Upload Image';
        
        if (response.ok) {
            // Success
            alert('✅ File uploaded successfully!');
            
            // Reset form
            uploadForm.reset();
            fileInfo.classList.add('hidden');
        } else {
            // Error response from server
            const errorText = await response.text();
            alert(`❌ Upload failed: ${errorText}`);
        }
    } catch (error) {
        // Network or other error
        uploadBtn.classList.remove('loading');
        uploadBtn.innerHTML = '<span class="btn-icon">⬆️</span>Upload Image';
        alert('❌ Upload failed: Network error or server is not responding.');
        console.error('Upload error:', error);
    }
});

// Drag and drop support
const fileLabel = document.querySelector('.file-label');

fileLabel.addEventListener('dragover', function(e) {
    e.preventDefault();
    this.style.borderColor = '#764ba2';
    this.style.background = '#eef0ff';
});

fileLabel.addEventListener('dragleave', function(e) {
    e.preventDefault();
    this.style.borderColor = '#667eea';
    this.style.background = '#f8f9ff';
});

fileLabel.addEventListener('drop', function(e) {
    e.preventDefault();
    this.style.borderColor = '#667eea';
    this.style.background = '#f8f9ff';
    
    if (e.dataTransfer.files && e.dataTransfer.files[0]) {
        fileInput.files = e.dataTransfer.files;
        
        // Trigger change event
        const event = new Event('change', { bubbles: true });
        fileInput.dispatchEvent(event);
    }
});
