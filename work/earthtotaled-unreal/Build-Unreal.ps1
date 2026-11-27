param(
    [ValidateSet('Editor','Content','Play','Package')][string]$Mode='Editor',
    [string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',
    [string]$OutputDirectory
)
$ErrorActionPreference='Stop'
$projectRoot=$PSScriptRoot
$projectFile=Join-Path $projectRoot 'EARTHtotaled.uproject'
$build=Join-Path $EngineRoot 'Engine\Build\BatchFiles\Build.bat'
$editor=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor.exe'
$editorCmd=Join-Path $EngineRoot 'Engine\Binaries\Win64\UnrealEditor-Cmd.exe'
if (!(Test-Path -LiteralPath $build) -or !(Test-Path -LiteralPath $editorCmd)) {throw 'Unreal Engine installation is incomplete. Finish installing 5.8 in Epic Games Launcher.'}
if (!(Test-Path -LiteralPath (Join-Path $projectRoot 'Content\Data\design.json'))) {throw 'Missing game data. Run node Tools/export-legacy.mjs.'}
if (!(Test-Path -LiteralPath (Join-Path $projectRoot 'ArtSource\manifest.json'))) {throw 'Missing original art. Run python Tools/make-art.py.'}
& $build EARTHtotaledEditor Win64 Development "-Project=$projectFile" -WaitMutex -NoHotReloadFromIDE -NoUBA
if ($LASTEXITCODE -ne 0) {throw 'Unreal C++ editor build failed. See Saved/Logs and the UnrealBuildTool log.'}
if ($Mode -eq 'Editor') {Write-Output 'Editor module compiled.'; return}
if ($Mode -eq 'Content' -or !(Test-Path -LiteralPath (Join-Path $projectRoot 'Saved\content-ready.txt'))) {
    $pythonFile=Join-Path $projectRoot 'Tools\bootstrap-content.py'
    $importLog=Join-Path $projectRoot 'Saved\content-import-output.log'
    $readyMarker=Join-Path $projectRoot 'Saved\content-ready.txt'
    $importStarted=Get-Date
    Write-Output "Importing art in Unreal. Details: $importLog"
    & $editorCmd $projectFile "-ExecutePythonScript=$pythonFile" -unattended -nosplash -stdout *> $importLog
    if ($LASTEXITCODE -ne 0 -or !(Test-Path -LiteralPath $readyMarker) -or (Get-Item -LiteralPath $readyMarker).LastWriteTime -lt $importStarted) {throw 'Unreal art import did not complete. See Saved/Logs.'}
}
if ($Mode -eq 'Content') {Write-Output 'Content imported by Unreal Editor.'; return}
if ($Mode -eq 'Play') {
    Start-Process -FilePath $editor -ArgumentList @(('"'+$projectFile+'"'),'/Game/EARTHtotaled/Maps/ExclusionZone','-game','-windowed','-ResX=1600','-ResY=900') -WindowStyle Normal
    return
}
if (!$OutputDirectory) {$OutputDirectory=Join-Path $projectRoot '..\..\outputs\EARTHtotaled-Unreal-Windows'}
& (Join-Path $projectRoot 'Tools/Setup/Build-Setup.ps1') -EngineRoot $EngineRoot
$uat=Join-Path $EngineRoot 'Engine\Build\BatchFiles\RunUAT.bat'
& $uat BuildCookRun "-project=$projectFile" -noP4 -platform=Win64 -clientconfig=Shipping -build -cook -stage -pak -iostore -archive "-archivedirectory=$OutputDirectory" -utf8output
if ($LASTEXITCODE -ne 0) {throw 'Unreal Windows packaging failed.'}
Write-Output "Packaged native game: $OutputDirectory"
