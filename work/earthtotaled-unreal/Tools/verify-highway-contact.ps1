param([Parameter(Mandatory)][string]$GameRoot,[Parameter(Mandatory)][string]$QaRoot,
      [string]$SurvivorFile,[switch]$Editor,
      [ValidateSet('close','fire','shards','truce','night')][string[]]$Cases=@('close','fire','shards','truce','night'))
$ErrorActionPreference='Stop'
$base=[IO.Path]::GetFullPath($QaRoot)
$GameRoot=(Resolve-Path -LiteralPath $GameRoot).Path
$exe=if($Editor){Join-Path $GameRoot 'Engine/Binaries/Win64/UnrealEditor-Cmd.exe'}else{Join-Path $GameRoot 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'}
if($Editor){$project=(Resolve-Path (Join-Path $PSScriptRoot '../EARTHtotaled.uproject')).Path}
$failures=@()
foreach($case in $Cases){
    $profile=Join-Path $base $case
    if(Test-Path -LiteralPath $profile){throw "Choose a fresh profile for $case."}
    $slots=Join-Path $profile 'Saved/SaveGames'
    [IO.Directory]::CreateDirectory($slots) | Out-Null
    if($SurvivorFile){Copy-Item -LiteralPath $SurvivorFile -Destination (Join-Path $slots 'EARTHtotaled-Unreal-01.sav')}
    $options=@("-ETWakeTest=highway-contact-$case",('-UserDir="'+$profile+'"'),'-nullrhi','-unattended','-nosound','-UseFixedTimeStep','-FPS=30')
    if($Editor){$options=@(('"'+$project+'"'),'/Game/EARTHtotaled/Maps/ExclusionZone','-game','-NoSourceControl','-nosplash',('-abslog="'+$profile+'\engine.log"'))+$options}
    $start=[Diagnostics.ProcessStartInfo]::new($exe,($options -join ' '))
    $start.UseShellExecute=$false
    $start.CreateNoWindow=$true
    $start.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    # The prebuilt editor can play a map while the Windows shipping target compiles.
    if($Editor){$start.EnvironmentVariables['UE_SKIP_UBT_SDK_SETUP']='1'}
    $p=[Diagnostics.Process]::Start($start)
    try{
        if(!$p.WaitForExit(150000)){$p.Kill();$failures+="$case timed out";continue}
        $code=$p.ExitCode
    }finally{$p.Dispose()}
    $file=Join-Path $profile 'Saved/highway-contact.txt'
    if(!(Test-Path -LiteralPath $file)){$failures+="$case did not report";continue}
    $lines=Get-Content -LiteralPath $file
    "$case / "+($lines -join ' | ')
    if($code -or $lines[-1] -ne 'TOTAL 0 failures'){$failures+=$case}
}
if($failures.Count){throw ("Highway contact checks failed / "+($failures -join ', '))}
