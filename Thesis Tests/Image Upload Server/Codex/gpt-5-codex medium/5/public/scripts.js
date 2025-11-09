document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("upload-form");
    const status = document.getElementById("status");

    form.addEventListener("submit", async (event) => {
        event.preventDefault();
        status.textContent = "Uploading…";
        status.className = "";

        const formData = new FormData(form);
        try {
            const response = await fetch("/", {
                method: "POST",
                body: formData
            });

            const text = await response.text();
            if (response.ok) {
                status.textContent = text;
                status.className = "success";
                form.reset();
            } else {
                status.textContent = text || "Upload failed.";
                status.className = "error";
            }
        } catch (error) {
            status.textContent = "Network error occurred.";
            status.className = "error";
            console.error(error);
        }
    });
});
