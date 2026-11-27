#include "ETRingLine.h"
#include "ETGame.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
    UStaticMesh* RailMesh(const TCHAR* Name) {
        const FString Asset=FString(TEXT("/Game/EARTHtotaled/Art/SM_Ring"))+Name;
        return LoadObject<UStaticMesh>(nullptr,*Asset);
    }
    constexpr double Step=UE_TWO_PI/ETRing::Spans;
}

AETRingLine::AETRingLine()
{
    PrimaryActorTick.bCanEverTick=true;
    PrimaryActorTick.TickGroup=TG_PrePhysics;
    auto* Anchor=CreateDefaultSubobject<USceneComponent>(TEXT("Track origin"));
    Anchor->SetMobility(EComponentMobility::Static);
    SetRootComponent(Anchor);
    Deck=CreateDefaultSubobject<UBoxComponent>(TEXT("Freight deck"));
    Deck->SetupAttachment(Anchor);
    Deck->SetMobility(EComponentMobility::Movable);
    Deck->SetBoxExtent(FVector(1180,306,29));
    Deck->SetCollisionProfileName(TEXT("BlockAllDynamic"));
    Deck->SetGenerateOverlapEvents(false);
    Deck->CanCharacterStepUpOn=ECB_Yes;
    Sled=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Flatbed"));
    Sled->SetupAttachment(Deck);
    Sled->SetRelativeLocation(FVector(0,0,29));
    Sled->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Guide=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Guideway spans"));
    Piers=CreateDefaultSubobject<UHierarchicalInstancedStaticMeshComponent>(TEXT("Viaduct piers"));
    for(auto* H:{Guide,Piers}) {
        H->SetupAttachment(Anchor);
        H->SetMobility(EComponentMobility::Static);
        H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    }
    Piers->SetCollisionProfileName(TEXT("BlockAll"));
}

void AETRingLine::Initialize(AETWorld* World)
{
    Zone=World;
    Sled->SetStaticMesh(RailMesh(TEXT("Sled")));
    Guide->SetStaticMesh(RailMesh(TEXT("Guide")));
    Piers->SetStaticMesh(RailMesh(TEXT("Pier")));
    Heights.SetNumUninitialized(ETRing::Spans);
    TArray<double> Ground;
    Ground.Reserve(ETRing::Spans);
    for(int I=0;I<ETRing::Spans;++I) {
        const FVector2D P=ETRing::Position(I*Step);
        Ground.Add(Zone->Height(P.X,P.Y));
    }
    for(int I=0;I<ETRing::Spans;++I) {
        // Span neighbouring peaks, so the viaduct neither clips a crest nor follows every dip.
        double Bed=Ground[I];
        for(int J=-8;J<=8;++J) Bed=FMath::Max(Bed,Ground[(I+J+ETRing::Spans)%ETRing::Spans]-FMath::Abs(J)*175.);
        Heights[I]=Bed+3600.;
    }
    TArray<FTransform> Rails,Supports;
    Rails.Reserve(ETRing::Spans); Supports.Reserve(ETRing::Spans/2);
    for(int I=0;I<ETRing::Spans;++I) {
        const auto A=RoutePoint(I*Step),B=RoutePoint((I+1)*Step);
        const auto D=B-A;
        Rails.Emplace(D.Rotation(),(A+B)*.5,FVector((D.Size()+3.)/2200.,1,1));
        if(I%2==0) {
            const double Top=A.Z-226,Scale=(Top-Ground[I]+72.)/898.;
            Supports.Emplace(FRotator(0,D.Rotation().Yaw,0),FVector(A.X,A.Y,Top+101.*Scale),FVector(1,1,Scale));
        }
    }
    Guide->AddInstances(Rails,false,false,false);
    Piers->AddInstances(Supports,false,false,false);
    auto* StationMesh=RailMesh(TEXT("Station"));
    for(int I=0;I<ETRing::Stops;++I) {
        const auto Pose=Platform(I);
        auto* Building=NewObject<UStaticMeshComponent>(this);
        Building->SetupAttachment(GetRootComponent());
        Building->SetMobility(EComponentMobility::Static);
        Building->SetRelativeTransform(Pose);
        Building->SetStaticMesh(StationMesh);
        Building->SetCollisionProfileName(TEXT("BlockAll"));
        Building->RegisterComponent();
        auto* Sign=NewObject<UTextRenderComponent>(this);
        Sign->SetupAttachment(Building);
        Sign->SetRelativeLocation(FVector(0,790,195));
        Sign->SetRelativeRotation(FRotator(0,-90,0));
        Sign->SetHorizontalAlignment(EHTA_Center);
        Sign->SetVerticalAlignment(EVRTA_TextCenter);
        Sign->SetWorldSize(55);
        Sign->SetTextRenderColor(FColor(138,244,221));
        Sign->SetText(FText::FromString(FString::Printf(TEXT("RINGLINE   %02d\n%s"),I+1,*FString(ETRing::Name(I)).ToUpper())));
        Sign->SetCullDistance(28000);
        Sign->RegisterComponent();
    }
    PositionCar(Zone->State->RingClock);
    MatchViewDistance(Zone->State->ViewDistance);
}

void AETRingLine::MatchViewDistance(int Metres)
{
    if(Metres==LastDistance) return;
    LastDistance=Metres;
    const int Far=Metres*100;
    Guide->SetCullDistances(Far*85/100,Far);
    Piers->SetCullDistances(Far*85/100,Far);
    TInlineComponentArray<UStaticMeshComponent*> Meshes(this);
    for(auto* Mesh:Meshes) if(Mesh!=Guide&&Mesh!=Piers) Mesh->SetCullDistance(Far);
}

FVector AETRingLine::RoutePoint(double Angle) const
{
    const double Index=Angle/Step;
    const int Base=FMath::FloorToInt32(Index);
    const int I=((Base%ETRing::Spans)+ETRing::Spans)%ETRing::Spans;
    return FVector(ETRing::Position(Angle), FMath::Lerp(Heights[I],Heights[(I+1)%ETRing::Spans],Index-Base));
}

FTransform AETRingLine::Platform(int Stop) const
{
    const double A=ETRing::Schedule().Angles[Stop];
    const auto P=RoutePoint(A);
    return FTransform(FRotator(0,FMath::RadiansToDegrees(A)+90.,0),P,FVector(1,1,(P.Z-Zone->Height(P.X,P.Y))/3600.));
}

void AETRingLine::PositionCar(double Clock,float Delta)
{
    Phase=ETRing::Schedule().At(Clock);
    const FVector Forward=RoutePoint(Phase.Angle+Step*.5)-RoutePoint(Phase.Angle-Step*.5);
    FRotator Heading=Forward.Rotation();
    if(Phase.Docked) Heading.Pitch=0;
    const auto P=RoutePoint(Phase.Angle)-Heading.RotateVector(FVector(0,0,29));
    const auto Prev=Deck->GetComponentLocation();
    Deck->SetWorldLocationAndRotation(P,Heading);
    Deck->ComponentVelocity=Delta>SMALL_NUMBER?(P-Prev)/Delta:FVector::ZeroVector;
}
void AETRingLine::Tick(float Delta)
{
    Super::Tick(Delta);
    if(!Zone||!Zone->State||Delta<=0) return;
    Zone->State->RingClock=ETRing::Schedule().Wrap(Zone->State->RingClock+Delta);
    PositionCar(Zone->State->RingClock,Delta);
}
bool AETRingLine::Riding(const AETCharacter& P) const {return P.GetMovementBaseObject()==Deck;}
void AETRingLine::Capture(AETCharacter& P)
{
    Zone->State->OnRing=Riding(P);
    if(Zone->State->OnRing) Zone->State->RingOffset=Deck->GetComponentTransform().InverseTransformPosition(P.GetActorLocation());
}
FVector AETRingLine::SavedPosition() const
{
    const FVector Local=Zone->State->RingOffset;
    return Deck->GetComponentTransform().TransformPosition(FVector(FMath::Clamp(Local.X,-1160.,1160.),FMath::Clamp(Local.Y,-285.,285.),FMath::Clamp(Local.Z,80.,145.)));
}
void AETRingLine::Restore(AETCharacter& P)
{
    P.GetCharacterMovement()->AddTickPrerequisiteActor(this);
    if(!Zone->State->OnRing) return;
    P.SetActorLocation(SavedPosition(),false,nullptr,ETeleportType::TeleportPhysics);
    P.GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    P.SetBase(Deck);
}
int AETRingLine::BoardingArea(const AETCharacter& P,bool& Street) const
{
    for(int I=0;I<ETRing::Stops;++I) {
        if(FVector2D::DistSquared(ETRing::Station(I),FVector2D(P.GetActorLocation()))>FMath::Square(2100.)) continue;
        const FVector L=Platform(I).InverseTransformPosition(P.GetActorLocation());
        if(FMath::Abs(L.X)>1225||L.Y<315||L.Y>1660) continue;
        Street=L.Z<-3230&&L.Z>-3720;
        if(Street||(L.Z>10&&L.Z<320)) return I;
    }
    return INDEX_NONE;
}
FString AETRingLine::Prompt(const AETCharacter& P) const
{
    if(Riding(P))
        return Phase.Docked?FString::Printf(TEXT("RINGLINE   /   %s   /   departs in %ds   /   Space to jump off"),ETRing::Name(Phase.Stop),FMath::CeilToInt(Phase.Seconds)):
            FString::Printf(TEXT("RINGLINE   /   %d km/h   /   %s in %ds"),FMath::RoundToInt(Phase.Speed*.036),ETRing::Name(Phase.Stop),FMath::CeilToInt(Phase.Seconds));
    bool Street=false; const int I=BoardingArea(P,Street);
    if(I==INDEX_NONE) return {};
    if(Street) return TEXT("[E] Freight lift to the Ringline platform");
    if(Phase.Docked&&Phase.Stop==I) return TEXT("Ringline docked   /   jump across the yellow edge to board");
    return TEXT("[E] Wait for the Ringline   /   jump aboard when it docks");
}
bool AETRingLine::UseStation(AETCharacter& P)
{
    if(Riding(P)) return false;
    bool Street=false; const int I=BoardingArea(P,Street);
    if(I==INDEX_NONE) return false;
    if(Street) {
        const FVector Top=Platform(I).TransformPosition(FVector(0,615,0))+FVector(0,0,P.GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4);
        if(P.TeleportTo(Top,P.GetActorRotation())) P.GetCharacterMovement()->StopMovementImmediately();
    } else if(!Phase.Docked||Phase.Stop!=I) {
        Zone->State->RingClock=ETRing::Schedule().Arrival[I];
        PositionCar(Zone->State->RingClock);
    }
    return true;
}
