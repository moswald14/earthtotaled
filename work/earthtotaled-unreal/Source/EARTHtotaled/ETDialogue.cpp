#include "ETGame.h"
#include "Components/AudioComponent.h"
#include "HAL/PlatformTime.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"

void AETWorld::LoadDialogue()
{
    FString Text; TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Text,*(FPaths::ProjectContentDir()/TEXT("Data/dialogue.json")))||
       !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text),Root)||!Root) {
        UE_LOG(LogTemp,Warning,TEXT("EARTHtotaled dialogue data was not found.")); return;
    }
    const TArray<TSharedPtr<FJsonValue>>* Lines=nullptr;
    if(!Root->TryGetArrayField(TEXT("lines"),Lines)) return;
    for(const auto& Value:*Lines) {
        auto Object=Value->AsObject(); if(!Object) continue;
        FString Id; FETVoiceLine Line;
        if(!Object->TryGetStringField(TEXT("id"),Id)||!Object->TryGetStringField(TEXT("speaker"),Line.Speaker)||!Object->TryGetStringField(TEXT("text"),Line.Text)||Id.IsEmpty()) continue;
        VoiceLines.Add(Id,Line);
        const FString Asset=TEXT("S_Voice_")+Id;
        Sounds.Add(TEXT("Voice_")+Id,LoadObject<USoundBase>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Asset+TEXT(".")+Asset)));
    }
}

bool AETWorld::Speak(const FString& Id,const FString& SpeakerOverride)
{
    const FETVoiceLine* Line=VoiceLines.Find(Id); if(!Line) return false;
    const double Now=FPlatformTime::Seconds();
    // Repeated clicks may refresh a panel, but must not repeatedly restart its voice.
    if(Id==VoiceId&&Now-VoiceStartedAt<1.5) return false;
    USoundBase* Clip=Sounds.FindRef(TEXT("Voice_")+Id);
    VoiceId=Id; VoiceStartedAt=Now;
    SubtitleSpeaker=SpeakerOverride.IsEmpty()?Line->Speaker:SpeakerOverride;
    SubtitleText=Line->Text;
    SubtitleUntil=Now+(Clip?FMath::Clamp(Clip->GetDuration()+.8f,3.f,35.f):12.f);
    if(!DialogueAudio) {
        DialogueAudio=NewObject<UAudioComponent>(this);
        DialogueAudio->bAutoActivate=false; DialogueAudio->bAutoDestroy=false;
        DialogueAudio->bAllowSpatialization=false; DialogueAudio->SetUISound(true);
        DialogueAudio->RegisterComponent(); DialogueAudio->SetVolumeMultiplier(.95f);
    }
    // One component owns dialogue, so a reply replaces the greeting instead of overlapping it.
    DialogueAudio->Stop(); DialogueAudio->SetSound(Clip);
    if(Clip&&!SelfTest) DialogueAudio->Play();
    if(Review&&!SelfTest&&Clip) {
        const FString Result=FString::Printf(TEXT("cue=%s\nplaying=%s\nui_sound=%s\nworld_paused=%s\nduration=%.2f\n"),*Id,DialogueAudio->IsPlaying()?TEXT("true"):TEXT("false"),DialogueAudio->bIsUISound?TEXT("true"):TEXT("false"),UGameplayStatics::IsGamePaused(this)?TEXT("true"):TEXT("false"),Clip->GetDuration());
        FFileHelper::SaveStringToFile(Result,*(FPaths::ProjectSavedDir()/TEXT("voice-playback-verification.txt")));
    }
    return true;
}

bool AETWorld::HasSubtitle() const
{
    // Real time continues while the conversation menu pauses survival and the world.
    return !SubtitleText.IsEmpty()&&FPlatformTime::Seconds()<SubtitleUntil;
}
