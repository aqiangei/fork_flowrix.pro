"""Pull and verify a Beijing recovery bundle, without unpacking private data."""
from datetime import datetime, timezone
import hashlib
import json
import lzma
import os
from pathlib import Path
import subprocess
import tempfile
import zipfile


def verify(path):
    with zipfile.ZipFile(path) as archive:
        manifest = json.loads(archive.read("SHA256.json"))
        names = {"feedback/feedback.dat", "feedback/feedback-private.chat.dat",
                 "feedback/feedback-private.dat", "feedback/accounts.dat",
                 "feedback/feedback-salt.dat", "feedback/feedback-private.key",
                 "feedback/progress.json", "council/polls.json"}
        for role in ("one", "two", "three", "flowrix"):
            names.update({f"games/{role}/inventory.json.xz",
                          f"games/{role}/backup-metadata.json"})
        if names != set(manifest) or sorted(archive.namelist()) != sorted(names | {"SHA256.json"}):
            raise ValueError("unexpected recovery bundle contents")
        for name, digest in manifest.items():
            if hashlib.sha256(archive.read(name)).hexdigest() != digest:
                raise ValueError(f"SHA-256 mismatch for {name}")
        for role in ("one", "two", "three", "flowrix"):
            prefix = f"games/{role}/"
            metadata = json.loads(archive.read(prefix + "backup-metadata.json"))
            data = lzma.decompress(archive.read(prefix + "inventory.json.xz"))
            if (metadata["role"] != role or metadata["verified"] is not True
                    or len(data) != metadata["bytes"]
                    or hashlib.sha256(data).hexdigest() != metadata["sha256"]):
                raise ValueError(f"invalid game snapshot for {role}")


def main():
    home = Path(__file__).resolve().parent
    destination = home / "archives"
    destination.mkdir(exist_ok=True)
    stamp = datetime.now(timezone.utc).strftime("%Y%m%dT%H%M%S%fZ")
    with tempfile.TemporaryDirectory(prefix=".incoming-", dir=destination) as temp:
        candidate = Path(temp) / "beijing.zip"
        with candidate.open("wb") as output, (home / "backup.log").open("ab") as log:
            subprocess.run(
                [str(Path(os.environ["WINDIR"]) / "System32/OpenSSH/ssh.exe"),
                 "-T", "-F", "NUL", "-o", "BatchMode=yes", "-o", "IdentitiesOnly=yes",
                 "-o", "StrictHostKeyChecking=yes", "-o", "ConnectTimeout=15",
                 "-o", "GlobalKnownHostsFile=NUL",
                 "-o", f"UserKnownHostsFile={home / 'known_hosts'}",
                 "-i", str(home / "id_ed25519"), "root@154.8.157.208"],
                stdin=subprocess.DEVNULL, stdout=output, stderr=log, check=True, timeout=300,
            )
            output.flush()
            os.fsync(output.fileno())
        verify(candidate)
        candidate.replace(destination / f"beijing-{stamp}.zip")
    with (home / "backup.log").open("a", encoding="utf-8") as log:
        log.write(f"{stamp} verified games, feedback and council\n")


if __name__ == "__main__":
    try:
        main()
    except Exception:
        import traceback
        with Path(__file__).with_name("backup.log").open("a", encoding="utf-8") as log:
            traceback.print_exc(file=log)
        raise
