#!/bin/bash
# Desinstala Panic Raw Tek y también el RawTek viejo (plugins, presets y caché de Audio Units).
# Tus presets guardados (categoría User) se respaldan en el Escritorio.
cd "$(dirname "$0")"

if pgrep -xq "Live" || pgrep -fq "Ableton Live"; then
    echo "!! Cerrá Ableton antes de desinstalar y volvé a correr este script."
    exit 1
fi

VST3="$HOME/Library/Audio/Plug-Ins/VST3"
AU="$HOME/Library/Audio/Plug-Ins/Components"

echo ">> Borrando plugins (Panic Raw Tek y RawTek viejo)..."
rm -rf "$VST3/Panic Raw Tek.vst3" "$AU/Panic Raw Tek.component" "$VST3/RawTek.vst3" "$AU/RawTek.component"
for p in "/Library/Audio/Plug-Ins/VST3/Panic Raw Tek.vst3" "/Library/Audio/Plug-Ins/Components/Panic Raw Tek.component" \
         "/Library/Audio/Plug-Ins/VST3/RawTek.vst3" "/Library/Audio/Plug-Ins/Components/RawTek.component"; do
    [ -e "$p" ] && echo "   Hay una copia en $p (pide contraseña)" && sudo rm -rf "$p"
done

BK="$HOME/Desktop/PanicRawTek_User_Presets_backup"
for d in "$HOME/Library/Audio/Presets/Panic Audio/Panic Raw Tek/User" "$HOME/Library/Audio/Presets/RawTek Audio/RawTek/User"; do
    if [ -d "$d" ]; then
        mkdir -p "$BK" && cp -R "$d/." "$BK/"
        echo ">> Respaldé tus presets User en: $BK"
    fi
done
echo ">> Borrando carpetas de presets..."
rm -rf "$HOME/Library/Audio/Presets/Panic Audio" "$HOME/Library/Audio/Presets/RawTek Audio"

echo ">> Limpiando caché de Audio Units..."
rm -rf "$HOME/Library/Caches/AudioUnitCache" 2>/dev/null
rm -f "$HOME/Library/Preferences/com.apple.audio.InfoHelper.plist" 2>/dev/null
killall -9 AudioComponentRegistrar >/dev/null 2>&1 || true

echo ""
echo "✅ Desinstalado."
