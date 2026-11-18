Add-Type -AssemblyName System.Drawing
$desktopSource = $PSScriptRoot
$workspaceRoot = (Resolve-Path (Join-Path $PSScriptRoot '../../..')).Path
Set-Location $workspaceRoot
$sizes = @(16,32,48,256)
$images = @()
foreach ($size in $sizes) {
 $bmp = New-Object Drawing.Bitmap($size,$size)
 $g = [Drawing.Graphics]::FromImage($bmp)
 $g.SmoothingMode = [Drawing.Drawing2D.SmoothingMode]::AntiAlias
 $g.TextRenderingHint = [Drawing.Text.TextRenderingHint]::AntiAliasGridFit
 $g.Clear([Drawing.Color]::FromArgb(19,35,28))
 $accent = New-Object Drawing.SolidBrush([Drawing.Color]::FromArgb(216,238,140))
 $ink = New-Object Drawing.SolidBrush([Drawing.Color]::FromArgb(239,240,221))
 $font = New-Object Drawing.Font('Segoe UI',($size*0.55),[Drawing.FontStyle]::Bold,[Drawing.GraphicsUnit]::Pixel)
 $fmt = New-Object Drawing.StringFormat
 $fmt.Alignment = [Drawing.StringAlignment]::Center
 $fmt.LineAlignment = [Drawing.StringAlignment]::Center
 $g.DrawString('Et',$font,$ink,(New-Object Drawing.RectangleF(0,0,$size,($size*0.91))),$fmt)
 $g.FillRectangle($accent,[single]($size*0.16),[single]($size*0.82),[single]($size*0.68),[single]($size*0.045))
 $ms = New-Object IO.MemoryStream
 $bmp.Save($ms,[Drawing.Imaging.ImageFormat]::Png)
 $images += ,$ms.ToArray()
 $g.Dispose(); $bmp.Dispose(); $font.Dispose(); $ink.Dispose(); $accent.Dispose(); $ms.Dispose()
}
$stream = [IO.File]::Create((Join-Path $desktopSource 'EARTHtotaled.ico'))
$bw = New-Object IO.BinaryWriter($stream)
$bw.Write([uint16]0); $bw.Write([uint16]1); $bw.Write([uint16]4)
$offset = 6 + 16*4
for ($i=0; $i -lt 4; $i++) {
 $dimension = if ($sizes[$i] -eq 256) {0} else {$sizes[$i]}
 $bw.Write([byte]$dimension); $bw.Write([byte]$dimension); $bw.Write([byte]0); $bw.Write([byte]0)
 $bw.Write([uint16]1); $bw.Write([uint16]32); $bw.Write([uint32]$images[$i].Length); $bw.Write([uint32]$offset)
 $offset += $images[$i].Length
}
foreach($bytes in $images){$bw.Write([byte[]]$bytes)}
$bw.Dispose()
$releaseDir = Join-Path (Get-Location) 'outputs/EARTHtotaled-Windows'
Copy-Item "$desktopSource/vendor/webview2/lib/net462/Microsoft.Web.WebView2.Core.dll" $releaseDir -Force
Copy-Item "$desktopSource/vendor/webview2/lib/net462/Microsoft.Web.WebView2.WinForms.dll" $releaseDir -Force
Copy-Item "$desktopSource/vendor/webview2/runtimes/win-x64/native/WebView2Loader.dll" $releaseDir -Force
Copy-Item "$desktopSource/EARTHtotaled.ico" $releaseDir -Force
& 'C:/Windows/Microsoft.NET/Framework64/v4.0.30319/csc.exe' /nologo /main:Bootstrap /target:winexe /platform:x64 /optimize+ /codepage:65001 ("/out:"+(Join-Path $releaseDir "EARTHtotaled.exe")) ("/win32icon:"+(Join-Path $desktopSource "EARTHtotaled.ico")) /reference:System.Windows.Forms.dll /reference:System.Drawing.dll ("/reference:"+(Join-Path $releaseDir "Microsoft.Web.WebView2.Core.dll")) ("/reference:"+(Join-Path $releaseDir "Microsoft.Web.WebView2.WinForms.dll")) '/resource:outputs/EARTHtotaled.html,GameHtml' ("/resource:"+(Join-Path $desktopSource "EARTHtotaled.ico")+",GameIcon") ("/resource:"+(Join-Path $releaseDir "Microsoft.Web.WebView2.Core.dll")+",CoreDll") ("/resource:"+(Join-Path $releaseDir "Microsoft.Web.WebView2.WinForms.dll")+",FormsDll") ("/resource:"+(Join-Path $releaseDir "WebView2Loader.dll")+",LoaderDll") ("/resource:"+(Join-Path $desktopSource 'desktop-bridge.js')+",DesktopBridge") (Join-Path $desktopSource 'DesktopMouse.cs') (Join-Path $desktopSource 'Program.cs')
if($LASTEXITCODE -ne 0){throw 'Desktop compilation failed'}
