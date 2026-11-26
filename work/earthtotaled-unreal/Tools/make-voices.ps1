param([string]$VoiceName='',[string[]]$LineIds=@())
$ErrorActionPreference='Stop'
Add-Type -AssemblyName System.Speech
$projectRoot=Split-Path -Parent $PSScriptRoot
$dialogue=Get-Content -Raw -LiteralPath (Join-Path $projectRoot 'Content\Data\dialogue.json') | ConvertFrom-Json
$outputRoot=Join-Path $projectRoot 'ArtSource'
$speech=New-Object System.Speech.Synthesis.SpeechSynthesizer
try {
    $available=@($speech.GetInstalledVoices() | Where-Object { $_.Enabled -and $_.VoiceInfo.Culture.Name.StartsWith('en') })
    if (!$available.Count) {throw 'No enabled English Windows speech voice is installed.'}
    if (!$VoiceName) {$VoiceName=$available[0].VoiceInfo.Name}
    $speech.SelectVoice($VoiceName)
    $format=New-Object System.Speech.AudioFormat.SpeechAudioFormatInfo(22050,[System.Speech.AudioFormat.AudioBitsPerSample]::Sixteen,[System.Speech.AudioFormat.AudioChannel]::Mono)
    foreach($line in $dialogue.lines) {
        if ($LineIds.Count -and $line.id -notin $LineIds) {continue}
        if ($line.id -notmatch '^[a-z0-9]+$') {throw 'Unsafe dialogue asset identifier.'}
        $file=Join-Path $outputRoot ('S_Voice_'+$line.id+'.wav')
        $speech.Rate=[int]$line.rate
        $speech.Volume=100
        $speech.SetOutputToWaveFile($file,$format)
        $speech.Speak([string]$line.text)
        $speech.SetOutputToNull()
    }
} finally {$speech.Dispose()}
& python (Join-Path $PSScriptRoot 'finish-voices.py') @LineIds
if ($LASTEXITCODE -ne 0) {throw 'Voice processing or validation failed.'}
Write-Output ('Updated the selected original dialogue clips using '+$VoiceName+'.')
