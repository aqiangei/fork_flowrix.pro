param(
    [string]$BackupDirectory = 'D:\ashflor\backups\beijing-recovery',
    [string]$PythonPath = (Get-Command python.exe -ErrorAction Stop).Source
)
$ErrorActionPreference = 'Stop'
$runner = Join-Path $BackupDirectory 'pull-beijing-recovery.py'
if (!(Test-Path -LiteralPath $runner) -or !(Test-Path -LiteralPath $PythonPath)) {
    throw 'The recovery script and Python must be installed before registration.'
}
$action = New-ScheduledTaskAction -Execute $PythonPath -Argument ('-I "' + $runner + '"') -WorkingDirectory $BackupDirectory
$triggers = @(
    (New-ScheduledTaskTrigger -AtStartup),
    (New-ScheduledTaskTrigger -Daily -At '03:30')
)
$settings = New-ScheduledTaskSettingsSet -StartWhenAvailable -MultipleInstances IgnoreNew -RestartCount 3 -RestartInterval (New-TimeSpan -Minutes 5) -ExecutionTimeLimit (New-TimeSpan -Minutes 10) -AllowStartIfOnBatteries -DontStopIfGoingOnBatteries
$principal = New-ScheduledTaskPrincipal -UserId 'SYSTEM' -LogonType ServiceAccount -RunLevel Highest
Register-ScheduledTask -TaskName 'Florr Beijing Recovery Backup' -Action $action -Trigger $triggers -Settings $settings -Principal $principal -Force | Out-Null
Start-ScheduledTask -TaskName 'Florr Beijing Recovery Backup'
