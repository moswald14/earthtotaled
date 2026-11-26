param(
    [Parameter(Mandatory=$true)][string]$GameRoot,
    [Parameter(Mandatory=$true)][string]$QaRoot,
    [ValidateSet('garden','signal')][string]$Ending='garden',
    [ValidateRange(30,3600)][int]$Seconds=720,
    [switch]$StoryOnly,
    [switch]$FollowRoad,
    [switch]$Resume
)
$ErrorActionPreference='Stop'
$gameRoot=[IO.Path]::GetFullPath($GameRoot)
$qaRoot=[IO.Path]::GetFullPath($QaRoot)
$exe=Join-Path $gameRoot 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
if(!(Test-Path -LiteralPath $exe)){throw 'Packaged Windows executable missing'}
New-Item -ItemType Directory -Path (Join-Path $qaRoot 'Saved') -Force | Out-Null
$arguments=@("-ETJourney=$Ending","-ETJourneySeconds=$Seconds",'-unattended','-nosound','-windowed','-ResX=1920','-ResY=1080',('-UserDir="'+$qaRoot+'"'))
if($StoryOnly){$arguments+='-ETStoryOnly'}
if($FollowRoad){$arguments+='-ETFollowRoad'}
if($Resume){$arguments+='-ETJourneyResume'}
$started=Get-Date
$process=Start-Process -FilePath $exe -ArgumentList $arguments -WindowStyle Hidden -PassThru
$nextUpdate=(Get-Date).AddSeconds(30)
$deadline=$started.AddSeconds($Seconds+240)
while(!$process.WaitForExit(1000)){
    if((Get-Date) -gt $deadline){Stop-Process -Id $process.Id; throw 'Journey exceeded its bounded test duration'}
    if((Get-Date) -ge $nextUpdate){
        $samples=Join-Path $qaRoot 'Saved/journey-samples.csv'
        if(Test-Path $samples){Get-Content -LiteralPath $samples -Tail 1}
        $nextUpdate=(Get-Date).AddSeconds(30)
    }
}
$report=Join-Path $qaRoot 'Saved/journey-verification.txt'
if(!(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTime -lt $started){throw 'No fresh journey result'}
$result=Get-Content -LiteralPath $report
Write-Output $result
if($process.ExitCode -ne 0 -or $result[0] -notmatch '^ET_JOURNEY (garden|signal) \d+ checks / 0 failures$'){throw 'Journey checks failed'}
Get-Content -LiteralPath (Join-Path $qaRoot 'Saved/journey-metrics.json')
