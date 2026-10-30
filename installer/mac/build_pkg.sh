#!/bin/bash
# =============================================================================
#  PANIC RAW TEK — genera el instalador .pkg para macOS
#
#  Uso:  installer/mac/build_pkg.sh [carpeta_de_artefactos]
#        (por defecto build/PanicRawTek_artefacts/Release, ya compilado)
#
#  Firma opcional (si tenés cuenta de Apple Developer):
#    export APP_SIGN_ID="Developer ID Application: Tu Nombre (TEAMID)"
#    export INSTALLER_SIGN_ID="Developer ID Installer: Tu Nombre (TEAMID)"
#  Sin esas variables, los plugins se firman ad-hoc y el .pkg queda sin firmar.
# =============================================================================
set -euo pipefail
cd "$(dirname "$0")/../.."
ROOT="$(pwd)"

VERSION=$(sed -nE 's/^project\(PanicRawTek VERSION ([0-9.]+).*/\1/p' CMakeLists.txt)
ART="${1:-build/PanicRawTek_artefacts/Release}"
WORK="build/pkg"
OUT="dist"
NAME="Panic Raw Tek"

for f in "$ART/VST3/$NAME.vst3" "$ART/AU/$NAME.component" "$ART/Standalone/$NAME.app"; do
    [ -e "$f" ] || { echo "!! Falta $f — compilá primero (build_mac.sh o cmake --build)."; exit 1; }
done

echo ">> Preparando payloads (v$VERSION)..."
rm -rf "$WORK"
mkdir -p "$WORK/root-vst3" "$WORK/root-au" "$WORK/root-app" "$WORK/pkgs" "$WORK/resources" "$OUT"
cp -R "$ART/VST3/$NAME.vst3"      "$WORK/root-vst3/"
cp -R "$ART/AU/$NAME.component"   "$WORK/root-au/"
cp -R "$ART/Standalone/$NAME.app" "$WORK/root-app/"

SIGN="${APP_SIGN_ID:--}"
for b in "$WORK/root-vst3/$NAME.vst3" "$WORK/root-au/$NAME.component" "$WORK/root-app/$NAME.app"; do
    xattr -cr "$b"
    if [ "$SIGN" = "-" ]; then
        codesign --force --deep --sign - "$b"
    else
        codesign --force --deep --options runtime --timestamp --sign "$SIGN" "$b"
    fi
done

# Sin esto, Installer "relocaliza" el bundle si encuentra otra copia en otro lado
nonrelocatable() {   # $1 = root, $2 = plist de salida
    pkgbuild --analyze --root "$1" "$2" >/dev/null
    local n
    n=$(/usr/libexec/PlistBuddy -c "Print" "$2" | grep -c "BundleIsRelocatable" || true)
    for ((i = 0; i < n; i++)); do
        /usr/libexec/PlistBuddy -c "Set :$i:BundleIsRelocatable false" "$2"
    done
}

chmod +x installer/mac/scripts/*
echo ">> Creando paquetes de componentes..."
nonrelocatable "$WORK/root-vst3" "$WORK/vst3.plist"
nonrelocatable "$WORK/root-au"   "$WORK/au.plist"
nonrelocatable "$WORK/root-app"  "$WORK/app.plist"

pkgbuild --nopayload --scripts installer/mac/scripts \
         --identifier com.panicaudio.panicrawtek.core --version "$VERSION" \
         "$WORK/pkgs/core.pkg"
pkgbuild --root "$WORK/root-vst3" --component-plist "$WORK/vst3.plist" \
         --install-location "/Library/Audio/Plug-Ins/VST3" \
         --identifier com.panicaudio.panicrawtek.vst3 --version "$VERSION" \
         "$WORK/pkgs/vst3.pkg"
pkgbuild --root "$WORK/root-au" --component-plist "$WORK/au.plist" \
         --install-location "/Library/Audio/Plug-Ins/Components" \
         --identifier com.panicaudio.panicrawtek.au --version "$VERSION" \
         "$WORK/pkgs/au.pkg"
pkgbuild --root "$WORK/root-app" --component-plist "$WORK/app.plist" \
         --install-location "/Applications" \
         --identifier com.panicaudio.panicrawtek.app --version "$VERSION" \
         "$WORK/pkgs/app.pkg"

echo ">> Armando el instalador..."
cp installer/mac/resources/* "$WORK/resources/"
sed "s/@VERSION@/$VERSION/g" installer/mac/distribution.xml > "$WORK/distribution.xml"
for f in welcome.html conclusion.html; do
    sed "s/@VERSION@/$VERSION/g" "installer/mac/resources/$f" > "$WORK/resources/$f"
done

UNSIGNED="$WORK/$NAME $VERSION (unsigned).pkg"
FINAL="$OUT/Panic Raw Tek $VERSION macOS.pkg"
productbuild --distribution "$WORK/distribution.xml" --resources "$WORK/resources" \
             --package-path "$WORK/pkgs" "$UNSIGNED"

if [ -n "${INSTALLER_SIGN_ID:-}" ]; then
    productsign --sign "$INSTALLER_SIGN_ID" "$UNSIGNED" "$FINAL"
    echo "   Firmado con: $INSTALLER_SIGN_ID"
    echo "   (para notarizar: xcrun notarytool submit \"$FINAL\" --keychain-profile <perfil> --wait && xcrun stapler staple \"$FINAL\")"
else
    mv "$UNSIGNED" "$FINAL"
fi

echo ""
echo "✅ Instalador listo: $ROOT/$FINAL"
