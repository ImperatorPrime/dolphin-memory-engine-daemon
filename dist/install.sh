#!/bin/bash
set -e

if [ "$EUID" -ne 0 ]; then
    echo "Please run as root: sudo ./install.sh"
    exit 1
fi

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

install -m 755 "$SCRIPT_DIR/dolphin_memory_engine_daemon" /usr/bin/dolphin_memory_engine_daemon
install -m 644 "$SCRIPT_DIR/dolphin-memory-engine-daemon.service" /lib/systemd/system/dolphin-memory-engine-daemon.service

systemctl daemon-reload
systemctl start dolphin-memory-engine-daemon

echo ""
read -p "Enable dolphin-memory-engine-daemon on boot? [y/N] " -n 1 -r
echo ""
if [[ $REPLY =~ ^[Yy]$ ]]; then
    systemctl enable dolphin-memory-engine-daemon
    echo "Service enabled."
fi

echo "Done."
