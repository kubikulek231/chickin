#!/usr/bin/env bash
set -euo pipefail

# ===== User configuration =====
DEVICE="/dev/video3"
INPUT_FORMAT="h264"
WIDTH="1920"
HEIGHT="1080"
FRAMERATE="30"
HLS_DIR="/dev/shm/hls"
PLAYLIST_NAME="stream.m3u8"
SEGMENT_PATTERN="seg_%03d.ts"
HLS_TIME="0.5"
HLS_LIST_SIZE="3"
LOG_LEVEL="warning"
FFMPEG_BIN="ffmpeg"

# Optional extra ffmpeg input args
FFMPEG_INPUT_EXTRA=""
# Optional extra ffmpeg output args
FFMPEG_OUTPUT_EXTRA=""
# =================================

require_cmd() {
  command -v "$1" >/dev/null 2>&1 || {
    echo "Missing required command: $1" >&2
    exit 1
  }
}

require_cmd "$FFMPEG_BIN"

mkdir -p "$HLS_DIR"
rm -f "$HLS_DIR"/*.m3u8 "$HLS_DIR"/*.ts

PLAYLIST_PATH="$HLS_DIR/$PLAYLIST_NAME"
SEGMENT_PATH="$HLS_DIR/$SEGMENT_PATTERN"

echo "Starting HLS generation from $DEVICE"
echo "Input: ${WIDTH}x${HEIGHT} @ ${FRAMERATE} fps, format=$INPUT_FORMAT"
echo "Output directory: $HLS_DIR"
echo "Playlist path: $PLAYLIST_PATH"
echo "Press Ctrl+C to stop."

exec "$FFMPEG_BIN" \
  -hide_banner \
  -loglevel "$LOG_LEVEL" \
  -fflags nobuffer \
  -flags low_delay \
  -probesize 32 \
  -analyzeduration 0 \
  -f v4l2 \
  ${FFMPEG_INPUT_EXTRA} \
  -input_format "$INPUT_FORMAT" \
  -video_size "${WIDTH}x${HEIGHT}" \
  -framerate "$FRAMERATE" \
  -i "$DEVICE" \
  -map 0:v:0 \
  -c:v copy \
  ${FFMPEG_OUTPUT_EXTRA} \
  -f hls \
  -hls_time "$HLS_TIME" \
  -hls_list_size "$HLS_LIST_SIZE" \
  -hls_flags delete_segments+independent_segments+append_list \
  -hls_segment_filename "$SEGMENT_PATH" \
  "$PLAYLIST_PATH"
