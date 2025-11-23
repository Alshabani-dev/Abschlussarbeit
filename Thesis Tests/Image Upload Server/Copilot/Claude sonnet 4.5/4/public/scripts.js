// File input change event
document.getElementById('fileInput').addEventListener('change', function(e) {
    const fileName = e.target.files[0]?.name || 'Choose an image file';
    document.getElementById('fileName').textContent = fileName;
    
    // Show preview
    if (e.target.files && e.target.files[0]) {
        const reader = new FileReader();
        reader.onload = function(event) {
            const preview = document.getElementById('preview');
            const previewImage = document.getElementById('previewImage');
            previewImage.src = event.target.result;
            preview.style.display = 'block';
        };
        reader.readAsDataURL(e.target.files[0]);
    }
});

// Form submission
document.getElementById('uploadForm').addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const fileInput = document.getElementById('fileInput');
    const uploadBtn = document.querySelector('.upload-btn');
    
    if (!fileInput.files || !fileInput.files[0]) {
        alert('Please select a file to upload.');
        return;
    }
    
    // Disable button during upload
    uploadBtn.disabled = true;
    uploadBtn.textContent = 'Uploading...';
    
    // Create FormData
    const formData = new FormData();
    formData.append('file', fileInput.files[0]);
    
    try {
        // Send POST request
        const response = await fetch('/', {
            method: 'POST',
            body: formData
        });
        
        const responseText = await response.text();
        
        if (response.ok) {
            // Success
            alert('✓ Success!\n\n' + responseText);
            
            // Reset form
            fileInput.value = '';
            document.getElementById('fileName').textContent = 'Choose an image file';
            document.getElementById('preview').style.display = 'none';
        } else {
            // Error
            alert('✗ Upload Failed\n\n' + responseText);
        }
    } catch (error) {
        alert('✗ Network Error\n\nFailed to connect to server: ' + error.message);
    } finally {
        // Re-enable button
        uploadBtn.disabled = false;
        uploadBtn.textContent = 'Upload Image';
    }
});
