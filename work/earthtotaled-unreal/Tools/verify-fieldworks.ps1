param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot
)
$ErrorActionPreference='Stop'
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
$qa=[IO.Path]::GetFullPath($QaRoot)
if (!(Test-Path -LiteralPath $exe)) {throw 'Packaged game missing'}
if (Test-Path -LiteralPath (Join-Path $qa 'Saved/SaveGames/EARTHtotaled-QA-Fieldworks.sav')) {throw 'Use a new test folder for the first-run scenario'}
New-Item -ItemType Directory -Force -Path (Join-Path $qa 'Saved') | Out-Null
foreach ($pass in @('play','restart')) {
    $started=Get-Date
    $gameArgs=@('-ETFieldJourney','-ETInputAudit','-unattended','-nosound',('-UserDir="'+$qa+'"'))
    if ($pass -eq 'restart') {$gameArgs+='-ETFieldRead'}
    $process=Start-Process -FilePath $exe -ArgumentList $gameArgs -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(240000)) {$process.Kill(); throw "Fullscreen $pass check timed out"}
    $file=Join-Path $qa "Saved/fieldworks-$pass.txt"
    if (!(Test-Path -LiteralPath $file) -or (Get-Item -LiteralPath $file).LastWriteTime -lt $started) {throw "Missing $pass report"}
    $report=Get-Content -LiteralPath $file
    Write-Output $report
    if ($report[0] -notmatch '^ET_FIELD \d+ checks / 0 failures$' -or $process.ExitCode -ne 0) {throw "Fieldworks $pass check failed"}
}
