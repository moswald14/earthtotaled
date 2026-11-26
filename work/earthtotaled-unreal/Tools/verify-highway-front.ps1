param(
    [Parameter(Mandatory)][string]$GameRoot,
    [Parameter(Mandatory)][string]$QaRoot
)
$ErrorActionPreference='Stop'
$exe=Join-Path ([IO.Path]::GetFullPath($GameRoot)) 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'
foreach($site in @('supertech','bug')) {
    $qa=Join-Path ([IO.Path]::GetFullPath($QaRoot)) $site
    $saved=Join-Path $qa 'Saved'
    New-Item -ItemType Directory -Force -Path $saved | Out-Null
    $started=Get-Date
    $flags=@("-ETReview=$site",'-ETHighwayTour','-unattended','-nosound',('-UserDir="'+$qa+'"'))
    $p=Start-Process -FilePath $exe -ArgumentList $flags -WindowStyle Hidden -PassThru
    if(!$p.WaitForExit(125000)) {$p.Kill();throw "$site tour timed out"}
    $path=Join-Path $saved 'front-tour.txt'
    if(!(Test-Path -LiteralPath $path) -or (Get-Item -LiteralPath $path).LastWriteTime -lt $started) {throw "No new $site report"}
    $result=Get-Content -LiteralPath $path
    $result
    if($result[0] -ne "ET_FRONT $site PASS" -or $p.ExitCode -ne 0) {throw "$site battle check failed"}
    if(!(Test-Path -LiteralPath (Join-Path $saved ($site+'.png')))) {throw "Missing $site screenshot"}
}
