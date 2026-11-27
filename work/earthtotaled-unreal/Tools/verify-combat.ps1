param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot,
    [switch]$Render
)
$ErrorActionPreference='Stop'
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
$root=[IO.Path]::GetFullPath($QaRoot)
$routes=@('combat')
if($Render){$routes+='combat-view'}
foreach($route in $routes){
    $profile=Join-Path $root $route
    if(Test-Path -LiteralPath $profile){throw "Choose a fresh combat test folder. $profile already exists."}
    [IO.Directory]::CreateDirectory($profile) | Out-Null
    $options=@(('-ETWakeTest='+$route),('-UserDir="'+$profile+'"'),'-unattended','-windowed','-ResX=1280','-ResY=800','-ForceRes','-AudioMixer')
    $process=Start-Process -FilePath $exe -ArgumentList $options -WindowStyle Hidden -PassThru
    if(!$process.WaitForExit(240000)){$process.Kill();throw "$route timed out"}
    if($route -eq 'combat'){
        $lines=Get-Content -LiteralPath (Join-Path $profile 'Saved/combat-checks.txt')
        $lines
        if($lines[-1] -ne 'TOTAL 0 failures'){throw 'The combat checks failed.'}
    }else{
        foreach($name in 'shield-hit','low-hp','roadster','survey-plane','feedback-settings'){
            $file=Get-Item -LiteralPath (Join-Path $profile ('Saved/'+$name+'.png'))
            if($file.Length -lt 2048){throw "Missing combat capture $name"}
        }
    }
    if($process.ExitCode){throw "$route exited with $($process.ExitCode)"}
}
