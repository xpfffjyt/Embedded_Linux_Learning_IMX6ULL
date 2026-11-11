#Requires -Version 5.1
<#
.SYNOPSIS
    Serial console probe / boot-log capture for the i.MX6ULL board on COM8.

.DESCRIPTION
    Opens the board's USB-UART port, dumps everything that arrives to both the
    console and a timestamped log file, and gives an actionable hint when the
    port is already held by another program (the most common failure).

.PARAMETER Port
    Serial port name. Default COM8.

    IMPORTANT (verified on this board): COM8 is the USB-GADGET console
    (ttyGS0, created by the g_multi module after Linux has booted), NOT the
    real debug UART. It disappears for ~20s on every reboot, so it can never
    show U-Boot or the kernel boot log.

    U-Boot and the kernel console (console=ttymxc0) go to UART1. That port
    only appears once the board's own USB-UART (CH340) connector, or an
    external USB-TTL adapter wired to the UART header, is plugged into the PC.
    Use this script with -Port COM8 for shell work, and with the CH340/TTL port
    when you need to capture a boot log or interrupt the autoboot countdown.

.PARAMETER Baud
    Baud rate. The i.MX6ULL console is always 115200 8N1.

.PARAMETER Seconds
    How long to capture. Use 30s and power-cycle the board to get a full boot log.

.PARAMETER OutFile
    Log path. Defaults to bootlog-<timestamp>.log in the current directory.

.PARAMETER Nudge
    Send a CR every 3s, so an already-booted board shows its shell prompt.

.EXAMPLE
    .\serial-log.ps1 -Seconds 30
    # power-cycle the board while this runs to capture the complete boot log

.EXAMPLE
    .\serial-log.ps1 -Seconds 8 -Nudge
    # board already up: wake the console and look for a login/shell prompt
#>
[CmdletBinding()]
param(
    [string]$Port    = 'COM8',
    [int]   $Baud    = 115200,
    [int]   $Seconds = 20,
    [string]$OutFile,
    [switch]$Nudge
)

$ErrorActionPreference = 'Stop'

if (-not $OutFile) {
    $OutFile = Join-Path (Get-Location) ("bootlog-{0}.log" -f (Get-Date -Format 'yyyyMMdd-HHmmss'))
}

# ---------------------------------------------------------------- open port
try {
    $sp = New-Object System.IO.Ports.SerialPort(
        $Port, $Baud, [System.IO.Ports.Parity]::None, 8, [System.IO.Ports.StopBits]::One)
    $sp.ReadTimeout  = 200
    $sp.WriteTimeout = 1000
    $sp.Open()
}
catch {
    $why = $_.Exception.InnerException.Message
    Write-Host ''
    Write-Host "[FAIL] cannot open $Port : $why" -ForegroundColor Red
    Write-Host ''
    Write-Host '  The port is most likely held by another program:'
    Write-Host '    serial assistant (XCOM/SSCOM), MobaXterm, Xshell,'
    Write-Host '    SecureCRT, VS Code Serial Monitor, PlatformIO, ...'
    Write-Host ''
    Write-Host "  Close it, then retry. To find the owner: Process Explorer -> Ctrl+F -> '$Port'."
    Write-Host ''
    exit 1
}

Write-Host ("[ OK ] {0} opened @ {1} 8N1" -f $Port, $Baud) -ForegroundColor Green
Write-Host ("       capturing {0}s -> {1}" -f $Seconds, $OutFile)
Write-Host  '       (power-cycle the board now to capture the full boot)'
Write-Host  ''

# ---------------------------------------------------------------- capture
$sp.DiscardInBuffer()
$sp.DtrEnable = $true
$sp.RtsEnable = $true

$sw        = [System.Diagnostics.Stopwatch]::StartNew()
$sb        = New-Object System.Text.StringBuilder
$lastNudge = 0.0

try {
    while ($sw.Elapsed.TotalSeconds -lt $Seconds) {
        if ($sp.BytesToRead -gt 0) {
            $chunk = $sp.ReadExisting()
            [void]$sb.Append($chunk)
            Write-Host -NoNewline $chunk
        }
        if ($Nudge -and (($sw.Elapsed.TotalSeconds - $lastNudge) -ge 3)) {
            $sp.Write("`r")
            $lastNudge = $sw.Elapsed.TotalSeconds
        }
        Start-Sleep -Milliseconds 40
    }
}
finally {
    $sp.Close()
    $sp.Dispose()
}

$text = $sb.ToString()
[System.IO.File]::WriteAllText($OutFile, $text, (New-Object System.Text.UTF8Encoding($false)))

Write-Host ''
Write-Host ''
Write-Host ("captured {0} bytes -> {1}" -f $text.Length, $OutFile)

if ($text.Length -eq 0) {
    Write-Host ''
    Write-Host 'Nothing received. Check in this order:' -ForegroundColor Yellow
    Write-Host '  1. baud rate must be 115200 8N1 (the i.MX6ULL console is fixed)'
    Write-Host '  2. the USB cable must carry data (charge-only cables are common)'
    Write-Host '  3. board powered? CH340 enumerating? (Get-PnpDevice -Class Ports)'
    Write-Host '  4. RX/TX reversed if you wired an external adapter'
}
