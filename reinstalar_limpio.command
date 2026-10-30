#!/bin/bash
# Desinstala todo (incluido RawTek viejo), recompila desde cero ESTA carpeta e instala Panic Raw Tek.
cd "$(dirname "$0")"

if ! grep -q "project(PanicRawTek VERSION 3" CMakeLists.txt 2>/dev/null; then
    echo "!! Esta carpeta no es Panic Raw Tek v3."
    echo "   Descomprimí PanicRawTek-source.zip y corré este script desde esa carpeta."
    echo "   Carpeta actual: $(pwd)"
    exit 1
fi

chmod +x desinstalar.command build_mac.sh install_presets.command
./desinstalar.command || exit 1

echo ""
echo ">> Borrando compilación anterior (build/)..."
rm -rf build

./build_mac.sh || exit 1

echo ""
echo ">> Verificando instalación..."
V1=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "$HOME/Library/Audio/Plug-Ins/VST3/Panic Raw Tek.vst3/Contents/Info.plist" 2>/dev/null)
V2=$(/usr/libexec/PlistBuddy -c "Print CFBundleShortVersionString" "$HOME/Library/Audio/Plug-Ins/Components/Panic Raw Tek.component/Contents/Info.plist" 2>/dev/null)
echo "   VST3 versión: ${V1:-NO INSTALADO}"
echo "   AU   versión: ${V2:-NO INSTALADO}"

echo ""
echo "✅ Listo. En Ableton:"
echo "   1. Settings > Plug-Ins > mantené Option (⌥) y click en 'Rescan'."
echo "   2. Browser > Plug-Ins > Panic Audio > Panic Raw Tek."
echo "   3. Si tenías RawTek en algún proyecto, reemplazalo por Panic Raw Tek (es un plugin nuevo)."
