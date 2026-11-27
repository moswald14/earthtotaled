#include "ETWake.h"
#include "ETUnderstory.h"
#include "ETGame.h"
#include "ETGeology.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

namespace {
constexpr int Span=1280;
FIntPoint PlotAt(FVector P) {return {FMath::FloorToInt32(P.X/Span),FMath::FloorToInt32(P.Y/Span)};}
struct FPlot {
    FIntPoint Key=FIntPoint::ZeroValue;bool Assigned=false;
    UInstancedStaticMeshComponent* Blades=nullptr;
    UInstancedStaticMeshComponent* Weeds=nullptr;
};
struct FUnderstory {
    TArray<FPlot> Plots;
    int32 LastCuts=INDEX_NONE;
    UInstancedStaticMeshComponent* Layer(AETWorld* W,const TCHAR* Name) {
        auto* C=NewObject<UInstancedStaticMeshComponent>(W);
        const auto Path=FString(TEXT("/Game/EARTHtotaled/Art/"))+Name+TEXT(".")+Name;
        C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);C->bAffectDistanceFieldLighting=false;
        C->SetCullDistances(2400,3800);C->RegisterComponent();W->AddInstanceComponent(C);return C;
    }
    void Fill(AETWorld* W,AETRegion& R,FPlot& Plot,FIntPoint Key) {
        Plot.Assigned=true;Plot.Key=Key;
        const FVector Origin(double(Key.X)*Span,double(Key.Y)*Span,0);
        const FVector Center=Origin+FVector(Span/2,Span/2,0);
        if(W->InCity(Center)||R.InHighwayGrounds(FVector2D(Center),600)) return;
        const FVector HouseA=W->HouseAnchor(R.Coord,0),HouseB=W->HouseAnchor(R.Coord,1);
        TArray<FTransform> Grass,Weeds;FRandomStream Dice(W->SeedFor(Key,2231));Grass.Reserve(64);
        for(int Y=0;Y<8;++Y) for(int X=0;X<8;++X) {
            const FVector Local((X+Dice.FRandRange(.05f,.95f))*Span/8.,(Y+Dice.FRandRange(.05f,.95f))*Span/8.,0);
            const FVector P=Origin+Local;const double Patch=FMath::PerlinNoise2D(FVector2D(P)*.00065);
            if(Dice.FRand()>FMath::Clamp(.78+Patch*.48,.39,.98)||R.OnHighway(FVector2D(P),12)) continue;
            if(ETWake::Paved(W->State->Wake,FVector2D(P))||FVector::Dist2D(P,HouseA)<1120||FVector::Dist2D(P,HouseB)<1120||W->Geology->SurfaceOpening(P.X,P.Y)<25) continue;
            const double Z=W->SurfaceHeight(P.X,P.Y);const float Scale=Dice.FRandRange(.66,1.5);
            const FTransform T(FRotator(0,Dice.FRandRange(0,360),0),FVector(Local.X,Local.Y,Z-3),FVector(Scale));
            if(Dice.RandRange(0,19)==0) Weeds.Add(T);else Grass.Add(T);
        }
        if(Grass.IsEmpty()&&Weeds.IsEmpty()) return;
        if(!Plot.Blades) {Plot.Blades=Layer(W,TEXT("SM_BladePatch"));Plot.Weeds=Layer(W,TEXT("SM_RoadWeed"));}
        Plot.Blades->SetWorldLocation(Origin);Plot.Weeds->SetWorldLocation(Origin);
        Plot.Blades->AddInstances(Grass,false,false,false);Plot.Weeds->AddInstances(Weeds,false,false,false);
    }
    void Update(AETWorld* W) {
        const auto* P=UGameplayStatics::GetPlayerPawn(W,0);if(!P||!W->State||!W->Geology) return;
        const auto Here=P->GetActorLocation();const auto Center=PlotAt(Here);
        const int Cuts=W->State->BoreCuts.Num()+W->State->SquareCuts.Num();
        for(auto& Plot:Plots) if(Plot.Assigned&&(Cuts!=LastCuts||FMath::Abs(Plot.Key.X-Center.X)>2||FMath::Abs(Plot.Key.Y-Center.Y)>2)) {
            if(Plot.Blades) {Plot.Blades->ClearInstances();Plot.Weeds->ClearInstances();}Plot.Assigned=false;
        }
        LastCuts=Cuts;
        if(W->InCity(Here)||Here.Z<W->SurfaceHeight(Here.X,Here.Y)-350) return;
        int Budget=4;
        for(int Ring=0;Ring<=2;++Ring) for(int Y=-Ring;Y<=Ring;++Y) for(int X=-Ring;X<=Ring;++X) {
            if(FMath::Max(FMath::Abs(X),FMath::Abs(Y))!=Ring) continue;
            const FIntPoint Key=Center+FIntPoint(X,Y);
            if(Plots.ContainsByPredicate([&](const FPlot& Plot){return Plot.Assigned&&Plot.Key==Key;})) continue;
            const FVector2D Mid((Key.X+.5)*Span,(Key.Y+.5)*Span);
            auto* R=W->Regions.FindRef({FMath::FloorToInt32(Mid.X/AETWorld::RegionSize),FMath::FloorToInt32(Mid.Y/AETWorld::RegionSize)});
            if(!R||!R->TerrainGrid) continue;
            auto* Next=Plots.FindByPredicate([](const FPlot& Plot){return !Plot.Assigned;});
            if(!Next) Next=&Plots.AddDefaulted_GetRef();Fill(W,*R,*Next,Key);
            if(--Budget==0) return;
        }
    }
};
}
void ETFlora::Start(AETWorld* W)
{
    if(W->SelfTest) return;
    const auto Cover=MakeShared<FUnderstory>();Cover->Plots.Reserve(25);
    FTimerHandle Handle;
    W->GetWorld()->GetTimerManager().SetTimer(Handle,FTimerDelegate::CreateWeakLambda(W,[W,Cover](){Cover->Update(W);}),.14f,true);
}
