#!/bin/sh
#
# Boots a NetBSD port under QEMU with anita(1) and runs ci/netbsd-ci.sh in it.
# Used for the ports that vmactions/netbsd-vm does not offer (i386, earmv7hf).
# Runs on the Linux CI host, from the top of a git checkout.
#
# usage: ci/anita-run.sh <port> [anita options...]
#   ANITA            path to the anita script (required)
#   NETBSD_RELEASE   release to test (default 10.2)

set -eu

port=$1
shift
release=${NETBSD_RELEASE:-10.2}
work=${RUNNER_TEMP:-/tmp}/anita-$port
serve=${RUNNER_TEMP:-/tmp}/serve-$port

# The VM fetches the sources over QEMU user networking, where the host's
# loopback interface is reachable as 10.0.2.2.
mkdir -p "$serve"
git archive --format=tar.gz -o "$serve/src.tgz" HEAD
python3 -m http.server 8000 --bind 127.0.0.1 --directory "$serve" >/dev/null 2>&1 &
server=$!
trap 'kill $server' EXIT

# anita types the command on the serial console, so it has to stay short.
python3 "$ANITA" --workdir "$work" --disk-size 16G --memory-size 2G \
	--run-timeout 10800 "$@" \
	--run '(dhcpcd -w -q || sleep 15); ftp -o /tmp/s.tgz http://10.0.2.2:8000/src.tgz && mkdir -p /tmp/s && cd /tmp/s && tar -xzf /tmp/s.tgz && sh ci/netbsd-ci.sh' \
	boot "https://cdn.netbsd.org/pub/NetBSD/NetBSD-$release/$port/"
