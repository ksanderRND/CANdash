#!/usr/bin/env bash
# Brings up a virtual CAN interface. Safe to rerun.
set -euo pipefail

IFACE="vcan0"

sudo modprobe vcan

if ! ip link show "$IFACE" >/dev/null 2>&1; then
    sudo ip link add dev "$IFACE" type vcan
fi

sudo ip link set up "$IFACE"

ip -br link show "$IFACE"
