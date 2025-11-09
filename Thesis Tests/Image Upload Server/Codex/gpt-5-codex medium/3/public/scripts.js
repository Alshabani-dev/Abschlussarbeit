document.addEventListener("DOMContentLoaded", () => {
    const form = document.getElementById("uploadForm");
    const fileInput = document.getElementById("fileInput");

    if (!form || !fileInput) {
        return;
    }

    form.addEventListener("submit", async (event) => {
        event.preventDefault();

        if (!fileInput.files || fileInput.files.length === 0) {
            window.alert("Choose an image before uploading.");
            return;
        }

        const formData = new FormData(form);

        try {
            const response = await fetch("/", {
                method: "POST",
                body: formData
            });

            let payload = null;
            try {
                payload = await response.json();
            } catch {
                payload = null;
            }

            if (response.ok && payload && payload.success) {
                window.alert(payload.message || "Upload successful.");
                form.reset();
            } else {
                const message = payload && typeof payload.message === "string"
                    ? payload.message
                    : "Upload failed. Please try again.";
                window.alert(message);
            }
        } catch (error) {
            window.alert(`Network error: ${error instanceof Error ? error.message : "Unexpected failure."}`);
        }
    });
});
