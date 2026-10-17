#!/bin/sh
#
# Builds OctoSnap on NetBSD and runs tests/netbsd-e2e.sh. Runs as root inside
# the CI virtual machine, from the top of the source tree, on any port.

set -eu

release=$(uname -r)
machine=$(uname -m)      # amd64, i386, evbarm
arch=$(uname -p)         # x86_64, i386, aarch64, earmv7hf

# Release directories are per port; evbarm is split by CPU architecture.
case $machine in
evbarm) port=evbarm-$arch ;;
*)      port=$machine ;;
esac

echo "== NetBSD $release $port ($arch), $(sysctl -n hw.ncpu) CPU(s), $(($(sysctl -n hw.physmem64) / 1048576)) MB"

# Qt6 binary packages link against X11 libraries from the xbase set and
# Qt's CMake files need the xcomp headers. Sets are .tar.xz on amd64 and
# aarch64 but .tgz on i386 and earmv7hf, so try both.
if [ ! -e /usr/X11R7/lib/libxcb.so ]; then
	sets=https://cdn.netbsd.org/pub/NetBSD/NetBSD-$release/$port/binary/sets
	for set in xbase xcomp; do
		echo "== installing set $set"
		if ftp -o /tmp/$set.tar.xz "$sets/$set.tar.xz" >/dev/null 2>&1; then
			tar -xJpf /tmp/$set.tar.xz -C /
		else
			ftp -o /tmp/$set.tgz "$sets/$set.tgz"
			tar -xzpf /tmp/$set.tgz -C /
		fi
		rm -f /tmp/$set.tar.xz /tmp/$set.tgz
	done
fi

# Images that already carry packages (vmactions) configure pkgin for the
# pkgsrc branch those packages came from; pkg_add against another branch
# fails on dependency versions (e.g. pcre2), so let pkgin resolve them.
# Freshly installed systems (anita) have neither packages nor pkgin.
pkgs="cmake ninja-build qt6-qtbase qt6-qttools"
if command -v pkgin >/dev/null 2>&1; then
	pkgin -y install $pkgs
else
	# Binary packages are published per major release (10.0 covers 10.x).
	export PKG_PATH="https://cdn.netbsd.org/pub/pkgsrc/packages/NetBSD/$arch/${release%%.*}.0/All"
	for pkg in $pkgs; do
		pkg_info -qe "$pkg" || pkg_add "$pkg"
	done
fi

echo "== toolchain"
c++ --version | head -1
cmake --version | head -1
pkg_info -qe qt6-qtbase && pkg_info -I qt6-qtbase

echo "== build"
cmake -B build -S . -G Ninja -DCMAKE_BUILD_TYPE=Release \
	-DCMAKE_PREFIX_PATH=/usr/pkg/qt6
if ! cmake --build build >build.log 2>&1; then
	cat build.log
	exit 1
fi
cat build.log
echo "== compiler warnings: $(grep -c 'warning:' build.log || true)"
file build/octosnap build/octosnap-helper

sh tests/netbsd-e2e.sh build
