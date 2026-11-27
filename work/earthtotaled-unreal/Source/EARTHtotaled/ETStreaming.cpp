#include "ETGame.h"
#include "ETRingLine.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Components/PrimitiveComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "HAL/PlatformTime.h"

double AETWorld::RegionDistance(FIntPoint C,FVector P) const
{
    // Distance to the tile bounds, not its center: no empty slivers at corners.
    double X=FMath::Max(0.0,FMath::Max(C.X*RegionSize-P.X,P.X-(C.X+1)*RegionSize));
    double Y=FMath::Max(0.0,FMath::Max(C.Y*RegionSize-P.Y,P.Y-(C.Y+1)*RegionSize));
    return FMath::Sqrt(X*X+Y*Y);
}
void AETWorld::StreamAround(FVector P,bool Preload)
{
    const double Visible=State->ViewDistance*100.0, Load=Visible+8000, Unload=Load+RegionSize;
    const FIntPoint Center(FMath::FloorToInt(P.X/RegionSize),FMath::FloorToInt(P.Y/RegionSize));
    const int32 Radius=FMath::CeilToInt(Load/RegionSize)+1;
    TArray<FIntPoint> Wanted;
    const auto* Player=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    const bool Aboard=RingTrain&&Player&&RingTrain->Riding(*Player);
    FVector Lead=P;
    if(Aboard) {
        const auto D=ETRing::Radial(RingTrain->Phase.Angle);
        Lead+=FVector(-D.Y,D.X,0)*20000.;
    }
    for(int32 Y=-Radius;Y<=Radius;++Y) for(int32 X=-Radius;X<=Radius;++X) {
        const FIntPoint C=Center+FIntPoint(X,Y);
        if(RegionDistance(C,P)<=Load||(Aboard&&RegionDistance(C,Lead)<Load-4000)) Wanted.Add(C);
    }
    Wanted.Sort([&](const FIntPoint& A,const FIntPoint& B) {
        const auto Priority=[&](FIntPoint C){return FMath::Min(RegionDistance(C,P),RegionDistance(C,Lead)+6000.);};
        const double DA=Priority(A),DB=Priority(B);
        if(!FMath::IsNearlyEqual(DA,DB)) return DA<DB;
        return FVector::DistSquared2D(FVector((A.X+.5)*RegionSize,(A.Y+.5)*RegionSize,0),P)<FVector::DistSquared2D(FVector((B.X+.5)*RegionSize,(B.Y+.5)*RegionSize,0),P);
    });
    if(Preload) {
        for(auto C:Wanted) EnsureRegion(C);
        return;
    }
    // Complete nearby collision first. Buildings/foliage are generated in separate slices.
    const double Deadline=FPlatformTime::Seconds()+(Aboard?.009:.006);
    for(auto C:Wanted) {
        AETRegion* Region=Regions.FindRef(C);
        if(!Region) {Region=GetWorld()->SpawnActor<AETRegion>(); Region->BeginGenerate(this,C); Regions.Add(C,Region);}
        const double D=RegionDistance(C,P);
        if(D<16000&&!Aboard) Region->WantedTerrainGrid=160;
        else if(D>24000) Region->WantedTerrainGrid=80;
        if(Region->Stage<6) {
            bool Done=Region->AdvanceGenerate(); if(Done) RestoreDrops(Region);
            if(FPlatformTime::Seconds()>=Deadline) break;
        } else if(Region->TerrainGrid!=Region->WantedTerrainGrid||Region->TerrainBuildGrid!=0) {
            Region->BuildTerrainSlice(); if(FPlatformTime::Seconds()>=Deadline) break;
        }
    }
    TArray<FIntPoint> Remove;
    for(const auto& It:Regions) if(RegionDistance(It.Key,P)>Unload) Remove.Add(It.Key);
    for(auto C:Remove) {Regions[C]->Destroy(); Regions.Remove(C);}
}
void AETWorld::UpdateVisibility(FVector P)
{
    if(RingTrain) RingTrain->MatchViewDistance(State->ViewDistance);
    const float Distance=State->ViewDistance*100.f;
    auto Apply=[&](AActor* A) {
        if(!IsValid(A)) return;
        const bool Near=FVector::DistSquared2D(A->GetActorLocation(),P)<FMath::Square(Distance+5000);
        if(auto* Agent=Cast<AETAgent>(A)) Agent->UpdatePresence(P);
        else A->SetActorHiddenInGame(!Near);
        TInlineComponentArray<UPrimitiveComponent*> Components(A);
        for(auto Component:Components) {
            float Cap=Component->ComponentHasTag(TEXT("ET_DetailNear"))?14000.f:Component->ComponentHasTag(TEXT("ET_DetailMid"))?24000.f:Component->ComponentHasTag(TEXT("ET_DetailFar"))?35000.f:Distance;
            Component->SetCullDistance(FMath::Min(Distance,Cast<AETItem>(A)?16000.f:Cap));
        }
    };
    for(auto A:StoryActors) Apply(A);
    for(auto& It:LiveThreats) if(IsValid(It.Value)) It.Value->UpdatePresence(P);
    for(auto It:Regions) {
        auto R=It.Value;
        for(auto H:R->Instances) {
            float Cap=H->ComponentHasTag(TEXT("ET_DetailNear"))?14000.f:H->ComponentHasTag(TEXT("ET_DetailMid"))?24000.f:H->ComponentHasTag(TEXT("ET_DetailFar"))?35000.f:Distance;
            Cap=FMath::Min(Cap,Distance); H->SetCullDistances(Cap*.75f,Cap);
        }
        for(auto A:R->Residents) Apply(A);
    }
}
