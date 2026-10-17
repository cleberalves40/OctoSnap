#!/bin/sh
#
# End-to-end test of octosnap-helper against a real fss(4) device, followed
# by a start-up smoke test of the GUI. NetBSD only; must run as root.
#
# A scratch FFS file system is created on a vnd(4) image, so the system's
# own file systems are only touched for the snapshot backing store.
#
# usage: tests/netbsd-e2e.sh [build-dir]

set -eu

build=$(cd "${1:-build}" && pwd)
helper=$build/octosnap-helper
gui=$build/octosnap

step() { echo "== $*"; }
fail() { echo "FAIL: $*" >&2; exit 1; }
expect() { [ "$1" = "$2" ] || fail "$3: expected '$2', got '$1'"; }

[ "$(id -u)" -eq 0 ] || fail "must run as root"
[ -x "$helper" ] || fail "missing $helper"

tab=$(printf '\t')
vnd=vnd0
snap=e2e-$$
work=$(mktemp -d /var/tmp/octosnap-e2e.XXXXXX)
live=$work/mnt
ro=$work/ro

cleanup() {
	set +e
	"$helper" destroy "$snap-2" --remove-backing >/dev/null 2>&1
	"$helper" destroy "$snap" --remove-backing >/dev/null 2>&1
	umount "$live" 2>/dev/null
	vnconfig -u "$vnd" 2>/dev/null
	rm -rf "$work"
}
trap cleanup EXIT

case $(sysctl -n kern.rawpartition) in
2) part=c ;;
3) part=d ;;
*) fail "unexpected raw partition" ;;
esac

step "fss(4) availability"
fssconfig -l >/dev/null 2>&1 || modload fss 2>/dev/null || true
fssconfig -l >/dev/null || fail "fss(4) is not available"

step "scratch FFS on $vnd"
dd if=/dev/zero of="$work/fs.img" bs=1m count=64 2>/dev/null
vnconfig "$vnd" "$work/fs.img"
# -I: the unlabeled vnd has no 4.2BSD partition type for newfs to check.
newfs -I -O 2 "/dev/r$vnd$part" >/dev/null
mkdir "$live" "$ro" "$work/outside"
mount -t ffs "/dev/$vnd$part" "$live"

printf 'v1\n' >"$live/a.txt"
chmod 640 "$live/a.txt"
touch -t 202001020304 "$live/a.txt"
mkdir -p "$live/dir/sub"
printf 'inner\n' >"$live/dir/sub/b.txt"
ln -s a.txt "$live/link"

step "create"
out=$("$helper" create "$snap" "$live")
echo "$out"
case $out in "ok fss"*) ;; *) fail "create: $out" ;; esac

# Change the live file system after the snapshot.
printf 'v2\n' >"$live/a.txt"
rm -rf "$live/dir" "$live/link"
printf 'new\n' >"$live/new.txt"

step "list"
"$helper" list
# Columns: name unit mountpoint backing created rodir active size
line=$("$helper" list | grep "^$snap$tab") || fail "snapshot missing from list"
expect "$(echo "$line" | cut -f7)" 1 "active flag"

step "second snapshot gets another unit"
out2=$("$helper" create "$snap-2" "$live")
echo "$out2"
[ "$out2" != "$out" ] || fail "both snapshots got ${out#ok }"
"$helper" destroy "$snap-2" --remove-backing
expect "$(fssconfig -l | grep -c "^${out2#ok }: not in use")" 1 "unit released by destroy"

step "mount-ro"
"$helper" mount-ro "$snap" "$ro"
expect "$(cat "$ro/a.txt")" v1 "snapshot content"

step "restore --rename-existing"
"$helper" restore "$snap" --rename-existing a.txt dir link
expect "$(cat "$live/a.txt")" v1 "restored file"
expect "$(cat "$live"/a.txt.octosnap-*)" v2 "backup of the current version"
expect "$(cat "$live/dir/sub/b.txt")" inner "restored directory"
expect "$(readlink "$live/link")" a.txt "restored symlink"
expect "$(stat -f %Lp "$live/a.txt")" 640 "restored mode"
expect "$(stat -f %Sm -t %Y%m%d%H%M "$live/a.txt")" 202001020304 "restored mtime"
expect "$(cat "$live/new.txt")" new "file created after the snapshot"

step "restore (overwrite)"
printf 'v3\n' >"$live/a.txt"
"$helper" restore "$snap" a.txt
expect "$(cat "$live/a.txt")" v1 "overwritten file"
expect "$(ls "$live" | grep -c '^a\.txt\.octosnap-')" 1 "backups after overwrite"

step "restore refuses unsafe paths"
for bad in ../etc /etc dir/../a.txt ./a.txt; do
	if "$helper" restore "$snap" "$bad" 2>/dev/null; then
		fail "accepted path '$bad'"
	fi
done
rm -rf "$live/dir"
ln -s "$work/outside" "$live/dir"
if "$helper" restore "$snap" dir/sub/b.txt 2>/dev/null; then
	fail "restored through a symlinked parent directory"
fi
expect "$(ls -A "$work/outside")" "" "directory behind the symlink"

step "umount and destroy"
"$helper" umount "$ro"
"$helper" destroy "$snap" --remove-backing
if "$helper" list | grep -q "^$snap$tab"; then
	fail "snapshot still listed after destroy"
fi
expect "$(fssconfig -l | grep -c "^${out#ok }: not in use")" 1 "unit released by destroy"

if [ -x "$gui" ]; then
	# The GUI has no batch mode; it must stay up until timeout(1) stops it
	# (status 124). Any other status means it crashed or failed to start.
	step "GUI start-up (offscreen)"
	set +e
	QT_QPA_PLATFORM=offscreen timeout 20 "$gui" >"$work/gui.log" 2>&1
	rc=$?
	set -e
	cat "$work/gui.log"
	expect "$rc" 124 "GUI exit status"
fi

step "all tests passed"
