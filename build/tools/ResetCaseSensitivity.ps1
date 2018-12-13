# Verify that case sensitivity forcing has not broken WSL mounting. The 
# firmware makefile will verify that each required folder is case sensitive.
# Offer to fix the registry automatically.
function FixCaseSensitivityPopup
{
    param ($badRegistry)

    $message1 = @"
Forced case sensitivity is on in WSL, but the DrvFsAllowForceCaseSensitivity registry value is not set correctly!
For details see https://blogs.msdn.microsoft.com/commandline/2018/02/28/per-directory-case-sensitivity-and-wsl/

Would you like to set the registry key automatically? (MUST RUN AS ADMINISTRATOR)
"@

    $message2 = @"
The DrvFsAllowForceCaseSensitivity registry value is already set.

Would you like to set the registry key again? (MUST RUN AS ADMINISTRATOR)
"@

    $title1 = "UpdateFirmware.ps1 ERROR: Case sensitivity check failed"
    $title2 = "UpdateFirmware.ps1 ERROR: Case sensitivity check passed"
    #seconds before defaulting to NO (won't freeze if the script is automated)
    $timeout = 1000
    # 0x4 = Yes/No buttons
    # 0x10 = Stop/error icon
    # 0x10000 = Set foreground
    $popupOptions = 0x4 -bor 0x10 -bor 0x10000
    
    $popup = New-Object -ComObject wscript.shell
    if ($badRegistry) {
        Write-Warning $message1
        $choice = $popup.popup($message1,$timeout,$title1,$popupOptions)
    } else {
        Write-Warning $message2
        $choice = $popup.popup($message2,$timeout,$title2,$popupOptions)
    }

    #6 is the "Yes" button return value
    #-1 is the timeout return value
    if($choice -eq 6) {
        return $true
    } elseif ($choice -eq -1) {
        Write-Warning "Case sensitivity auto fix popup timed out, run with -ForceWSLCaseSensitivity to skip popup."
        return $false
    } else {
        return $false
    }
}

# If bash is running then the changes to case sensitivity will not take effect.
function AlertAboutBashInstance
{
    $message = @"
Bash is already running, case sensitive Windows drives will not mount until all bash processes are closed.

Force kill all current bash proccesses, leave bash processes, or cancel command?
"@
    $title = "Error updating case sensitivity"
    #seconds before defaulting to NO (won't freeze if the script is automated)
    $timeout = 1000
    # 0x3 = Yes/No/Cancel buttons
    # 0x30 = Warning icon
    # 0x10000 = Set foreground
    # 0x200 = Default button three (Cancel)
    $popupOptions = 0x3 -bor 0x30 -bor 0x10000 -bor 0x200

    do
    {
        $bashProccessesRunning = Get-Process -Name bash -ErrorAction SilentlyContinue
        $isBashRunning = $bashProccessesRunning.Length -gt 0
        if($isBashRunning)
        {
            Write-Warning $message
            $bashProccessesRunning | Format-List | Out-String | Write-Host 
            $popup = New-Object -ComObject wscript.shell
            $choice = $popup.popup($message,$timeout,$title,$popupOptions)

            #7 is the "No" button
            #6 is the "Yes" button        
            #2 is the "Cancel" button
            #-1 is the timeout value
            if($choice -eq 6)
            {
                Stop-Process -Name bash
                sleep -Seconds 5
            }
            elseif($choice -eq 7)
            {
                Write-Warning "Bash processes are still running, mounts may fail until they are all stopped"
                return $true
            }
            elseif(($choice -eq 2) -or ($choice -eq -1))
            {
                return $false
            }
        }
    } while($isBashRunning)
    return $true
}

function CheckSuccess
{
    $mount = wsl mount
    return ($mount -like "*:\ on /mnt/*")
}

function VerifyCaseSensitivityValidMount
{
    Write-Host "Verifying WSL mount"

    $regPath = "HKLM:\SYSTEM\CurrentControlSet\Services\lxss\"
    $regName = "DrvFsAllowForceCaseSensitivity"
    $badMount = $false

    $confContents = wsl cat /etc/wsl.conf 2>&1
    if(($confContents -like "*case=force*")) {
        Write-Host ("Linux expects forced case sensitivity")

        if(-not (Test-Path $regPath)) {
            $badMount = $true
        } else {
            $key = Get-Item $regPath
            $value = $key.GetValue($regName, $null)
            if ($value -ne 1) {
                $badMount = $true
            }
        }

        Write-Host "Checking if Bash is running"
        $killBash = 
        if(-not (AlertAboutBashInstance))
        {
            Write-Error "Bash is still running, either the command was cancled, or the popup timed out."
            exit 1
        }

        Write-Host "Enabling registry key"
        if ( FixCaseSensitivityPopup($badMount) ) {
            if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) {
                Write-Error "Need to run as administrator to udpate registry"
                exit 1
            }

            Write-Host ("Creating registry path")
            if(-not (Test-Path $regPath)) {
                New-Item -Path $regPath -Force
            }
            New-ItemProperty -Path $regPath -Name $regName -Value 1 -Force -PropertyType DWORD

            sc stop lxssmanager
            sc start lxssmanager

        } else {
            Write-Error "WSL Case sensitivity check failed!"
            exit 1
        }
    } else {
        Write-Host "Everything seems to be configured properly!"
    }

    Write-Host "Verifying success..."
    Sleep -Seconds 1
    Write-Host "Mount not yet available, restarting can speed this up"
    while(-not( CheckSuccess ))
    {
        Write-Host "..."
        Sleep -Seconds 10
    }
    Write-Host "Ready!"
}

VerifyCaseSensitivityValidMount
