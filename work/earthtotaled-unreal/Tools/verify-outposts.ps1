param([Parameter(Mandatory)][string]$GameRoot,[Parameter(Mandatory)][string]$QaRoot)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($QaRoot)
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
if(Test-Path -LiteralPath (Join-Path $root 'Saved/SaveGames/EARTHtotaled-QA-Slot-03.sav')) {throw 'Select a fresh, isolated test directory.'}
New-Item -ItemType Directory -Path $root -Force | Out-Null
$portable=Join-Path $root 'Saved/Transfer survivor.etsave'
Add-Type -TypeDefinition @'
using System;
using System.Text;
using System.Runtime.InteropServices;
public static class ETNativeTransfer {
    private delegate bool EachWindow(IntPtr window, IntPtr state);
    [DllImport("user32.dll")] private static extern bool EnumWindows(EachWindow visit, IntPtr state);
    [DllImport("user32.dll")] private static extern uint GetWindowThreadProcessId(IntPtr window, out uint process);
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] private static extern int GetWindowText(IntPtr window, StringBuilder text, int size);
    [DllImport("user32.dll")] private static extern IntPtr GetDlgItem(IntPtr window, int id);
    [DllImport("user32.dll")] private static extern bool IsWindowVisible(IntPtr window);
    [DllImport("user32.dll")] private static extern bool IsWindowEnabled(IntPtr window);
    [DllImport("user32.dll")] private static extern bool PostMessage(IntPtr window, uint code, IntPtr wParam, IntPtr lParam);
    private static string Caption(IntPtr window) {
        var text=new StringBuilder(256); GetWindowText(window,text,text.Capacity); return text.ToString();
    }
    public static bool Confirm(uint pid,string title,string button) {
        bool sent=false;
        EnumWindows((window,state)=>{
            uint owner; GetWindowThreadProcessId(window,out owner);
            if(owner!=pid || Caption(window)!=title || !IsWindowVisible(window) || !IsWindowEnabled(window)) return true;
            var accept=GetDlgItem(window,1);
            if(accept!=IntPtr.Zero && IsWindowEnabled(accept) && Caption(accept).Replace("&","")==button)
                sent=PostMessage(window,0x0111,new IntPtr(1),accept);
            return !sent;
        },IntPtr.Zero);
        return sent;
    }
}
'@
foreach($pass in @('play','reopen')){
    $gameArgs=@('-ETSlotQA','-ETInputAudit','-unattended','-nosound',('-UserDir="'+$root+'"'))
    if($pass -eq 'reopen'){$gameArgs+='-ETSlotCold'}
    $process=Start-Process -FilePath $exe -ArgumentList $gameArgs -WindowStyle Hidden -PassThru
    $expires=(Get-Date).AddMinutes(5)
    $exported=$false
    $imported=$false
    $nextDialogTry=[DateTime]::UtcNow.AddSeconds(1)
    while(!$process.HasExited -and (Get-Date) -lt $expires) {
        if($pass -eq 'play' -and [DateTime]::UtcNow -ge $nextDialogTry) {
            $nextDialogTry=[DateTime]::UtcNow.AddSeconds(1.5)
            # A delivered click can precede the shell's folder load. Wait for the
            # exported file before moving to Open, retrying only the same dialog.
            if(!(Test-Path -LiteralPath $portable)) {
                if([ETNativeTransfer]::Confirm($process.Id,'Export EARTHtotaled survivor','Save')) {$exported=$true}
            } elseif([ETNativeTransfer]::Confirm($process.Id,'Import EARTHtotaled survivor','Open')) {$imported=$true}
        }
        Start-Sleep -Milliseconds 220
        $process.Refresh()
    }
    if(!$process.HasExited){$process.Kill(); throw "The isolated $pass test timed out. Export accepted $exported. Import accepted $imported."}
    $lines=Get-Content -LiteralPath (Join-Path $root "Saved/outposts-$pass.txt")
    Write-Output $lines
    if($pass -eq 'play' -and !(Test-Path -LiteralPath $portable)){throw 'Expected transfer file missing from isolated folder'}
    if($lines[0] -notmatch '^ET_OUTPOST \d+ checks / 0 failures$' -or $process.ExitCode -ne 0) {throw "Failed $pass test"}
}

