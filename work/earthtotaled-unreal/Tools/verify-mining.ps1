param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot
)
$ErrorActionPreference='Stop'
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
$root=[IO.Path]::GetFullPath($QaRoot)
if (!(Test-Path -LiteralPath $exe)) {throw 'Packaged game missing'}
New-Item -ItemType Directory -Force -Path (Join-Path $root 'Saved') | Out-Null
function Invoke-MineCheck([string[]]$Flags,[string]$Report,[string]$Expected) {
    $start=Get-Date
    $argsForGame=@('-unattended','-nosound',('-UserDir="'+$root+'"'))+$Flags
    $process=Start-Process -FilePath $exe -ArgumentList $argsForGame -WindowStyle Hidden -PassThru
    if (!$process.WaitForExit(300000)) { $process.Kill(); throw 'The isolated mining check timed out' }
    $path=Join-Path (Join-Path $root 'Saved') $Report
    if (!(Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTime -lt $start) {throw "Missing fresh $Report"}
    $lines=Get-Content -LiteralPath $path
    Write-Output $lines[0]
    if ($lines[0] -notmatch $Expected -or $process.ExitCode -ne 0) {
        $lines | Where-Object {$_ -match '^(FAIL|Depth)'}
        throw "Game check failed in $Report"
    }
}
Invoke-MineCheck -Flags @('-ETSelfTest','-ETWriteMineQA') -Report 'native-verification.txt' -Expected '^ET_VERIFY \d+ checks / 0 failures$'
Invoke-MineCheck -Flags @('-ETReadMineQA') -Report 'faultline-restart.txt' -Expected '^ET_MINE_RESTART PASS$'
Invoke-MineCheck -Flags @('-ETReview=mine','-ETWalkMine') -Report 'mine-walk.txt' -Expected '^ET_MINE_WALK PASS$'
