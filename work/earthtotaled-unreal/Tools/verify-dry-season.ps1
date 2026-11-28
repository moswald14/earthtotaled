param([Parameter(Mandatory)][string]$GameRoot,[Parameter(Mandatory)][string]$QaRoot,[string]$LegacySlot)
$ErrorActionPreference='Stop'
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
$base=[IO.Path]::GetFullPath($QaRoot)
if(!(Test-Path -LiteralPath $exe)){throw 'Packaged game is missing.'}
function Invoke-FieldCheck([string]$phase,[string]$profile) {
    [IO.Directory]::CreateDirectory((Join-Path $profile 'Saved')) | Out-Null
    $report=Join-Path $profile 'Saved/dry-season-checks.txt'
    $start=Get-Date
    $process=Start-Process -FilePath $exe -ArgumentList @(('-ETWakeTest='+$phase),'-nullrhi','-unattended','-nosound',('-UserDir="'+$profile+'"')) -WindowStyle Hidden -PassThru
    if(!$process.WaitForExit(210000)){$process.Kill();throw "Field check timed out in $phase"}
    if(!(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTime -lt $start){throw "No fresh $phase report"}
    Copy-Item -LiteralPath $report -Destination (Join-Path $base ($phase+'.txt'))
    $lines=Get-Content -LiteralPath $report
    $lines | Where-Object {$_ -match '^(FAIL|TOTAL)'}
    if($process.ExitCode -ne 0 -or $lines[-1] -ne 'TOTAL 0 failures'){throw "Field check failed in $phase"}
}
$fresh=Join-Path $base ('fresh-'+[guid]::NewGuid().ToString('N'))
if($LegacySlot){
    $profile=Join-Path $base ('legacy-'+[guid]::NewGuid().ToString('N'))
    $slots=Join-Path $profile 'Saved/SaveGames';[IO.Directory]::CreateDirectory($slots) | Out-Null
    Copy-Item -LiteralPath $LegacySlot -Destination (Join-Path $slots 'EARTHtotaled-Unreal-01.sav')
    Invoke-FieldCheck 'dry-legacy' $profile
}
Invoke-FieldCheck 'dry-season' $fresh
Invoke-FieldCheck 'dry-resume' $fresh
