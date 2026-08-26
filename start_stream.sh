#!/bin/bash

if [ -z "$1" ]; then
    echo "Usage: $0 <path_to_video_file>"
    exit 1
fi

VIDEO_FILE="$1"

if [ ! -f "$VIDEO_FILE" ]; then
    echo "Error: File '$VIDEO_FILE' not found!"
    exit 1
fi

# Read framerate using ffprobe
FRAMERATE=$(ffprobe -v error -select_streams v:0 -show_entries stream=r_frame_rate -of default=noprint_wrappers=1:nokey=1 "$VIDEO_FILE")

if [ -z "$FRAMERATE" ]; then
    echo "Could not detect framerate. Defaulting to 30."
    FRAMERATE="30"
fi

echo "Detected framerate: $FRAMERATE"
echo "Configuring and starting stream using native framerate..."

# Stop any existing background ffmpeg stream from previous runs
if [ -f extern/mediamtx/ffmpeg_stream.pid ]; then
    echo "Stopping existing background stream..."
    kill $(cat extern/mediamtx/ffmpeg_stream.pid) 2>/dev/null
    rm -f extern/mediamtx/ffmpeg_stream.pid
fi

# Handle CTRL-C (SIGINT) to cleanly exit
trap "echo -e '\nStopping stream...'; exit 0" SIGINT SIGTERM

echo "Starting stream in foreground. Press CTRL-C to stop."

# We use -re to ensure ffmpeg reads the file at the native framerate.
# -c:v copy is used to prevent CPU-intensive transcoding.
# Run in the foreground so it immediately works and can be killed with CTRL-C
ffmpeg -re -stream_loop -1 -i "$VIDEO_FILE" -c:v copy -c:a copy -f rtsp rtsp://localhost:8554/stream
