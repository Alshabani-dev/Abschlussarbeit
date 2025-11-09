document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("upload-form");
    const statusBox = document.getElementById("status");

    const showStatus = (message, type) => {
        statusBox.textContent = message;
        statusBox.className = `status ${type}`;
        statusBox.classList.remove("hidden");
    };

    form.addEventListener("submit", async (event) => {
        event.preventDefault();
        const formData = new FormData(form);

        try {
            const response = await fetch("/", {
                method: "POST",
                body: formData,
            });

            const text = await response.text();
            if (response.ok) {
                showStatus(text || "Upload succeeded!", "success");
                form.reset();
            } else {
                showStatus(text || `Upload failed (${response.status})`, "error");
            }
        } catch (error) {
            console.error(error);
            showStatus("Network error while uploading file.", "error");
        }
    });
});
