param([Parameter(Mandatory)][string]$GameRoot,[Parameter(Mandatory)][string]$TestRoot,
      [Parameter(Mandatory)][string]$PreviousSetup)
$ErrorActionPreference='Stop'
$game=(Resolve-Path $GameRoot).Path
$previous=(Resolve-Path $PreviousSetup).Path
$tag='v'+([regex]::Match((Get-Content (Join-Path $PSScriptRoot '../Config/DefaultGame.ini') -Raw),'ProjectVersion=([\d.]+)-unreal').Groups[1].Value)
$run=Join-Path ([IO.Path]::GetFullPath($TestRoot)) ([guid]::NewGuid().ToString('N'))
$local=Join-Path $run 'Local'
$managed=Join-Path $local 'Programs/EARTHtotaled'
$version=Join-Path $managed "Versions/$tag"
[IO.Directory]::CreateDirectory($version) | Out-Null
foreach($file in Get-ChildItem -LiteralPath $game -File -Recurse){
    $relative=$file.FullName.Substring($game.Length+1)
    if($file.Extension -in '.pdb','.debug','.map' -or $relative -match '(^|[\\/])Saved[\\/]'){continue}
    $dest=Join-Path $version $relative
    [IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($dest)) | Out-Null
    try {New-Item -ItemType HardLink -Path $dest -Target $file.FullName -ErrorAction Stop | Out-Null}
    catch {Copy-Item -LiteralPath $file.FullName -Destination $dest}
}
Copy-Item -LiteralPath $previous -Destination (Join-Path $managed 'EARTHtotaled.exe')
[IO.File]::WriteAllLines((Join-Path $managed 'active.txt'),@($tag,'primary',$tag))
[IO.File]::WriteAllText((Join-Path $version '.release'),'startup fixture')
[IO.File]::WriteAllText((Join-Path $managed 'portable-origins.txt'),(Join-Path $game 'EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe'))
Add-Type -AssemblyName System.Drawing
foreach($kind in 'installed','portable'){
    $suffix='ETStartup'+[guid]::NewGuid().ToString('N')
    $base=if($kind -eq 'installed'){$version}else{$game}
    $info=New-Object Diagnostics.ProcessStartInfo
    $info.FileName=Join-Path $base 'EARTHtotaled.exe'
    $info.WorkingDirectory=$base
    $info.UseShellExecute=$false
    $info.WindowStyle=[Diagnostics.ProcessWindowStyle]::Hidden
    $info.Arguments="-ETTitleProbe -SaveToUserDir -saveddirsuffix=$suffix -RenderOffscreen -unattended"
    $info.EnvironmentVariables['LOCALAPPDATA']=$local
    $watch=[Diagnostics.Stopwatch]::StartNew()
    $boot=[Diagnostics.Process]::Start($info)
    if(!$boot.WaitForExit(90000)){
        # The bootstrap and its own descendants belong to this test profile.
        & (Join-Path $env:WINDIR 'System32/taskkill.exe') /PID $boot.Id /T /F | Out-Null
        throw "$kind did not draw its title menu within 90 seconds."
    }
    if($boot.ExitCode -ne 0){throw "$kind startup exited with $($boot.ExitCode)"}
    $expected=if($kind -eq 'installed'){'title'}else{$tag}
    $folders=@([Environment]::GetFolderPath('LocalApplicationData'),$local) |
        ForEach-Object {Join-Path $_ "EARTHtotaled/Saved_$suffix"}
    $report=$folders | ForEach-Object {Join-Path $_ 'startup-title.txt'} | Where-Object {Test-Path -LiteralPath $_} | Select-Object -First 1
    if(!$report -or (Get-Content -LiteralPath $report -Raw).Trim() -ne $expected){throw "$kind did not show the expected title choice."}
    $png=Join-Path (Split-Path $report) 'startup-title.png'
    $frame=[Drawing.Bitmap]::new($png)
    try {
        $bright=0
        for($y=0;$y -lt $frame.Height;$y+=8){for($x=0;$x -lt $frame.Width;$x+=8){
            $px=$frame.GetPixel($x,$y)
            if($px.R -gt 120 -and $px.G -gt 120 -and $px.B -gt 110){$bright++}
        }}
        if($bright -lt 25){throw "$kind rendered a black frame."}
    } finally {$frame.Dispose()}
    Copy-Item -LiteralPath $png -Destination (Join-Path $run "$kind.png")
    "PASS $kind painted its title with an older launcher present ($([math]::Round($watch.Elapsed.TotalSeconds,1)) s including exit)."
}
if((Get-FileHash (Join-Path $managed 'EARTHtotaled.exe')).Hash -ne (Get-FileHash $previous).Hash){throw 'Opening the game modified the test launcher.'}
