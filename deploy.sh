#!/usr/bin/env bash
set -euo pipefail

# ===== User configuration =====
APP_USER="orangepi"
APP_HOME="/home/$APP_USER"

SCRIPT_SRC_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
WWW_SRC_DIR="$SCRIPT_SRC_DIR/www"
WEB_ROOT_DST="/var/www/orangepi-camera"

CAPTURE_SCRIPT_SRC="$SCRIPT_SRC_DIR/run_capture.sh"
WEBSERVER_SCRIPT_SRC="$SCRIPT_SRC_DIR/run_webserver.sh"

CAPTURE_SCRIPT_DST="$APP_HOME/run_capture.sh"
WEBSERVER_SCRIPT_DST="$APP_HOME/run_webserver.sh"

CAPTURE_SERVICE_SRC="$SCRIPT_SRC_DIR/run_capture.service"
WEBSERVER_SERVICE_SRC="$SCRIPT_SRC_DIR/run_webserver.service"

SYSTEMD_DIR="/etc/systemd/system"
NGINX_RUNTIME_DIR="/tmp/orangepi_hls_nginx"
# =================================

require_root() {
  if [[ "$EUID" -ne 0 ]]; then
    echo "Please run as root: sudo ./deploy.sh" >&2
    exit 1
  fi
}

require_file() {
  [[ -f "$1" ]] || {
    echo "Missing file: $1" >&2
    exit 1
  }
}

require_root

require_file "$CAPTURE_SCRIPT_SRC"
require_file "$WEBSERVER_SCRIPT_SRC"
require_file "$CAPTURE_SERVICE_SRC"
require_file "$WEBSERVER_SERVICE_SRC"

mkdir -p "$WEB_ROOT_DST"
mkdir -p "$APP_HOME"

if [[ -d "$WWW_SRC_DIR" ]]; then
  cp -a "$WWW_SRC_DIR"/. "$WEB_ROOT_DST"/
fi

install -o "$APP_USER" -g "$APP_USER" -m 0755 "$CAPTURE_SCRIPT_SRC" "$CAPTURE_SCRIPT_DST"
install -o "$APP_USER" -g "$APP_USER" -m 0755 "$WEBSERVER_SCRIPT_SRC" "$WEBSERVER_SCRIPT_DST"

chown -R "$APP_USER:$APP_USER" "$WEB_ROOT_DST"

install -o root -g root -m 0644 "$CAPTURE_SERVICE_SRC" "$SYSTEMD_DIR/run_capture.service"
install -o root -g root -m 0644 "$WEBSERVER_SERVICE_SRC" "$SYSTEMD_DIR/run_webserver.service"

# Free port 80 for the custom nginx instance.
systemctl stop nginx 2>/dev/null || true
systemctl stop apache2 2>/dev/null || true
fuser -k 80/tcp 2>/dev/null || true

# Stop capture service before touching the camera again.
systemctl stop run_capture.service 2>/dev/null || true
fuser -k /dev/video3 2>/dev/null || true

# Remove stale runtime files from the custom nginx instance.
rm -f "$NGINX_RUNTIME_DIR/nginx.pid"

# Reload systemd and restart services.
systemctl daemon-reload
systemctl enable run_capture.service run_webserver.service
systemctl restart run_webserver.service
sleep 1
systemctl restart run_capture.service

echo
echo "Deployment complete."
echo "Website root: $WEB_ROOT_DST"
echo "Capture script: $CAPTURE_SCRIPT_DST"
echo "Webserver script: $WEBSERVER_SCRIPT_DST"
echo
echo "Service status:"
systemctl --no-pager --full --lines=12 status run_webserver.service || true
echo
systemctl --no-pager --full --lines=12 status run_capture.service || true
