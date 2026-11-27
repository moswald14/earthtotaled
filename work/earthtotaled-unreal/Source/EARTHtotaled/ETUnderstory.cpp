#include "ETUnderstory.h"
#include "ETWake.h"
#include "ETDayCycle.h"
#include "ETLandfall.h"
#include "ETGame.h"
#include "ETGeology.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"
#include "Engine/DamageEvents.h"

namespace {
constexpr int Span=1280;
enum ECover { Blades, Weeds, Deadfall, Caps, Bracken, Cones, LayerCount };
const TCHAR* const Meshes[]={TEXT("SM_BladePatch"),TEXT("SM_RoadWeed"),TEXT("SM_CedarFall"),
    TEXT("SM_AmberCaps"),TEXT("SM_BentBracken"),TEXT("SM_ConeBed")};
FIntPoint PlotAt(FVector P) {return {FMath::FloorToInt32(P.X/Span),FMath::FloorToInt32(P.Y/Span)};}
struct FPlot {
    FIntPoint Key=FIntPoint::ZeroValue;bool Assigned=false;
    UInstancedStaticMeshComponent* Layers[LayerCount]={};
};
struct FUnderstory {
    TArray<FPlot> Plots;
    int32 LastCuts=INDEX_NONE,LastBuilds=INDEX_NONE;
    UInstancedStaticMeshComponent* Layer(AETWorld* W,const TCHAR* Name) {
        auto* C=NewObject<UInstancedStaticMeshComponent>(W);
        const auto Path=FString(TEXT("/Game/EARTHtotaled/Art/"))+Name+TEXT(".")+Name;
        C->SetMobility(EComponentMobility::Movable);C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path));C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCanEverAffectNavigation(false);C->SetCastShadow(false);C->bAffectDistanceFieldLighting=false;
        C->ComponentTags.Add(TEXT("Understory"));C->SetCullDistances(2400,3800);C->RegisterComponent();W->AddInstanceComponent(C);return C;
    }
    void Fill(AETWorld* W,AETRegion& R,FPlot& Plot,FIntPoint Key) {
        Plot.Assigned=true;Plot.Key=Key;
        const FVector Origin(double(Key.X)*Span,double(Key.Y)*Span,0);
        const FVector Center=Origin+FVector(Span/2,Span/2,0);
        if(W->InCity(Center)||R.InHighwayGrounds(FVector2D(Center),600)) return;
        const FVector HouseA=W->HouseAnchor(R.Coord,0),HouseB=W->HouseAnchor(R.Coord,1);
        TArray<FTransform> Cover[LayerCount];FRandomStream Dice(W->SeedFor(Key,2231)),Litter(W->SeedFor(Key,8309));
        Cover[Blades].Reserve(64);
        TArray<FBox> Foundations;
        for(const auto& It:W->LiveBuilds)if(const auto* Built=It.Value.Get())
            if(FVector::DistSquared2D(Center,Built->GetActorLocation())<FMath::Square(1450.)){
                const auto Extent=AETConstruction::Extents(Built->Design.Piece);
                Foundations.Add(FBox(-Extent,Extent).TransformBy(AETConstruction::Shape(Built->Design)).ExpandBy(FVector(58,58,72)));
            }
        for(int Y=0;Y<8;++Y) for(int X=0;X<8;++X) {
            const FVector Local((X+Dice.FRandRange(.05f,.95f))*Span/8.,(Y+Dice.FRandRange(.05f,.95f))*Span/8.,0);
            const FVector P=Origin+Local;const double Patch=FMath::PerlinNoise2D(FVector2D(P)*.00065);
            if(Dice.FRand()>FMath::Clamp(.78+Patch*.48,.39,.98)||R.OnHighway(FVector2D(P),12)) continue;
            if(ETWake::Paved(W->State->Wake,FVector2D(P))||FVector::Dist2D(P,HouseA)<1120||FVector::Dist2D(P,HouseB)<1120||W->Geology->SurfaceOpening(P.X,P.Y)<25) continue;
            const double Z=W->SurfaceHeight(P.X,P.Y);const float Scale=Dice.FRandRange(.66,1.5);
            const FTransform T(FRotator(0,Dice.FRandRange(0,360),0),FVector(Local.X,Local.Y,Z-3),FVector(Scale));
            const int LeafLayer=Dice.RandRange(0,19)==0?Weeds:Blades;
            if(Foundations.ContainsByPredicate([&](const FBox& B){return B.IsInside(FVector(P.X,P.Y,Z));}))continue;
            Cover[LeafLayer].Add(T);
            const double Shade=FMath::PerlinNoise2D(FVector2D(P)*.0012+FVector2D(16,52));
            if(Litter.FRand()>FMath::Clamp(.22+Shade*.21,.10,.32))continue;
            const double Pick=Litter.FRand()+Shade*.24;
            const int Kind=Pick<.14?Deadfall:Pick<.35?Cones:Pick<.66?Bracken:Caps;
            const double Angle=Litter.FRandRange(0,360),Size=Litter.FRandRange(.8,1.6);
            const FVector Axis=FRotator(0,Angle,0).Vector();
            const double Pitch=FMath::RadiansToDegrees(FMath::Atan2(W->SurfaceHeight(P.X+Axis.X*35,P.Y+Axis.Y*35)-Z,35.));
            const FVector Across(-Axis.Y,Axis.X,0);
            const double Roll=-FMath::RadiansToDegrees(FMath::Atan2(W->SurfaceHeight(P.X+Across.X*35,P.Y+Across.Y*35)-Z,35.));
            if(FMath::Abs(Pitch)<35&&FMath::Abs(Roll)<35){
                Cover[Kind].Emplace(FRotator(Pitch,Angle,Roll),FVector(Local.X,Local.Y,Z-.4),FVector(Size));
                // A rotting limb or fruiting patch breaks up the grass at its feet.
                if(Kind!=Bracken)Cover[LeafLayer].Last().SetScale3D(FVector(Scale*(Kind==Caps?.36:.61)));
            }
        }
        for(int I=0;I<LayerCount;++I)if(!Cover[I].IsEmpty()){
            auto*& C=Plot.Layers[I];if(!C)C=Layer(W,Meshes[I]);
            C->SetWorldLocation(Origin);C->AddInstances(Cover[I],false,false,false);
        }
    }
    void Update(AETWorld* W) {
        const auto* P=UGameplayStatics::GetPlayerPawn(W,0);if(!P||!W->State||!W->Geology) return;
        const auto Here=P->GetActorLocation();const auto Center=PlotAt(Here);
        const int Cuts=W->State->BoreCuts.Num()+W->State->SquareCuts.Num();
        const int Builds=W->State->Builds.Num();
        for(auto& Plot:Plots) if(Plot.Assigned&&(Cuts!=LastCuts||Builds!=LastBuilds||FMath::Abs(Plot.Key.X-Center.X)>2||FMath::Abs(Plot.Key.Y-Center.Y)>2)) {
            for(auto* C:Plot.Layers)if(C)C->ClearInstances();Plot.Assigned=false;
        }
        LastCuts=Cuts;LastBuilds=Builds;
        const double Elevation=Here.Z-W->SurfaceHeight(Here.X,Here.Y);
        if(W->InCity(Here)||Elevation< -350||Elevation>4000)return;
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

void ETFlora::Trial(AETWorld& W,AETCharacter& P,TFunctionRef<void(bool,FString)> Check)
{
    W.State->ArrivalStage=2;P.TestInvincible=false;
    if(P.Panel=="title"){P.Panel.Empty();UGameplayStatics::SetGamePaused(&W,false);P.SetMouseMode(false);}
    else if(!P.Panel.IsEmpty())P.ClosePanel();
    auto* Survey=W.GetWorld()->SpawnActor<AETRegion>();
    int Dens=0;FVector Nearest=FVector::ZeroVector;FIntPoint DenKey=FIntPoint::ZeroValue;bool Stable=true;
    for(int X=-8;X<=8;++X)for(int Y=-8;Y<=8;++Y){
        Survey->LocalRoads.Reset();Survey->HighwaySites.Reset();Survey->BeginGenerate(&W,{X,Y});FVector At,Again;
        if(!Survey->BearDen(At))continue;
        ++Dens;Stable&=Survey->BearDen(Again)&&At.Equals(Again,.01)&&!W.InCity(At)&&!Survey->OnHighway(FVector2D(At),1500)&&!ETWake::Reserved(W.State->Wake,FVector2D(At),3200);
        if(Nearest.IsZero()||At.SizeSquared2D()<Nearest.SizeSquared2D()){Nearest=At;DenKey={X,Y};}
    }
    Survey->Destroy();Check(Dens>0&&Dens<36&&Stable,FString::Printf(TEXT("%d stable off-road bear dens in 289 surveyed districts / closest %d,%d"),Dens,DenKey.X,DenKey.Y));
    if(!Dens)return;
    W.State->ClockOffset=ETDay::At(22)-W.State->Elapsed;
    W.EnsureRegion(DenKey);auto* Region=W.Regions.FindRef(DenKey);if(!Region){Check(false,TEXT("Streamed the den district"));return;}
    auto Resident=[&]()->AETAgent*{for(auto* A:Region->Residents)if(auto* B=Cast<AETAgent>(A);IsValid(B)&&B->Kind=="bear")return B;return nullptr;};
    auto* Bear=Resident();Check(Bear&&Bear->Parts.Num()==5&&Bear->Health==360,TEXT("Region streams a complete bear with four articulated limbs and 360 health"));
    if(!Bear)return;
    auto Ground=[&](FVector At){At.Z=W.SurfaceHeight(At.X,At.Y)+96;return At;};
    auto Approach=[&](double Range){
        const auto Base=Bear->GetActorLocation();FCollisionQueryParams Q;Q.AddIgnoredActor(Bear);Q.AddIgnoredActor(&P);
        for(int K=0;K<12;++K){
            const auto Goal=Ground(Base+FRotator(0,K*30.,0).Vector()*Range);
            if(FMath::Abs(Goal.Z-Base.Z-90)<100&&!W.GetWorld()->LineTraceTestByChannel(Base+FVector(0,0,65),Goal,ECC_Visibility,Q)){
                P.SetActorLocation(Goal);return true;}
        }
        return false;
    };
    Check(P.Panel.IsEmpty()&&W.IsNight()&&Approach(205),TEXT("Living survivor is in the world on a clear night approach to the den"));
    W.AtRefuge=false;P.EnemyReacquireTime=0;
    Bear->UpdatePresence(P.GetActorLocation());
    W.State->Shield=100;const auto Face=Bear->GetActorLocation();
    for(int I=0;I<70&&Bear->BearWindup<=0;++I)Bear->Tick(.04f);
    Check(Bear->BearWindup>0&&W.State->Shield==100,TEXT("A nearby bear warns before its swipe"));
    P.SetActorLocation(Ground(Face+FVector(0,540,0)));
    for(int I=0;I<23;++I)Bear->Tick(.04f);
    Check(W.State->Shield==100,TEXT("Stepping out of reach avoids the swipe"));
    Approach(150);Bear->AttackTime=0;
    for(int I=0;I<23;++I)Bear->Tick(.04f);
    Check(W.State->Shield==64,TEXT("Standing within the arc takes one 36-point hit on the shield"));
    W.State->ClockOffset=ETDay::At(12)-W.State->Elapsed;Bear->UpdatePresence(P.GetActorLocation());
    Check(Bear->IsHidden()&&!Bear->GetActorEnableCollision(),TEXT("The bear withdraws in daylight"));
    W.State->ClockOffset=ETDay::At(22)-W.State->Elapsed;Bear->UpdatePresence(P.GetActorLocation());
    const FString Id=Bear->Id;const float Shot=W.Weapons[ETLandfall::Pistol].Damage;
    const FVector Body=Bear->AimPoint();FHitResult Hit;FCollisionQueryParams Ray;Ray.AddIgnoredActor(&P);
    const bool BodyHit=W.GetWorld()->LineTraceSingleByChannel(Hit,Body+FVector(0,0,260),Body,ECC_Visibility,Ray)&&Hit.GetActor()==Bear;
    Check(BodyHit,TEXT("The rendered bear body blocks the weapon trace"));
    UGameplayStatics::ApplyPointDamage(Bear,Shot,FVector::DownVector,Hit,P.Controller,&P,nullptr);Bear->Destroy();
    Region->AddBearDen();Bear=Resident();
    Check(Bear&&Bear->Health==360-Shot,TEXT("A wounded bear keeps its health when streamed back in"));
    if(!Bear)return;
    int Hits=1;while(IsValid(Bear)&&Hits<60){Bear->TakeDamage(Shot,FDamageEvent(),P.Controller,&P);++Hits;}
    Region->AddBearDen();
    Check(Hits>=6&&W.State->Consumed.Contains(Id)&&!Resident()&&W.LooseActors.FindRef(Id+TEXT("-drop")).IsValid(),
        FString::Printf(TEXT("Bear fell after %d base-pistol hits, left harvest and stayed gone"),Hits));

    FUnderstory Stream;
    auto Fill=[&](){for(int N=0;N<12;++N)Stream.Update(&W);};
    auto Layout=[&](){
        TArray<FVector> Positions;
        for(int Kind=Deadfall;Kind<LayerCount;++Kind){
            int Count=0;
            for(const auto& Plot:Stream.Plots)if(Plot.Layers[Kind]){
                auto* C=Plot.Layers[Kind];for(int I=0;I<C->GetInstanceCount();++I){
                    FTransform T;C->GetInstanceTransform(I,T,true);Positions.Add(T.GetLocation());++Count;}
            }
            Check(Count&&Count<1100,FString::Printf(TEXT("Grass plots stream %d instances of %s"),Count,Meshes[Kind]));
        }
        Positions.Sort([](const FVector& A,const FVector& B){return A.X==B.X?A.Y<B.Y:A.X<B.X;});return Positions;
    };
    const auto Here=Ground(Nearest);P.SetActorLocation(Here);Fill();const auto First=Layout();
    P.SetActorLocation(Here+FVector(22000,0,-20000));Fill();
    bool Empty=true;for(const auto& Plot:Stream.Plots)for(auto* C:Plot.Layers)if(C)Empty&=C->GetInstanceCount()==0;
    Check(Empty,TEXT("Moving away releases every grass and forest-floor instance"));
    P.SetActorLocation(Here);Fill();const auto Second=Layout();
    Check(Stream.Plots.Num()<=25&&First==Second&&!First.IsEmpty(),TEXT("Revisiting reproduces the same clutter in the fixed plot pool"));
}
