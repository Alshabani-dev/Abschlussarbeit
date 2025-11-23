// Display selected file name
document.getElementById('file').addEventListener('change', function(e) {
    const fileName = this.files[0] ? this.files[0].name : '';
    const fileNameDisplay = document.getElementById('fileName');
    
    if (fileName) {
        fileNameDisplay.textContent = '📎 Selected: ' + fileName;
    } else {
        fileNameDisplay.textContent = '';
    }
});

// Form submission handling
document.getElementById('uploadForm').addEventListener('submit', function(e) {
    const fileInput = document.getElementById('file');
    
    if (!fileInput.files || !fileInput.files[0]) {
        e.preventDefault();
        alert('Please select a file to upload');
        return false;
    }
    
    // Check file type
    const file = fileInput.files[0];
    const validExtensions = ['jpg', 'jpeg', 'png', 'gif', 'bmp'];
    const fileExtension = file.name.split('.').pop().toLowerCase();
    
    if (!validExtensions.includes(fileExtension)) {
        e.preventDefault();
        alert('Invalid file type! Please upload JPG, JPEG, PNG, GIF, or BMP images only.');
        return false;
    }
    
    // Show loading state (optional)
    const submitButton = this.querySelector('button[type="submit"]');
    submitButton.textContent = '⏳ Uploading...';
    submitButton.disabled = true;
});
