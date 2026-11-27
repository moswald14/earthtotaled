param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot,
    [ValidateSet('All','Excursion','Doors','Shuttles')][string]$Suite='All'
)
$ErrorActionPreference='Stop'
$game=(Resolve-Path (Join-Path $GameRoot 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe')).Path
$root=[IO.Path]::GetFullPath($QaRoot)
function Run-Trip([string]$Name,[string[]]$Options,[int]$Limit) {
    $folder=Join-Path $root $Name
    $save=Join-Path $folder 'Saved'
    $return=$Options -contains '-ETTransitReturn'
    if(!(Test-Path -LiteralPath $save)) { [IO.Directory]::CreateDirectory($save) | Out-Null }
    elseif(!$return) {throw "Choose an empty test folder for $Name."}
    $report=Join-Path $save 'transit.txt'
    $flags=@('-ETInterchange','-unattended','-nosound','-windowed','-ResX=1280','-ResY=720',('-UserDir="'+$folder+'"'))+$Options
    $started=Get-Date
    $process=Start-Process -FilePath $game -ArgumentList $flags -WindowStyle Hidden -PassThru
    if(!$process.WaitForExit($Limit*1000)) {$process.Kill();throw "$Name exceeded $Limit seconds."}
    if(!(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTime -lt $started) {throw "$Name did not return a fresh report."}
    $lines=@(Get-Content -LiteralPath $report)
    $lines
    if($process.ExitCode -ne 0 -or $lines[-1] -notmatch '^PASS '){throw "$Name exited with $($process.ExitCode). $($lines[-1])"}
    if(!$return){Copy-Item -LiteralPath $report -Destination (Join-Path $save 'trip.txt')}
}
if($Suite -in 'All','Excursion'){Run-Trip 'excursion' @() 350}
if($Suite -in 'All','Doors'){
    Run-Trip 'doors' @('-ETDoors') 950
    Run-Trip 'doors' @('-ETDoors','-ETTransitReturn') 90
}
if($Suite -in 'All','Shuttles'){
    Run-Trip 'shuttles' @('-ETShuttles') 630
    $save=Join-Path $root 'shuttles/Saved'
    Copy-Item -LiteralPath (Join-Path $save 'link-checkpoint.sav') -Destination (Join-Path $save 'SaveGames/EARTHtotaled-Unreal-01.sav') -Force
    Run-Trip 'shuttles' @('-ETShuttles','-ETTransitReturn') 100
}
