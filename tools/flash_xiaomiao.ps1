[CmdletBinding()]
param(
    [string]$Port = "COM8",
    [switch]$BackupFirst,
    [switch]$LauncherOnly,
    [switch]$Help
)

$ErrorActionPreference = "Stop"
$root = Split-Path -Parent $PSScriptRoot

if ($Help) {
    Write-Host "小猫 Retro-Go 一键刷机"
    Write-Host "用法：powershell -ExecutionPolicy Bypass -File .\\tools\\flash_xiaomiao.ps1 [-Port COM8] [-BackupFirst] [-LauncherOnly]"
    Write-Host "  -BackupFirst  刷写前备份 4MB Flash"
    Write-Host "  -LauncherOnly 只构建/刷写 launcher"
    exit 0
}
Push-Location $root
try {
    $idf = Join-Path $env:USERPROFILE ".platformio\packages\framework-espidf@3.50301.0"
    $idfPython = Join-Path $env:USERPROFILE ".espressif\python_env\idf5.3_py3.12_env\Scripts\python.exe"
    $xtensa = Join-Path $env:USERPROFILE ".espressif\tools\xtensa-esp-elf\esp-13.2.0_20240530\xtensa-esp-elf\bin"
    $ninja = Join-Path $env:USERPROFILE ".platformio\packages\tool-ninja"
    $cmake = Join-Path $env:USERPROFILE ".platformio\packages\tool-cmake\bin"

    if (-not (Test-Path (Join-Path $idf "tools\idf.py"))) {
        throw "找不到 ESP-IDF：$idf。请确认 PlatformIO 的 ESP-IDF 5.3 已安装。"
    }
    if (-not (Test-Path $idfPython)) {
        throw "找不到 ESP-IDF Python：$idfPython。"
    }

    $env:IDF_PATH = $idf
    $env:IDF_TOOLS_PATH = Join-Path $env:USERPROFILE ".espressif"
    $env:IDF_TARGET = "esp32"
    $env:Path = "$([IO.Path]::GetDirectoryName($idfPython));$xtensa;$ninja;$cmake;$env:Path"

    Write-Host "IDF_PATH = $env:IDF_PATH" -ForegroundColor DarkGray
    Write-Host "使用串口：$Port" -ForegroundColor Cyan

    if ($BackupFirst) {
        $stamp = Get-Date -Format "yyyyMMdd_HHmmss"
        $evidence = Join-Path $root "tmp\flash-backups"
        New-Item -ItemType Directory -Force -Path $evidence | Out-Null
        $backup = Join-Path $evidence "xiaomiao_before_retro_go_$stamp.bin"
        Write-Host "正在备份完整 4MB Flash，可能需要几分钟..." -ForegroundColor Yellow
        & $idfPython -m esptool --chip esp32 --port $Port --baud 460800 read_flash 0x0 0x400000 $backup
        if ($LASTEXITCODE -ne 0) { throw "Flash 备份失败，已停止刷写。" }
        Write-Host "备份完成：$backup" -ForegroundColor Green
    }

    if ($LauncherOnly) {
        & $idfPython rg_tool.py --target xiaomiao --port $Port flash launcher
    } else {
        & $idfPython rg_tool.py --target xiaomiao --port $Port install
    }
    if ($LASTEXITCODE -ne 0) { throw "构建或刷写失败。" }

    Write-Host "小猫 Retro-Go 刷写完成，设备已自动复位。" -ForegroundColor Green
    Write-Host "串口日志：$idfPython -m idf_monitor --port $Port --baud 115200"
}
finally {
    Pop-Location
}
