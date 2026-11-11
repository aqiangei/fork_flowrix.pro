#!/usr/bin/env python3
"""Collect verified copies of the independent, already-persisted game saves."""

import argparse
import array
from concurrent.futures import ThreadPoolExecutor
from datetime import datetime, timedelta, timezone
import fcntl
from getpass import getpass
import hashlib
import hmac
import io
import json
import lzma
import os
from pathlib import Path
import re
import shutil
import stat
import subprocess
import sys
import tempfile

TARGETS = {
    "flowrix": ("154.219.105.161", "akuah", "/opt/akuah", "/var/www/akuah-maintenance"),
    "one": ("154.219.105.158", "florrclone", "/var/lib/florrclone", "/var/www/florrclone-maintenance"),
    "two": ("154.219.105.161", "florr-fresh", "/var/lib/florr-fresh", "/var/www/florr-fresh-maintenance"),
    "three": ("154.219.105.164", "florr-third", "/var/lib/florr-third", "/var/lib/florr-third/maintenance"),
}
MAX_BYTES = 256 * 1024**2
STAMP = "%Y%m%dT%H%M%S%fZ"
ARCHIVE_ROOT = Path("/var/lib/florrclone-backup")
DELETE_PASSWORD = Path("/etc/florrclone-backup/delete-password.json")
SNAPSHOT_NAME = re.compile(r"\d{8}T\d{12}Z")


class Maintenance(Exception):
    pass


def snapshot_guard(service, state, gate, source):
    if os.path.lexists(state + "/deploy.lock") or os.path.lexists(gate + "/enabled"):
        raise Maintenance
    output = subprocess.check_output(
        ["systemctl", "show", service, "-p", "ActiveState", "-p", "MainPID", "-p", "InvocationID"],
        text=True, timeout=5,
    )
    status = dict(line.split("=", 1) for line in output.splitlines())
    if status["ActiveState"] != "active" or int(status["MainPID"]) <= 0:
        raise ValueError("game service is inactive")
    proc = Path("/proc") / status["MainPID"]
    args = (proc / "cmdline").read_bytes().split(b"\0")
    # Verify the live process owns this particular path, not a retired cache.
    if (b"--db", os.fsencode(source)) not in zip(args, args[1:]):
        raise ValueError("game process uses another save path")
    if any(entry.startswith(b"FLIX_AUTHORITY_MODULE=") and entry.split(b"=", 1)[1]
           for entry in (proc / "environ").read_bytes().split(b"\0")):
        raise ValueError("game process uses shared storage")
    return status["MainPID"], status["InvocationID"]


def inventory_summary(raw):
    db = json.loads(raw)
    if not isinstance(db, dict) or any(not isinstance(db.get(k), dict) for k in ("users", "players")):
        raise ValueError("invalid inventory tables")
    catalog = db.get("customSkins", {})
    if not isinstance(catalog, dict):
        raise ValueError("invalid custom skin catalog")
    equipped = {
        player_id: player["equippedSkinId"]
        for player_id, player in db["players"].items()
        if isinstance(player, dict) and player.get("equippedSkinId")
    }
    skin_data = json.dumps([catalog, equipped], sort_keys=True, separators=(",", ":")).encode()
    return {
        "users": len(db["users"]), "players": len(db["players"]),
        "skins": {
            "custom": len(catalog), "equipped": len(equipped),
            "sha256": hashlib.sha256(skin_data).hexdigest(),
        },
    }


def source_path(role):
    state = TARGETS[role][2]
    relative = "/inventory.json" if role == "flowrix" else "/cpp-state/inventory.json"
    return state + relative


def export_snapshot(role):
    os.nice(8)
    _, service, state, gate = TARGETS[role]
    source = source_path(role)
    try:
        before = snapshot_guard(service, state, gate, source)
        # Game saves are published by atomic rename. Read and hash one open inode.
        with os.fdopen(os.open(source, os.O_RDONLY | os.O_NOFOLLOW), "rb") as stream:
            info = os.fstat(stream.fileno())
            if not stat.S_ISREG(info.st_mode) or not 0 < info.st_size <= MAX_BYTES:
                raise ValueError("invalid snapshot size or file type")
            raw = stream.read(MAX_BYTES + 1)
        if len(raw) != info.st_size:
            raise ValueError("snapshot size changed during read")
        summary = inventory_summary(raw)
        packed = lzma.compress(raw, preset=1)
        if snapshot_guard(service, state, gate, source) != before:
            raise ValueError("game service changed during snapshot")
    except Maintenance:
        print(json.dumps({"status": "skipped", "role": role, "reason": "maintenance"}))
        return
    metadata = {
        "status": "captured", "role": role, "source": source,
        "sourceModifiedUtc": datetime.fromtimestamp(info.st_mtime, timezone.utc).isoformat(),
        "bytes": len(raw), "sha256": hashlib.sha256(raw).hexdigest(), "encoding": "xz", **summary,
    }
    out = sys.stdout.buffer
    out.write(json.dumps(metadata).encode() + b"\n")
    out.write(packed)


def verify_snapshot(role, response):
    line, separator, packed = response.partition(b"\n")
    if not separator or len(line) > 4096:
        raise ValueError("missing snapshot header")
    info = json.loads(line)
    if info.get("role") != role:
        raise ValueError("unexpected backup source")
    if info.get("status") == "skipped" and info.get("reason") == "maintenance" and not packed:
        return info, None
    size = info.get("bytes")
    if (info.get("status") != "captured" or info.get("source") != source_path(role)
            or info.get("encoding") != "xz"
            or type(size) is not int or not 0 < size <= MAX_BYTES):
        raise ValueError("invalid snapshot metadata")
    with lzma.LZMAFile(io.BytesIO(packed)) as stream:
        raw = stream.read(size + 1)
    if len(raw) != size or hashlib.sha256(raw).hexdigest() != info.get("sha256"):
        raise ValueError("snapshot SHA256 or size mismatch")
    summary = inventory_summary(raw)
    if any(info.get(key) != summary[key] for key in ("users", "players")):
        raise ValueError("snapshot table count mismatch")
    if "skins" in info and info["skins"] != summary["skins"]:
        raise ValueError("snapshot skin metadata mismatch")
    # Older exporters still send the same complete database and file checksum.
    info["skins"] = summary["skins"]
    return info, packed


def write_durable(path, data):
    with path.open("xb") as stream:
        stream.write(data)
        stream.flush()
        os.fsync(stream.fileno())


def sync_directory(path):
    fd = os.open(path, os.O_RDONLY | os.O_DIRECTORY)
    try:
        os.fsync(fd)
    finally:
        os.close(fd)


def publish(root, info, packed):
    now = datetime.now(timezone.utc)
    destination = root / now.strftime(STAMP)
    root.mkdir(mode=0o700, parents=True, exist_ok=True)
    with tempfile.TemporaryDirectory(prefix=".incoming-", dir=root) as temp:
        stage = Path(temp) / "snapshot"
        stage.mkdir(mode=0o700)
        write_durable(stage / "inventory.json.xz", packed)
        info.update(status="verified", verified=True, collectedAtUtc=now.isoformat())
        write_durable(stage / "backup-metadata.json", (json.dumps(info, indent=2) + "\n").encode())
        sync_directory(stage)
        stage.rename(destination)
        sync_directory(root)
    return destination


def set_immutable(fd, enabled):
    # Linux FS_IOC_GETFLAGS and FS_IOC_SETFLAGS use a native long.
    flags = array.array("L", [0])
    ioctl_type = (flags.itemsize << 16) | (ord("f") << 8)
    fcntl.ioctl(fd, (2 << 30) | ioctl_type | 1, flags, True)
    immutable = 0x10  # FS_IMMUTABLE_FL
    wanted = flags[0] | immutable if enabled else flags[0] & ~immutable
    if wanted != flags[0]:
        flags[0] = wanted
        fcntl.ioctl(fd, (1 << 30) | ioctl_type | 2, flags, True)


def snapshot_members(path):
    if not SNAPSHOT_NAME.fullmatch(path.name) or path.is_symlink() or not path.is_dir():
        return None
    names = set(os.listdir(path))
    for extension in ("xz", "gz"):
        expected = {"backup-metadata.json", f"inventory.json.{extension}"}
        if names == expected:
            return sorted(expected)
    return None


def protect_snapshot(path, members, enabled):
    directory = os.open(path, os.O_RDONLY | os.O_DIRECTORY | os.O_NOFOLLOW)
    try:
        for name in members:
            fd = os.open(name, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK, dir_fd=directory)
            try:
                info = os.fstat(fd)
                if not stat.S_ISREG(info.st_mode) or info.st_nlink != 1:
                    raise ValueError("unexpected snapshot member")
                set_immutable(fd, enabled)
            finally:
                os.close(fd)
        set_immutable(directory, enabled)
    finally:
        os.close(directory)


def remove_snapshot(path, members):
    try:
        protect_snapshot(path, members, False)
        shutil.rmtree(path)
    except Exception:
        if path.exists():
            remaining = [name for name in members if (path / name).is_file()]
            protect_snapshot(path, remaining, True)
        raise


def prune(root):
    # Keep every capture for two days and the newest one per UTC day for 30 days.
    now = datetime.now(timezone.utc)
    daily = set()
    for child in sorted(root.iterdir(), reverse=True):
        members = snapshot_members(child)
        if members is None:
            continue
        protect_snapshot(child, members, True)
        captured = datetime.strptime(child.name, STAMP).replace(tzinfo=timezone.utc)
        age = now - captured
        if age >= timedelta(days=30) or (age >= timedelta(days=2) and captured.date() in daily):
            remove_snapshot(child, members)
        else:
            daily.add(captured.date())


def collect_one(role, root, key, known_hosts):
    host = TARGETS[role][0]
    result = subprocess.run(
        ["ssh", "-T", "-i", key, "-o", "IdentitiesOnly=yes", "-o", "BatchMode=yes",
         "-o", "StrictHostKeyChecking=yes", "-o", "GlobalKnownHostsFile=/dev/null",
         "-o", f"UserKnownHostsFile={known_hosts}", "-o", "ConnectTimeout=12",
         "-o", "ServerAliveInterval=10", "-o", "ServerAliveCountMax=2", f"root@{host}"]
        + (["export flowrix"] if role == "flowrix" else []),
        stdin=subprocess.DEVNULL, capture_output=True, timeout=255, check=False,
    )
    if result.returncode != 0:
        raise ValueError(f"SSH or export failed ({result.returncode})")
    info, packed = verify_snapshot(role, result.stdout)
    if packed is not None:
        directory = publish(root / role, info, packed)
        info["directory"] = str(directory)
    return info


def collect(root, key, known_hosts):
    root.mkdir(mode=0o700, parents=True, exist_ok=True)
    with (root / ".collect.lock").open("ab") as lock:
        try:
            fcntl.flock(lock, fcntl.LOCK_EX | fcntl.LOCK_NB)
        except BlockingIOError:
            print('{"status":"skipped","reason":"collector-already-running"}')
            return 0
        failed = False
        with ThreadPoolExecutor(max_workers=len(TARGETS)) as executor:
            jobs = {role: executor.submit(collect_one, role, root, key, known_hosts) for role in TARGETS}
            for role, job in jobs.items():
                try:
                    print(json.dumps(job.result()), flush=True)
                except Exception as exc:
                    # Avoid tracebacks and raw SSH output, which may contain private details.
                    reason = str(exc) if type(exc) is ValueError else type(exc).__name__
                    print(json.dumps({"status": "failed", "role": role, "reason": reason}), flush=True)
                    failed = True
        return int(failed)


def authorize_delete():
    if not sys.stdin.isatty():
        raise ValueError("manual deletion requires an interactive terminal")
    verifier = json.loads(DELETE_PASSWORD.read_bytes())
    password = getpass("Backup deletion password: ")
    digest = hashlib.scrypt(password.encode(), salt=bytes.fromhex(verifier["salt"]),
                            n=2**15, r=8, p=1, maxmem=64 * 1024**2)
    if not hmac.compare_digest(digest.hex(), verifier["hash"]):
        raise ValueError("incorrect deletion password")


def manage_archives(args):
    lock_path = ARCHIVE_ROOT / ".collect.lock"
    if args.command == "delete":
        authorize_delete()
    lock = os.open(lock_path, os.O_RDONLY | os.O_NOFOLLOW | os.O_NONBLOCK)
    try:
        if not stat.S_ISREG(os.fstat(lock).st_mode):
            raise ValueError("invalid collector lock")
        fcntl.flock(lock, fcntl.LOCK_EX)
        if args.command == "maintain":
            for role in TARGETS:
                root = ARCHIVE_ROOT / role
                if root.is_dir() and not root.is_symlink():
                    prune(root)
        else:
            if not SNAPSHOT_NAME.fullmatch(args.stamp):
                raise ValueError("invalid snapshot timestamp")
            root = ARCHIVE_ROOT / args.role
            if root.is_symlink():
                raise ValueError("invalid archive directory")
            path = root / args.stamp
            members = snapshot_members(path)
            if members is None:
                raise ValueError("snapshot does not exist or has an unexpected layout")
            remove_snapshot(path, members)
            print(f"Deleted {args.role}/{args.stamp}", flush=True)
    finally:
        os.close(lock)


def main():
    os.umask(0o077)
    parser = argparse.ArgumentParser(description=__doc__)
    commands = parser.add_subparsers(dest="command", required=True)
    export = commands.add_parser("export")
    export.add_argument("role", choices=TARGETS)
    commands.add_parser("export-from-two")
    pull = commands.add_parser("collect")
    pull.add_argument("root", type=Path)
    pull.add_argument("key")
    pull.add_argument("known_hosts")
    commands.add_parser("maintain")
    delete = commands.add_parser("delete")
    delete.add_argument("role", choices=TARGETS)
    delete.add_argument("stamp")
    args = parser.parse_args()
    if args.command in ("maintain", "delete"):
        try:
            manage_archives(args)
            return 0
        except (OSError, ValueError) as exc:
            print(f"Archive operation failed: {exc}", file=sys.stderr)
            return 1
    if args.command in ("export", "export-from-two"):
        if args.command == "export-from-two":
            request = os.environ.get("SSH_ORIGINAL_COMMAND", "")
            if request not in ("", "export flowrix"):
                parser.error("unsupported snapshot request")
            role = "flowrix" if request else "two"
        else:
            role = args.role
        try:
            export_snapshot(role)
            return 0
        except Exception as exc:
            print(f"Snapshot export failed ({type(exc).__name__})", file=sys.stderr)
            return 1
    return collect(args.root, args.key, args.known_hosts)


if __name__ == "__main__":
    sys.exit(main())
