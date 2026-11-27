#include "ETGame.h"
#include "ETGeology.h"
#include "ETWake.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

bool AETRegion::BearDen(FVector& Site) const
{
    FRandomStream R(World->SeedFor(Coord,67031));
    if(R.RandRange(1,16)!=1)return false;
    const FVector2D Origin=FVector2D(Coord.X,Coord.Y)*AETWorld::RegionSize;
    for(int Attempt=0;Attempt<8;++Attempt){
        const double X=R.FRandRange(3100,28500),Y=R.FRandRange(3100,28500);
        const FVector2D XY=Origin+FVector2D(X,Y);
        Site=FVector(XY,World->SurfaceHeight(XY.X,XY.Y)+8);
        if(XY.Size()<24000||World->InCity(Site)||World->SafeSettlement(Site)||OnHighway(XY,1500)||
           InHighwayGrounds(XY,1800)||ETWake::Reserved(World->State->Wake,XY,3200))continue;
        if(FVector::Dist2D(Site,World->HouseAnchor(Coord,0))<2500||FVector::Dist2D(Site,World->HouseAnchor(Coord,1))<2500||
           World->Geology->SurfaceOpening(XY.X,XY.Y)<80)continue;
        return true;
    }
    return false;
}

void AETRegion::AddBearDen()
{
    FVector Site;
    const FString Id=FString::Printf(TEXT("r%d,%d-bear"),Coord.X,Coord.Y);
    if(World->State->Consumed.Contains(Id)||!BearDen(Site))return;
    if(auto* Bear=GetWorld()->SpawnActor<AETAgent>(Site,FRotator(0,World->SeedFor(Coord,409)%360,0))){
        Bear->SetOwner(this);Bear->Configure(World,Id,TEXT("bear"));Residents.Add(Bear);
        const FVector2D Mark=FVector2D(Site)+FVector2D(180,-135);
        auto* Tracks=World->Static(TEXT("SM_BearSign"),TEXT("Ground"),FVector(Mark,World->SurfaceHeight(Mark.X,Mark.Y)+1),FVector(1.2));
        Tracks->SetActorEnableCollision(false);Residents.Add(Tracks);
    }
}

void AETAgent::AttachBearLegs()
{
    for(int I=0;I<4;++I){
        const bool Front=I<2;
        auto* Mesh=NewObject<UStaticMeshComponent>(this);Mesh->SetupAttachment(Pivot);
        const TCHAR* Path=Front?TEXT("/Game/EARTHtotaled/Art/SM_BearForeleg.SM_BearForeleg"):TEXT("/Game/EARTHtotaled/Art/SM_BearHindleg.SM_BearHindleg");
        Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,Path));
        Mesh->SetRelativeLocation(FVector(Front?39:-63,I%2?29:-29,67));
        Mesh->SetCollisionProfileName(TEXT("BlockAll"));Mesh->RegisterComponent();Parts.Add(Mesh);
    }
}

void AETAgent::PoseBear(float Stride,float Windup)
{
    if(Parts.Num()!=5)return;
    for(int I=0;I<4;++I){
        const float Swing=FMath::Sin(Stride+(I==0||I==3?0.f:PI))*20.f;
        Parts[I+1]->SetRelativeRotation(FRotator(I==0&&Windup>0?65.f*Windup:Swing,0,I==0?-13.f*Windup:0));
    }
    if(FlinchTime<=0)Parts[0]->SetRelativeRotation(FRotator(Windup*9,0,-Windup*4));
}

bool AETAgent::BearSwipe(AETCharacter& Player,float Delta,bool Clear)
{
    const FVector Reach=Player.GetActorLocation()-(GetActorLocation()+FVector(0,0,84));
    const bool Vulnerable=Player.EnemyReacquireTime<=0&&Clear&&Reach.Size()<245;
    if(BearWindup<=0){
        if(AttackTime>0||!Vulnerable)return false;
        SetActorRotation(FRotator(0,Reach.Rotation().Yaw,0));BearWindup=.82f;
        World->Sound(TEXT("BearHuff"),.72f);
    }
    BearWindup=FMath::Max(0.f,BearWindup-Delta);
    PoseBear(BearStride,FMath::Clamp((.82f-BearWindup)*3.7f,0.f,1.f)*FMath::Clamp(BearWindup*8.f,0.f,1.f));
    if(BearWindup<=0){
        if(Vulnerable&&GetActorForwardVector().Dot(Reach.GetSafeNormal2D())>.5)
            Player.TakeDamage(36,FDamageEvent(),nullptr,this);
        AttackTime=2.f;
    }
    return true;
}
