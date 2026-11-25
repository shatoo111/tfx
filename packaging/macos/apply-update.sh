#!/bin/bash
set -euo pipefail
export PATH=/usr/bin:/bin:/usr/sbin:/sbin

mode=$1
target=$2
work=$3
new="$work/TFX.app"
[[ "$target" == *.app && -d "$target/Contents" ]]
[[ "$work" == "$(dirname "$target")"/.TFX-update-* && -d "$work" && ! -L "$work" ]]

if [[ "$mode" == prepare ]]; then
    archive=$4
    arch=$5
    ditto -x -k "$archive" "$work"
    id=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleIdentifier' "$new/Contents/Info.plist")
    [[ "$id" == org.telegrameffects.desktop && -x "$new/Contents/MacOS/TFX" ]]
    file -b "$new/Contents/MacOS/TFX" | grep -qw "$arch"
    codesign --verify --deep --strict "$new"
    spctl --assess --type execute "$new"
    team=$(codesign -dv --verbose=4 "$target" 2>&1 | sed -n 's/^TeamIdentifier=//p')
    if [[ -n "$team" && "$team" != "not set" ]]; then
        codesign --verify -R="anchor apple generic and certificate leaf[subject.OU] = \"$team\"" "$new"
    fi
    exit 0
fi

[[ "$mode" == apply ]]
pid=$4
backup=false
cleanup() {
    if [[ "$backup" == false ]]; then rm -rf "$work"; fi
}
failure() {
    /usr/bin/osascript - "${1:-$target}" <<'APPLESCRIPT' || true
on run argv
    display alert "Обновление TFX не завершено" message ("Прежняя версия сохранена здесь: " & item 1 of argv) as warning giving up after 15
end run
APPLESCRIPT
}
trap cleanup EXIT

for ((i=0; i<150; i++)); do
    if ! kill -0 "$pid" 2>/dev/null; then break; fi
    sleep 0.1
done
if kill -0 "$pid" 2>/dev/null; then
    failure
    exit 1
fi
if ! "$target/Contents/Helpers/tfx-swap" "$target" "$new"; then
    open "$target" || true
    failure
    exit 1
fi
backup=true

# Run the candidate as our child so a crash or timeout is observable.
"$target/Contents/MacOS/TFX" --update-ready-file "$work/ready" >/dev/null 2>&1 &
candidate=$!
for ((i=0; i<250; i++)); do
    if ! kill -0 "$candidate" 2>/dev/null; then break; fi
    if [[ -f "$work/ready" && $(cat "$work/ready") == ready ]]; then
        sleep 0.5
        if kill -0 "$candidate" 2>/dev/null; then
            backup=false
            exit 0
        fi
        break
    fi
    sleep 0.1
done

# Only terminate the candidate started above, then atomically restore the old package.
if kill -0 "$candidate" 2>/dev/null; then
    kill -TERM "$candidate" 2>/dev/null || true
    for ((i=0; i<20; i++)); do
        if ! kill -0 "$candidate" 2>/dev/null; then break; fi
        sleep 0.1
    done
    kill -KILL "$candidate" 2>/dev/null || true
fi
wait "$candidate" 2>/dev/null || true
if "$new/Contents/Helpers/tfx-swap" "$target" "$new"; then
    backup=false
    open "$target" || true
    failure
else
    failure "$new"
fi
exit 1
