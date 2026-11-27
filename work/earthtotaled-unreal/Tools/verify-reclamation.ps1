param([Parameter(Mandatory)][string]$GameRoot,[Parameter(Mandatory)][string]$QaRoot,[string]$SurvivorFile)
$ErrorActionPreference='Stop'
$game=(Resolve-Path (Join-Path $GameRoot 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe')).Path
if($SurvivorFile) {
    $legacy=[IO.Path]::GetFullPath((Join-Path $QaRoot 'returning'))
    $slots=Join-Path $legacy 'Saved/SaveGames'
    New-Item -ItemType Directory -Force -Path $slots | Out-Null
    $copy=Join-Path $slots 'EARTHtotaled-QA-Fieldworks.sav'
    Copy-Item -LiteralPath $SurvivorFile -Destination $copy -Force
    $before=Get-Date
    $flags=@('-ETFieldJourney','-ETFieldLegacy','-ETInputAudit','-unattended','-nosound',('-UserDir="'+$legacy+'"'))
    $process=Start-Process -FilePath $game -ArgumentList $flags -PassThru -WindowStyle Hidden
    if(!$process.WaitForExit(180000)) {$process.Kill();throw 'Returning survivor timed out'}
    $report=Get-Item -LiteralPath (Join-Path $legacy 'Saved/fieldworks-upgrade.txt')
    $lines=Get-Content -LiteralPath $report.FullName
    if($report.LastWriteTime -lt $before -or $process.ExitCode -ne 0 -or $lines[0] -notmatch '^ET_FIELD \d+ checks / 0 failures$') {throw ($lines -join [Environment]::NewLine)}
    Write-Output $lines[0]
}
foreach($scene in @('keys','workshop','cistern','infected','forest')) {
    $profile=[IO.Path]::GetFullPath((Join-Path $QaRoot $scene))
    New-Item -ItemType Directory -Force -Path (Join-Path $profile 'Saved') | Out-Null
    $started=Get-Date
    $flags=@("-ETReview=$scene",'-ETPhoto','-unattended','-nosound','-windowed','-ResX=1600','-ResY=900',('-UserDir="'+$profile+'"'))
    $process=Start-Process -FilePath $game -ArgumentList $flags -PassThru -WindowStyle Hidden
    if(!$process.WaitForExit(150000)) {$process.Kill();throw "Timed out rendering $scene"}
    $frame=Get-Item -LiteralPath (Join-Path $profile "Saved/view-$scene.png") -ErrorAction SilentlyContinue
    if($scene -eq 'keys') {Get-Content -LiteralPath (Join-Path $profile 'Saved/reclamation-keys.txt')}
    if($process.ExitCode -ne 0 -or !$frame -or $frame.Length -lt 1024 -or $frame.LastWriteTime -lt $started){throw "Incomplete capture for $scene"}
    if($scene -eq 'cistern') {
        # This fixed clear-weather view has unobstructed morning sky to the left of the tower.
        Add-Type -AssemblyName System.Drawing
        $frameImage=[Drawing.Bitmap]::new($frame.FullName)
        try {
            $blueSamples=0
            foreach($fraction in @(.18,.22,.26,.30,.34)) {
                $pixel=$frameImage.GetPixel([int]($frameImage.Width*$fraction),[int]($frameImage.Height*.26))
                if($pixel.B -gt ($pixel.R+10) -and $pixel.B -gt ($pixel.G+4)) {$blueSamples++}
            }
            if($blueSamples -lt 3){throw 'The clear morning sky around the hill tower has lost its blue daylight.'}
        } finally {$frameImage.Dispose()}
    }
    Write-Output "$scene rendered."
}
