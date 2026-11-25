#!/bin/bash
# Run after all deployment/install_name_tool work, before creating the archive.
set -euo pipefail
bundle=${1:?Usage: finalize-app.sh path/to/TFX.app}
identity=${MACOS_SIGN_IDENTITY:--}
if [[ ! -d "$bundle/Contents" ]]; then
    echo "Not an app bundle: $bundle" >&2
    exit 1
fi
if [[ "$identity" != - && "$identity" != 'Developer ID Application:'* ]]; then
    echo 'Distribution signing requires a Developer ID Application identity.' >&2
    exit 1
fi
sign() {
    if [[ "$identity" == - ]]; then
        codesign --force --sign - "$1"
    else
        codesign --force --sign "$identity" --timestamp --options runtime "$1"
    fi
}
# Qt deployment rewrites load commands, invalidating existing Mach-O signatures.
# Sign every nested executable/library first, then framework bundles, then the app.
executable=$(/usr/libexec/PlistBuddy -c 'Print :CFBundleExecutable' "$bundle/Contents/Info.plist")
main="$bundle/Contents/MacOS/$executable"
while IFS= read -r -d '' binary; do
    # codesign treats the main executable as the enclosing app bundle.
    [[ "$binary" != "$main" ]] || continue
    if file -b "$binary" | grep -q 'Mach-O'; then sign "$binary"; fi
done < <(find "$bundle/Contents" -type f -print0)
while IFS= read -r -d '' nested; do sign "$nested"; done < <(
    find "$bundle/Contents" -depth -type d \( -name '*.framework' -o -name '*.app' -o -name '*.xpc' -o -name '*.bundle' \) -print0
)
sign "$bundle"
codesign --verify --deep --strict --verbose=2 "$bundle"
# A valid ad-hoc signature fixes integrity; it is NOT an Apple trust certificate.
if [[ -n ${MACOS_NOTARY_PROFILE:-} ]]; then
    [[ "$identity" != - ]] || { echo 'Notarization requires Developer ID.' >&2; exit 1; }
    work=$(mktemp -d)
    trap 'rm -rf "$work"' EXIT
    ditto -c -k --keepParent "$bundle" "$work/notarize.zip"
    args=(--keychain-profile "$MACOS_NOTARY_PROFILE")
    if [[ -n ${MACOS_NOTARY_KEYCHAIN:-} ]]; then args+=(--keychain "$MACOS_NOTARY_KEYCHAIN"); fi
    xcrun notarytool submit "$work/notarize.zip" "${args[@]}" --wait
    xcrun stapler staple "$bundle"
    xcrun stapler validate "$bundle"
    spctl --assess --type execute --verbose=2 "$bundle"
fi
