import cv2
import os
from datetime import datetime

STREAM_URL = "http://10.140.172.57:81/stream"

RECORDING_FOLDER = "BlindCurve_Recordings"
FILE_DURATION = 300       # 5 minutes
FPS = 10

os.makedirs(RECORDING_FOLDER, exist_ok=True)

cap = cv2.VideoCapture(STREAM_URL)

if not cap.isOpened():
    print("ERROR: Cannot connect to ESP32-CAM")
    exit()

print("ESP32-CAM connected!")
print("Continuous recording started.")
print("Press Q to stop.")

width = int(cap.get(cv2.CAP_PROP_FRAME_WIDTH))
height = int(cap.get(cv2.CAP_PROP_FRAME_HEIGHT))

writer = None
recording_start = None

while True:

    ret, frame = cap.read()

    if not ret:
        print("Stream lost. Trying to reconnect...")
        cap.release()
        cap = cv2.VideoCapture(STREAM_URL)
        continue

    # ==========================================
    # ADD DATE AND TIME TO THE VIDEO FRAME
    # ==========================================

    current_time = datetime.now().strftime("%d-%m-%Y  %H:%M:%S")

    cv2.putText(
        frame,
        current_time,
        (10, 30),
        cv2.FONT_HERSHEY_SIMPLEX,
        0.7,
        (255, 255, 255),
        2,
        cv2.LINE_AA
    )

    # ==========================================
    # START NEW 5-MINUTE RECORDING
    # ==========================================

    if writer is None or (
        datetime.now() - recording_start
    ).total_seconds() >= FILE_DURATION:

        if writer is not None:
            writer.release()

        timestamp = datetime.now().strftime("%Y-%m-%d_%H-%M-%S")

        filename = os.path.join(
            RECORDING_FOLDER,
            f"BlindCurve_{timestamp}.mp4"
        )

        fourcc = cv2.VideoWriter_fourcc(*"mp4v")

        writer = cv2.VideoWriter(
            filename,
            fourcc,
            FPS,
            (width, height)
        )

        recording_start = datetime.now()

        print(f"Started: {filename}")

    # Save frame
    writer.write(frame)

    # Show live video
    cv2.imshow(
        "ESP32-CAM - Blind Curve Recording",
        frame
    )

    # Press Q to stop
    if cv2.waitKey(1) & 0xFF == ord("q"):
        break

# ==========================================
# CLEANUP
# ==========================================

if writer is not None:
    writer.release()

cap.release()
cv2.destroyAllWindows()

print("Recording stopped.")
