#include "ETGame.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Sound/SoundBase.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/PlatformTime.h"

namespace { constexpr int32 MaxRain=256; }

FString AETWorld::WeatherName() const
{
    const TCHAR* Names[]={TEXT("CLEAR"),TEXT("OVERCAST"),TEXT("MIST"),TEXT("RAIN"),TEXT("THUNDERSTORM")};
    return Names[FMath::Clamp(Weather.Kind,0,4)];
}

int32 AETWorld::MusicFor(FVector Position) const
{
    return Hour()<6||Hour()>=19?2:InCity(Position)?1:0;
}

void AETWorld::InitAtmosphere()
{
    WeatherRandom.Initialize(State->Seed+713);
    State->MusicVolume=FMath::Clamp(State->MusicVolume,0,100);
    State->WeatherVolume=FMath::Clamp(State->WeatherVolume,0,100);
    FString Override;
    if(Review&&FParse::Value(FCommandLine::Get(),TEXT("ETWeather="),Override)) {
        const TCHAR* Names[]={TEXT("clear"),TEXT("overcast"),TEXT("mist"),TEXT("rain"),TEXT("storm")};
        for(int32 I=0;I<5;++I) if(Override==Names[I]) WeatherOverride=I;
    }
    Weather=WeatherOverride>=0?ETClimate::Profile(WeatherOverride):ETClimate::Sample(State->Elapsed,State->Seed);
    for(const TCHAR* Name:{TEXT("MusicForest"),TEXT("MusicCity"),TEXT("MusicNight"),TEXT("WeatherRain"),TEXT("WeatherWind"),TEXT("WeatherThunder")}) {
        const FString Id=Name;
        auto Clip=LoadObject<USoundBase>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/S_")+Id+TEXT(".S_")+Id));
        Sounds.Add(Id,Clip);
        if(Id==TEXT("WeatherThunder")) continue;
        auto A=NewObject<UAudioComponent>(this);
        A->bAutoActivate=false; A->bAutoDestroy=false; A->bAllowSpatialization=false; A->SetUISound(true);
        A->SetSound(Clip); A->SetVolumeMultiplier(0); A->RegisterComponent();
        // Silent loops are virtualized and keep their playhead, so travel never restarts a song.
        if(!SelfTest&&Clip) A->Play();
        if(Id.StartsWith(TEXT("Music"))) Music.Add(A); else WeatherAudio.Add(A);
    }
    Clouds=NewObject<UStaticMeshComponent>(this);
    Clouds->SetMobility(EComponentMobility::Movable); Clouds->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Clouds->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Clouds->SetCastShadow(false); Clouds->SetTranslucentSortPriority(-10); Clouds->RegisterComponent();
    Clouds->SetWorldScale3D(FVector(2500,2500,380));
    CloudMaterial=UMaterialInstanceDynamic::Create(Material(TEXT("WeatherClouds")),this);
    Clouds->SetMaterial(0,CloudMaterial);
    RainMesh=NewObject<UInstancedStaticMeshComponent>(this);
    RainMesh->SetMobility(EComponentMobility::Movable); RainMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    RainMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
    RainMesh->SetMaterial(0,Material(TEXT("WeatherRain"))); RainMesh->SetCastShadow(false); RainMesh->RegisterComponent();
    for(int32 I=0;I<MaxRain;++I) {
        RainMesh->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
        RainPositions.Add(FVector::ZeroVector); RainFloors.Add(0);
    }
    LightningLight=GetWorld()->SpawnActor<ADirectionalLight>();
    LightningLight->SetActorRotation(FRotator(-65,32,0));
    auto Light=LightningLight->GetLightComponent(); Light->SetMobility(EComponentMobility::Movable);
    Light->SetLightColor(FLinearColor(.68f,.8f,1)); Light->SetCastShadows(false); Light->SetIntensity(0);
    ThunderTimer=WeatherRandom.FRandRange(14,24);
}

bool AETWorld::IsSheltered(FVector Eye) const
{
    FHitResult Hit; FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); Params.AddIgnoredActor(UGameplayStatics::GetPlayerCharacter(this,0));
    return GetWorld()->LineTraceSingleByChannel(Hit,Eye+FVector(0,0,15),Eye+FVector(0,0,3500),ECC_Visibility,Params);
}

float AETWorld::RainFloor(FVector Position) const
{
    FHitResult Hit; FCollisionQueryParams Params;
    Params.AddIgnoredActor(this); Params.AddIgnoredActor(UGameplayStatics::GetPlayerCharacter(this,0));
    const FVector End=Position-FVector(0,0,3200);
    return GetWorld()->LineTraceSingleByChannel(Hit,Position,End,ECC_Visibility,Params)?Hit.ImpactPoint.Z+12:End.Z;
}

void AETWorld::UpdateRain(float Delta,FVector Eye)
{
    if(!RainMesh) return;
    RainMesh->SetVisibility(Weather.Rainfall>.005f);
    if(Weather.Rainfall<=.005f) return;
    const int32 Active=FMath::RoundToInt(MaxRain*Weather.Rainfall);
    const FVector Velocity(125+Weather.Wind*210,65,-2200-Weather.Rainfall*350);
    const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Velocity.GetSafeNormal());
    TArray<FTransform> Transforms; Transforms.Reserve(MaxRain);
    for(int32 I=0;I<MaxRain;++I) {
        FVector& P=RainPositions[I]; P+=Velocity*Delta;
        if(I<Active&&(P.Z<RainFloors[I]||FMath::Abs(P.X-Eye.X)>1800||FMath::Abs(P.Y-Eye.Y)>1800||P.Z>Eye.Z+1700)) {
            P=Eye+FVector(WeatherRandom.FRandRange(-1800,1800),WeatherRandom.FRandRange(-1800,1800),1450);
            RainFloors[I]=RainFloor(P);
            P.Z=WeatherRandom.FRandRange(FMath::Max(Eye.Z-1200,RainFloors[I]),P.Z);
        }
        // Cached vertical hits keep drops above rooftops. Suppress nearby indoor streaks too.
        const bool Visible=I<Active&&P.Z>RainFloors[I]&&!(Sheltered&&FVector::DistSquared(P,Eye)<700*700);
        Transforms.Add(FTransform(Rotation,P,Visible?FVector(.008,.008,.45):FVector::ZeroVector));
    }
    RainMesh->BatchUpdateInstancesTransforms(0,Transforms,true,true,true);
}

void AETWorld::UpdateAtmosphereAudio(float Delta,bool Immediate)
{
    auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)); if(!P||!State) return;
    const int32 Track=MusicFor(P->GetActorLocation());
    const float Duck=HasSubtitle()?.22f:P->Panel.IsEmpty()?1.f:.65f;
    for(int32 I=0;I<Music.Num();++I) {
        const float Target=I==Track?State->MusicVolume*.01f*Duck:0;
        Music[I]->SetVolumeMultiplier(Immediate?Target:FMath::FInterpTo(Music[I]->VolumeMultiplier,Target,Delta,.65f));
    }
    const float Outside=Sheltered?.23f:1.f, Menu=P->Panel.IsEmpty()?1.f:.5f;
    const float Levels[]={Weather.Rainfall*.75f,(.12f+Weather.Wind*.48f)};
    for(int32 I=0;I<WeatherAudio.Num();++I) {
        const float Target=State->WeatherVolume*.01f*Outside*Menu*Levels[I];
        WeatherAudio[I]->SetVolumeMultiplier(Immediate?Target:FMath::FInterpTo(WeatherAudio[I]->VolumeMultiplier,Target,Delta,2.f));
        // UE's live filter setters only update active sounds. Retain the same settings
        // on the component so a virtualized/restarted loop also resumes muffled indoors.
        WeatherAudio[I]->bEnableLowPassFilter=Sheltered;
        WeatherAudio[I]->LowPassFilterFrequency=Sheltered?1400.f:20000.f;
        WeatherAudio[I]->SetLowPassFilterEnabled(Sheltered);
        WeatherAudio[I]->SetLowPassFilterFrequency(WeatherAudio[I]->LowPassFilterFrequency);
    }
}

void AETWorld::TickAtmosphere(float Delta)
{
    auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)); if(!State||!P) return;
    Weather=WeatherOverride>=0?ETClimate::Profile(WeatherOverride):ETClimate::Sample(State->Elapsed,State->Seed);
    const double Now=FPlatformTime::Seconds();
    const float AudioDelta=AudioLastTick>0?float(FMath::Clamp(Now-AudioLastTick,0.,.1)):Delta;
    AudioLastTick=Now; UpdateAtmosphereAudio(AudioDelta);
    if(UGameplayStatics::IsGamePaused(this)) return;
    const FVector Eye=P->Camera->GetComponentLocation();
    AtmosphereAge+=Delta; ShelterTick-=Delta;
    if(ShelterTick<=0) {ShelterTick=.3f; Sheltered=IsSheltered(Eye);}
    if(Clouds) Clouds->SetWorldLocation(FVector(Eye.X,Eye.Y,Eye.Z-800));
    RainTick+=Delta;
    if(RainTick>=.05f) {UpdateRain(FMath::Min(RainTick,.2f),Eye); RainTick=0;}
    Lightning=FMath::Max(0.f,Lightning-Delta*1.8f);
    if(Weather.Storminess>.6f) {
        ThunderTimer-=Delta;
        if(ThunderTimer<=0) {Lightning=1; ThunderDelay=WeatherRandom.FRandRange(1.3f,3.3f); ThunderTimer=WeatherRandom.FRandRange(28,52);}
    } else ThunderTimer=FMath::Max(ThunderTimer,10.f);
    if(LightningLight) LightningLight->GetLightComponent()->SetIntensity(Lightning*3.5f);
    if(ThunderDelay>=0) {
        ThunderDelay-=Delta;
        if(ThunderDelay<0&&!SelfTest) Sound(TEXT("WeatherThunder"),State->WeatherVolume*.006f*(Sheltered?.35f:1.f),WeatherRandom.FRandRange(.9f,1.04f));
    }
    if(Review&&!SelfTest&&!AtmosphereAuditWritten&&AtmosphereAge>=8) {
        AtmosphereAuditWritten=true;
        const int32 Track=MusicFor(P->GetActorLocation());
        const FString Report=FString::Printf(TEXT("weather=%s\nrain=%.3f\ncloud=%.3f\nsheltered=%s\nmusic_track=%d\nmusic_playing=%s\nmusic_volume=%.3f\nrain_playing=%s\nwind_playing=%s\nrain_pool=%d\n"),
            *WeatherName(),Weather.Rainfall,Weather.Cloud,Sheltered?TEXT("true"):TEXT("false"),Track,
            Music.IsValidIndex(Track)&&Music[Track]->IsPlaying()?TEXT("true"):TEXT("false"),Music.IsValidIndex(Track)?Music[Track]->VolumeMultiplier:0,
            WeatherAudio.Num()>0&&WeatherAudio[0]->IsPlaying()?TEXT("true"):TEXT("false"),WeatherAudio.Num()>1&&WeatherAudio[1]->IsPlaying()?TEXT("true"):TEXT("false"),RainMesh?RainMesh->GetInstanceCount():0);
        FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("atmosphere-playback.txt")));
    }
}
