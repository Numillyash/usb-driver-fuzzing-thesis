$ErrorActionPreference = "Stop"

$RepoRoot = Split-Path $PSScriptRoot -Parent
$Uf2Path = Join-Path $RepoRoot "build\portable_demo.uf2"
$resolvedUf2 = (Resolve-Path $Uf2Path).Path

$target = Get-Volume | Where-Object FileSystemLabel -eq "RPI-RP2" | Select-Object -First 1
if (-not $target) {
    throw "RPI-RP2 drive not found. Put the board into BOOTSEL mode first."
}

$dest = ($target.DriveLetter + ":\")
Copy-Item -Path $resolvedUf2 -Destination $dest -Force
Write-Host "UF2 copied to $dest"