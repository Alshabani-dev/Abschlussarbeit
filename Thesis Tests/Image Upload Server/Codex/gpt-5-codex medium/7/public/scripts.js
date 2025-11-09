document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("upload-form");
    if (!form) {
        return;
    }

    form.addEventListener("submit", async (event) => {
        event.preventDefault();

        const formData = new FormData(form);
        try {
            const response = await fetch("/", {
                method: "POST",
                body: formData
            });
            const text = await response.text();
            if (response.ok) {
                alert("Upload successful: " + text);
            } else {
                alert("Upload failed: " + text);
            }
        } catch (error) {
            alert("Network error: " + error);
        }
    });
});
