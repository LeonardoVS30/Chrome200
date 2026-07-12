param(
    [ValidateSet('Debug', 'Release')]
    [string]$Configuration = 'Release',
    [string]$JucePath = ''
)

$ErrorActionPreference = 'Stop'
$root = Split-Path -Parent $MyInvocation.MyCommand.Path
$cmakeCommand = Get-Command cmake -ErrorAction SilentlyContinue
if ($cmakeCommand) {
    $cmake = $cmakeCommand.Source
}
elseif (Test-Path -LiteralPath 'C:\Program Files\CMake\bin\cmake.exe') {
    $cmake = 'C:\Program Files\CMake\bin\cmake.exe'
}
else {
    throw 'CMake 3.24 or newer was not found. Install CMake or add it to PATH.'
}

Push-Location $root
try {
    if ($JucePath) {
        $JucePath = [System.IO.Path]::GetFullPath($JucePath)
        $juceCMakeFile = Join-Path $JucePath 'CMakeLists.txt'
        if (-not (Test-Path -LiteralPath $juceCMakeFile -PathType Leaf)) {
            throw "JUCE was not found at '$JucePath'. Pass the path to a real JUCE checkout, or omit -JucePath to use the bundled .deps/JUCE copy."
        }
    }

    # Always set the cache variable, including an empty value. This clears a stale
    # custom path when a later invocation goes back to the bundled JUCE checkout.
    $arguments = @('--preset', 'vs2022-x64', "-DCHROME2000_JUCE_PATH=$JucePath")
    & $cmake @arguments
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & $cmake --build --preset $Configuration.ToLowerInvariant()
    if ($LASTEXITCODE -ne 0) { throw 'Compilation failed.' }
    Write-Host "CHROME 2000 $Configuration build completed." -ForegroundColor Cyan
    Write-Host "Artifacts: $root\build\vs2022-x64\Chrome2000_artefacts\$Configuration"
}
finally { Pop-Location }
