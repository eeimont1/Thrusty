param([string]$App = 'bin/Thrusty.exe', [string]$Installer = '')
$ErrorActionPreference = 'Stop'
function Invoke-Checked([string]$File, [string]$Arguments) {
    $p = Start-Process -FilePath (Resolve-Path $File) -ArgumentList $Arguments -PassThru
    if (-not $p.WaitForExit(30000)) {
        $p.Kill()
        throw "Timed out: $File $Arguments"
    }
    if ($p.ExitCode -ne 0) { throw "$File failed with exit code $($p.ExitCode)" }
}
Invoke-Checked $App '--self-test'
Invoke-Checked $App '--ui-smoke'
if ($Installer) {
    $location = Join-Path $env:TEMP 'Thrusty-CI-Install'
    Invoke-Checked $Installer "/S /D=$location"
    $installed = Join-Path $location 'Thrusty.exe'
    if (-not (Test-Path $installed)) { throw 'Installer did not place Thrusty.exe' }
    $expected = (Get-Content VERSION -Raw).Trim()
    $actual = (Get-Item $installed).VersionInfo.ProductVersion
    if ($actual -ne $expected) { throw "Version mismatch: $actual != $expected" }
    $registration = Get-ItemProperty 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty'
    if ($registration.DisplayVersion -ne $expected) { throw 'Uninstall registration version mismatch' }
    Invoke-Checked $installed '--self-test'
    Invoke-Checked $installed '--ui-smoke'
    # NSIS may spawn its temporary child; allow a bounded completion interval.
    $uninstaller = Join-Path $location 'Uninstall.exe'
    Invoke-Checked $uninstaller '/S'
    for ($i=0; $i -lt 40 -and (Test-Path $installed); $i++) { Start-Sleep -Milliseconds 250 }
    if (Test-Path $installed) { throw 'Uninstall did not remove the app' }
    if (Test-Path 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Uninstall\Thrusty') {
        throw 'Uninstall registration remains'
    }
}
Write-Host 'Windows native and UI startup smoke checks passed. No physical wheel tested.'
