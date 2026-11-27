param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot,
    [string]$SurvivorFile,
    [switch]$TripOnly
)
$ErrorActionPreference='Stop'
$exe=(Resolve-Path (Join-Path $GameRoot 'EARTHtotaled\Binaries\Win64\EARTHtotaled-Win64-Shipping.exe')).Path
$qa=[IO.Path]::GetFullPath($QaRoot)
if(Test-Path -LiteralPath $qa){throw 'Choose a new verification directory.'}
[IO.Directory]::CreateDirectory($qa) | Out-Null
function Run-Game([string]$Case,[string[]]$Options,[string]$Report){
    $where=Join-Path $qa $Case
    [IO.Directory]::CreateDirectory((Join-Path $where 'Saved')) | Out-Null
    $flags=@('-unattended',('-UserDir="'+$where+'"')) + $Options
    $p=Start-Process -FilePath $exe -ArgumentList $flags -PassThru -WindowStyle Hidden
    if(!$p.WaitForExit(300000)){$p.Kill();throw "$Case timed out."}
    $text=@(Get-Content -LiteralPath (Join-Path $where "Saved\$Report"))
    Write-Output $text[0]
    if($p.ExitCode -or ($text -match '^FAIL ')){throw (($text | Where-Object {$_ -match 'FAIL|Timed out|missed|could not'}) -join [Environment]::NewLine)}
}
if(!$TripOnly){Run-Game 'native' @('-ETSelfTest','-nosound') 'native-verification.txt'}
Run-Game 'flight' @('-ETLandfallQA','-ETInputAudit','-windowed','-ResX=1280','-ResY=720') 'landfall-trip.txt'
if($SurvivorFile){
    $slots=Join-Path $qa 'returning\Saved\SaveGames';[IO.Directory]::CreateDirectory($slots) | Out-Null
    Copy-Item -LiteralPath $SurvivorFile -Destination (Join-Path $slots 'EARTHtotaled-QA-Fieldworks.sav')
    Run-Game 'returning' @('-ETFieldJourney','-ETFieldLegacy','-ETInputAudit','-nosound') 'fieldworks-upgrade.txt'
}
