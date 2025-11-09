import http.server
import socketserver
import requests
import os

# Test GET request
def test_get():
    try:
        response = requests.get('http://localhost:8080/')
        print(f"GET request status code: {response.status_code}")
        print(f"GET request content type: {response.headers.get('Content-Type')}")
        return response.status_code == 200
    except Exception as e:
        print(f"GET request failed: {e}")
        return False

# Test file upload
def test_upload():
    try:
        # Create a test file
        with open('test.jpg', 'wb') as f:
            f.write(b'TEST IMAGE DATA')

        # Upload the file
        with open('test.jpg', 'rb') as f:
            files = {'file': ('test.jpg', f)}
            response = requests.post('http://localhost:8080/', files=files)

        print(f"Upload status code: {response.status_code}")
        print(f"Upload response: {response.text}")

        # Clean up
        os.remove('test.jpg')

        return response.status_code == 200
    except Exception as e:
        print(f"Upload failed: {e}")
        return False

if __name__ == "__main__":
    print("Testing server functionality...")
    get_success = test_get()
    upload_success = test_upload()

    if get_success and upload_success:
        print("All tests passed!")
    else:
        print("Some tests failed.")
