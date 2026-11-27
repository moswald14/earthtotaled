#include "ETRingLine.h"
#include "ETGame.h"
#include "ETRailDistricts.h"
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

void AETRingLine::Initialize(AETWorld* World,int Line)
{
    Zone=World; Service=Line;
    const bool Straight=ETRing::Shuttle(Service);
    const int Spans=ETRing::SpanCount(Service),Samples=Spans+(Straight?1:0);
    const double Step=ETRing::StepSize(Service);
    Sled->SetStaticMesh(RailMesh(TEXT("Sled")));
    Guide->SetStaticMesh(RailMesh(TEXT("Guide")));
    Piers->SetStaticMesh(RailMesh(TEXT("Pier")));
    Heights.SetNumUninitialized(Samples);
    TArray<double> Ground;
    Ground.Reserve(Samples);
    for(int I=0;I<Samples;++I) {
        const FVector2D P=ETRing::Track(Service,I*Step);
        Ground.Add(Zone->Height(P.X,P.Y));
    }
    if(Straight) {
        // An express span keeps one grade between terminals, high enough for the tallest crossing.
        double Clearance=3600.;
        for(int I=0;I<Samples;++I) {
            Heights[I]=FMath::Lerp(Ground[0],Ground.Last(),double(I)/Spans);
            Clearance=FMath::Max(Clearance,Ground[I]+3600.-Heights[I]);
        }
        for(double& Height:Heights) Height+=Clearance;
    } else {
        for(int I=0;I<Samples;++I) {
            double Bed=Ground[I];
            for(int J=-8;J<=8;++J) Bed=FMath::Max(Bed,Ground[(I+J+Spans)%Spans]-FMath::Abs(J)*175.);
            Heights[I]=Bed+3600.;
        }
    }
    TArray<FTransform> Rails,Supports;
    Rails.Reserve(Spans); Supports.Reserve(Spans/2+1);
    for(int I=0;I<Spans;++I) {
        const auto A=RoutePoint(I*Step),B=RoutePoint((I+1)*Step);
        const auto D=B-A;
        Rails.Emplace(D.Rotation(),(A+B)*.5,FVector((D.Size()+3.)/2200.,1,1));
        if(I%2==0) {
            const double Top=A.Z-226,Scale=(Top-Ground[I]+72.)/898.;
            Supports.Emplace(FRotator(0,D.Rotation().Yaw,0),FVector(A.X,A.Y,Top+101.*Scale),FVector(1,1,Scale));
        }
        if(Straight&&I==Spans-1) {
            const double Top=B.Z-226,Scale=(Top-Ground.Last()+72.)/898.;
            Supports.Emplace(FRotator::ZeroRotator,FVector(B.X,B.Y,Top+101.*Scale),FVector(1,1,Scale));
        }
    }
    Guide->AddInstances(Rails,false,false,false);
    Piers->AddInstances(Supports,false,false,false);
    auto* StationMesh=RailMesh(TEXT("Station"));
    for(int I=0;I<ETRing::StopCount(Service);++I) {
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
        const int City=ETRing::CityAt(Service,I);
        Sign->SetTextRenderColor(ETRailDistrict::Get(City).Color);
        Sign->SetWorldSize(43);
        Sign->SetText(FText::FromString(FString::Printf(TEXT("%s   /   %s\n%s"),*FString(ETRing::ServiceName(Service)).ToUpper(),*FString(ETRing::CityName(City)).ToUpper(),ETRailDistrict::Get(City).Yard)));
        Sign->SetCullDistance(28000);
        Sign->RegisterComponent();
        const FVector Lobby=StreetLift(I);
        auto* CallBox=NewObject<UStaticMeshComponent>(this);
        CallBox->SetupAttachment(GetRootComponent());CallBox->SetMobility(EComponentMobility::Static);
        CallBox->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EARTHtotaled/Art/SM_Sign")));
        Zone->DressMesh(CallBox,TEXT("Greenline"));
        CallBox->SetRelativeLocation(Lobby+FVector(155,0,0));CallBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);CallBox->RegisterComponent();
        for(int Side:{-1,1}) {
            auto* Label=NewObject<UTextRenderComponent>(this);Label->SetupAttachment(CallBox);
            Label->SetRelativeLocation(FVector(0,Side*18.,328));Label->SetRelativeRotation(FRotator(0,Side*90.,0));
            Label->SetHorizontalAlignment(EHTA_Center);Label->SetVerticalAlignment(EVRTA_TextCenter);
            Label->SetWorldSize(26);Label->SetTextRenderColor(ETRailDistrict::Get(City).Color);
            Label->SetText(FText::FromString(FString::Printf(TEXT("%s\n[E]  STREET LIFT"),*FString(ETRing::ServiceName(Service)).ToUpper())));
            Label->SetCullDistance(13500);Label->RegisterComponent();
        }
    }
    PositionCar(Clock());
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
    const bool Straight=ETRing::Shuttle(Service);
    const int Spans=ETRing::SpanCount(Service);
    const double Index=(Straight?FMath::Clamp(Angle,0.,1.):Angle)/ETRing::StepSize(Service);
    const int Base=FMath::FloorToInt32(Index);
    const int I=Straight?FMath::Min(Base,Spans):((Base%Spans)+Spans)%Spans;
    const int Next=Straight?FMath::Min(I+1,Spans):(I+1)%Spans;
    return FVector(ETRing::Track(Service,Angle), FMath::Lerp(Heights[I],Heights[Next],Index-Base));
}

FTransform AETRingLine::Platform(int Stop) const
{
    const double A=ETRing::DockParam(Service,Stop);
    const auto P=RoutePoint(A);
    return FTransform(FRotator(0,ETRing::Shuttle(Service)?0.:FMath::RadiansToDegrees(A)+90.,0),P,FVector(1,1,(P.Z-Zone->Height(P.X,P.Y))/3600.));
}
FVector AETRingLine::StreetLift(int Stop) const
{
    const auto XY=ETRing::LiftPoint(Service,Stop);
    return FVector(XY,Zone->Height(XY.X,XY.Y));
}

void AETRingLine::PositionCar(double Time,float Delta)
{
    const double Step=ETRing::StepSize(Service);
    Phase=ETRing::ServiceAt(Service,Time);
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
    double& Time=Clock();
    Time=FMath::Fmod(Time+Delta,ETRing::Period(Service));
    PositionCar(Time,Delta);
}
double& AETRingLine::Clock() const
{
    auto& S=*Zone->State;
    switch(Service) {
    case ETRing::East: return S.EastRingClock;
    case ETRing::NorthLink: return S.NorthLinkClock;
    case ETRing::SouthLink: return S.SouthLinkClock;
    default: return S.RingClock;
    }
}
FVector2D AETRingLine::TravelDirection() const
{
    if(ETRing::Shuttle(Service)) return FVector2D(Phase.Direction,0);
    const auto R=ETRing::Radial(Phase.Angle);
    return FVector2D(-R.Y,R.X);
}
bool AETRingLine::Riding(const AETCharacter& P) const {return P.GetMovementBaseObject()==Deck;}
AETRingLine* AETRingLine::PassengerLine(const AETCharacter& P)
{
    auto* Line=Cast<AETRingLine>(APawn::GetMovementBaseActor(&P));
    return Line&&Line->Riding(P)?Line:nullptr;
}
void AETRingLine::Capture(AETCharacter& P)
{
    Zone->State->OnRing=Riding(P);
    if(Zone->State->OnRing) {
        Zone->State->RailService=Service;
        Zone->State->RingOffset=Deck->GetComponentTransform().InverseTransformPosition(P.GetActorLocation());
    }
}
FVector AETRingLine::SavedPosition() const
{
    const FVector Local=Zone->State->RingOffset;
    return Deck->GetComponentTransform().TransformPosition(FVector(FMath::Clamp(Local.X,-1160.,1160.),FMath::Clamp(Local.Y,-285.,285.),FMath::Clamp(Local.Z,80.,145.)));
}
void AETRingLine::Restore(AETCharacter& P)
{
    P.GetCharacterMovement()->AddTickPrerequisiteActor(this);
    if(!Zone->State->OnRing||Zone->State->RailService!=Service) return;
    P.SetActorLocation(SavedPosition(),false,nullptr,ETeleportType::TeleportPhysics);
    P.GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    P.SetBase(Deck);
}
int AETRingLine::BoardingArea(const AETCharacter& P,bool& Street) const
{
    for(int I=0;I<ETRing::StopCount(Service);++I) {
        if(FVector2D::DistSquared(ETRing::Dock(Service,I),FVector2D(P.GetActorLocation()))>FMath::Square(2800.)) continue;
        const auto Lobby=StreetLift(I),Here=P.GetActorLocation();
        if(FVector::DistSquared2D(Here,Lobby)<FMath::Square(430.)&&FMath::Abs(Here.Z-Lobby.Z-95.)<185.) {Street=true;return I;}
        const FVector L=Platform(I).InverseTransformPosition(P.GetActorLocation());
        if(FMath::Abs(L.X)>1225||L.Y<315||L.Y>1660) continue;
        Street=L.Z<-3230&&L.Z>-3720;
        if(Street||(L.Z>10&&L.Z<320)) return I;
    }
    return INDEX_NONE;
}
FString AETRingLine::Prompt(const AETCharacter& P) const
{
    const auto Line=ETRing::ServiceName(Service);
    if(Riding(P)) {
        const int City=ETRing::CityAt(Service,Phase.Stop);
        return Phase.Docked?FString::Printf(TEXT("%s  /  %s  /  %s  /  Space to get off  /  %ds"),Line,ETRing::CityName(City),ETRailDistrict::Get(City).Yard,FMath::CeilToInt(Phase.Seconds)):
            FString::Printf(TEXT("%s  /  %d km/h  /  %s in %ds"),Line,FMath::RoundToInt(Phase.Speed*.036),ETRing::CityName(City),FMath::CeilToInt(Phase.Seconds));
    }
    if(IsValid(P.Focus)||P.FocusBuild.IsValid()||P.TreeInReach) return {};
    bool Street=false; const int I=BoardingArea(P,Street);
    if(I==INDEX_NONE) return {};
    if(Street) {
        const int City=ETRing::CityAt(Service,I),Link=ETRing::Connection(City);
        if(Link!=INDEX_NONE) {
            const bool Eastward=ETRing::Shuttle(Service)?City>=ETRing::Stops:City<ETRing::Stops;
            const int Other=ETRing::Shuttle(Service)?City/ETRing::Stops:Link;
            return FString::Printf(TEXT("[E] Lift to %s  /  %s: 68 m %s"),Line,ETRing::ServiceName(Other),Eastward?TEXT("east"):TEXT("west"));
        }
        return FString::Printf(TEXT("[E] Lift to %s  /  M then L for local loot"),Line);
    }
    if(Platform(I).InverseTransformPosition(P.GetActorLocation()).Y>=735) return FString::Printf(TEXT("[E] Street exit  /  %s"),ETRailDistrict::Get(ETRing::CityAt(Service,I)).Yard);
    if(Phase.Docked&&Phase.Stop==I) return FString::Printf(TEXT("%s docked  /  Space across the yellow edge  /  street lift at rear"),Line);
    const double ETA=FMath::Fmod(ETRing::DockTime(Service,I)-Clock()+ETRing::Period(Service),ETRing::Period(Service));
    return FString::Printf(TEXT("[E] Wait for %s  /  arrives in %ds  /  street lift at rear"),Line,FMath::CeilToInt(ETA));
}
bool AETRingLine::UseStation(AETCharacter& P)
{
    if(Riding(P)) return false;
    bool Street=false; const int I=BoardingArea(P,Street);
    if(I==INDEX_NONE) return false;
    if(Street) {
        const FVector Top=Platform(I).TransformPosition(FVector(0,615,0))+FVector(0,0,P.GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+4);
        if(P.TeleportTo(Top,P.GetActorRotation())) P.GetCharacterMovement()->StopMovementImmediately();
    } else if(Platform(I).InverseTransformPosition(P.GetActorLocation()).Y>=735) {
        const FVector StreetExit=StreetLift(I)+FVector(0,0,P.GetCapsuleComponent()->GetScaledCapsuleHalfHeight()+5);
        if(P.TeleportTo(StreetExit,P.GetActorRotation())) P.GetCharacterMovement()->StopMovementImmediately();
    } else if(!Phase.Docked||Phase.Stop!=I) {
        Clock()=ETRing::DockTime(Service,I);
        PositionCar(Clock());
    }
    return true;
}
