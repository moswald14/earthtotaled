param([Parameter(Mandatory)][string]$TestRoot,[string]$PreviousZip,[string]$GameZip,[string]$SurvivorFile)
$ErrorActionPreference='Stop'
$root=[IO.Path]::GetFullPath($TestRoot)
[IO.Directory]::CreateDirectory($root) | Out-Null
$exe=Join-Path $root 'SetupChecks.exe'
& (Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe') /nologo ("/win32manifest:"+(Join-Path $PSScriptRoot 'setup.manifest')) "/out:$exe" /reference:System.IO.Compression.dll /reference:System.IO.Compression.FileSystem.dll /reference:System.Net.Http.dll /reference:System.Web.Extensions.dll /reference:Microsoft.CSharp.dll (Join-Path $PSScriptRoot 'Manager.cs') (Join-Path $PSScriptRoot 'SetupChecks.cs')
if($LASTEXITCODE){throw 'Installer checks did not compile.'}
$options=@($root,'--online')
if($PreviousZip -and $GameZip -and $SurvivorFile){$options=@($root,'--packages',(Resolve-Path $PreviousZip).Path,(Resolve-Path $GameZip).Path,(Resolve-Path $SurvivorFile).Path)}
& $exe @options
if($LASTEXITCODE){throw 'Installer verification failed.'}
