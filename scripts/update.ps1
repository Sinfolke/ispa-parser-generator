param([switch]$Install)
$ErrorActionPreference = 'Stop'
$ProgressPreference = 'SilentlyContinue'

$repo = if ($env:ISPA_REPOSITORY) { $env:ISPA_REPOSITORY } else { 'Sinfolke/ispa-parser-generator' }
$root = if ($env:ISPA_INSTALL_DIR) { $env:ISPA_INSTALL_DIR } else { Join-Path $env:LOCALAPPDATA 'Programs\ISPA' }
$bin = if ($env:ISPA_BIN_DIR) { $env:ISPA_BIN_DIR } else { Join-Path $root 'bin' }
if ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture -ne [System.Runtime.InteropServices.Architecture]::X64) {
    throw 'ISPA currently publishes Windows x64 binaries only.'
}

function Parse-VersionTag([string]$tag) {
    if ($tag -notmatch '^v?(\d+(?:\.\d+){0,3})(?:-([0-9A-Za-z.-]+))?$') {
        throw "Invalid version tag: $tag"
    }
    $components = @($Matches[1].Split('.'))
    while ($components.Count -lt 4) { $components += '0' }
    return @{ Numeric = [version]($components -join '.'); Prerelease = [string]$Matches[2] }
}
function Is-Newer([string]$candidate, [string]$installed) {
    $a = Parse-VersionTag $candidate
    $b = Parse-VersionTag $installed
    if ($a.Numeric -gt $b.Numeric) { return $true }
    if ($a.Numeric -lt $b.Numeric) { return $false }
    if (-not $a.Prerelease -and $b.Prerelease) { return $true }
    if ($a.Prerelease -and -not $b.Prerelease) { return $false }
    return ([string]::CompareOrdinal($a.Prerelease, $b.Prerelease) -gt 0)
}

$release = Invoke-RestMethod -Uri "https://api.github.com/repos/$repo/releases/latest" -Headers @{'Accept'='application/vnd.github+json'; 'User-Agent'='ispa-update'}
$latest = [string]$release.tag_name
[void](Parse-VersionTag $latest)
$versionFile = Join-Path $root 'current-version'
$current = if (Test-Path -LiteralPath $versionFile) { (Get-Content -LiteralPath $versionFile -Raw).Trim() } else { '' }
if ($current -eq $latest) {
    Write-Host "ISPA is up to date ($current)."
    return
}
if ($current -and -not (Is-Newer $latest $current)) {
    Write-Host "Installed ISPA $current is already newer than or equal to latest stable release $latest."
    return
}

Write-Host "Installing ISPA $latest (previous: $current)"
$asset = 'ispa-windows-x64.zip'
$url = "https://github.com/$repo/releases/download/$latest/$asset"
$temp = Join-Path ([IO.Path]::GetTempPath()) "ispa-update-$([Guid]::NewGuid().ToString('N'))"
New-Item -ItemType Directory -Path $temp -Force | Out-Null
try {
    $archive = Join-Path $temp $asset
    $extracted = Join-Path $temp 'extracted'
    Invoke-WebRequest -Uri $url -OutFile $archive
    Add-Type -AssemblyName System.IO.Compression
    $zip = [IO.Compression.ZipFile]::OpenRead($archive)
    try {
        foreach ($entry in $zip.Entries) {
            $name = $entry.FullName.Replace('\', '/')
            if ($name.StartsWith('/') -or $name -match '^[A-Za-z]:' -or @($name.Split('/')) -contains '..') {
                throw "Unsafe ZIP entry: $name"
            }
        }
    } finally { $zip.Dispose() }
    Expand-Archive -LiteralPath $archive -DestinationPath $extracted
    if (-not (Test-Path -LiteralPath (Join-Path $extracted 'ispa.exe') -PathType Leaf)) {
        throw 'Release archive does not contain ispa.exe at its root.'
    }

    # Install the updater in the same version directory as the executable.
    $self = $MyInvocation.MyCommand.Path
    if (-not $self -or -not (Test-Path -LiteralPath $self -PathType Leaf)) {
        throw 'Run update.ps1 from a saved script file.'
    }
    foreach ($file in @('ispa-pkg.exe', 'ispadbg.exe', 'update.ps1', 'install.ps1')) {
        if (-not (Test-Path (Join-Path $extracted $file))) { throw "Release archive does not contain $file" }
    }
    Set-Content -LiteralPath (Join-Path $extracted 'INSTALL_VERSION') -Value $latest

    $versions = Join-Path $root 'versions'
    New-Item -ItemType Directory -Path $versions -Force | Out-Null
    $destination = Join-Path $versions $latest
    if (-not (Test-Path -LiteralPath $destination)) {
        Move-Item -LiteralPath $extracted -Destination $destination
    } elseif (-not ((Test-Path (Join-Path $destination 'ispa.exe')) -and (Test-Path (Join-Path $destination 'update.ps1')))) {
        throw "Incomplete existing installation: $destination"
    }
    if (-not (Test-Path (Join-Path $destination 'ispa-pkg.exe'))) {
        Copy-Item -LiteralPath (Join-Path $extracted 'ispa-pkg.exe') -Destination (Join-Path $destination 'ispa-pkg.exe')
    }
    if (-not (Test-Path (Join-Path $destination 'ispadbg.exe'))) {
        Copy-Item -LiteralPath (Join-Path $extracted 'ispadbg.exe') -Destination (Join-Path $destination 'ispadbg.exe')
    }

    New-Item -ItemType Directory -Path $bin -Force | Out-Null
    # Generate robust wrappers for custom installation locations.
    $escapedRoot = $root.Replace('%','%%')
    $ispaLauncher = "@echo off`r`nsetlocal`r`nset /p ISPA_VERSION=<`"$escapedRoot\current-version`"`r`n`"$escapedRoot\versions\%ISPA_VERSION%\ispa.exe`" %*`r`n"
    $updateLauncher = "@echo off`r`nsetlocal`r`nset /p ISPA_VERSION=<`"$escapedRoot\current-version`"`r`npowershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$escapedRoot\versions\%ISPA_VERSION%\update.ps1`" %*`r`n"
    Set-Content -LiteralPath (Join-Path $bin 'ispa.cmd') -Value $ispaLauncher -Encoding Ascii
    Set-Content -LiteralPath (Join-Path $bin 'ispa-update.cmd') -Value $updateLauncher -Encoding Ascii
    $pkgLauncher = "@echo off`r`nsetlocal`r`nset /p ISPA_VERSION=<`"$escapedRoot\current-version`"`r`n`"$escapedRoot\versions\%ISPA_VERSION%\ispa-pkg.exe`" %*`r`n"
    Set-Content -LiteralPath (Join-Path $bin 'ispa-pkg.cmd') -Value $pkgLauncher -Encoding Ascii
    $debuggerLauncher = "@echo off`r`nsetlocal`r`nset /p ISPA_VERSION=<`"$escapedRoot\current-version`"`r`n`"$escapedRoot\versions\%ISPA_VERSION%\ispadbg.exe`" %*`r`n"
    Set-Content -LiteralPath (Join-Path $bin 'ispadbg.cmd') -Value $debuggerLauncher -Encoding Ascii
    Set-Content -LiteralPath $versionFile -Value $latest -NoNewline

    $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
    $entries = @($userPath -split ';' | Where-Object { $_ })
    if ($entries -notcontains $bin) {
        [Environment]::SetEnvironmentVariable('Path', (($entries + $bin) -join ';'), 'User')
        Write-Host "Added $bin to user PATH. Open a new terminal."
    }
    Write-Host "Installed ISPA $latest. Commands: ispa, ispa-pkg, ispadbg, ispa-update"
}
finally {
    Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
