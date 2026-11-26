#include "ETGeology.h"
#include "ETGame.h"
#include "ETTerrainMath.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMeshActor.h"
#include "ProceduralMeshComponent.h"

using namespace ETRock;

namespace
{
    constexpr double GalleryWidth=4800, GalleryDepth=2400, VeinWidth=1150, VeinDepth=800;
    const FVector BoxHalf(250,250,210);
    double BoxDistance(FVector Point,FVector Center)
    {
        const FVector Delta=(Point-Center).GetAbs()-BoxHalf;
        return Delta.GetMax();
    }
    FIntVector CellFor(FVector P,double Width,double Depth)
    {
        return FIntVector(FMath::FloorToInt(P.X/Width),FMath::FloorToInt(P.Y/Width),FMath::FloorToInt(P.Z/Depth));
    }
    template<typename Func> void ForCells(FIntVector First,FIntVector Last,Func Visit)
    {
        for(int32 Z=First.Z;Z<=Last.Z;++Z)
            for(int32 Y=First.Y;Y<=Last.Y;++Y)
                for(int32 X=First.X;X<=Last.X;++X) Visit(FIntVector(X,Y,Z));
    }
    uint32 RockSeed(int32 Seed,FIntVector C,uint32 Salt)
    {
        return ETMath::Mix(uint32(Seed)^Salt^ETMath::Mix(uint32(C.X))^ETMath::Mix(uint32(C.Y)*19349663u)^ETMath::Mix(uint32(C.Z)*83492791u));
    }
}

FIntVector ETRock::Chunk(FVector P) {return CellFor(P-FVector(0,0,ZOffset),Span,Span);}
FIntPoint ETRock::Region(FIntVector Key)
{
    constexpr int32 PerRegion=int32(AETWorld::RegionSize)/Span;
    static_assert(PerRegion*Span==int32(AETWorld::RegionSize));
    return FIntPoint(FMath::FloorToInt(double(Key.X)/PerRegion),FMath::FloorToInt(double(Key.Y)/PerRegion));
}
double FCavity::Distance(FVector P) const
{
    return FVector::Dist(P,FMath::ClosestPointOnSegment(P,A,B))-Radius;
}
FBox FCavity::Bounds() const {return FBox(A.ComponentMin(B),A.ComponentMax(B)).ExpandBy(Radius);}
double FChunkField::AirDistance(FVector P,double SurfaceZ) const
{
    double D=1.e10;
    for(const FCavity& Shape:Voids) D=FMath::Min(D,Shape.Distance(P));
    // The same small displacement is sampled on both sides of every chunk seam.
    if(D<250) D+=12*FMath::Sin(.008*P.X+.011*P.Z)+9*FMath::Sin(.014*P.Y-.006*P.Z);
    const FVector Absolute(P.X,P.Y,P.Z+SurfaceZ);
    for(const FVector& Center:BoxCenters) D=FMath::Min(D,BoxDistance(Absolute,Center));
    if(!Fallen.IsEmpty()) {
        const FVector Grid=(P-FVector(0,0,ZOffset))/Step;
        const FIntVector Base(FMath::FloorToInt32(Grid.X),FMath::FloorToInt32(Grid.Y),FMath::FloorToInt32(Grid.Z));
        for(int Z=0;Z<=1;++Z) for(int Y=0;Y<=1;++Y) for(int X=0;X<=1;++X) {
            const FIntVector Node=Base+FIntVector(X,Y,Z);
            if(Fallen.Contains(Node)) D=FMath::Min(D,(Grid-FVector(Node)).GetAbsMax()*Step-76.);
        }
    }
    return D;
}
FChunkField& UETGeology::TouchChunk(FIntVector Key,TSet<FIntVector>& Dirty)
{
    RegionChunks.FindOrAdd(Region(Key)).Add(Key);
    int32& Low=LowestCut.FindOrAdd(FIntPoint(Key.X,Key.Y),Key.Z); Low=FMath::Min(Low,Key.Z);
    Dirty.Add(Key); return Fields.FindOrAdd(Key);
}
void UETGeology::AddCavity(const FCavity& Shape,TSet<FIntVector>& Dirty)
{
    const FBox Box=Shape.Bounds().ExpandBy(Step+25);
    ForCells(Chunk(Box.Min),Chunk(Box.Max),[&](FIntVector Key) {
        if(Key.Z>=0) return;
        TouchChunk(Key,Dirty).Voids.Add(Shape);
    });
}
TArray<FCavity> UETGeology::Gallery(FIntVector Cell) const
{
    // Each chamber and both branches fit inside their geology cell. A pulse
    // discovering any branch materializes the entire gallery, including its far wall.
    FRandomStream R(RockSeed(World->State->Seed,Cell,0x15a797u));
    const double Angle=R.FRandRange(0,2*PI);
    const FVector Along(FMath::Cos(Angle),FMath::Sin(Angle),0), Side(-Along.Y,Along.X,0);
    const double Depth=R.FRandRange(640,980);
    const FVector Center((Cell.X+.5)*GalleryWidth,(Cell.Y+.5)*GalleryWidth,(Cell.Z+1)*GalleryDepth-Depth);
    const FVector Far=Center+Along*1050+Side*260-FVector(0,0,170);
    const FVector Branch=Center-Along*570+Side*730-FVector(0,0,110);
    return {{Center-Along*1120,Center,255},{Center,Far,305},{Far,Far,430},{Center-Along*350,Branch,225}};
}
void UETGeology::Initialize(AETWorld* InWorld)
{
    World=InWorld;
    Fields.Reset(); RegionChunks.Reset(); LowestCut.Reset(); IndexedCuts.Reset(); IndexedBoxes.Reset(); ActiveGalleries.Reset();
    TSet<FIntVector> Dirty;
    // The original shelter adit remains walkable in every save.
    AddCavity({FVector(770,-2750,300),FVector(3120,-2750,-760),270},Dirty);
    // Raise the wider chamber's centreline so its floor meets the ramp tangentially.
    AddCavity({FVector(3040,-2750,-565),FVector(3570,-2750,-805),410},Dirty);
    AddCavity({FVector(3250,-2780,-810),FVector(3490,-3500,-930),220},Dirty);
    for(FIntVector Cut:World->State->BoreCuts) IndexCut(Cut,Dirty,false,true);
    for(FIntVector Cut:World->State->SquareCuts) IndexCut(Cut,Dirty,true,true);
    for(FIntVector Node:World->State->FallenRock) IndexFallen(Node,Dirty);
}
void UETGeology::IndexCut(FIntVector Cut,TSet<FIntVector>& Dirty,bool Square,bool Restoring)
{
    auto& Seen=Square?IndexedBoxes:IndexedCuts;
    if(Seen.Contains(Cut)) return;
    Seen.Add(Cut);
    FVector P=FVector(Cut)*(Square?double(Step):CutQuantum);
    if(Square) {
        // Store cutter planes in world space. A tunnel stays level beneath sloping ground.
        const FVector Margin=BoxHalf+FVector(Step+25);
        FVector Low=P-Margin, High=P+Margin;
        double MinGround=DBL_MAX,MaxGround=-DBL_MAX;
        for(int32 Y=-4;Y<=4;++Y) for(int32 X=-4;X<=4;++X) {
            const double Ground=World->SurfaceHeight(P.X+X*Step,P.Y+Y*Step);
            MinGround=FMath::Min(MinGround,Ground); MaxGround=FMath::Max(MaxGround,Ground);
        }
        Low.Z-=MaxGround; High.Z-=MinGround;
        ForCells(Chunk(Low),Chunk(High),[&](FIntVector Key) {
            if(Key.Z>=0) return;
            TouchChunk(Key,Dirty).BoxCenters.Add(P);
        });
        P.Z-=World->SurfaceHeight(P.X,P.Y);
    } else AddCavity({P,P,PulseRadius},Dirty);
    const FVector Margin(PulseRadius+Step+45);
    ForCells(CellFor(P-Margin,GalleryWidth,GalleryDepth),CellFor(P+Margin,GalleryWidth,GalleryDepth),[&](FIntVector Cell) {
        if(Cell.Z>=0||ActiveGalleries.Contains(Cell)||(Restoring&&World->State->SealedGalleries.Contains(Cell))) return;
        const auto Shapes=Gallery(Cell);
        for(const FCavity& Shape:Shapes) if(Shape.Distance(P)<PulseRadius+Step+45) {
            if(!Restoring) {
                for(const FCavity& Part:Shapes) {
                    const FBox Bounds=Part.Bounds().ExpandBy(Step*.5+21);
                    if(DigRestriction(FBox2D(FVector2D(Bounds.Min),FVector2D(Bounds.Max)))) {
                        World->State->SealedGalleries.Add(Cell); return;
                    }
                }
                World->State->SealedGalleries.Remove(Cell);
            }
            ActiveGalleries.Add(Cell);
            for(const FCavity& Part:Shapes) AddCavity(Part,Dirty);
            break;
        }
    });
}
TArray<FVein> UETGeology::Veins(const FBox& Bounds) const
{
    TArray<FVein> Result;
    auto Add=[&](const FVein& Vein) {
        if(!World->State->MinedVeins.Contains(Vein.Id)&&Vein.Shape.Bounds().Intersect(Bounds)) Result.Add(Vein);
    };
    // Positive Z IDs identify these two authored exposures. All generated IDs have negative Z.
    Add({{0,0,1},{{3980,-2890,-838},{3995,-2630,-886},138},true,3});
    Add({{0,0,2},{{2630,-2482,-590},{3060,-2440,-678},150},false,5});
    // North-east of Shelter 09, shallow outcrops give new survivors a legal first claim.
    Add({{0,0,3},{{31900,-15300,-135},{32100,-15300,-155},104},true,3});
    Add({{0,0,4},{{32900,-15300,-145},{33100,-15300,-172},130},false,5});
    ForCells(CellFor(Bounds.Min,VeinWidth,VeinDepth),CellFor(Bounds.Max,VeinWidth,VeinDepth),[&](FIntVector Cell) {
        if(Cell.Z>=0) return;
        FRandomStream R(RockSeed(World->State->Seed,Cell,0x6e4b21u));
        // Keep draw order explicit so seed generation is portable between compilers.
        const double X=R.FRandRange(.36f,.64f), Y=R.FRandRange(.36f,.64f), Z=R.FRandRange(.38f,.60f);
        FVector Center((Cell.X+X)*VeinWidth,(Cell.Y+Y)*VeinWidth,(Cell.Z+Z)*VeinDepth);
        const double Angle=R.FRandRange(0,2*PI), Length=R.FRandRange(125,200);
        const FVector Axis(FMath::Cos(Angle)*Length,FMath::Sin(Angle)*Length,R.FRandRange(-65,65));
        const bool Gold=R.RandRange(0,5)==0;
        Add({Cell,{Center-Axis,Center+Axis,Gold?105.:150.},Gold,Gold?R.RandRange(1,3):R.RandRange(3,6)});
    });
    return Result;
}
bool UETGeology::RemainingOre(const FVein& Vein,FVector From,FVector& Target) const
{
    const FVector Nearest=FMath::ClosestPointOnSegment(From,Vein.Shape.A,Vein.Shape.B);
    const double R=Vein.Shape.Radius*.72;
    const FVector Offsets[]={FVector::ZeroVector,{R,0,0},{-R,0,0},{0,R,0},{0,-R,0},{0,0,R},{0,0,-R}};
    double Best=DBL_MAX;
    for(const FVector& Axis:{Nearest,Vein.Shape.A,Vein.Shape.B}) for(const FVector& Off:Offsets) {
        const FVector Sample=Axis+Off;
        const double Distance=FVector::DistSquared(Sample,From);
        if(Distance>=Best) continue;
        const auto* Field=Fields.Find(Chunk(Sample));
        if(Field&&Field->AirDistance(Sample,World->SurfaceHeight(Sample.X,Sample.Y))<0) continue;
        Best=Distance; Target=Sample;
    }
    return Best<DBL_MAX;
}
bool UETGeology::ExtractNear(FVector Anchor,int32& Iron,int32& Gold)
{
    FVector Local=Anchor; Local.Z-=World->SurfaceHeight(Anchor.X,Anchor.Y);
    const FBox Area(Local-FVector(1700,1700,3400),Local+FVector(1700,1700,-90));
    double Best=DBL_MAX;
    TOptional<FVein> Choice;
    for(const auto& V:Veins(Area)) {
        const FVector Center=(V.Shape.A+V.Shape.B)*.5;
        const double D=FVector::DistSquared(Local,Center);
        FVector Solid;
        if(D>=Best||!Area.IsInsideOrOn(Center)||!RemainingOre(V,Local,Solid)) continue;
        const auto B=V.Shape.Bounds();
        if(!DigRestriction(FBox2D(FVector2D(B.Min),FVector2D(B.Max)))) {Best=D; Choice=V;}
    }
    if(!Choice) return false;
    const auto& V=Choice.GetValue();
    World->State->MinedVeins.Add(V.Id); (V.Gold?Gold:Iron)+=V.Yield*Grade(-V.Shape.A.Z);
    const FBox B=V.Shape.Bounds().ExpandBy(Step);
    ForCells(Chunk(B.Min),Chunk(B.Max),[&](FIntVector K){
        if(auto* R=World->Regions.FindRef(Region(K));R&&R->RockMeshes.Contains(K)) Rebuild(*R,K);
    });
    return true;
}
bool UETGeology::ReplacesSurface(FIntPoint Patch) const {return Fields.Contains(FIntVector(Patch.X,Patch.Y,-1));}
bool UETGeology::IsOpen(FVector Position) const
{
    const double Ground=World->SurfaceHeight(Position.X,Position.Y);
    Position.Z-=Ground;
    if(Position.Z>=0) return true;
    const FChunkField* Field=Fields.Find(Chunk(Position));
    return Field&&Field->AirDistance(Position,Ground)<0;
}
double UETGeology::SurfaceOpening(double X,double Y) const
{
    const FVector P(X,Y,0);
    const FChunkField* Field=Fields.Find(Chunk(P));
    return Field?Field->AirDistance(P,World->SurfaceHeight(X,Y)):1.e10;
}
bool UETGeology::PrepareRegionSlice(AETRegion& R)
{
    if(const auto* Keys=RegionChunks.Find(R.Coord)) for(FIntVector Key:*Keys) if(!R.RockMeshes.Contains(Key)) {
        Rebuild(R,Key); return false;
    }
    return true;
}
void UETGeology::FinishRegion(AETRegion& R)
{
    if(const auto* Keys=RegionChunks.Find(R.Coord))
        for(FIntVector Key:*Keys) if(Key.Z==-1) ClearScatter(R,{Key.X,Key.Y});
}
bool UETGeology::CanBore(const FHitResult& Hit) const
{
    const auto* Component=Hit.GetComponent();
    if(!Component) return false;
    if(const auto* R=Cast<AETRegion>(Hit.GetActor())) return Component==R->Terrain||Component->ComponentHasTag(TEXT("ET_Rock"));
    return Component->ComponentHasTag(TEXT("ET_Paving"))||Component->ComponentHasTag(TEXT("ET_LooseRock"));
}
bool UETGeology::Blast(const FHitResult& Hit,FString& Reward,bool Square,bool Recover)
{
    Reward.Reset();
    if(!CanBore(Hit)) {
        if(const auto* Why=DigRestriction(FBox2D(FVector2D(Hit.ImpactPoint),FVector2D(Hit.ImpactPoint)))) Reward=Why;
        return false;
    }
    FVector Center=Hit.ImpactPoint-Hit.ImpactNormal*90;
    if(Hit.GetComponent()->ComponentHasTag(TEXT("ET_LooseRock"))) {
        Center=Hit.GetActor()->GetActorLocation();
        Center.Z=World->SurfaceHeight(Center.X,Center.Y)-90;
    }
    if(!Square) Center.Z-=World->SurfaceHeight(Center.X,Center.Y);
    const double Quantum=Square?double(Step):CutQuantum;
    const FIntVector Cut(FMath::RoundToInt(Center.X/Quantum),FMath::RoundToInt(Center.Y/Quantum),FMath::RoundToInt(Center.Z/Quantum));
    const FVector2D XY(double(Cut.X)*Quantum,double(Cut.Y)*Quantum);
    // Include displacement of the rock face and the interpolated collision edge.
    const FVector2D Radius((Square?BoxHalf.X:PulseRadius+21.)+Step*.5);
    if(const auto* Why=DigRestriction(FBox2D(XY-Radius,XY+Radius))) {Reward=Why;return false;}
    if((Square?IndexedBoxes:IndexedCuts).Contains(Cut)) return false;
    Center=FVector(Cut)*Quantum;
    const FVector Absolute=Center;
    if(Square) Center.Z-=World->SurfaceHeight(Center.X,Center.Y);
    TSet<FIntVector> Dirty;
    int32 Gold=0,Iron=0;
    const FVector Reach=Square?BoxHalf+FVector(Step):FVector(PulseRadius);
    for(const FVein& V:Veins(FBox(Center-Reach,Center+Reach))) {
        // Breaking the core yields once. Grazing a deposit first exposes its coloured face.
        FVector Ore;
        if(!RemainingOre(V,Center,Ore)) continue;
        if(Square) {
            if(BoxDistance(Ore+FVector(0,0,World->SurfaceHeight(Ore.X,Ore.Y)),Absolute)>-18) continue;
        } else if(FVector::Dist(Center,FMath::ClosestPointOnSegment(Center,V.Shape.A,V.Shape.B))>PulseRadius-28||
                  FVector::DistSquared(Center,Ore)>FMath::Square(PulseRadius-28)) continue;
        World->State->MinedVeins.Add(V.Id);
        if(Recover) (V.Gold?Gold:Iron)+=V.Yield*Grade(-V.Shape.A.Z);
        const FBox B=V.Shape.Bounds().ExpandBy(Step);
        ForCells(Chunk(B.Min),Chunk(B.Max),[&](FIntVector K){if(Fields.Contains(K)) Dirty.Add(K);});
    }
    (Square?World->State->SquareCuts:World->State->BoreCuts).Add(Cut);
    IndexCut(Cut,Dirty,Square);
    const FBox Detached=BreakDetachedRock(Center,Reach,Dirty);
    if(Gold) {World->State->Inventory.FindOrAdd(TEXT("goldore"))+=Gold; Reward=FString::Printf(TEXT("+%d gold ore"),Gold);}
    if(Iron) {
        World->State->Inventory.FindOrAdd(TEXT("ironore"))+=Iron;
        if(!Reward.IsEmpty()) Reward+=TEXT("  /  ");
        Reward+=FString::Printf(TEXT("+%d iron ore"),Iron);
    }
    TSet<AETRegion*> Surfaces,RoadSurfaces;
    TSet<AStaticMeshActor*> Props;
    for(FIntVector Key:Dirty) {
        AETRegion* R=World->Regions.FindRef(Region(Key));
        if(!IsValid(R)||(!R->TerrainGrid&&!R->RockMeshes.Contains(Key))) continue;
        if(Key.Z==-1&&!R->RockMeshes.Contains(Key)) Surfaces.Add(R);
        Rebuild(*R,Key);
        if(Key.Z!=-1) continue;
        if(R->RoadMesh) RoadSurfaces.Add(R);
        ClearScatter(*R,{Key.X,Key.Y});
        if(auto* List=GroundProps.Find({Key.X,Key.Y})) {
            List->RemoveAllSwap([](const auto& A){return !A.IsValid();});
            for(auto A:*List) Props.Add(A.Get());
        }
    }
    for(AETRegion* R:Surfaces) R->RefreshSurfaceCollision();
    for(AETRegion* R:RoadSurfaces) R->BuildRoadSurface();
    const FVector Origin(Center.X,Center.Y,Center.Z+World->SurfaceHeight(Center.X,Center.Y));
    FBox Changed(Origin-Reach,Origin+Reach);
    if(Detached.IsValid) Changed+=Detached;
    for(AStaticMeshActor* A:Props) {
        auto* Mesh=A->GetStaticMeshComponent(); const bool Visible=Mesh->IsVisible();
        UpdateProp(*A);
        if(Visible&&!Mesh->IsVisible()&&Mesh->ComponentHasTag(TEXT("ET_Foundation"))) Changed+=Mesh->Bounds.GetBox();
    }
    World->CheckFoundations(Changed.GetCenter(),Changed.GetExtent());
    return true;
}
FString UETGeology::Survey(FVector Eye,FVector Forward) const
{
    FVector Local=Eye;
    Local.Z-=World->SurfaceHeight(Eye.X,Eye.Y);
    const FVector Extent(2300);
    double Best=1.e10; FVein Target{}; FVector Offset{};
    for(const FVein& V:Veins(FBox(Local-Extent,Local+Extent))) {
        FVector Ore;
        if(!RemainingOre(V,Local,Ore)) continue;
        const FVector Delta=Ore-Local;
        const double Metres=Delta.Size()*.01, Dot=FVector::DotProduct(Delta.GetSafeNormal(),Forward);
        if(Dot<.35||Metres>23) continue;
        const double Score=Metres*(2.3-1.3*Dot);
        if(Score<Best&&!DigRestriction(FBox2D(FVector2D(Ore),FVector2D(Ore)))) {Best=Score; Target=V; Offset=Delta;}
    }
    if(Best==1.e10) return TEXT("NO RETURN  /  sweep the ground");
    return FString::Printf(TEXT("%s  /  %.1f m  /  %s"),Target.Gold?TEXT("GOLD"):TEXT("IRON"),Offset.Size()*.01,
        Offset.Z<-95?TEXT("BELOW"):Offset.Z>95?TEXT("ABOVE"):TEXT("LEVEL"));
}
