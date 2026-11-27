param([string]$EngineRoot='C:\Program Files\Epic Games\UE_5.8',[string]$Destination)
$ErrorActionPreference='Stop'
if(!$Destination){$Destination=Join-Path $PSScriptRoot '..\..\Content\UpdateTools\EARTHtotaled-Setup.exe'}
$Destination=[IO.Path]::GetFullPath($Destination)
[IO.Directory]::CreateDirectory([IO.Path]::GetDirectoryName($Destination)) | Out-Null
$redist=Join-Path $EngineRoot 'Engine\Extras\Redist\en-us'
$compiler=Join-Path $env:WINDIR 'Microsoft.NET\Framework64\v4.0.30319\csc.exe'
$refs=@('System.Net.Http','System.Management','System.IO.Compression','System.IO.Compression.FileSystem','System.Web.Extensions','System.Windows.Forms','System.Drawing','Microsoft.CSharp')
$icon=[IO.Path]::GetFullPath((Join-Path $PSScriptRoot '..\..\Build\Windows\Application.ico'))
$manifest=Join-Path $PSScriptRoot 'setup.manifest'
$arguments=@("/win32manifest:$manifest","/win32icon:$icon",'/nologo','/optimize+','/target:winexe','/platform:x64',"/out:$Destination")
foreach($ref in $refs){$arguments+="/reference:$ref.dll"}
foreach($name in 'vc_redist.x64.exe','GameInputRedist.msi'){
    $binary=Join-Path $redist $name
    $signature=Get-AuthenticodeSignature -LiteralPath $binary
    if($signature.Status -ne 'Valid' -or $signature.SignerCertificate.GetNameInfo('SimpleName',$false) -ne 'Microsoft Corporation'){throw "Microsoft signature did not verify on $name"}
    $arguments+="/resource:$binary,$name"
}
& $compiler @arguments (Join-Path $PSScriptRoot 'Manager.cs') (Join-Path $PSScriptRoot 'GameSessions.cs') (Join-Path $PSScriptRoot 'Program.cs')
if($LASTEXITCODE){throw 'Windows installer compile failed.'}
Get-Item -LiteralPath $Destination | Select-Object Name,Length
