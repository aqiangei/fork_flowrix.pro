#!/usr/bin/env python3
"""Export one recovery bundle to the Windows backup task over SSH."""
import hashlib
import json
from pathlib import Path
import sys
import zipfile


def main():
    root = Path("/var/lib/florrclone-backup")
    paths = {}
    for role in ("one", "two", "three", "flowrix"):
        snapshots = sorted((root / role).glob("*/backup-metadata.json"), reverse=True)
        for metadata in snapshots:
            snapshot = json.loads(metadata.read_bytes())
            inventory = metadata.with_name("inventory.json.xz")
            if snapshot.get("verified") is True and inventory.is_file():
                paths[f"games/{role}/inventory.json.xz"] = inventory
                paths[f"games/{role}/backup-metadata.json"] = metadata
                break
        else:
            raise ValueError(f"no verified snapshot for {role}")
    feedback = Path("/var/lib/ashflor-feedback")
    for name in ("feedback.dat", "feedback-private.chat.dat", "feedback-private.dat",
                 "accounts.dat", "feedback-salt.dat", "feedback-private.key"):
        paths[f"feedback/{name}"] = feedback / name
    paths["feedback/progress.json"] = Path("/var/www/ashflor-feedback/progress.json")
    paths["council/polls.json"] = Path("/var/lib/ashflor-council/polls.json")

    manifest = {}
    with zipfile.ZipFile(sys.stdout.buffer, "w", compression=zipfile.ZIP_DEFLATED) as archive:
        for name, path in paths.items():
            data = path.read_bytes()
            if not data:
                raise ValueError(f"empty backup member {name}")
            if name.endswith(".json"):
                json.loads(data)
            archive.writestr(name, data)
            manifest[name] = hashlib.sha256(data).hexdigest()
        archive.writestr("SHA256.json", json.dumps(manifest))


if __name__ == "__main__":
    main()
