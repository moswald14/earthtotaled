#include "ETGame.h"
#include "ETAgentMovement.h"
#include "ETTerrainMath.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Components/TextRenderComponent.h"

namespace {
    constexpr float Shift=720.f;
    constexpr int Wage=8;
    FString Prefix(FIntPoint C) {return FString::Printf(TEXT("town/%d,%d/"),C.X,C.Y);}
    FGuid WorksId(FIntPoint C,int Piece) {return FGuid(0x45544f57,uint32(C.X),uint32(C.Y),uint32(Piece+1));}
    int CountBits(int Mask) {int N=0; for(;Mask;Mask&=Mask-1) ++N; return N;}
}
int AETWorld::TownPopulation(FIntPoint Cell,int Wave) const
{
    if(Cell==FIntPoint::ZeroValue) return 0;
    const uint32 H=SeedFor(Cell,9301);
    if(!Wave&&FMath::Abs(Cell.X)+FMath::Abs(Cell.Y)>1&&H%4==0) return 0;
    return 5+int(H%4)+FMath::Min(Wave,3);
}
int AETWorld::TownRemaining(FIntPoint Cell) const
{
    const auto* T=State->Towns.Find(Cell);
    if(T&&T->Claimed) return 0;
    return FMath::Max(0,TownPopulation(Cell,T?T->Wave:0)-(T?CountBits(T->Dead):0));
}
FString AETWorld::TownStatus(FIntPoint Cell) const
{
    if(Cell==FIntPoint::ZeroValue) return TEXT("SHELTER 09  /  SANCTUARY");
    const auto* T=State->Towns.Find(Cell);
    if(T&&T->Claimed) return TEXT("RECLAIMED  /  SURVIVOR DISTRICT");
    const int N=TownRemaining(Cell);
    return N?FString::Printf(TEXT("INFESTED  /  %d ZOMBIES"),N):TEXT("FREE TOWN  /  REFUGE");
}
FVector AETWorld::TowerLocation(FIntPoint C) const
{
    if(const auto* Cached=TowerCache.Find(C)) return *Cached;
    const auto Center=FVector2D(C)*ETNav::DistrictSpacing;
    FRandomStream R(SeedFor(C,713)); const double Az=R.FRandRange(0.f,2.f*PI);
    FVector Best(Center, -1.e12); double Score=-1.e12;
    for(int I=0;I<32;++I) {
        const double A=Az+I*(2.*PI/32.),Range=R.FRandRange(31000.f,41800.f);
        const auto P=Center+FVector2D(FMath::Cos(A),FMath::Sin(A))*Range;
        bool Clear=true; FBox2D Bounds(P-FVector2D(1200.),P+FVector2D(1200.));
        ETNav::Roads(Bounds,[&](const ETNav::FRoad& Road){if(Road.Distance(P)<Road.Width*.5+800) Clear=false;});
        ETNav::Sites(Bounds,[&](const ETNav::FSite& Site){if(Site.InGrounds(P,1200)) Clear=false;});
        const FIntPoint Tile(FMath::FloorToInt(P.X/RegionSize),FMath::FloorToInt(P.Y/RegionSize));
        for(int House=0;House<2;++House) if(FVector2D::Distance(P,FVector2D(HouseAnchor(Tile,House)))<3000) Clear=false;
        const double Z=SurfaceHeight(P.X,P.Y),Rank=Z+(Clear?100000.:0.);
        if(Rank>Score) {Score=Rank;Best=FVector(P,Z);}
    }
    if(TowerCache.Num()>=64) TowerCache.Reset();TowerCache.Add(C,Best);return Best;
}
void AETRegion::AddTownLandmarks()
{
    const FVector Center=GetActorLocation()+FVector(16000,16000,0);
    const auto Cell=ETNav::District(FVector2D(Center));
    auto Owns=[&](FVector P){return Coord==FIntPoint(FMath::FloorToInt(P.X/AETWorld::RegionSize),FMath::FloorToInt(P.Y/AETWorld::RegionSize));};
    FVector Board(ETNav::Refuge(Cell)+FVector2D(640,-150),0); Board.Z=World->SurfaceHeight(Board.X,Board.Y);
    if(Owns(Board)) if(auto* B=World->Item(Prefix(Cell)+TEXT("board"),TEXT("townboard"),TEXT("Read settlement board"),Board)) {
        Residents.Add(B);auto* Text=NewObject<UTextRenderComponent>(B);Text->SetupAttachment(B->Mesh);
        Text->SetText(FText::FromString(TEXT("TOWN WATCH")));Text->SetHorizontalAlignment(EHTA_Center);
        Text->SetWorldSize(29);Text->SetTextRenderColor(FColor(239,223,176));Text->SetRelativeLocation(FVector(0,-19,308));
        Text->SetRelativeRotation(FRotator(0,-90,0));Text->SetCullDistance(9000);Text->RegisterComponent();
    }
    const auto Tower=World->TowerLocation(Cell);
    if(Owns(Tower)) {
        if(auto* Tap=World->Item(Prefix(Cell)+TEXT("tap"),TEXT("tower"),TEXT("Draw water from the hill cistern"),Tower)) {
            Tap->Destination=FVector(Cell.X,Cell.Y,0); Residents.Add(Tap);
        }
    }
}
void AETWorld::WalkerFell(AETAgent* A)
{
    auto& T=State->Towns.FindOrAdd(A->TownCell);
    if(T.Claimed||A->TownIndex<0||A->TownIndex>=TownPopulation(A->TownCell,T.Wave)) return;
    T.Dead|=1<<A->TownIndex; TownUnits.Remove(A->Id);
    if(TownRemaining(A->TownCell)) return;
    T.Claimed=true; T.Upkeep=State->Elapsed+Shift;
    Say(TEXT("Town reclaimed. The refuge keeper has reopened trade. Use the settlement board to secure it."));
    Sound(TEXT("OreChime"),.8f);
}
void AETWorld::TickTowns(AETCharacter* P,float D)
{
    TownTick-=D; if(TownTick>0) return;TownTick=.55f;
    for(auto It=TownUnits.CreateIterator();It;++It) if(!It.Value().IsValid()) It.RemoveCurrent();
    const FVector Here=P->GetActorLocation();const auto Cell=ETNav::District(FVector2D(Here));
    if(Cell==FIntPoint::ZeroValue) return;
    const FVector2D Base=FVector2D(Cell)*ETNav::DistrictSpacing;
    if(FVector2D::Distance(FVector2D(Here),Base)>28800) return;
    auto* Existing=State->Towns.Find(Cell);
    if(!Existing&&!TownPopulation(Cell)) return;
    auto& T=State->Towns.FindOrAdd(Cell);
    if(T.Claimed) {
        if(State->Elapsed<T.Upkeep) return;
        const int Owed=FMath::Clamp(FMath::FloorToInt((State->Elapsed-T.Upkeep)/Shift)+1,1,1000000);
        if(T.Treasury>=Wage*Owed) {T.Treasury-=Wage*Owed; T.Upkeep+=Shift*Owed; if(T.Defenses>0) TownFortifications(Cell,T.Defenses,true); return;}
        T.Treasury=0; T.Upkeep=State->Elapsed+Shift; return;
    }
    const int Quota=TownPopulation(Cell,T.Wave);
    const FVector2D Refuge=ETNav::Refuge(Cell);
    for(int I=0;I<Quota;++I) {
        if(T.Dead&(1<<I)) continue;
        const FString Id=Prefix(Cell)+FString::Printf(TEXT("%d/%d"),T.Wave,I);
        if(TownUnits.FindRef(Id).IsValid()) continue;
        FRandomStream R(SeedFor(Cell,9830+I+T.Wave*17));
        const double X=I%2?3200.:-3200.,Y=I<4?R.FRandRange(3500.f,6600.f):R.FRandRange(-5000.f,3200.f);
        FVector Spawn(Base+FVector2D(X+R.FRandRange(-200.f,200.f),Y),0);
        if(T.Wave) Spawn=FVector(Refuge+FVector2D(X,Y*.45),0);
        const FIntPoint Tile(FMath::FloorToInt(Spawn.X/RegionSize),FMath::FloorToInt(Spawn.Y/RegionSize));
        auto* Region=Regions.FindRef(Tile); if(!Region||!Region->TerrainGrid) continue;
        if(!ETAgentMovement::FindGroundSpawn(*this,Spawn))continue;
        auto* A=GetWorld()->SpawnActor<AETAgent>(Spawn,FRotator::ZeroRotator); if(!A) continue;
        A->TownCell=Cell; A->TownIndex=I; A->SetOwner(Region); A->Configure(this,Id,TEXT("walker"));
        Region->Residents.Add(A);TownUnits.Add(Id,A);
    }
}
void AETWorld::TownFortifications(FIntPoint Cell,int Tier,bool Repair)
{
    const auto Base=ETNav::Refuge(Cell); int Index=0;
    auto Place=[&](int Piece,FVector2D At,int Facing,int Material) {
        const auto Id=WorksId(Cell,Index++);
        if(auto* B=State->Builds.Find(Id)) {if(Repair) B->Integrity=AETConstruction::Durability(Piece,B->MaterialTier);return;}
        FETBuild Build; Build.Piece=Piece;Build.MaterialTier=Material;Build.FromKit=false;Build.Facing=Facing;
        Build.Anchor=FVector(Base+At,0); Build.Anchor.Z=SurfaceHeight(Build.Anchor.X,Build.Anchor.Y)+4;
        Build.Integrity=AETConstruction::Durability(Piece,Material); State->Builds.Add(Id,Build); SpawnBuild(Id,Build);
    };
    if(Tier>=1) for(int Side:{-1,1}) for(int I=-3;I<=3;++I) if(I!=0) {
        Place(0,FVector2D(Side*1400,I*400),Side>0?0:2,1);
        Place(0,FVector2D(I*400,Side*1400),Side>0?1:3,1);
    }
    if(Tier>=2) {
        Place(AETConstruction::Core,FVector2D(-450,600),0,2);
        for(int Side:{-1,1}) Place(AETConstruction::Turret,FVector2D(Side*1220,Side*1150),0,2);
    }
}
void AETCharacter::TownPanel(FIntPoint Cell)
{
    BoardCell=Cell; const auto* T=World->State->Towns.Find(Cell);
    FString Info=World->TownStatus(Cell)+TEXT("\n\n");
    if(Cell==FIntPoint::ZeroValue) Info+=TEXT("The shelter watch is on duty. Mark the hill cistern or track an infested town on your field atlas.");
    else if(T&&T->Claimed) Info+=FString::Printf(TEXT("Watch fund  %d wrappers     /     Next shift in %.0f min\nThe watch costs 8 wrappers every 12 minutes. A paid watch repairs fortifications. Reclaimed streets stay settled.\nDefenses  %s"),T->Treasury,FMath::CeilToFloat(FMath::Max(0.f,T->Upkeep-World->State->Elapsed)/60.f),T->Defenses==2?TEXT("walls, relay and two sentries"):T->Defenses==1?TEXT("gated stone walls"):TEXT("open quarter"));
    else if(World->TownRemaining(Cell)) Info+=TEXT("Clear the zombies on the avenues around the refuge. The last kill reopens trade. Return to this board to build walls and sentries.");
    else Info+=TEXT("This town has its own watch. The refuge keeper trades supplies here.");
    OpenPanel(TEXT("town"),TEXT("SETTLEMENT BOARD"),Info);
}
