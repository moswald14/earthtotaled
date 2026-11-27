param([Parameter(Mandatory)][string]$GameRoot,[Parameter(Mandatory)][string]$QaRoot)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path (Join-Path $GameRoot 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe')).Path
$qa=[IO.Path]::GetFullPath($QaRoot)
if(Test-Path -LiteralPath $qa){throw 'Choose a fresh test folder for the rail survivor.'}
[IO.Directory]::CreateDirectory((Join-Path $qa 'Saved')) | Out-Null
$flags=@('-ETRingRide','-unattended','-windowed','-ResX=1600','-ResY=900','-nosound',('-UserDir="'+$qa+'"'))
function Run([string[]]$Options,[int]$Limit) {
    $game=Start-Process -FilePath $exe -ArgumentList ($Options+$flags) -WindowStyle Hidden -PassThru
    if(!$game.WaitForExit($Limit*1000)){ $game.Kill(); throw 'Rail ride exceeded its time limit.' }
    $report=@(Get-Content -LiteralPath (Join-Path $qa 'Saved/ring-ride.txt'))
    $report
    if($game.ExitCode -ne 0 -or $report[-1] -notmatch '^PASS '){ throw "Rail ride exited with $($game.ExitCode). $($report[-1])" }
}
Run @() 660
Copy-Item -LiteralPath (Join-Path $qa 'Saved/ring-ride.txt') -Destination (Join-Path $qa 'Saved/complete-lap.txt')
Copy-Item -LiteralPath (Join-Path $qa 'Saved/cruise-checkpoint.sav') -Destination (Join-Path $qa 'Saved/SaveGames/EARTHtotaled-Unreal-01.sav') -Force
Run @('-ETRingReturn') 110
