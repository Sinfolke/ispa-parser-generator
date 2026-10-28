# Install: irm https://raw.githubusercontent.com/Sinfolke/ispa-parser-generator/main/scripts/install.ps1 | iex
$ErrorActionPreference = 'Stop'
$repo = 'Sinfolke/ispa-parser-generator'
$version = if ($env:ISPA_VERSION) { $env:ISPA_VERSION } else { 'latest' }
$platform = switch ([System.Runtime.InteropServices.RuntimeInformation]::OSArchitecture.ToString()) {
  'X64' { 'ispa-windows-x64' }
  default { throw 'ISPA currently has a Windows x64 release only.' }
}
$root = if ($env:ISPA_INSTALL_DIR) { $env:ISPA_INSTALL_DIR } else { Join-Path $env:LOCALAPPDATA 'Programs\ISPA' }
$bin = if ($env:ISPA_BIN_DIR) { $env:ISPA_BIN_DIR } else { Join-Path $root 'bin' }
$api = if ($version -eq 'latest') { "https://api.github.com/repos/$repo/releases/latest" } else { "https://api.github.com/repos/$repo/releases/tags/$version" }
$release = Invoke-RestMethod -Uri $api -Headers @{ 'User-Agent' = 'ISPA-installer'; 'Accept'='application/vnd.github+json' }
$tag = [string]$release.tag_name
if ($tag -notmatch '^[A-Za-z0-9][A-Za-z0-9._+-]*$') { throw "Invalid release tag '$tag'" }
$archive = "$platform.zip"
$asset = @($release.assets | Where-Object { $_.name -eq $archive }) | Select-Object -First 1
if (-not $asset) { throw "Asset $archive is not available for $tag (release may still be publishing)." }
$temp = Join-Path ([IO.Path]::GetTempPath()) ("ispa-install-" + [guid]::NewGuid().ToString('N'))
New-Item -ItemType Directory -Force -Path $temp | Out-Null
try {
  $zipPath = Join-Path $temp $archive
  Invoke-WebRequest -Uri $asset.browser_download_url -OutFile $zipPath
  $extractDir = Join-Path $temp 'unpacked'
  # Validate ZIP entry paths before Expand-Archive.
  Add-Type -AssemblyName System.IO.Compression
  $zip = [IO.Compression.ZipFile]::OpenRead($zipPath)
  try {
    foreach ($entry in $zip.Entries) {
      $name = $entry.FullName.Replace('\','/')
      if ($name.StartsWith('/') -or $name -match '^[A-Za-z]:' -or @($name.Split('/')) -contains '..') {
        throw "Unsafe ZIP entry: $name"
      }
    }
  } finally { $zip.Dispose() }
  Expand-Archive -LiteralPath $zipPath -DestinationPath $extractDir
  if (-not (Test-Path (Join-Path $extractDir 'ispa.exe'))) { throw 'Archive does not contain ispa.exe' }
  foreach ($file in @('ispa-pkg.exe', 'ispadbg.exe', 'ispa-ai.exe', 'install.ps1', 'update.ps1')) {
    if (-not (Test-Path (Join-Path $extractDir $file))) { throw "Archive does not contain $file" }
  }
  foreach ($tool in @('ispa.exe', 'ispa-pkg.exe', 'ispadbg.exe', 'ispa-ai.exe')) {
    & (Join-Path $extractDir $tool) --help *> $null
    if ($LASTEXITCODE -ne 0) { throw "$tool from release $tag failed its smoke check" }
  }
  $versions = Join-Path $root 'versions'
  $target = Join-Path $versions $tag
  New-Item -ItemType Directory -Force -Path $versions, $bin | Out-Null
  if (-not (Test-Path $target)) {
    Move-Item -LiteralPath $extractDir -Destination $target
  } else {
    foreach ($file in @('ispa-pkg.exe', 'ispadbg.exe', 'ispa-ai.exe')) {
      if (-not (Test-Path (Join-Path $target $file))) {
        Copy-Item -LiteralPath (Join-Path $extractDir $file) -Destination (Join-Path $target $file)
      }
    }
  }

  # Stable Windows launcher, forwarding arguments to the active release.
  $exe = Join-Path $target 'ispa.exe'
  $shim = Join-Path $bin 'ispa.cmd'
  "@echo off`r`n`"$exe`" %*`r`n" | Set-Content -LiteralPath $shim -Encoding ASCII
  $pkg = Join-Path $target 'ispa-pkg.exe'
  "@echo off`r`n`"$pkg`" %*`r`n" | Set-Content -LiteralPath (Join-Path $bin 'ispa-pkg.cmd') -Encoding ASCII
  $debugger = Join-Path $target 'ispadbg.exe'
  "@echo off`r`n`"$debugger`" %*`r`n" | Set-Content -LiteralPath (Join-Path $bin 'ispadbg.cmd') -Encoding ASCII
  $ai = Join-Path $target 'ispa-ai.exe'
  "@echo off`r`n`"$ai`" %*`r`n" | Set-Content -LiteralPath (Join-Path $bin 'ispa-ai.cmd') -Encoding ASCII
  $updater = Join-Path $target 'update.ps1'
  "@echo off`r`npowershell.exe -NoProfile -ExecutionPolicy Bypass -File `"$updater`" %*`r`n" | Set-Content -LiteralPath (Join-Path $bin 'ispa-update.cmd') -Encoding ASCII
  Set-Content -LiteralPath (Join-Path $root 'current-version') -Value $tag -NoNewline
  $userPath = [Environment]::GetEnvironmentVariable('Path', 'User')
  if (-not $userPath) { $userPath = '' }
  $entries = $userPath.Split(';', [StringSplitOptions]::RemoveEmptyEntries)
  if ($entries -notcontains $bin) {
    [Environment]::SetEnvironmentVariable('Path', (($entries + $bin) -join ';'), 'User')
  }
  if (($env:Path.Split(';')) -notcontains $bin) { $env:Path += ";$bin" }
  Write-Host "Installed ISPA $tag to $target"
  Write-Host 'Open a new terminal and run: ispa-ai --help'
} finally {
  Remove-Item -LiteralPath $temp -Recurse -Force -ErrorAction SilentlyContinue
}
