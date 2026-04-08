param(
    [string]$Uf2Path = ".\build\portable_demo.uf2"
)

$resolvedUf2 = (Resolve-Path $Uf2Path).Path
$volume = Get-Volume | Where-Object { $_.FileSystemLabel -eq 'RPI-RP2' } | Select-Object -First 1

if (-not $volume) {
    throw "Не найден накопитель RPI-RP2. Переведи плату в BOOTSEL/UF2 режим и попробуй снова."
}

$drive = "$($volume.DriveLetter):\"
$target = Join-Path $drive ([System.IO.Path]::GetFileName($resolvedUf2))
Copy-Item -Path $resolvedUf2 -Destination $target -Force
Write-Host "UF2 copied to $target"
