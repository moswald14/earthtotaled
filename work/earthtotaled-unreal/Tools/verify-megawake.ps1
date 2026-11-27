param([Parameter(Mandatory)][string]$GameRoot,[Parameter(Mandatory)][string]$QaRoot,[ValidateSet('bug','frame','crossed')][string[]]$Routes=@('bug','frame','crossed'))
$ErrorActionPreference='Stop'
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
if(!(Test-Path -LiteralPath $exe)){throw 'The packaged game is missing.'}
$root=[IO.Path]::GetFullPath($QaRoot)
foreach($route in $Routes){
    $profile=Join-Path $root $route
    if(Test-Path -LiteralPath (Join-Path $profile 'Saved/SaveGames')){throw "Choose a fresh test folder. $route already has a survivor."}
    [IO.Directory]::CreateDirectory((Join-Path $profile 'Saved')) | Out-Null
    foreach($phase in @($route,($route+'airresume'),($route+'formresume'),($route+'resume'))){
        $began=Get-Date
        $args=@(('-ETWakeTest='+$phase),'-nullrhi','-nosound','-unattended',('-UserDir="'+$profile+'"'))
        $process=Start-Process -FilePath $exe -ArgumentList $args -WindowStyle Hidden -PassThru
        if(!$process.WaitForExit(240000)){$process.Kill();throw "Test session $phase timed out."}
        $report=Join-Path $profile 'Saved/megawake-journey.txt'
        if(!(Test-Path -LiteralPath $report) -or (Get-Item -LiteralPath $report).LastWriteTime -lt $began){throw "No new $phase report. Exit $($process.ExitCode)."}
        $lines=Get-Content -LiteralPath $report
        Copy-Item -LiteralPath $report -Destination (Join-Path $root ($phase+'.txt'))
        if($process.ExitCode -ne 0 -or $lines[-1] -ne 'TOTAL 0 failures'){$lines;throw "Campaign check failed for $phase."}
        "$phase / $(@($lines | Where-Object { $_ -like 'PASS *' }).Count) checks passed"
    }
}
