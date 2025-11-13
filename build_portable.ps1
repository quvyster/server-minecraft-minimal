<#
PowerShell helper to build a portable server executable.
Steps:
 - ensure MSYS2 MinGW is on PATH (adjust path if installed elsewhere)
 - stop running server processes
 - run `make portable`
 - if static build fails, copy runtime DLLs next to dynamic build (fallback)
#>

Param(
    [string]$MsysPath = 'C:\msys64\mingw64\bin',
    [switch]$ForceCopyDLLs
)

Write-Host "MSYS2 MinGW path: $MsysPath"
if (Test-Path (Join-Path $MsysPath 'gcc.exe')) {
    $env:PATH = "$MsysPath;C:\msys64\usr\bin;" + $env:PATH
    Write-Host "Added MSYS2 to PATH"
} else {
    Write-Warning "MSYS2 MinGW not found at $MsysPath. Please install MSYS2 and mingw-w64 toolchain or set -MsysPath"
}

# stop server if running
Write-Host "Stopping any running server processes..."
Get-Process -ErrorAction SilentlyContinue -Name server-portable,server | ForEach-Object {
    Write-Host "Stopping PID $($_.Id) ($($_.ProcessName))"
    Stop-Process -Id $_.Id -Force -ErrorAction SilentlyContinue
}

# run make portable
Write-Host "Running make portable..."
$make = 'make'
$proc = Start-Process -FilePath $make -ArgumentList 'portable' -NoNewWindow -Wait -PassThru -ErrorAction SilentlyContinue
if ($proc -and $proc.ExitCode -eq 0) {
    Write-Host "Portable build succeeded: build\server-portable.exe"
    exit 0
}

Write-Warning "Static portable build failed (see make output above)."
if (-not (Test-Path 'build')) { New-Item -ItemType Directory -Path build | Out-Null }

# fallback: copy DLLs
$dlls = @('libwinpthread-1.dll','libgcc_s_seh-1.dll','libstdc++-6.dll')
if ($ForceCopyDLLs -or (Read-Host "Copy runtime DLLs from $MsysPath to build\? (y/N)") -match '^[yY]') {
    foreach ($d in $dlls) {
        $src = Join-Path $MsysPath $d
        if (Test-Path $src) {
            Copy-Item $src -Destination build -Force
            Write-Host "Copied $d"
        } else {
            Write-Warning "Not found: $src"
        }
    }
    if (Test-Path 'build\server.exe') {
        Write-Host "Copied DLLs. Try running: .\build\server.exe"
        exit 0
    } else {
        Write-Warning "Dynamic build not found (build\server.exe). Cannot complete fallback."
        exit 1
    }
} else {
    Write-Host "Skipping DLL copy. Exiting."
    exit 1
}
