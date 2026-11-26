param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot,
    [Parameter(Mandatory)][string]$SurvivorFile
)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path -LiteralPath (Join-Path $GameRoot 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe')).Path
$original=(Resolve-Path -LiteralPath $SurvivorFile).Path
$qa=[IO.Path]::GetFullPath($QaRoot)
if(Test-Path -LiteralPath $qa){throw 'Select a fresh test folder for the checkpoint replay'}
$saves=Join-Path $qa 'Saved/SaveGames'
New-Item -ItemType Directory -Path $saves -Force | Out-Null
$before=(Get-FileHash -LiteralPath $original).Hash
Copy-Item -LiteralPath $original -Destination (Join-Path $saves 'EARTHtotaled-QA-BorerRecovery.sav')
$process=Start-Process -FilePath $exe -ArgumentList @('-ETBorerRecoveryQA','-ETInputAudit','-unattended','-nosound',('-UserDir="'+$qa+'"')) -WindowStyle Hidden -PassThru
if(!$process.WaitForExit(150000)){$process.Kill(); throw 'Isolated borer replay timed out'}
if((Get-FileHash -LiteralPath $original).Hash -ne $before){throw 'The source checkpoint changed during testing'}
$lines=Get-Content -LiteralPath (Join-Path $qa 'Saved/borer-recovery.txt')
$lines
if($process.ExitCode -ne 0 -or $lines[0] -notmatch '^ET_BORER \d+ checks / 0 failures$'){throw 'Borer recovery regression failed'}
