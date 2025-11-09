// File preview functionality
const fileInput = document.getElementById('fileInput');
const fileName = document.getElementById('fileName');
const preview = document.getElementById('preview');
const previewImage = document.getElementById('previewImage');

fileInput.addEventListener('change', function(e) {
    if (e.target.files.length > 0) {
        const file = e.target.files[0];
        fileName.textContent = file.name;
        
        // Show preview
        const reader = new FileReader();
        reader.onload = function(e) {
            previewImage.src = e.target.result;
            preview.style.display = 'block';
        };
        reader.readAsDataURL(file);
    } else {
        fileName.textContent = 'Choose an image file';
        preview.style.display = 'none';
    }
});

// Form submission with AJAX
const uploadForm = document.getElementById('uploadForm');
const uploadBtn = document.getElementById('uploadBtn');
const btnText = document.getElementById('btnText');
const btnLoader = document.getElementById('btnLoader');

uploadForm.addEventListener('submit', async function(e) {
    e.preventDefault();
    
    const file = fileInput.files[0];
    if (!file) {
        showAlert('Please select a file', 'error');
        return;
    }
    
    // Validate file type on client side
    const validExtensions = ['jpg', 'jpeg', 'png', 'gif', 'bmp'];
    const fileExtension = file.name.split('.').pop().toLowerCase();
    
    if (!validExtensions.includes(fileExtension)) {
        showAlert('Invalid file type. Only JPG, PNG, GIF, BMP allowed.', 'error');
        return;
    }
    
    // Prepare form data
    const formData = new FormData();
    formData.append('file', file);
    
    // Show loading state
    uploadBtn.disabled = true;
    btnText.style.display = 'none';
    btnLoader.style.display = 'block';
    
    try {
        const response = await fetch('/', {
            method: 'POST',
            body: formData
        });
        
        const responseText = await response.text();
        
        if (response.status === 200) {
            showAlert('✅ File uploaded successfully!', 'success');
            
            // Reset form after successful upload
            setTimeout(() => {
                uploadForm.reset();
                fileName.textContent = 'Choose an image file';
                preview.style.display = 'none';
            }, 1500);
        } else {
            showAlert('❌ Upload failed: ' + responseText, 'error');
        }
    } catch (error) {
        showAlert('❌ Network error: ' + error.message, 'error');
    } finally {
        // Hide loading state
        uploadBtn.disabled = false;
        btnText.style.display = 'block';
        btnLoader.style.display = 'none';
    }
});

// Alert notification system
function showAlert(message, type) {
    // Remove existing alerts
    const existingAlerts = document.querySelectorAll('.alert');
    existingAlerts.forEach(alert => alert.remove());
    
    // Create alert element
    const alert = document.createElement('div');
    alert.className = `alert alert-${type}`;
    alert.textContent = message;
    
    // Add to page
    document.body.appendChild(alert);
    
    // Auto remove after 4 seconds
    setTimeout(() => {
        alert.style.animation = 'slideOut 0.3s ease';
        setTimeout(() => {
            alert.remove();
        }, 300);
    }, 4000);
}
