#!/usr/bin/env bash
set -euo pipefail

NGINX_BIN="/usr/sbin/nginx"
NGINX_CONF_DIR="/tmp/orangepi_hls_nginx"
NGINX_CONF_FILE="$NGINX_CONF_DIR/nginx.conf"

HTTP_PORT="80"
SERVER_NAME="_"

WEB_ROOT="/var/www/orangepi-camera"
HLS_DIR="/dev/shm/hls"
HLS_URL_PREFIX="/hls/"

ACCESS_LOG="$NGINX_CONF_DIR/access.log"
ERROR_LOG="$NGINX_CONF_DIR/error.log"

mkdir -p "$NGINX_CONF_DIR"
mkdir -p "$WEB_ROOT"
mkdir -p "$HLS_DIR"

cat > "$NGINX_CONF_FILE" <<EOF
worker_processes 1;
error_log $ERROR_LOG warn;
pid /run/orangepi-hls-nginx.pid;
daemon off;

events {
    worker_connections 1024;
}

http {
    include /etc/nginx/mime.types;
    default_type application/octet-stream;
    access_log $ACCESS_LOG;
    sendfile on;
    tcp_nopush on;
    tcp_nodelay on;
    keepalive_timeout 65;

    server {
        listen $HTTP_PORT;
        server_name $SERVER_NAME;

        root $WEB_ROOT;
        index index.html;

        location / {
            try_files \$uri \$uri/ /index.html;
        }

        location $HLS_URL_PREFIX {
            alias $HLS_DIR/;
            types {
                application/vnd.apple.mpegurl m3u8;
                video/mp2t ts;
            }
            add_header Cache-Control no-cache;
            add_header Access-Control-Allow-Origin *;
        }
    }
}
EOF

echo "Starting nginx in foreground..."
echo "Website root: $WEB_ROOT"
echo "HLS dir: $HLS_DIR"
echo "URL: http://127.0.0.1/"
echo "HLS: http://127.0.0.1${HLS_URL_PREFIX}stream.m3u8"
echo "Logs: $ACCESS_LOG and $ERROR_LOG"

exec "$NGINX_BIN" -c "$NGINX_CONF_FILE"
