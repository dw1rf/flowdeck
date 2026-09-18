param(
    [string]$BuildDirectory = 'build',
    [string]$OutputDirectory = 'dist',
    [string]$PythonVersion = '3.12.10'
)

$ErrorActionPreference = 'Stop'
$root = (Resolve-Path (Join-Path $PSScriptRoot '..')).Path
$build = Join-Path $root $BuildDirectory
$output = Join-Path $root $OutputDirectory
$bundle = Join-Path $output 'FlowDeck-x64'
$exe = Join-Path $build 'Release/flowdeck.exe'
$nativePlugin = Join-Path $build 'plugins/example-cpp/Release/example_cpp.dll'

foreach ($required in @($exe, $nativePlugin)) {
    if (-not (Test-Path -LiteralPath $required)) {
        throw "Required build output is missing: $required"
    }
}

$versionParts = $PythonVersion.Split('.')
if ($versionParts.Length -ne 3 -or $versionParts[0] -ne '3' -or $versionParts[1] -ne '12') {
    throw 'This package script currently supports Python 3.12.x only.'
}

New-Item -ItemType Directory -Force -Path $bundle | Out-Null
New-Item -ItemType Directory -Force -Path (Join-Path $bundle 'plugins') | Out-Null
$embedZip = Join-Path $output "python-$PythonVersion-embed-amd64.zip"
$embedDirectory = Join-Path $output 'python-embed'
Invoke-WebRequest -Uri "https://www.python.org/ftp/python/$PythonVersion/python-$PythonVersion-embed-amd64.zip" -OutFile $embedZip
Expand-Archive -LiteralPath $embedZip -DestinationPath $embedDirectory -Force

Copy-Item -LiteralPath $exe -Destination $bundle
Copy-Item -LiteralPath (Join-Path $root 'LICENSE') -Destination $bundle
Copy-Item -LiteralPath (Join-Path $root 'README.md') -Destination $bundle
Copy-Item -LiteralPath (Join-Path $root 'plugins/example-hello') -Destination (Join-Path $bundle 'plugins/example-hello') -Recurse
Copy-Item -LiteralPath (Join-Path $root 'plugins/example-lua') -Destination (Join-Path $bundle 'plugins/example-lua') -Recurse
Copy-Item -LiteralPath $nativePlugin -Destination (Join-Path $bundle 'plugins')

Get-ChildItem -LiteralPath $embedDirectory -File |
    Where-Object { $_.Extension -ne '.exe' } |
    Copy-Item -Destination $bundle

$qtDir = 'C:/Qt/6.8.3/msvc2022_64'
$deploy = Join-Path $qtDir 'bin/windeployqt.exe'
if (-not (Test-Path -LiteralPath $deploy)) { throw "Qt deployment tool missing: $deploy" }
& $deploy --release --qmldir (Join-Path $root 'qml') --no-translations (Join-Path $bundle 'flowdeck.exe')
if ($LASTEXITCODE -ne 0) { throw 'Qt deployment failed' }

$pythonLicense = Join-Path $bundle 'LICENSE.txt'
if (Test-Path -LiteralPath $pythonLicense) {
    Move-Item -LiteralPath $pythonLicense -Destination (Join-Path $bundle 'PYTHON-LICENSE.txt') -Force
}

foreach ($required in @('flowdeck.exe', 'python312.dll', 'python312.zip', 'python312._pth', 'plugins/example_cpp.dll', 'plugins/example-hello/manifest.json')) {
    if (-not (Test-Path -LiteralPath (Join-Path $bundle $required))) {
        throw "Package is missing $required"
    }
}

$archive = Join-Path $output 'FlowDeck-windows-x64.zip'
Compress-Archive -LiteralPath $bundle -DestinationPath $archive -Force
Write-Host "Created $archive"
