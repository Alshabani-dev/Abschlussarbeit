const form = document.getElementById("uploadForm");
const statusEl = document.getElementById("status");

function setStatus(message, type) {
    statusEl.textContent = message;
    statusEl.className = type ? type : "";
}

form.addEventListener("submit", async (event) => {
    event.preventDefault();
    setStatus("Uploading…", "");

    const data = new FormData(form);
    if (!data.get("file") || !(data.get("file") instanceof File)) {
        setStatus("Please choose an image first.", "error");
        window.alert("Please choose an image before uploading.");
        return;
    }

    try {
        const response = await fetch("/", {
            method: "POST",
            body: data,
        });

        const text = await response.text();
        let payload = {};
        try {
            payload = JSON.parse(text);
        } catch {
            payload = { message: text };
        }

        if (response.ok) {
            setStatus(payload.message || "Upload successful", "success");
            window.alert(payload.message || "Upload successful!");
            form.reset();
        } else {
            const message = payload.message || `Upload failed (${response.status})`;
            setStatus(message, "error");
            window.alert(message);
        }
    } catch (error) {
        const message = error instanceof Error ? error.message : "Unexpected error";
        setStatus(message, "error");
        window.alert(message);
    }
});
