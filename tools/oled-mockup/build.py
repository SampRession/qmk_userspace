#!/usr/bin/env python3
"""Build lily58-oled.html: inline core.js and the Lily58 logo into template.html."""
import json
import re
from pathlib import Path

HERE = Path(__file__).resolve().parent
LOGO_H = HERE.parents[1] / "keyboards/lily58/keymaps/crrptd/lily58_logo.h"

body = LOGO_H.read_text().split("{", 1)[1].rsplit("}", 1)[0]
logo = [int(x, 16) for x in re.findall(r"0x[0-9A-Fa-f]{2}", body)]
assert len(logo) == 512, f"expected 512 logo bytes, got {len(logo)}"

html = (HERE / "template.html").read_text()
html = html.replace("/*CORE*/", (HERE / "core.js").read_text())
html = html.replace("/*LOGO*/", json.dumps(logo))
(HERE / "lily58-oled.html").write_text(html)
print("wrote", HERE / "lily58-oled.html")
