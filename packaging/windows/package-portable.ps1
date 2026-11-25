param(
    [Parameter(Mandatory = $true)][string]$InstallDirectory,
    [Parameter(Mandatory = $true)][string]$MinGWBin,
    [Parameter(Mandatory = $true)][string]$OutputFile,
    [string]$NoticesDirectory
)

$ErrorActionPreference = 'Stop'
$install = (Resolve-Path -LiteralPath $InstallDirectory).Path
$compiler = (Resolve-Path -LiteralPath $MinGWBin).Path
$output = $ExecutionContext.SessionState.Path.GetUnresolvedProviderPathFromPSPath($OutputFile)
$notices = if ($NoticesDirectory) { (Resolve-Path -LiteralPath $NoticesDirectory).Path }
$tempRoot = (Resolve-Path -LiteralPath ([IO.Path]::GetTempPath())).Path
$stage = Join-Path $tempRoot ("TelegramEffects-" + [guid]::NewGuid().ToString('N'))
$payload = Join-Path $stage 'app'
$payloadBin = Join-Path $payload 'bin'
$env:Path = "$compiler;$env:Path"

function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}

New-Item -ItemType Directory -Path $stage, $payload | Out-Null
Push-Location $stage
try {
    Copy-Item -LiteralPath (Join-Path $install 'bin') -Destination $payloadBin -Recurse -Container
    Copy-Item -LiteralPath (Join-Path $install 'tfx.exe') -Destination $payloadBin
    foreach ($directory in 'plugins', 'qml', 'translations') {
        Copy-Item -LiteralPath (Join-Path $install $directory) -Destination $payload -Recurse
    }
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../README.md') -Destination (Join-Path $payload 'Packaging-notes.md')
    if ($notices) {
        Copy-Item -LiteralPath $notices -Destination (Join-Path $payload 'licenses') -Recurse
    }

    Push-Location $payload
    try {
        $entries = @(Get-ChildItem -Name)
        Invoke-Checked cmake (@('-E', 'tar', 'czf', '../payload.tar.gz', '--format=gnutar', '--') + $entries)
    } finally {
        Pop-Location
    }

    $hash = (Get-FileHash payload.tar.gz -Algorithm SHA256).Hash.ToLowerInvariant()
    ('#define PAYLOAD_VERSION L"' + $hash + '"') | Set-Content -Encoding ascii 'payload-version.h'
    Copy-Item -LiteralPath (Join-Path $PSScriptRoot '../../assets/tfx-icon.ico') -Destination 'tfx-icon.ico'
    @('101 RCDATA "payload.tar.gz"', '1 ICON "tfx-icon.ico"') | Set-Content -Encoding ascii 'payload.rc'
    Invoke-Checked (Join-Path $compiler 'windres.exe') @('-i', 'payload.rc', '-O', 'coff', '-o', 'payload.res.o')
    Invoke-Checked (Join-Path $compiler 'g++.exe') @(
        '-std=c++20', '-O2', '-Wall', '-Wextra', '-DUNICODE', '-D_UNICODE',
        '-include', 'payload-version.h', '-static', '-municode', '-mwindows',
        (Join-Path $PSScriptRoot 'portable-launcher.cpp'), 'payload.res.o',
        '-lole32', '-lshell32', '-luuid', '-o', 'tfx.exe'
    )
    Copy-Item -LiteralPath 'tfx.exe' -Destination $output -Force
    Write-Host "Portable executable: $output"
    Write-Host "Payload SHA-256: $hash"
} finally {
    Pop-Location
    $resolvedStage = (Resolve-Path -LiteralPath $stage).Path
    if ($resolvedStage -eq $stage -and (Split-Path -Parent $resolvedStage) -eq $tempRoot) {
        Remove-Item -LiteralPath $resolvedStage -Recurse -Force
    }
}
