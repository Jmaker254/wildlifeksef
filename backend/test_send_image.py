"""
============================================================
WILLARY ROBOTICS — Test: Simulate ESP32-CAM image send
============================================================
Use this to test the backend WITHOUT the ESP32-CAM.
Point it at any image file on your laptop.

Usage:
  python test_send_image.py path/to/animal_image.jpg
  python test_send_image.py              (uses webcam)
============================================================
"""

import requests
import sys
import os
import cv2

SERVER_URL = "http://localhost:5000/detect"

def send_image_file(path: str):
    """Send an image file to the backend."""
    if not os.path.exists(path):
        print(f"File not found: {path}")
        return
    with open(path, "rb") as f:
        data = f.read()
    print(f"Sending image: {path} ({len(data)} bytes)")
    response = requests.post(SERVER_URL, data=data, headers={"Content-Type": "image/jpeg"})
    print(f"Response ({response.status_code}):")
    print(response.json())

def capture_from_webcam():
    """Capture one frame from webcam and send it."""
    cap = cv2.VideoCapture(0)
    if not cap.isOpened():
        print("No webcam found")
        return
    print("Capturing from webcam...")
    ret, frame = cap.read()
    cap.release()
    if not ret:
        print("Capture failed")
        return
    _, encoded = cv2.imencode(".jpg", frame, [cv2.IMWRITE_JPEG_QUALITY, 85])
    data = encoded.tobytes()
    print(f"Webcam image captured: {len(data)} bytes")
    response = requests.post(SERVER_URL, data=data, headers={"Content-Type": "image/jpeg"})
    print(f"Response ({response.status_code}):")
    import json
    print(json.dumps(response.json(), indent=2))

if __name__ == "__main__":
    if len(sys.argv) > 1:
        send_image_file(sys.argv[1])
    else:
        capture_from_webcam()
