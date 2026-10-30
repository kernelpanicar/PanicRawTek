#!/bin/bash
# Opcional: copia los .rtkpreset a la carpeta de presets (los presets ya vienen dentro del plugin;
# esto sirve para editarlos, compartirlos o agregar los tuyos).
cd "$(dirname "$0")"
DEST="$HOME/Library/Audio/Presets/Panic Audio/Panic Raw Tek"
[ -d Presets ] || { echo "!! No encontré la carpeta 'Presets'."; exit 1; }
mkdir -p "$DEST"
for cat in Presets/*/; do
    name="$(basename "$cat")"
    mkdir -p "$DEST/$name"
    cp -f "$cat"*.rtkpreset "$DEST/$name/" 2>/dev/null
done
echo "✅ Presets copiados a: $DEST"
