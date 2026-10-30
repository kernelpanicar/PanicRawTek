#!/usr/bin/env python3
"""Genera la landing desde docs/index.template.html.
  python3 tools/build_landing.py [usuario/repo] [salida_artifact.html]
- docs/index.html: documento completo para GitHub Pages (Settings > Pages > main /docs)
- opcional: versión solo-contenido para publicar como artifact"""
import json, os, re, sys
ROOT = os.path.normpath(os.path.join(os.path.dirname(os.path.abspath(__file__)), ".."))
repo = sys.argv[1] if len(sys.argv) > 1 else "__REPO__"
t = open(os.path.join(ROOT, "docs/index.template.html"), encoding="utf-8").read()
data = json.load(open(os.path.join(ROOT, "docs/landing_data.json")))
version = [l for l in open(os.path.join(ROOT, "CMakeLists.txt")) if l.startswith("project(PanicRawTek VERSION")][0].split()[2]
body = (t.replace("__DATA__", json.dumps(data, separators=(",", ":")))
         .replace('"__REPO__"', json.dumps(repo))
         )
body = re.sub(r'version: "[0-9.]+"', 'version: ' + json.dumps(version), body)
head, rest = body.split('<header class="top">', 1)
full = ('<!doctype html>\n<html lang="es">\n<head>\n<meta charset="utf-8">\n'
        '<meta name="viewport" content="width=device-width, initial-scale=1, viewport-fit=cover">\n'
        + head + '</head>\n<body>\n<header class="top">' + rest + '\n</body>\n</html>\n')
open(os.path.join(ROOT, "docs/index.html"), "w", encoding="utf-8").write(full)
print("docs/index.html", len(full), "bytes · repo =", repo, "· v" + version)
if len(sys.argv) > 2:
    open(sys.argv[2], "w", encoding="utf-8").write(body)
    print(sys.argv[2], len(body), "bytes")
