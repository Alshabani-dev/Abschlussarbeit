document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("upload-form");
    const statusEl = document.getElementById("status");

    form.addEventListener("submit", async (event) => {
        event.preventDefault();
        statusEl.textContent = "Uploading...";

        const formData = new FormData(form);
        try {
            const response = await fetch("/", {
                method: "POST",
                body: formData
            });
            const text = await response.text();
            statusEl.textContent = `${response.status} ${response.statusText}\n${text}`;
        } catch (error) {
            statusEl.textContent = `Upload failed: ${error}`;
        }
    });
});
