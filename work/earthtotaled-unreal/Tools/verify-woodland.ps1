param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot,
    [string]$LegacySlot,
    [switch]$Render
)
$ErrorActionPreference='Stop'
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
$root=[IO.Path]::GetFullPath($QaRoot)
function Invoke-WoodlandTrial([string]$route,[string]$profile,[string[]]$options) {
    [IO.Directory]::CreateDirectory((Join-Path $profile 'Saved')) | Out-Null
    $argsList=@(('-ETWakeTest='+$route),'-unattended',('-UserDir="'+$profile+'"'))+$options
    $process=Start-Process -FilePath $exe -ArgumentList $argsList -WindowStyle Hidden -PassThru
    if(!$process.WaitForExit(240000)){$process.Kill();throw "Timed out on $route"}
    $report=Join-Path $profile 'Saved/megawake-journey.txt'
    if($process.ExitCode -ne 0){if(Test-Path -LiteralPath $report){Get-Content $report};throw "$route failed with exit $($process.ExitCode)"}
    if($route -ne 'woodland-view'){
        $lines=Get-Content -LiteralPath $report
        if($lines[-1] -ne 'TOTAL 0 failures'){throw "$route has incomplete checks"}
        $lines
    }
}
$wild=Join-Path $root 'woodland'
if(Test-Path -LiteralPath (Join-Path $wild 'Saved/SaveGames')){throw 'Use a fresh test folder for wildlife and loot persistence.'}
$result=Invoke-WoodlandTrial 'woodland' $wild @('-nullrhi','-nosound')
$result
if($LegacySlot){
    $profile=Join-Path $root 'old-survivor'
    $saves=Join-Path $profile 'Saved/SaveGames';[IO.Directory]::CreateDirectory($saves) | Out-Null
    [IO.File]::Copy((Resolve-Path $LegacySlot).Path,(Join-Path $saves 'EARTHtotaled-Unreal-01.sav'),$false)
    Invoke-WoodlandTrial 'trail-legacy' $profile @('-nullrhi','-nosound')
}
if($Render){
    $location=$result | Where-Object {$_ -match 'closest -?\d+,-?\d+'} | Select-Object -First 1
    if($location -notmatch 'closest (-?\d+),(-?\d+)'){throw 'Missing surveyed den'}
    $profile=Join-Path $root 'render'
    Invoke-WoodlandTrial 'woodland-view' $profile @(('-DenX='+$Matches[1]),('-DenY='+$Matches[2]),'-windowed','-ResX=1280','-ResY=800','-ForceRes')
    foreach($name in 'woodland-bear.png','forest-floor.png','woodland-voice.txt'){
        $file=Get-Item -LiteralPath (Join-Path $profile ('Saved/'+$name))
        if(!$file.Length){throw "Empty capture $name"}
        $file.FullName
    }
}
