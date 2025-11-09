document.addEventListener("DOMContentLoaded", () => {
  const form = document.getElementById("uploadForm");
  const statusEl = document.getElementById("status");

  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    statusEl.textContent = "Uploading…";

    try {
      const response = await fetch("/", {
        method: "POST",
        body: new FormData(form),
      });

      const text = await response.text();
      if (response.ok) {
        statusEl.textContent = text || "Upload successful!";
      } else {
        statusEl.textContent = text || `Upload failed (${response.status})`;
      }
    } catch (error) {
      statusEl.textContent = `Error: ${error.message}`;
    }
  });
});
