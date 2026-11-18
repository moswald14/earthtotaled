#include "ETGame.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWave.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"

void AETWorld::RunAtmosphereChecks(TFunctionRef<void(bool,const FString&)> Check)
{
    auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    const float OriginalTime=State->Elapsed;
    const int32 OriginalOverride=WeatherOverride;
    auto Defaults=NewObject<UETSave>();
    Check(Defaults->MusicVolume==50&&Defaults->WeatherVolume==75&&Defaults->Schema==1,TEXT("Older saves receive moderate audio defaults without a schema reset"));
    for(const TCHAR* Name:{TEXT("MusicForest"),TEXT("MusicCity"),TEXT("MusicNight"),TEXT("WeatherRain"),TEXT("WeatherWind"),TEXT("WeatherThunder")}) {
        auto Clip=Cast<USoundWave>(Sounds.FindRef(Name));
        const bool Score=FString(Name).StartsWith(TEXT("Music"));
        Check(Clip&&Clip->GetDuration()>=7.9f&&Clip->NumChannels==(Score?2:1),FString(Name)+TEXT(" audio is cooked with its intended channel layout"));
        Check(Clip&&Clip->bLooping==(FString(Name)!=TEXT("WeatherThunder")),FString(Name)+TEXT(" has the intended loop behavior"));
    }
    Check(Music.Num()==3&&WeatherAudio.Num()==2&&Music[0]->bIsUISound&&!Music[0]->bAutoDestroy,TEXT("A bounded set of persistent audio channels supports paused menus"));
    Check(RainMesh&&RainMesh->GetInstanceCount()==256&&RainMesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("Rain uses a fixed nonblocking instanced particle pool"));
    Check(CloudMaterial&&Material(TEXT("WeatherClouds"))&&Material(TEXT("WeatherRain")),TEXT("Moving cloud and rain materials are present in the cooked build"));
    bool Seen[5]={}; bool Bounded=true,Continuous=true;
    for(int32 Seed:{0,84371,99119}) for(int32 Period=0;Period<100;++Period) {
        const auto F=ETClimate::Sample(Period*300.+80,Seed); Seen[F.Kind]=true;
        Bounded&=F.Cloud>=0&&F.Cloud<=1&&F.Rainfall>=0&&F.Rainfall<=1&&F.Haze>=0&&F.Haze<=1;
        if(Period>0) {
            const auto A=ETClimate::Sample(Period*300.-.01,Seed),B=ETClimate::Sample(Period*300.+.01,Seed);
            Continuous&=FMath::Abs(A.Cloud-B.Cloud)+FMath::Abs(A.Rainfall-B.Rainfall)+FMath::Abs(A.Haze-B.Haze)<.001;
        }
    }
    Check(Seen[0]&&Seen[1]&&Seen[2]&&Seen[3]&&Seen[4]&&Bounded,TEXT("Endless weather includes clear, overcast, mist, rain and storm with bounded intensity"));
    Check(Continuous,TEXT("Weather fronts transition continuously across saved time boundaries"));
    State->Elapsed=0; Check(MusicFor(FVector::ZeroVector)==1&&MusicFor(FVector(70000,0,0))==0,TEXT("Daytime travel selects city and forest themes"));
    State->Elapsed=780; Check(MusicFor(FVector::ZeroVector)==2&&MusicFor(FVector(70000,0,0))==2,TEXT("Night selects the night theme in both biomes"));
    State->Elapsed=0; Weather=ETClimate::Profile(ETClimate::Clear); UpdateDaylight();
    const float ClearSun=Sun->GetLightComponent()->Intensity,ClearFog=Fog->GetComponent()->FogDensity;
    Weather=ETClimate::Profile(ETClimate::Storm); UpdateDaylight();
    Check(Sun->GetLightComponent()->Intensity<ClearSun*.4f&&Fog->GetComponent()->FogDensity>ClearFog,TEXT("Storms darken daylight and thicken atmospheric haze"));
    const FVector Probe(1000000,5000,100);
    Check(!IsSheltered(Probe),TEXT("Open sky is exposed to weather"));
    auto Roof=Static(TEXT("/Engine/BasicShapes/Cube.Cube"),TEXT("Concrete"),Probe+FVector(0,0,400),FVector(20,20,.2));
    Check(IsSheltered(Probe),TEXT("A solid roof provides shelter from weather"));
    const float Floor=RainFloor(Probe+FVector(0,0,1450));
    Check(Floor>Probe.Z+410&&Floor<Probe.Z+430,TEXT("Rain stops on the roof instead of falling through its interior"));
    Roof->Destroy();
    Weather=ETClimate::Profile(ETClimate::Rain); Sheltered=false; UpdateRain(.05f,P->GetActorLocation()+FVector(0,0,70));
    int32 Visible=0; bool AboveSurface=true;
    for(int32 I=0;I<RainMesh->GetInstanceCount();++I) {
        FTransform T; RainMesh->GetInstanceTransform(I,T,true);
        if(T.GetScale3D().Z>0) {++Visible; AboveSurface&=T.GetLocation().Z>RainFloors[I];}
    }
    Check(Visible>100&&AboveSurface,TEXT("Visible rain streaks remain above their detected surfaces"));
    const FString OldText=SubtitleText; const double OldUntil=SubtitleUntil;
    SubtitleText.Empty(); State->MusicVolume=50; State->WeatherVolume=75; UpdateAtmosphereAudio(0,true);
    const int32 Track=MusicFor(P->GetActorLocation()); const float FullMusic=Music[Track]->VolumeMultiplier,OutdoorRain=WeatherAudio[0]->VolumeMultiplier;
    SubtitleText=TEXT("test dialogue"); SubtitleUntil=FPlatformTime::Seconds()+10; UpdateAtmosphereAudio(0,true);
    Check(Music[Track]->VolumeMultiplier<FullMusic*.3f,TEXT("Spoken dialogue ducks the score for intelligibility"));
    SubtitleText.Empty(); Sheltered=true; UpdateAtmosphereAudio(0,true);
    Check(WeatherAudio[0]->VolumeMultiplier<OutdoorRain*.3f&&WeatherAudio[0]->bEnableLowPassFilter,TEXT("Indoor shelter muffles rain and wind"));
    P->OpenPanel(TEXT("pause"),TEXT("test"),TEXT("test"));
    WeatherOverride=ETClimate::Storm; const float BeforeThunder=ThunderTimer,BeforeElapsed=State->Elapsed;
    TickAtmosphere(10); P->Tick(10);
    Check(ThunderTimer==BeforeThunder&&State->Elapsed==BeforeElapsed,TEXT("Pause holds weather timing and survival while leaving audio controls available"));
    for(int32 I=0;I<3;++I) P->Action(TEXT("music-volume"));
    Check(State->MusicVolume==0&&Music[Track]->VolumeMultiplier==0&&State->WeatherVolume==75,TEXT("Music can be muted without muting weather"));
    P->Action(TEXT("weather-volume")); P->Action(TEXT("weather-volume"));
    Check(State->WeatherVolume==0&&WeatherAudio[0]->VolumeMultiplier==0&&WeatherAudio[1]->VolumeMultiplier==0,TEXT("Weather sound can be independently muted"));
    P->ClosePanel(); State->MusicVolume=25; State->WeatherVolume=50;
    State->Elapsed=OriginalTime; WeatherOverride=OriginalOverride; Weather=ETClimate::Sample(State->Elapsed,State->Seed);
    Sheltered=false; SubtitleText=OldText; SubtitleUntil=OldUntil; UpdateDaylight(); UpdateAtmosphereAudio(0,true);
}
