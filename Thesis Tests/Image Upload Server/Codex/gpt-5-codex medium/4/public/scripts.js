document.addEventListener("DOMContentLoaded", () => {
  const form = document.getElementById("uploadForm");
  const message = document.getElementById("message");
  const fileInput = document.getElementById("fileInput");

  function setMessage(text, type) {
    message.textContent = text;
    message.className = type;
  }

  form.addEventListener("submit", async (event) => {
    event.preventDefault();
    if (!fileInput.files.length) {
      setMessage("Please choose an image before uploading.", "error");
      return;
    }

    const formData = new FormData(form);
    setMessage("Uploading…", "info");

    try {
      const response = await fetch("/", {
        method: "POST",
        body: formData,
      });
      const text = await response.text();
      if (response.ok) {
        setMessage("Upload successful!", "success");
      } else {
        setMessage("Upload failed: " + text.replace(/<[^>]+>/g, "").trim(), "error");
      }
    } catch (error) {
      setMessage("Network error: " + error.message, "error");
    }
  });
});
