# Beijing save backups

The timer runs the collector every five minutes under the dedicated
florr-save-backup service account. The collector verifies the source, size,
SHA-256 and inventory structure before publishing an archive under
/var/lib/florrclone-backup/<role>/. It retains all snapshots for two days and
one per UTC day thereafter, up to 30 days.

Install florr-save-backup.py as /usr/local/libexec/florr-save-backup.py on the
collector and on the game host when updating its exporter. Install the two
systemd units on the collector. Provision its SSH credential and pinned host
keys separately at the paths in the service unit.

The two and flowrix sources share a host. Its authorized_keys entry for the
collector is restricted to the Beijing source IP and the forced command
/usr/bin/python3 /usr/local/libexec/florr-save-backup.py export-from-two.
The forced command accepts an empty request for two or the exact request
export flowrix. Other game hosts can keep their existing fixed export command.

After changing the configuration, start florr-save-backup.service once and
verify that its journal reports verified for flowrix, one, two and three.



For a Windows recovery copy, install export-beijing-recovery.py in
/usr/local/libexec/ on Beijing. Give the Windows task a dedicated SSH key
whose server-side entry uses restrict and forces that exporter. It needs only
the public key on Beijing and a verified Beijing host key in the task's
known_hosts file. Keep the task's private key and archive directory restricted
to the machine's recovery administrators and SYSTEM. The task key itself
should be owned and readable by SYSTEM.

Copy pull-beijing-recovery.py to the Windows backup directory alongside
id_ed25519 and known_hosts. Run register-windows-recovery.ps1 from an elevated
PowerShell. The task runs as SYSTEM at startup and daily at 03:30 local time.
The Windows copy holds the latest verified save for each of the four games
plus feedback and council data. Each run publishes a timestamped archive only
after verifying it. Previous Windows archives are retained.
