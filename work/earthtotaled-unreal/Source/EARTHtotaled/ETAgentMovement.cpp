#include "ETAgentMovement.h"
#include "ETGame.h"
#include "ETGeology.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

namespace {
    bool Walkable(const FHitResult& Floor) {
        return Floor.ImpactNormal.Z>=.62&&!Cast<AETAgent>(Floor.GetActor())&&
            !Cast<APawn>(Floor.GetActor())&&!Cast<AETItem>(Floor.GetActor());
    }
}

bool ETAgentMovement::FindGroundSpawn(AETWorld& W,FVector& Position) {
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETGroundSpawn),false);
    for(int Attempt=0;Attempt<=8;++Attempt){
        const FVector XY=Position+(Attempt?FRotator(0,(Attempt-1)*45.,0).Vector()*240.:FVector::ZeroVector);
        const FVector Ground(XY.X,XY.Y,W.SurfaceHeight(XY.X,XY.Y));
        FHitResult Floor;
        if(!W.GetWorld()->LineTraceSingleByChannel(Floor,Ground+FVector(0,0,190),Ground-FVector(0,0,210),ECC_Visibility,Query)||!Walkable(Floor))continue;
        const FVector Feet=Floor.ImpactPoint+FVector(0,0,8);
        if(W.GetWorld()->OverlapBlockingTestByChannel(Feet+FVector(0,0,74),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(32,66),Query))continue;
        Position=Feet;return true;
    }
    return false;
}

void ETAgentMovement::Face(AETAgent& A,FVector D,float Delta) {
    if(!D.IsNearlyZero())A.SetActorRotation(FRotator(0,FMath::FixedTurn(float(A.GetActorRotation().Yaw),float(D.Rotation().Yaw),330.f*Delta),0));
}

void ETAgentMovement::Walk(AETAgent& A,FVector Direction,float Speed,float Delta,AActor* Target) {
    Direction=Direction.GetSafeNormal2D();
    if(Direction.IsNearlyZero())return;
    const float Hand=(GetTypeHash(A.Id)&1)?1.f:-1.f;
    const FVector Start=A.GetActorLocation();
    FVector Here=Start;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETGroundAgent),false,&A);
    if(Cast<APawn>(Target)||Cast<AETAgent>(Target))Query.AddIgnoredActor(Target);
    const bool Bug=A.IsBrood();
    double Left=Speed*FMath::Clamp(Delta,0.f,.5f);
    while(Left>.01) {
        const double Step=FMath::Min(Left,40.);Left-=Step;
        FVector Tangent=FVector::ZeroVector;
        bool Advanced=false;
        for(float Angle:{0.f,90.f,38.f,-38.f,72.f,-72.f,-95.f,110.f}){
            const FVector D=Angle==90.f&&!Tangent.IsNearlyZero()?Tangent:Direction.RotateAngleAxis(Angle*Hand,FVector::UpVector);
            FVector Next=Here+D*Step;
            FHitResult Obstacle;
            const FVector Hip=Here+FVector(0,0,65);
            if(A.GetWorld()->SweepSingleByChannel(Obstacle,Hip,Hip+D*(Step+12.),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(21),Query)){
                if(Angle==0.f)Tangent=FVector::CrossProduct(FVector::UpVector,Obstacle.ImpactNormal).GetSafeNormal2D()*Hand;
                continue;
            }
            if(Bug&&!A.World->Geology->BugCanStep(Here,Next))continue;
            FHitResult Floor;
            if(!A.GetWorld()->LineTraceSingleByChannel(Floor,Next+FVector(0,0,64),Next-FVector(0,0,118),ECC_Visibility,Query)||!Walkable(Floor))continue;
            Next.Z=Floor.ImpactPoint.Z+8.;
            Here=Next;Advanced=true;break;
        }
        if(!Advanced)break;
    }
    const FVector Moved=Here-Start;
    if(!Moved.IsNearlyZero()){
        A.SetActorLocation(Here);Face(A,Moved,Delta);
    }else if(!Bug){
        // Settle if excavation removed the ground beneath this actor; never step across the opening.
        FHitResult Floor;
        if(A.GetWorld()->LineTraceSingleByChannel(Floor,Here+FVector(0,0,8),Here-FVector(0,0,6000),ECC_Visibility,Query)&&Walkable(Floor)&&Floor.ImpactPoint.Z+24.<Here.Z)
            A.SetActorLocation(Here-FVector(0,0,FMath::Min(Here.Z-Floor.ImpactPoint.Z-8.,650.*Delta)));
    }
    if(A.Parts.Num()&&A.FlinchTime<=0&&A.Kind!=TEXT("bear")&&A.Kind!=TEXT("deer")){
        const float Stride=Moved.Size2D()>1?FMath::Sin(A.Phase*(A.Kind==TEXT("walker")?6.f:10.f)):0.f;
        A.Parts[0]->SetRelativeRotation(FRotator(Stride*2.f,0,Stride*1.6f));
    }
}

void ETAgentMovement::FlyTo(AETAgent& A,FVector Goal,float Speed,float Delta) {
    const FVector Here=A.GetActorLocation();
    const FVector Desired=(Goal-Here).GetClampedToMaxSize(Speed*FMath::Clamp(Delta,0.f,.4f));
    if(Desired.IsNearlyZero())return;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETFlyingAgent),false,&A);
    auto Clear=[&](FVector Step,FHitResult& Hit){
        return !A.GetWorld()->SweepSingleByChannel(Hit,Here,Here+Step,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(22),Query);
    };
    FHitResult Block;
    if(Clear(Desired,Block)){A.SetActorLocation(Here+Desired);return;}
    if(Block.bStartPenetrating) {
        const FVector Push=Block.Normal.GetSafeNormal()*FMath::Clamp(double(Block.PenetrationDepth)+2.,3.,48.);
        const FVector Free=Here+Push;
        if(!Push.IsNearlyZero()&&!A.GetWorld()->OverlapBlockingTestByChannel(Free,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(22),Query)){
            A.SetActorLocation(Free);return;
        }
    }
    const double Length=Desired.Size();
    const FVector Across=FVector::CrossProduct(FVector::UpVector,Block.ImpactNormal).GetSafeNormal2D()*((GetTypeHash(A.Id)&1)?1.:-1.);
    for(FVector Alternative:{FVector::VectorPlaneProject(Desired,Block.ImpactNormal),Across*Length,FVector::UpVector*Length,-Across*Length,-FVector::UpVector*Length}){
        FHitResult Hit;
        if(Alternative.SizeSquared()>1&&Clear(Alternative,Hit)){A.SetActorLocation(Here+Alternative);return;}
    }
}

void ETAgentMovement::Patrol(AETAgent& A,float Delta) {
    const float Angle=A.Phase*.23f+float(GetTypeHash(A.Id)%628)*.01f;
    FVector Goal=A.Home+FVector(480*FMath::Cos(Angle),350*FMath::Sin(Angle),0);
    if(A.Kind==TEXT("drone")||A.Kind==TEXT("skimmer")){
        Goal.Z+=FMath::Sin(A.Phase)*28.;const FVector From=A.GetActorLocation();
        FlyTo(A,Goal,145,Delta);Face(A,A.GetActorLocation()-From,Delta);
    }else Walk(A,Goal-A.GetActorLocation(),115,Delta,nullptr);
}
