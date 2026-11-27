#include "ETGame.h"
#include "ETGeology.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Materials/MaterialInterface.h"

bool AETWorld::SafeSettlement(FVector P) const
{
    if(P.Z<SurfaceHeight(P.X,P.Y)-360) return false;
    if(P.SizeSquared2D()<FMath::Square(2350.)) return true;
    for(TActorIterator<AETItem> It(GetWorld());It;++It) {
        if(It->Kind!=TEXT("human")&&It->Kind!=TEXT("friend")) continue;
        const auto At=It->GetActorLocation();
        if(FVector::DistSquared(P,At)>=FMath::Square(1450.)) continue;
        if(!InCity(At)||!TownRemaining(ETNav::District(FVector2D(At)))) return true;
    }
    return false;
}
AETAgent* AETWorld::AddThreat(const FString& Id,const FString& Kind,FVector At)
{
    if(auto* A=LiveThreats.FindRef(Id);IsValid(A)) return A;
    auto* A=GetWorld()->SpawnActor<AETAgent>(At,FRotator::ZeroRotator);
    if(!A) return nullptr;
    A->Managed=true; A->Configure(this,Id,Kind);
    if(!IsValid(A)) return nullptr;
    if(auto* Record=State->Threats.Find(Id)) {
        A->Health=Record->Health;
        A->AttackTime=Record->AttackCooldown>=0?Record->AttackCooldown:1.6f;
    } else {FETThreat R; R.Kind=Kind; A->Snapshot(R); State->Threats.Add(Id,R);}
    LiveThreats.Add(Id,A);
    return A;
}
void AETWorld::SplitRobot(AETAgent* Parent)
{
    const FVector Center=Parent->GetActorLocation()+FVector(0,0,100);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETDetach),false,Parent);
    for(int32 I=0;I<2;++I) {
        FVector Offset=Parent->GetActorRightVector()*(I?86:-86)+FVector(0,0,34);
        FHitResult Block;
        const bool Tight=GetWorld()->SweepSingleByChannel(Block,Center,Center+Offset,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(22),Q);
        const FVector At=Tight?FMath::Lerp(Center,Center+Offset,Block.Time*.8):Center+Offset;
        const FString Id=Parent->Id+FString::Printf(TEXT("/shard%d"),I);
        auto* Drone=Parent->FrontUnit?FrontFighter(Id,TEXT("drone"),At):AddThreat(Id,TEXT("drone"),At);
        if(Drone) {Drone->AttackTime=.85f+I*.45f; Drone->Phase=I*PI;}
    }
    Sound(TEXT("ShardChirp"),.52f);
    if(!Parent->FrontUnit||Parent->PlayerAggro>0) Say(TEXT("Supertech hull breached / two shard drones detaching"));
}
void AETWorld::TickPatrols(AETCharacter* P,float Delta)
{
    if(IsNight()) State->NextPatrol=FMath::Max(0.f,State->NextPatrol-Delta);
    PatrolTick-=Delta; if(PatrolTick>0) return; PatrolTick=1;
    const FVector Here=P->GetActorLocation();
    AtRefuge=SafeSettlement(Here);
    if(!IsNight()) {
        // Keep saved encounter records. Retire their actors while travelling in daylight.
        for(auto It=LiveThreats.CreateIterator();It;++It) {
            auto* A=It.Value();
            if(IsValid(A)&&FVector::DistSquared(Here,A->GetActorLocation())<=FMath::Square(14500.)) continue;
            if(IsValid(A)) {if(auto* R=State->Threats.Find(It.Key())) A->Snapshot(*R); A->Destroy();}
            It.RemoveCurrent();
        }
        return;
    }
    for(auto It=State->Threats.CreateIterator();It;++It) {
        auto* Live=LiveThreats.FindRef(It.Key());
        if(IsValid(Live)) Live->Snapshot(It.Value());
        const double D=FVector::DistSquared(Here,It.Value().Position);
        if(D>FMath::Square(14500.)) {
            if(IsValid(Live)) Live->Destroy();
            LiveThreats.Remove(It.Key());
            // Wanderers leave the area. Detached modules from fixed guards stay at the site.
            if(It.Key().StartsWith(TEXT("patrol-"))) It.RemoveCurrent();
        } else if(!IsValid(Live)) AddThreat(It.Key(),It.Value().Kind,It.Value().Position);
    }
    if(State->NextPatrol>0||!P->Panel.IsEmpty()||AtRefuge) return;
    const double Depth=SurfaceHeight(Here.X,Here.Y)-Here.Z;
    const bool Cave=Depth>200;
    int32 Nearby=0;
    for(TActorIterator<AETAgent> It(GetWorld());It;++It)
        if(It->Kind!=TEXT("deer")&&(!Cave||FMath::Abs(It->GetActorLocation().Z-Here.Z)<850)&&FVector::DistSquared(Here,It->GetActorLocation())<FMath::Square(6000.)) ++Nearby;
    if(Nearby>=4) {State->NextPatrol=6; return;}
    const bool Brood=Cave||(State->PatrolNumber%2==1);
    FRandomStream R(SeedFor({State->PatrolNumber,FMath::FloorToInt(State->Elapsed/5)},847));
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETPatrolGround),false,P);
    int32 Spawned=0;
    const double BaseAngle=P->GetControlRotation().Yaw+R.FRandRange(-55,55);
    for(int32 Attempt=0;Attempt<16&&Spawned<(Brood?3:1)&&Nearby+Spawned<5;++Attempt) {
        const double Angle=FMath::DegreesToRadians(BaseAngle+Attempt*31.);
        const double Range=Cave?R.FRandRange(380,1120):R.FRandRange(2500,4000);
        FVector Spot=Here+FVector(FMath::Cos(Angle)*Range,FMath::Sin(Angle)*Range,0);
        if(!Cave) Spot.Z=SurfaceHeight(Spot.X,Spot.Y)+100;
        FHitResult Floor;
        if(!GetWorld()->LineTraceSingleByChannel(Floor,Spot+FVector(0,0,Cave?60:270),Spot-FVector(0,0,Cave?260:430),ECC_Visibility,Q)||
           Floor.ImpactNormal.Z<.67||Cast<AETConstruction>(Floor.GetActor())||Cast<AETItem>(Floor.GetActor())||Cast<AETAgent>(Floor.GetActor())) continue;
        Spot=Floor.ImpactPoint+FVector(0,0,8);
        if((Cave&&!Geology->IsOpen(Spot+FVector(0,0,85)))||SafeSettlement(Spot)) continue;
        if(Cave&&GetWorld()->LineTraceTestByChannel(Here,Spot+FVector(0,0,54),ECC_Visibility,Q)) continue;
        if(GetWorld()->OverlapBlockingTestByChannel(Spot+FVector(0,0,80),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,65),Q)) continue;
        const FString Id=FString::Printf(TEXT("patrol-%d-%d"),State->PatrolNumber,Spawned);
        FString Kind=Brood?(Spawned==0?TEXT("spitter"):TEXT("runner")):TEXT("robot");
        if(Cave&&Depth>=1400&&Spawned==0) Kind=TEXT("bulwark");
        if(Brood&&!Cave&&ThreatTier()>=2&&Spawned==1) {Kind=TEXT("skimmer"); Spot.Z+=260;}
        if(auto* A=AddThreat(Id,Kind,Spot)) {A->AttackTime=1.6f+Spawned*.3f; ++Spawned;}
    }
    // In a tight deep shaft a siege beetle can bore a side chamber of its own.
    bool Excavated=false;
    if(!Spawned&&Depth>1400&&Geology->IsOpen(Here)) {
        for(int32 I=0;I<8;++I) {
            const FVector Ray=FRotator(0,BaseAngle+I*45,0).Vector();
            FHitResult Wall,Floor;
            if(!GetWorld()->LineTraceSingleByChannel(Wall,Here,Here+Ray*750,ECC_Visibility,Q)||!Geology->CanBore(Wall)||Wall.Distance<100) continue;
            const FVector Site=Wall.ImpactPoint+Ray*100;
            FString Empty; if(!Geology->Blast(Wall,Empty,false,false)) continue;
            Excavated=true;
            if(GetWorld()->LineTraceSingleByChannel(Floor,Site,Site-FVector(0,0,600),ECC_Pawn,Q)&&Floor.ImpactNormal.Z>.55) {
                const FVector Pos=Floor.ImpactPoint+FVector(0,0,9);
                if(Geology->IsOpen(Pos+FVector(0,0,70))&&!GetWorld()->OverlapBlockingTestByChannel(Pos+FVector(0,0,79),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(42,62),Q)) {
                    if(auto* A=AddThreat(FString::Printf(TEXT("patrol-%d-burrow"),State->PatrolNumber),TEXT("bulwark"),Pos)) {A->AttackTime=2.2f; ++Spawned;}
                }
            }
            break;
        }
    }
    if(Spawned) {
        ++State->PatrolNumber; State->NextPatrol=Cave&&Here.Z<SurfaceHeight(Here.X,Here.Y)-1400?23:InCity(Here)||Cave?42:52;
        Sound(Brood?TEXT("BroodCall"):TEXT("Robot"),.38f);
        Say(Brood?TEXT("BUG / spore calls nearby"):TEXT("SUPERTECH / hostile patrol signal nearby"));
    } else State->NextPatrol=Excavated?22:4;
}

AETEnemyBolt::AETEnemyBolt()
{
    PrimaryActorTick.bCanEverTick=true;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Tracer")); SetRootComponent(Mesh);
    Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision); Mesh->SetCastShadow(false);
}
void AETEnemyBolt::Launch(AETAgent* Source,FVector Target,bool Acid)
{
    SetOwner(Source); Field=Source->World;
    FrontBolt=Source->FrontUnit; BugBolt=Source->IsBrood();
    const FVector Aim=(Target-GetActorLocation()).GetSafeNormal();
    SetActorRotation(Aim.Rotation()); Velocity=Aim*(Acid?1250:2250); Power=Acid?11:(Source->Kind==TEXT("robot")?13:6);
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Mesh->SetMaterial(0,Field->Material(Acid?TEXT("Greenline"):TEXT("Signal")));
    Mesh->SetWorldScale3D(Acid?FVector(.25,.14,.14):FVector(.42,.065,.065)); SetLifeSpan(3.2f);
}
void AETEnemyBolt::Beam(AETWorld* W,FVector End)
{
    const FVector Start=GetActorLocation(),Line=End-Start;
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Mesh->SetMaterial(0,W->Material(TEXT("Greenline")));
    SetActorLocation((Start+End)*.5); SetActorRotation(Line.Rotation()); Mesh->SetWorldScale3D(FVector(Line.Size()/100.,.033,.033));
    SetActorTickEnabled(false); SetLifeSpan(.09f);
}
void AETEnemyBolt::Tick(float D)
{
    Super::Tick(D); if(!Field) return;
    if(!FrontBolt&&!Field->IsNight()) {Destroy(); return;}
    const FVector From=GetActorLocation(),To=From+Velocity*D;
    FHitResult Hit; FCollisionQueryParams Q(SCENE_QUERY_STAT(ETEnemyBolt),false,this);
    Q.AddIgnoredActor(GetOwner());
    if(!GetWorld()->SweepSingleByChannel(Hit,From,To,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(6),Q)) {SetActorLocation(To); return;}
    if(auto* Enemy=Cast<AETAgent>(Hit.GetActor());Enemy&&((BugBolt&&Enemy->IsMachine())||(!BugBolt&&Enemy->IsBrood())))
        Enemy->TakeDamage(Power,FDamageEvent(),nullptr,this);
    if(auto* P=Cast<AETCharacter>(Hit.GetActor());P&&Field->IsNight()&&!Field->AtRefuge&&P->EnemyReacquireTime<=0) P->TakeDamage(Power,FDamageEvent(),nullptr,this);
    if(auto* Wall=Cast<AETConstruction>(Hit.GetActor());Wall&&(!FrontBolt||Field->IsNight())) Field->HitBuild(Wall,Power);
    Destroy();
}
