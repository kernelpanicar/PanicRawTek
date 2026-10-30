#!/bin/bash
# PANIC RAW TEK — compila (VST3 + AU) en tu Mac y lo instala para Ableton.
# Requisitos: Xcode Command Line Tools y CMake.
set -e
cd "$(dirname "$0")"

if ! xcode-select -p >/dev/null 2>&1; then
    echo ">> Instalando Xcode Command Line Tools (aceptá el diálogo y volvé a correr este script)"
    xcode-select --install
    exit 1
fi

if ! command -v cmake >/dev/null 2>&1; then
    if command -v brew >/dev/null 2>&1; then
        echo ">> Instalando CMake con Homebrew..."
        brew install cmake
    else
        echo "!! Falta CMake. Instalalo desde https://cmake.org/download/ o con Homebrew (brew install cmake)."
        exit 1
    fi
fi

echo ">> Configurando (la primera vez descarga JUCE, tarda un par de minutos)..."
cmake -B build -DCMAKE_BUILD_TYPE=Release > build_config_log.txt 2>&1 || { echo "!! Falló la configuración:"; tail -20 build_config_log.txt; exit 1; }

echo ">> Compilando Panic Raw Tek... (tarda unos minutos; los warnings se guardan en build_log.txt)"
set -o pipefail
if ! cmake --build build --config Release -j"$(sysctl -n hw.ncpu)" --target PanicRawTek_VST3 PanicRawTek_AU 2>&1 \
        | tee build_log.txt | grep -E --line-buffered "error:|\[ *[0-9]+%\] (Built|Linking)"; then
    echo ""
    echo "!! La compilación falló. Errores encontrados:"
    grep -E "error:" build_log.txt | head -20
    echo "   (log completo en: $(pwd)/build_log.txt)"
    exit 1
fi
set +o pipefail

ART="build/PanicRawTek_artefacts/Release"
VST3_DIR="$HOME/Library/Audio/Plug-Ins/VST3"
AU_DIR="$HOME/Library/Audio/Plug-Ins/Components"
mkdir -p "$VST3_DIR" "$AU_DIR"

rm -rf "$VST3_DIR/Panic Raw Tek.vst3" "$AU_DIR/Panic Raw Tek.component"
cp -R "$ART/VST3/Panic Raw Tek.vst3" "$VST3_DIR/"
cp -R "$ART/AU/Panic Raw Tek.component" "$AU_DIR/"

for p in "$VST3_DIR/Panic Raw Tek.vst3" "$AU_DIR/Panic Raw Tek.component"; do
    xattr -cr "$p"
    codesign --force --deep --sign - "$p"
done

killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true

echo ""
echo "✅ Panic Raw Tek instalado:"
echo "   $VST3_DIR/Panic Raw Tek.vst3"
echo "   $AU_DIR/Panic Raw Tek.component"
echo "   (los presets vienen incluidos dentro del plugin)"
echo ""
echo "En Ableton: Settings > Plug-Ins > Option (⌥) + click en Rescan."
