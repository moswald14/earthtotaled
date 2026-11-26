#include "ETGame.h"
#include "ETGeology.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "EngineUtils.h"
#include "Engine/World.h"

namespace
{
    using Site=AETConstruction;
    using Corners=TStaticArray<FVector,4>;
    Corners FrameCorners(const FETBuild& B)
    {
        Corners C;
        C[0]=FVector(-200,-200,0); C[1]=FVector(200,-200,0);
        C[2]=FVector(200,200,0); C[3]=FVector(-200,200,0);
        if(B.Piece==0) {C[0].X=C[3].X=200; C[0].Z=C[3].Z=280;}
        if(B.Piece==2) C[1].Z=C[2].Z=280;
        const FRotator R(0,B.Facing*90,0);
        for(FVector& V:C) V=B.Anchor+R.RotateVector(V);
        return C;
    }
    bool OnGround(AETWorld& W,const FETBuild& B)
    {
        // A floor can span a hole. Test the bearing edges as well as the centre.
        TArray<FVector,TInlineAllocator<5>> Feet;
        if(B.Piece>=Site::Core) {
            const auto Extent=Site::Extents(B.Piece)*.70;
            for(int Y:{-1,1}) for(int X:{-1,1}) Feet.Add(B.Anchor+FRotator(0,B.Facing*90,0).RotateVector(FVector(X*Extent.X,Y*Extent.Y,0)));
        } else {
            for(FVector Corner:FrameCorners(B)) if(Corner.Z<B.Anchor.Z+1) Feet.Add(Corner);
        }
        FVector Mid=FVector::ZeroVector;
        for(FVector Foot:Feet) Mid+=Foot;
        Mid/=Feet.Num();
        for(FVector& Foot:Feet) Foot=FMath::Lerp(Mid,Foot,.91);
        Feet.Add(Mid);
        for(FVector Foot:Feet) {
            if(!W.Geology->IsOpen(Foot-FVector(0,0,27))) return true;
            // Foundations can also sit on a fixed building, pavement or cave floor.
            TArray<FHitResult> Hits;
            W.GetWorld()->LineTraceMultiByObjectType(Hits,Foot+FVector(0,0,9),Foot-FVector(0,0,34),FCollisionObjectQueryParams::AllObjects);
            for(const auto& H:Hits) {
                const auto* Static=Cast<AStaticMeshActor>(H.GetActor());
                const UStaticMesh* Mesh=Static?Static->GetStaticMeshComponent()->GetStaticMesh():nullptr;
                if((!Cast<AETRegion>(H.GetActor())&&!Static)||(Mesh&&AETWorld::IsTree(Mesh->GetFName()))) continue;
                if(Cast<UInstancedStaticMeshComponent>(H.GetComponent())) continue;
                if(H.ImpactNormal.Z>.55) return true;
            }
        }
        return false;
    }
    FIntVector Bucket(FVector P)
    {
        return FIntVector(FMath::FloorToInt32(P.X/500.),FMath::FloorToInt32(P.Y/500.),FMath::FloorToInt32(P.Z/500.));
    }
}
bool AETConstruction::Joined(const FETBuild& A,const FETBuild& B)
{
    if(A.Piece>=Core||B.Piece>=Core) {
        const FETBuild& Machine=A.Piece>=Core?A:B;
        const FETBuild& Floor=A.Piece>=Core?B:A;
        return Floor.Piece==1&&FVector::DistSquared2D(Machine.Anchor,Floor.Anchor)<FMath::Square(130.)&&
            FMath::IsNearlyEqual(Machine.Anchor.Z,Floor.Anchor.Z+17.,25.);
    }
    const auto AC=FrameCorners(A),BC=FrameCorners(B);
    // One matching corner is just a diagonal touch. Require an entire shared edge.
    for(int I=0;I<4;++I) for(int J=0;J<4;++J) {
        const auto& A2=AC[(I+1)%4]; const auto& B2=BC[(J+1)%4];
        if((AC[I].Equals(BC[J],24)&&A2.Equals(B2,24))||(AC[I].Equals(B2,24)&&A2.Equals(BC[J],24))) return true;
    }
    return false;
}
bool AETConstruction::HasFooting(AETWorld& W,const FETBuild& B) {return OnGround(W,B);}
int32 AETWorld::BreakBuild(const FGuid& Id)
{
    if(!State->Builds.Remove(Id)) return 0;
    if(auto* A=LiveBuilds.FindRef(Id).Get()) {
        if(auto* Region=Cast<AETRegion>(A->GetOwner())) Region->Residents.Remove(A);
        A->Destroy();
    }
    LiveBuilds.Remove(Id); return 1;
}
void AETWorld::RestoreFoundations(AETRegion* R)
{
    const FVector Center=R->GetActorLocation()+FVector(RegionSize*.5,RegionSize*.5,0);
    const FVector Reach(RegionSize*.5,RegionSize*.5,1.e8);
    if(State->Builds.Num()) CheckFoundations(Center,Reach,false);
    if(!R->RockMeshes.IsEmpty()&&(!State->SquareCuts.IsEmpty()||!State->BoreCuts.IsEmpty())) SettleLooseItems(Center,Reach,true);
}
void AETWorld::CheckFoundations(FVector Center,FVector Reach,bool InspectLoose)
{
    TMap<FIntVector,TArray<FGuid>> Bins;
    TArray<FGuid> Nearby;
    TSet<FGuid> Machines;
    const FBox Area(Center-Reach-FVector(230),Center+Reach+FVector(230,230,350));
    for(const auto& It:State->Builds) {
        Bins.FindOrAdd(Bucket(It.Value.Anchor)).Add(It.Key);
        if(Area.IsInsideOrOn(It.Value.Anchor)) {
            if(It.Value.Piece>=Site::Core) Machines.Add(It.Key); else Nearby.Add(It.Key);
        }
    }
    TSet<FGuid> Seen;
    for(FGuid Start:Nearby) {
        if(Seen.Contains(Start)) continue;
        TArray<FGuid> Group={Start}; Seen.Add(Start);
        bool Grounded=false;
        for(int I=0;I<Group.Num();++I) {
            const auto& Current=State->Builds.FindChecked(Group[I]);
            // Outside loaded terrain, preserve support on authored floors until that area loads.
            const FIntPoint Tile(FMath::FloorToInt(Current.Anchor.X/RegionSize),FMath::FloorToInt(Current.Anchor.Y/RegionSize));
            Grounded|=!Regions.Contains(Tile)||OnGround(*this,Current);
            const FIntVector At=Bucket(Current.Anchor);
            for(int X=-1;X<=1;++X) for(int Y=-1;Y<=1;++Y) for(int Z=-1;Z<=1;++Z) {
                auto* Adjacent=Bins.Find(At+FIntVector(X,Y,Z)); if(!Adjacent) continue;
                for(FGuid Next:*Adjacent) if(!Seen.Contains(Next)&&State->Builds.Contains(Next)&&Site::Joined(Current,State->Builds.FindChecked(Next))) {
                    if(State->Builds.FindChecked(Next).Piece>=Site::Core) Machines.Add(Next);
                    else {Seen.Add(Next); Group.Add(Next);}
                }
            }
        }
        if(!Grounded) {
            FBox Lost(ForceInit);
            for(FGuid Id:Group) {Lost+=State->Builds.FindChecked(Id).Anchor; BreakBuild(Id);}
            SettleLooseItems(Lost.GetCenter(),Lost.GetExtent()+FVector(240,240,320));
        }
    }
    for(FGuid Id:Machines) {
        const auto& B=State->Builds.FindChecked(Id);
        if(OnGround(*this,B)) continue;
        const FIntVector At=Bucket(B.Anchor); bool Supported=false;
        for(int Z=-1;Z<=1&&!Supported;++Z) if(const auto* Bay=Bins.Find(At+FIntVector(0,0,Z))) {
            for(auto Other:*Bay) if(const auto* Floor=State->Builds.Find(Other);Floor&&Site::Joined(B,*Floor)) {Supported=true; break;}
        }
        if(!Supported) BreakBuild(Id);
    }
    if(State->RaidStage&&!State->Builds.Contains(State->RaidCore)) EndRaid(false);
    if(InspectLoose) SettleLooseItems(Center,Reach);
}
void AETWorld::SettleLooseItems(FVector Center,FVector Reach,bool OpeningsOnly)
{
    for(TActorIterator<AETItem> It(GetWorld());It;++It) {
        auto* Item=*It; const FVector At=Item->GetActorLocation(),D=At-Center;
        if(FMath::Abs(D.X)>Reach.X||FMath::Abs(D.Y)>Reach.Y||D.Z < -Reach.Z-110||D.Z>Reach.Z+140) continue;
        if(OpeningsOnly&&!Geology->IsOpen(FVector(At.X,At.Y,FMath::Min(At.Z,SurfaceHeight(At.X,At.Y))-25))) continue;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ETLooseFooting),false,Item);
        TArray<FHitResult> Hits;
        GetWorld()->LineTraceMultiByObjectType(Hits,At+FVector(0,0,10),At-FVector(0,0,10000),FCollisionObjectQueryParams::AllObjects,Q);
        const FHitResult* Floor=Hits.FindByPredicate([](const FHitResult& H){
            return H.ImpactNormal.Z>.4&&(Cast<AETRegion>(H.GetActor())||Cast<AStaticMeshActor>(H.GetActor())||Cast<Site>(H.GetActor()));
        });
        if(Floor&&At.Z-Floor->ImpactPoint.Z<=110) continue;
        const bool Essential=Item->Kind==TEXT("human")||Item->Kind==TEXT("friend")||Item->Kind==TEXT("quest")||
            Item->Kind==TEXT("core")||Item->Kind==TEXT("uplink")||Item->Kind==TEXT("survey");
        if(Essential) {
            FVector Landing=Floor?Floor->ImpactPoint+FVector(0,0,8):At;
            if(!Floor) {
                // A very deep shaft may have no floor in reach. Find its first solid bank.
                bool Found=false;
                for(int Ring=1;Ring<=32&&!Found;++Ring) for(int Side=0;Side<12&&!Found;++Side) {
                    const FVector Spot=At+FRotator(0,Side*30,0).Vector()*Ring*300.;
                    const double Z=SurfaceHeight(Spot.X,Spot.Y);
                    if(!Geology->IsOpen(FVector(Spot.X,Spot.Y,Z-12))) {Landing=FVector(Spot.X,Spot.Y,Z+8); Found=true;}
                }
                if(!Found) continue;
            }
            Item->SetActorLocation(Landing); State->Settled.Add(Item->Id,Landing);
        } else {
            State->Consumed.Add(Item->Id); State->DropPositions.Remove(Item->Id);
            State->DropAmounts.Remove(Item->Id); State->DropKinds.Remove(Item->Id); Item->Destroy();
        }
    }
}
