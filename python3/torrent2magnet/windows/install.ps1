# PowerShell script to install torrent2magnet context menu for Windows
# Configured to use SingleInstance.exe to handle multiple file selections
# Run this script as Administrator

param(
    [switch]$Uninstall,
    [switch]$Test
)

$ErrorActionPreference = "Stop"

# --- Configuration ---
$ContextMenuName = "Convert to Magnet Link"
$FileTypes = @(".torrent")
$WrapperPath = Join-Path $PSScriptRoot "wrapper.bat"
$SingleInstancePath = Join-Path $PSScriptRoot "SingleInstance.exe"

function Test-Admin {
    $currentPrincipal = New-Object Security.Principal.WindowsPrincipal([Security.Principal.WindowsIdentity]::GetCurrent())
    return $currentPrincipal.IsInRole([Security.Principal.WindowsBuiltInRole]::Administrator)
}

function Write-Color {
    param([string]$Message, [string]$Color = "White")
    Write-Host $Message -ForegroundColor $Color
}

function Test-Torrent2Magnet {
    Write-Color "Testing torrent2magnet installation..." -Color "Yellow"
    try {
        $command = Get-Command torrent2magnet.exe -ErrorAction Stop
        Write-Color " torrent2magnet.exe found: $($command.Source)" -Color "Green"
        return $true
    }
    catch {
        Write-Color " torrent2magnet.exe not found in PATH." -Color "Red"
        return $false
    }
}

function Test-Dependencies {
    $allFound = $true
    Write-Color "Checking for dependencies..." -Color "Yellow"
    
    if (Test-Path $WrapperPath) {
        Write-Color " [OK] wrapper.bat found" -Color "Green"
    } else {
        Write-Color " [FAIL] wrapper.bat not found in $PSScriptRoot" -Color "Red"
        $allFound = $false
    }

    # Check this https://www.reddit.com/r/Batch/comments/18u720z/comment/kfju7su/?utm_source=share&utm_medium=web3x&utm_name=web3xcss&utm_term=1&utm_content=share_button
    if (Test-Path $SingleInstancePath) {
        Write-Color " [OK] SingleInstance.exe found" -Color "Green"
    } else {
        Write-Color " [FAIL] SingleInstance.exe not found in $PSScriptRoot" -Color "Red"
        $allFound = $false
    }
    
    return $allFound
}

function Install-ContextMenu {
    Write-Color "Installing torrent2magnet context menu with SingleInstance..." -Color "Yellow"

    foreach ($ext in $FileTypes) {
        $regBase = if (Test-Admin) { "HKLM:\Software\Classes" } else { "HKCU:\Software\Classes" }
        $regPath = "$regBase\SystemFileAssociations\$ext\shell\$ContextMenuName"

        # Create registry entries
        New-Item -Path $regPath -Force | Out-Null
        New-ItemProperty -Path $regPath -Name "MUIVerb" -Value $ContextMenuName -Force | Out-Null
        New-ItemProperty -Path $regPath -Name "Icon" -Value "torrent2magnet.exe,0" -Force | Out-Null
        
        # Command entry
        $commandPath = "$regPath\command"
        New-Item -Path $commandPath -Force | Out-Null

        # Syntax: SingleInstance.exe "%1" "cmd.exe" /c "wrapper.bat" "$files"
        # Using SingleInstance allows us to gather multiple right-clicks into one process call
        $fullCommand = "`"$SingleInstancePath`" `"%1`" `"$WrapperPath`" `"`$files`""
        
        New-ItemProperty -Path $commandPath -Name "(Default)" -Value $fullCommand -Force | Out-Null

        Write-Color " Registered context menu for $ext files (SingleInstance mode)" -Color "Green"
    }

    Write-Color "`n Installation complete!" -Color "Green"
    return $true
}

function Uninstall-ContextMenu {
    Write-Color "Uninstalling torrent2magnet context menu..." -Color "Yellow"
    foreach ($ext in $FileTypes) {
        $regPaths = @(
            "HKCU:\Software\Classes\SystemFileAssociations\$ext\shell\$ContextMenuName",
            "HKLM:\Software\Classes\SystemFileAssociations\$ext\shell\$ContextMenuName"
        )
        foreach ($regPath in $regPaths) {
            if (Test-Path $regPath) {
                Remove-Item -Path $regPath -Recurse -Force
                Write-Color " Removed $regPath" -Color "Green"
            }
        }
    }
}

# --- Main execution ---
Write-Color "`n=== torrent2magnet Context Menu Installer ===" -Color "Cyan"

if ($Test) {
    Test-Torrent2Magnet
    Test-Dependencies
    exit
}

if ($Uninstall) {
    Uninstall-ContextMenu
} elseif ((Test-Torrent2Magnet) -and (Test-Dependencies)) {
    Install-ContextMenu
} else {
    Write-Color "`n Installation aborted. Requirements not met." -Color "Red"
    exit 1
}