#!/bin/bash
set -euo pipefail
bundle=${1:?Usage: package.sh path/to/TFX.app output.zip}
archive=${2:?Provide output.zip}
script_dir=$(cd "$(dirname "$0")" && pwd)
bash "$script_dir/finalize-app.sh" "$bundle"
ditto -c -k --keepParent "$bundle" "$archive"
# Verify the delivered bytes, including symlinks and executable permissions.
work=$(mktemp -d)
trap 'rm -rf "$work"' EXIT
ditto -x -k "$archive" "$work"
restored="$work/$(basename "$bundle")"
codesign --verify --deep --strict --verbose=2 "$restored"
executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$restored/Contents/Info.plist")
test -x "$restored/Contents/MacOS/$executable"
if [[ ${3:-} == universal ]]; then
    while IFS= read -r -d '' binary; do
        if file -b "$binary" | grep -q 'Mach-O'; then
            lipo "$binary" -verify_arch arm64 x86_64
        fi
    done < <(find "$restored/Contents" -type f -print0)
fi
"$restored/Contents/MacOS/$executable" --smoke-test --update-ready-file "$work/ready"
test "$(cat "$work/ready")" = ready
shasum -a 256 "$archive" > "$archive.sha256"
