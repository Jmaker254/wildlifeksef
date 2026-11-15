"""
============================================================
WILLARY ROBOTICS — Wildlife Detector Backend
Phase 1: Image Reception + GPT-4o Vision Detection
============================================================
Receives JPEG from ESP32-CAM via HTTP POST,
sends to GPT-4o Vision API, returns detection result.
Run on your laptop on the same WiFi as the ESP32-CAM.
============================================================
"""

from flask import Flask, request, jsonify
import openai
import base64
import os
import json
import datetime
import logging

# ── Setup ────────────────────────────────────────────────────
app = Flask(__name__)
logging.basicConfig(level=logging.INFO, format="%(asctime)s [%(levelname)s] %(message)s")
log = logging.getLogger(__name__)

# ── OpenAI API Key ────────────────────────────────────────────
# Set this as an environment variable:
#   Windows:  set OPENAI_API_KEY=sk-...
#   Linux:    export OPENAI_API_KEY=sk-...
OPENAI_API_KEY = os.environ.get("OPENAI_API_KEY", "")
client = openai.OpenAI(api_key=OPENAI_API_KEY)

# ── Target Animals (for this project) ────────────────────────
TARGET_ANIMALS = ["elephant", "lion", "leopard", "hyena", "buffalo"]
SAFE_CLASSES   = ["human", "person", "dog", "cow", "livestock", "goat"]

# ── Log directory ─────────────────────────────────────────────
LOG_DIR = "detection_logs"
IMG_DIR = "captured_images"
os.makedirs(LOG_DIR, exist_ok=True)
os.makedirs(IMG_DIR, exist_ok=True)

# ─────────────────────────────────────────────────────────────
@app.route("/detect", methods=["POST"])
def detect():
    """
    Receives JPEG image from ESP32-CAM.
    Sends to GPT-4o Vision for animal detection.
    Returns JSON with species, threat level, and action.
    """
    if not request.data:
        return jsonify({"error": "No image data received"}), 400

    image_bytes = request.data
    log.info(f"Image received: {len(image_bytes)} bytes")

    # Save the captured image with timestamp
    timestamp = datetime.datetime.now().strftime("%Y%m%d_%H%M%S")
    img_path = os.path.join(IMG_DIR, f"capture_{timestamp}.jpg")
    with open(img_path, "wb") as f:
        f.write(image_bytes)
    log.info(f"Image saved: {img_path}")

    # Run GPT-4o Vision detection
    result = detect_with_gpt4o(image_bytes, timestamp)

    # Log the result
    log_detection(result, timestamp)

    # Print clean result to console
    print_result(result)

    return jsonify(result)


# ─────────────────────────────────────────────────────────────
def strip_code_fence(text: str) -> str:
    """Remove a leading/trailing ```json ... ``` markdown fence if present."""
    t = text.strip()
    if t.startswith("```"):
        t = t.split("\n", 1)[1] if "\n" in t else t[3:]
        if t.rstrip().endswith("```"):
            t = t.rstrip()[:-3]
    return t.strip()


# ─────────────────────────────────────────────────────────────
def detect_with_gpt4o(image_bytes: bytes, timestamp: str) -> dict:
    """
    Sends image to GPT-4o Vision API.
    Returns structured detection result.
    """
    if not OPENAI_API_KEY:
        log.error("OPENAI_API_KEY not set!")
        return {"error": "API key not configured", "timestamp": timestamp}

    # Encode image to base64
    image_b64 = base64.b64encode(image_bytes).decode("utf-8")

    # Prompt — designed to work well with toy/model animals
    prompt = """
You are an AI wildlife detection system for a conservation project in Kenya.
Analyse this image carefully.

Your task:
1. Identify if there is an animal in the image — real OR a toy/model/figurine of an animal
2. If yes, identify the species as specifically as possible
3. Classify it as one of: elephant, lion, leopard, hyena, buffalo, human, dog, livestock, other_animal, no_animal
4. Assess if it is a TARGET species (elephant, lion, leopard, hyena, buffalo) or SAFE class

IMPORTANT: Treat toy animals, plastic figurines, and model animals the same as real ones for this demo.

Respond ONLY in this exact JSON format (no extra text):
{
  "animal_detected": true or false,
  "species": "species name or none",
  "classification": "elephant|lion|leopard|hyena|buffalo|human|dog|livestock|other_animal|no_animal",
  "is_threat": true or false,
  "confidence": "high|medium|low",
  "action": "PLAY_DETERRENT|SEND_ALERT|NO_ACTION",
  "deterrent_sound": "bee_swarm|predator_call|alarm_tone|none",
  "description": "brief one-line description of what you see"
}
"""

    try:
        log.info("Calling GPT-4o Vision API...")
        response = client.chat.completions.create(
            model="gpt-4o",
            messages=[
                {
                    "role": "user",
                    "content": [
                        {"type": "text", "text": prompt},
                        {
                            "type": "image_url",
                            "image_url": {
                                "url": f"data:image/jpeg;base64,{image_b64}",
                                "detail": "high"
                            }
                        }
                    ]
                }
            ],
            max_tokens=300,
            response_format={"type": "json_object"},
        )

        raw = response.choices[0].message.content.strip()
        log.info(f"GPT-4o raw response: {raw}")

        # Parse JSON response (strip ```json fences if the model adds them)
        result = json.loads(strip_code_fence(raw))
        result["timestamp"] = timestamp
        result["model"] = "gpt-4o-vision"
        return result

    except json.JSONDecodeError as e:
        log.error(f"JSON parse error: {e} — Raw: {raw}")
        return {
            "error": "Failed to parse AI response",
            "raw_response": raw,
            "timestamp": timestamp
        }
    except Exception as e:
        log.error(f"GPT-4o API error: {e}")
        return {"error": str(e), "timestamp": timestamp}


# ─────────────────────────────────────────────────────────────
def log_detection(result: dict, timestamp: str):
    """Appends detection result to daily log file."""
    log_path = os.path.join(LOG_DIR, f"detections_{timestamp[:8]}.jsonl")
    with open(log_path, "a") as f:
        f.write(json.dumps(result) + "\n")


# ─────────────────────────────────────────────────────────────
def print_result(result: dict):
    """Prints a clean, readable detection summary to console."""
    print("\n" + "="*50)
    print("  WILLARY WILDLIFE DETECTOR — RESULT")
    print("="*50)
    if "error" in result:
        print(f"  ERROR: {result['error']}")
    else:
        detected   = result.get("animal_detected", False)
        species    = result.get("species", "none").upper()
        threat     = result.get("is_threat", False)
        action     = result.get("action", "NO_ACTION")
        confidence = result.get("confidence", "low")
        desc       = result.get("description", "")
        sound      = result.get("deterrent_sound", "none")

        print(f"  Animal Detected : {'YES' if detected else 'NO'}")
        print(f"  Species         : {species}")
        print(f"  Threat          : {'YES - TARGET SPECIES' if threat else 'NO - SAFE CLASS'}")
        print(f"  Confidence      : {confidence.upper()}")
        print(f"  Action          : {action}")
        if sound != "none":
            print(f"  Deterrent Sound : {sound}")
        print(f"  Description     : {desc}")
    print("="*50 + "\n")


# ─────────────────────────────────────────────────────────────
@app.route("/status", methods=["GET"])
def status():
    """Health check endpoint."""
    return jsonify({
        "status": "running",
        "api_key_set": bool(OPENAI_API_KEY),
        "model": "gpt-4o-vision",
        "project": "Willary Wildlife Detector"
    })


@app.route("/logs", methods=["GET"])
def get_logs():
    """Returns today's detection log."""
    today = datetime.datetime.now().strftime("%Y%m%d")
    log_path = os.path.join(LOG_DIR, f"detections_{today}.jsonl")
    if not os.path.exists(log_path):
        return jsonify({"detections": [], "count": 0})
    with open(log_path) as f:
        lines = [json.loads(l) for l in f if l.strip()]
    return jsonify({"detections": lines, "count": len(lines)})


# ─────────────────────────────────────────────────────────────
if __name__ == "__main__":
    print("\n" + "="*50)
    print("  WILLARY ROBOTICS — Wildlife Detector Backend")
    print("="*50)
    if not OPENAI_API_KEY:
        print("  WARNING: OPENAI_API_KEY not set!")
        print("  Run: set OPENAI_API_KEY=sk-your-key-here")
    else:
        print(f"  API Key: ...{OPENAI_API_KEY[-6:]}")
    print(f"  Server : http://0.0.0.0:5000")
    print(f"  Detect : POST http://YOUR_LAPTOP_IP:5000/detect")
    print(f"  Status : GET  http://YOUR_LAPTOP_IP:5000/status")
    print(f"  Logs   : GET  http://YOUR_LAPTOP_IP:5000/logs")
    print("="*50 + "\n")

    app.run(host="0.0.0.0", port=5000, debug=False)
