# Guía: publicar Panic Raw Tek en GitHub (instaladores + landing)

Tiempo total: ~30 minutos (la mayoría es esperar que compile).

## 1. Cuenta y app
1. Creá una cuenta gratis en https://github.com/signup
2. Instalá **GitHub Desktop**: https://desktop.github.com — abrilo e iniciá sesión con tu cuenta.

## 2. Subir el proyecto
1. Descomprimí `PanicRawTek-source.zip` (por ejemplo en Documentos). Queda la carpeta `PanicRawTek`.
2. En GitHub Desktop: **File → Add Local Repository…** → elegí la carpeta `PanicRawTek`.
3. Va a decir que no es un repositorio: hacé click en **create a repository** → **Create Repository**.
4. Arriba, click en **Publish repository**:
   - Name: `panic-raw-tek`
   - **Destildá "Keep this code private"** (tiene que ser público para que cualquiera pueda descargar los instaladores y para usar GitHub Pages gratis).
   - **Publish Repository**.

## 3. Compilar los instaladores (automático)
1. En el navegador: `https://github.com/TU_USUARIO/panic-raw-tek` → pestaña **Actions**.
2. Ya tiene que estar corriendo **Build installers**. Si pide habilitar workflows, hacé click en el botón verde para habilitarlos.
3. Esperá a que los dos jobs (macOS y Windows) queden en verde ✓ (~15–20 min).
   - Si alguno queda en rojo ✗: entrá, abrí el paso que falló y mandame el error.

## 4. Crear la versión pública (Release)
1. En el repo: columna derecha **Releases → Create a new release** (o `…/releases/new`).
2. **Choose a tag** → escribí `v3.2.0` → **Create new tag: v3.2.0 on publish**.
3. Title: `Panic Raw Tek 3.2.0`. Descripción a gusto.
4. **Publish release**.
5. Esto lanza de nuevo el workflow; al terminar (~20 min) el Release tiene adjuntos:
   - `PanicRawTek-macOS.pkg` y `PanicRawTek-Windows-Setup.exe` (los que usa la landing)
   - las mismas versiones con el número de versión en el nombre.

## 5. Publicar la landing
1. En el repo: **Settings → Pages**.
2. Source: **Deploy from a branch**. Branch: **main**, carpeta **/docs** → **Save**.
3. En 1–2 minutos queda en `https://TU_USUARIO.github.io/panic-raw-tek/`.
   Los botones de descarga detectan tu repo solos y bajan del último Release.

## 6. Sacar una versión nueva más adelante
1. Hacé los cambios en la carpeta (o pedímelos) y en GitHub Desktop: escribí un resumen → **Commit to main** → **Push origin**.
2. Subí el número en `CMakeLists.txt` (`project(PanicRawTek VERSION 3.2.0 …)`).
3. Creá un Release nuevo con el tag `v3.2.0` (paso 4). La landing ofrece siempre el último.
