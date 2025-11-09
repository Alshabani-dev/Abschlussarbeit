document.getElementById("uploadForm").addEventListener("submit", function (event) {
    event.preventDefault();
    const formData = new FormData(event.target);

    fetch("/", {
        method: "POST",
        body: formData
    })
        .then(async response => {
            if (!response.ok) {
                throw new Error(await response.text());
            }
            return response.text();
        })
        .then(text => {
            alert(text || "Upload successful");
        })
        .catch(err => {
            alert(err.message || "Upload failed");
        });
});
