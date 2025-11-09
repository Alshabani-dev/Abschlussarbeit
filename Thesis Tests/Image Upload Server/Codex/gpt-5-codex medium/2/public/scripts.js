document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("upload-form");
    const messageBox = document.getElementById("message");

    const showMessage = (text, type) => {
        messageBox.textContent = text;
        messageBox.className = `message ${type}`;
    };

    form.addEventListener("submit", async (event) => {
        event.preventDefault();

        const formData = new FormData(form);

        try {
            const response = await fetch("/", {
                method: "POST",
                body: formData,
            });

            if (response.ok) {
                showMessage("Upload successful!", "message success");
                form.reset();
            } else {
                const text = await response.text();
                showMessage(`Upload failed: ${text}`, "message error");
            }
        } catch (error) {
            showMessage(`Network error: ${error}`, "message error");
        }
    });
});
