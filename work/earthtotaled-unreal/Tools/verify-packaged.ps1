$ErrorActionPreference='Stop'
$projectRoot=Split-Path $PSScriptRoot
$gameRoot=[IO.Path]::GetFullPath((Join-Path $projectRoot '../../outputs/EARTHtotaled-Unreal-Windows/Windows'))
$qaRoot=[IO.Path]::GetFullPath((Join-Path $projectRoot '../../outputs/Unreal-QA'))
$exe=Join-Path $gameRoot 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
if(!(Test-Path -LiteralPath $exe)){throw 'Windows game has not been packaged'}
New-Item -ItemType Directory -Path (Join-Path $qaRoot 'Saved') -Force | Out-Null
$started=Get-Date
# Graphics checks require render data; Unreal intentionally discards it under -nullrhi.
$process=Start-Process -FilePath $exe -ArgumentList @('-ETSelfTest','-unattended','-nosound',('-UserDir="'+$qaRoot+'"')) -WindowStyle Normal -PassThru -Wait
$report=Join-Path $qaRoot 'Saved/native-verification.txt'
if($process.ExitCode -ne 0){if(Test-Path $report){Get-Content $report}; throw "Packaged verification failed with exit $($process.ExitCode)"}
if(!(Test-Path $report) -or (Get-Item $report).LastWriteTime -lt $started){throw 'No fresh verification report'}
$result=Get-Content -LiteralPath $report
if($result[0] -notmatch '^ET_VERIFY \d+ checks / 0 failures$'){Write-Output $result; throw 'Native integration checks failed'}
Write-Output $result
Get-FileHash -LiteralPath $exe -Algorithm SHA256
