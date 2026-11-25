#!/bin/bash
set -euo pipefail
swap=$1
updater=packaging/macos/apply-update.sh
base=$(mktemp -d)
trap 'rm -rf "$base"' EXIT
target="$base/Old TFX.app"
work="$base/.TFX-update-test"
new="$work/TFX.app"

setup_bundles() {
    mkdir -p "$target/Contents/Helpers" "$new/Contents/MacOS"
    cp "$swap" "$target/Contents/Helpers/tfx-swap"
    touch "$target/Contents/old-version"
}

apply_update() {
    (exit 0) &
    local stopped=$!
    wait "$stopped"
    bash "$updater" apply "$target" "$work" "$stopped"
}

setup_bundles
printf '#!/bin/bash\nexit 1\n' > "$new/Contents/MacOS/TFX"
chmod +x "$new/Contents/MacOS/TFX"
if apply_update; then exit 1; fi
test -f "$target/Contents/old-version"
test ! -e "$work"

setup_bundles
cat > "$new/Contents/MacOS/TFX" <<'CANDIDATE'
#!/bin/bash
test "$1" = --update-ready-file || exit 1
printf 'ready\n' > "$2"
sleep 2
CANDIDATE
chmod +x "$new/Contents/MacOS/TFX"
apply_update
test -x "$target/Contents/MacOS/TFX"
test ! -e "$target/Contents/old-version"
test ! -e "$work"
