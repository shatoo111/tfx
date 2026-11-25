param([Parameter(Mandatory=$true)][string]$QtRoot)
$ErrorActionPreference = 'Stop'
Set-Location (Join-Path $PSScriptRoot '../..')
function Invoke-Checked([string]$Program, [string[]]$Arguments) {
    & $Program @Arguments
    if ($LASTEXITCODE -ne 0) { throw "$Program failed with exit code $LASTEXITCODE" }
}
Invoke-Checked cmake @('-S', '.', '-B', 'build/windows', '-G', 'Ninja', '-DCMAKE_BUILD_TYPE=Release', "-DCMAKE_PREFIX_PATH=$QtRoot")
Invoke-Checked cmake @('--build', 'build/windows', '--parallel')
Invoke-Checked ctest @('--test-dir', 'build/windows', '--output-on-failure')
Invoke-Checked cmake @('--install', 'build/windows', '--prefix', "$PWD/dist/windows")
Write-Host "Unsigned portable app: $PWD/dist/windows/tfx.exe"
