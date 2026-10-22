#!/usr/bin/env python3
"""Checks every config under gameplay/levels against its schema and against the other files.

Usage: python3 tools/check_levels.py [levels-dir]   (needs `pip install jsonschema`)
Exits 1 on the first file with errors, so it can run in CI until the server's own loader does the same checks.
"""
import json
import sys
from pathlib import Path

from jsonschema import Draft202012Validator

WORLD_HEIGHT = 1080

root = Path(sys.argv[1] if len(sys.argv) > 1 else Path(__file__).resolve().parent.parent)
errors = []


def load(path, schema_name):
    data = json.loads(path.read_text())
    schema = json.loads((root / "schema" / schema_name).read_text())
    for e in Draft202012Validator(schema).iter_errors(data):
        where = "/".join(str(p) for p in e.absolute_path) or "(root)"
        errors.append(f"{path.relative_to(root)}: {where}: {e.message}")
    return data


enemies = load(root / "enemies.json", "enemies.schema.json")
formations = load(root / "formations.json", "formations.schema.json")["formations"]
campaign = load(root / "campaign.json", "campaign.schema.json")
enemy_ids = set(enemies.get("enemies", {}))
boss_ids = set(enemies.get("bosses", {}))

for rel in campaign.get("levels", []):
    path = root / rel
    if not path.exists():
        errors.append(f"campaign.json: missing level file {rel}")
        continue
    level = load(path, "level.schema.json")
    name = path.relative_to(root)
    for i, wave in enumerate(level.get("waves", [])):
        if wave.get("formation") not in formations:
            errors.append(f"{name}: waves/{i}: unknown formation '{wave.get('formation')}'")
        for enemy in wave.get("roster", {}):
            if enemy not in enemy_ids:
                errors.append(f"{name}: waves/{i}: unknown enemy '{enemy}'")
    terrain = level.get("terrain")
    if terrain:
        rows = terrain.get("map", [])
        if len({len(r) for r in rows}) > 1:
            errors.append(f"{name}: terrain/map: rows have different lengths")
        if len(rows) * terrain.get("tileSize", 0) > WORLD_HEIGHT:
            errors.append(f"{name}: terrain/map: {len(rows)} rows of {terrain['tileSize']} overflow the {WORLD_HEIGHT}-high world")
        unknown = {c for r in rows for c in r} - set(terrain.get("tiles", {})) - {" "}
        if unknown:
            errors.append(f"{name}: terrain/map: undefined tiles {sorted(unknown)}")
    boss = level.get("boss")
    if boss and boss.get("id") not in boss_ids:
        errors.append(f"{name}: boss: unknown boss '{boss.get('id')}'")

print("\n".join(errors) or f"OK: {len(campaign.get('levels', []))} levels")
sys.exit(1 if errors else 0)
