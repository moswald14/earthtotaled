#include "ETGame.h"
#include "ETGeology.h"
#include "ETLandfall.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundWaveProcedural.h"
#include "Audio.h"

namespace {
    void CrashCapsule(AETWorld& W)
    {
        for(auto* Actor:W.StoryActors) if(IsValid(Actor)&&Actor->ActorHasTag(TEXT("ET_FlightWreck"))) return;
        const FVector Rest(-580,-1610,W.SurfaceHeight(-580,-1610)+61);
        auto* Pod=W.GetWorld()->SpawnActor<AActor>(Rest,FRotator(9,41,-12)); if(!Pod) return;
        auto* Root=NewObject<USceneComponent>(Pod);Pod->SetRootComponent(Root);Root->RegisterComponent();
        Pod->SetActorLocationAndRotation(Rest,FRotator(9,41,-12));Pod->Tags.Add(TEXT("ET_FlightWreck"));W.StoryActors.Add(Pod);
        auto Part=[&](const TCHAR* Shape,const TCHAR* Material,FVector P,FVector Scale,FRotator Rotation=FRotator::ZeroRotator) {
            auto* Piece=NewObject<UStaticMeshComponent>(Pod);Piece->SetupAttachment(Root);
            FString Name=FString(TEXT("/Engine/BasicShapes/"))+Shape+TEXT(".")+Shape;
            Piece->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Name));Piece->SetMaterial(0,W.Material(Material));
            Piece->SetRelativeLocation(P);Piece->SetRelativeRotation(Rotation);Piece->SetRelativeScale3D(Scale);
            Piece->SetCollisionEnabled(ECollisionEnabled::NoCollision);Piece->SetCullDistance(16000);Piece->RegisterComponent();
        };
        Part(TEXT("Cylinder"),TEXT("AlloyHull"),{0,0,0},{1.42,1.46,2.72},{90,0,0});
        Part(TEXT("Sphere"),TEXT("DarkMetal"),{128,0,0},{1.24,1.43,1.42});
        Part(TEXT("Sphere"),TEXT("Glass"),{93,0,46},{1.30,1.14,.69},{-12,0,0});
        for(int X:{-105,14}) Part(TEXT("Cylinder"),TEXT("Rust"),{double(X),0,0},{1.47,1.51,.12},{90,0,0});
        for(int Side:{-1,1}) {
            Part(TEXT("Cube"),TEXT("Steel"),{-69,Side*76.,-16},{1.36,.76,.09},{0,Side*18.,Side*15.});
            Part(TEXT("Cylinder"),TEXT("DarkMetal"),{-145,Side*41.,-19},{.49,.50,.73},{90,0,0});
        }
        auto* Stencil=NewObject<UTextRenderComponent>(Pod);Stencil->SetupAttachment(Root);
        Stencil->SetRelativeLocation(FVector(0,-73,-4));Stencil->SetRelativeRotation(FRotator(0,-90,0));Stencil->SetWorldSize(18);
        Stencil->SetHorizontalAlignment(EHTA_Center);Stencil->SetText(FText::FromString(TEXT("fraudCORP  /  FC-09")));Stencil->SetTextRenderColor(FColor(226,183,94));Stencil->RegisterComponent();
    }
}

void AETCharacter::StartArrival()
{
    auto* PC=Cast<APlayerController>(Controller);
    OpenPanel(TEXT("arrival"),TEXT("fraudCORP"),TEXT(""));
    FlightCamera=GetWorld()->SpawnActor<ACameraActor>(); FlightSeconds=0;FlightCue=-1;
    ArrivalTime=FPlatformTime::Seconds();
    if(!PC||!FlightCamera) {FinishArrival();return;}
    Gun->SetVisibility(false);PC->SetViewTarget(FlightCamera);
    UGameplayStatics::SetGamePaused(this,false);TickArrival();
}
void AETCharacter::TickArrival()
{
    FlightSeconds=FPlatformTime::Seconds()-ArrivalTime;
    const float T=FlightSeconds;
    if(T>=19||!FlightCamera) {FinishArrival();return;}
    const int Cue=T<4?0:T<7?1:T<13?2:T<15?3:4;
    if(Cue!=FlightCue) {
        FlightCue=Cue;
        if(Cue==1) World->Sound(TEXT("BorerPulse"),.68f,.56f);
        if(Cue==2) World->Sound(TEXT("JetThrust"),.8f,.55f);
        if(Cue==3) {CrashCapsule(*World);World->Sound(TEXT("WeatherThunder"),.88f,1.4f);World->Sound(TEXT("BorerPulse"),.7f,.75f);}
        if(Cue==4) {
            TArray<uint8> Data; FWaveModInfo Info;
            if(FFileHelper::LoadFileToArray(Data,*(FPaths::ProjectContentDir()/TEXT("Data/arrival-line.wav")))&&Info.ReadWaveInfo(Data.GetData(),Data.Num())&&*Info.pBitsPerSample==16) {
                auto* Voice=NewObject<USoundWaveProcedural>(this);
                const int SampleRate=*Info.pSamplesPerSec,Channels=*Info.pChannels;
                Voice->SetSampleRate(SampleRate);Voice->NumChannels=Channels;Voice->Duration=float(Info.SampleDataSize)/(SampleRate*Channels*2);
                Voice->QueueAudio(Info.SampleDataStart,Info.SampleDataSize);
                FlightAudio=UGameplayStatics::SpawnSound2D(this,Voice,.88f);
            }
        }
    }
    FVector Eye,Target; float Roll=0, Fov=79;
    if(T<7) {
        Eye=FVector(-22000,-38000,34000); Target=Eye+FVector(1,1,1)*9000;Roll=FMath::Sin(T*.4f)*3;
        Fov=80+FMath::Clamp((T-4)*12.f,0.f,32.f);
    } else {
        const float A=FMath::Clamp((T-7)/6.f,0.f,1.f);
        Eye=FMath::Lerp(FVector(-15000,-28000,14800),FVector(-180,-1750,190),A*(2-A));
        Target=FVector(0,0,150);Roll=(1-A)*FMath::Sin(T*3)*10;Fov=96-A*16;
        if(T>=13) {const float J=FMath::Exp(-2*(T-13));Eye+=FVector(16*FMath::Sin(T*43),22*FMath::Sin(T*32),25*FMath::Cos(T*47))*J;Roll=-8+J*11;}
    }
    FRotator Rotation=(Target-Eye).Rotation();Rotation.Roll=Roll;
    FlightCamera->SetActorLocationAndRotation(Eye,Rotation);FlightCamera->GetCameraComponent()->SetFieldOfView(Fov);
}
void AETCharacter::FinishArrival()
{
    if(Panel!=TEXT("arrival")||World->State->ArrivalStage!=0) return;
    CrashCapsule(*World);
    auto& S=*World->State; S.ArrivalStage=1;S.IntroRead=true;
    S.Unlocked.Remove(0);S.Unlocked.AddUnique(ETLandfall::Pistol);SelectWeapon(ETLandfall::Pistol);Gun->SetVisibility(true);
    if(auto* PC=Cast<APlayerController>(Controller)) PC->SetViewTarget(this);
    if(FlightAudio){FlightAudio->Stop();FlightAudio=nullptr;}
    if(FlightCamera){FlightCamera->Destroy();FlightCamera=nullptr;}
    Panel=TEXT("handoff");ClosePanel();
    World->Say(TEXT("I am going to require a gun   /   Drifter .44 issued. G equips the trench borer."));
}
void AETWorld::MakeTrenchStakes()
{
    if(State->ArrivalStage>0) CrashCapsule(*this);
    for(int Side=0;Side<4;++Side) {
        if(State->TrenchSides&(1<<Side)) continue;
        const auto XY=ETLandfall::Stake(Side);
        const FString Label=FString::Printf(TEXT("%s bank / G then fire at the marker"),ETLandfall::Bank(Side));
        if(auto* A=Item(FString::Printf(TEXT("first-watch-%d"),Side),TEXT("stake"),Label,FVector(XY,SurfaceHeight(XY.X,XY.Y)),Side)) {
            A->SetActorRotation(FRotator(0,Side*90.f,0)); A->SetActorScale3D(FVector(.54));
            auto* Sign=NewObject<UTextRenderComponent>(A);Sign->SetupAttachment(A->Mesh);
            Sign->SetText(FText::FromString(TEXT("TRENCH / 09")));Sign->SetHorizontalAlignment(EHTA_Center);
            Sign->SetRelativeLocation(FVector(0,-20,310));Sign->SetRelativeRotation(FRotator(0,-90,0));Sign->SetWorldSize(39);Sign->RegisterComponent();StoryActors.Add(A);
        }
    }
}
void UETGeology::IndexTrench(int Side,TSet<FIntVector>& Dirty)
{
    AddCavity({FVector(ETLandfall::Corner(Side),-90),FVector(ETLandfall::Corner(Side+1),-90),212},Dirty);
}
bool UETGeology::CutTrench(int Side,FString& Reward)
{
    auto& S=*World->State;
    if(S.ArrivalStage!=1||Side<0||Side>3||(S.TrenchSides&(1<<Side))) return false;
    const FVector2D A=ETLandfall::Corner(Side),B=ETLandfall::Corner(Side+1);
    const auto Span=FBox2D(A.ComponentMin(B),A.ComponentMax(B)).ExpandBy(225);
    for(auto& Pair:S.Builds) {
        const auto& Build=Pair.Value;const FVector E=AETConstruction::Extents(Build.Piece);
        const FBox Box=FBox(-E,E).TransformBy(AETConstruction::Shape(Build));
        if(Span.Intersect(FBox2D(FVector2D(Box.Min),FVector2D(Box.Max)))) {Reward=TEXT("Salvage your structure on this bank before cutting the trench.");return false;}
    }
    S.TrenchSides|=1<<Side;
    TSet<FIntVector> Dirty;IndexTrench(Side,Dirty);
    TSet<AETRegion*> Refresh;
    for(auto Key:Dirty) if(auto* R=World->Regions.FindRef(ETRock::Region(Key));R&&R->TerrainGrid) {
        Rebuild(*R,Key); if(Key.Z==-1) {ClearScatter(*R,{Key.X,Key.Y});Refresh.Add(R);}
    }
    for(auto* R:Refresh) R->RefreshSurfaceCollision();
    const auto Center=Span.GetCenter();
    World->CheckFoundations(FVector(Center,0),FVector(Span.GetExtent(),400));
    for(auto* Actor:World->StoryActors) if(auto* Stake=Cast<AETItem>(Actor);IsValid(Stake)&&Stake->Kind==TEXT("stake")&&Stake->Amount==Side) Stake->Destroy();
    Reward=FString::Printf(TEXT("%s bank cut"),ETLandfall::Bank(Side));
    if(S.TrenchSides==15) {S.ArrivalStage=2;S.Gold+=30;S.Water+=3;Reward=TEXT("First Watch complete / +30 candybar wrappers and 3 water");}
    return true;
}
bool UETGeology::BugCanStep(FVector From,FVector To) const
{
    const double Ground=World->SurfaceHeight(From.X,From.Y);
    if(From.Z<Ground-360) return true; // Native cave populations can move inside their galleries.
    const int N=FMath::Clamp(FMath::CeilToInt(FVector::Dist2D(From,To)/55.),1,32);
    for(int I=1;I<=N;++I) {const auto P=FMath::Lerp(From,To,double(I)/N);if(SurfaceOpening(P.X,P.Y)<45) return false;}
    return true;
}
