#!/usr/bin/env bash
set -euo pipefail

image="${NICKELTC_IMAGE:-ghcr.io/nicoverbruggen/nickelbench:recent}"
workdir="${PWD}"
scratch="${workdir}/tmp/build"

engine_flags=()
if command -v podman >/dev/null 2>&1; then
    container_engine="podman"
    # Rootless podman manages cgroups through the systemd user session. Without one
    # (e.g. over SSH), crun fails with an sd-bus error; cgroupfs works there.
    if ! systemctl --user show-environment >/dev/null 2>&1; then
        engine_flags=(--cgroup-manager=cgroupfs)
    fi
elif command -v docker >/dev/null 2>&1; then
    container_engine="docker"
else
    echo "building NickelArticleFix requires Podman or Docker" >&2
    exit 127
fi

export COPYFILE_DISABLE=1

if [ "$#" -eq 0 ]; then
    set -- clean all strip koboroot check
fi

mkdir -p "${scratch}"

# The source tarball below leaves .git out, so the git describe in NickelHook.mk
# has nothing to read inside the container and every local build would report an
# unknown version. Read it here instead and hand it to make. CI is unaffected: it
# runs make directly against a checkout.
version="$(git describe --tags --always --dirty 2>/dev/null || true)"

# BSD tar (macOS) needs these to keep metadata out of the archive; GNU tar has no
# --no-mac-metadata/--no-fflags and stores no such metadata by default.
tar_meta_flags=()
if tar --version 2>/dev/null | grep -q bsdtar; then
    tar_meta_flags=(--no-mac-metadata --no-xattrs --no-acls --no-fflags)
fi

tar \
    "${tar_meta_flags[@]}" \
    -C "${workdir}" \
    --exclude=.git \
    --exclude=.DS_Store \
    --exclude=tmp \
    --exclude=KoboRoot.tgz \
    --exclude='*.o' \
    --exclude='*.moc' \
    --exclude=nhplugin.json \
    --exclude=src/libnickelarticlefix.so \
    -czf "${scratch}/source.tgz" .

"${container_engine}" "${engine_flags[@]}" run --rm -i \
    --entrypoint sh \
    --env "NH_BUILD_VERSION=${version}" \
    "${image}" \
    -lc '
        set -eu
        mkdir -p /work
        tar -C /work -xzf -
        cd /work
        # NickelHook only recognises the toolchain by the bare prefix. Keep its
        # directory on PATH inside the login shell and pass the expected prefix.
        PATH="/tc/arm-nickel-linux-gnueabihf/bin:$PATH"
        export PATH
        make "$@" \
            VERSION="${NH_BUILD_VERSION}" \
            CROSS_COMPILE=arm-nickel-linux-gnueabihf- \
            MOC=/tc/arm-nickel-linux-gnueabihf/arm-nickel-linux-gnueabihf/sysroot/usr/bin/moc \
            RCC=/tc/arm-nickel-linux-gnueabihf/arm-nickel-linux-gnueabihf/sysroot/usr/bin/rcc >&2
        if [ -f KoboRoot.tgz ] && [ -f src/libnickelarticlefix.so ]; then
            tar -czf - KoboRoot.tgz src/libnickelarticlefix.so
        fi
    ' sh "$@" < "${scratch}/source.tgz" > "${scratch}/artifacts.tgz"

if [ -s "${scratch}/artifacts.tgz" ]; then
    tar -xzf "${scratch}/artifacts.tgz" -C "${workdir}"
fi
