#!/bin/bash
# Doble click: compila Panic Raw Tek y genera el instalador .pkg en la carpeta dist/
cd "$(dirname "$0")"
if ! command -v cmake >/dev/null 2>&1; then
    echo "!! Falta CMake (brew install cmake)."; exit 1
fi
echo ">> Compilando (VST3 + AU + Standalone)..."
cmake -B build -DCMAKE_BUILD_TYPE=Release > build_config_log.txt 2>&1 || { tail -20 build_config_log.txt; exit 1; }
set -o pipefail
cmake --build build --config Release -j"$(sysctl -n hw.ncpu)" --target PanicRawTek_VST3 PanicRawTek_AU PanicRawTek_Standalone 2>&1 \
    | tee build_log.txt | grep -E --line-buffered "error:|\[ *[0-9]+%\] (Built|Linking)" || { echo "!! Falló la compilación (ver build_log.txt)"; exit 1; }
chmod +x installer/mac/build_pkg.sh
installer/mac/build_pkg.sh && open dist
