#include "ETWake.h"
#include "ETGame.h"
#include "ETWakeVehicle.h"
#include "ETGeology.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

AETItem* AETCharacter::NearbyWake() const {
    if(!World)return nullptr;const FVector Eye=Camera->GetComponentLocation(),Aim=Camera->GetForwardVector();
    AETItem* Best=nullptr;double Score=0;
    for(const auto& It:World->WakeObjects) {
        auto* A=It.Value;if(!IsValid(A)||A->IsHidden())continue;
        FVector Delta=A->GetActorLocation()+FVector(0,0,55)-Eye;const double Range=Delta.Size();
        if(Range>350)continue;double Facing=Aim.Dot(Delta.GetSafeNormal());if(Facing<.25&&Range>105)continue;
        FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(WakeUse),false,this);
        if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Delta,ECC_Visibility,Q)&&Hit.GetActor()!=A&&Hit.Distance+40<Range)continue;
        double Value=Facing+(350-Range)/410.;if(Value>Score){Best=A;Score=Value;}
    }
    return Best;
}
void AETCharacter::ApplyWakeBody() {
    if(!World||!World->State)return;
    const auto& S=World->State->Wake;bool Live=S.Seat==ETWake::Foot;
    Gun->SetVisibility(Live&&!Building&&S.Body!=ETWake::Bug);
    auto Limb=[&](UStaticMeshComponent*& LimbMesh,int Sign) {
        if(!LimbMesh){
            LimbMesh=NewObject<UStaticMeshComponent>(this);LimbMesh->SetupAttachment(Camera);
            LimbMesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
            LimbMesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);LimbMesh->SetCastShadow(false);LimbMesh->RegisterComponent();}
        const bool Bug=S.Body==ETWake::Bug;
        LimbMesh->SetRelativeLocation(FVector(49,Sign*(Bug?30:25),-27));
        LimbMesh->SetRelativeRotation(FRotator(Bug?22:3,Sign*(Bug?-20:3),Sign*18));
        LimbMesh->SetRelativeScale3D(Bug?FVector(.66,.10,.15):FVector(.39,.21,.23));
        LimbMesh->SetMaterial(0,World->Material(Bug?TEXT("Carapace"):TEXT("AlloyHull")));
        if(LimbMesh->GetNumChildrenComponents()==0){
            auto* Tip=NewObject<UStaticMeshComponent>(this);Tip->SetupAttachment(LimbMesh);
            Tip->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cone.Cone")));
            Tip->SetCollisionEnabled(ECollisionEnabled::NoCollision);Tip->SetCastShadow(false);
            Tip->SetAbsolute(false,false,true);Tip->SetRelativeLocation({36,0,15});Tip->SetRelativeRotation(FRotator(61,-Sign*27.,0));
            Tip->SetWorldScale3D({.09,.095,.295});Tip->SetMaterial(0,World->Material(TEXT("ResinLight")));Tip->RegisterComponent();}
        LimbMesh->SetVisibility(Live&&S.Body!=ETWake::Human&&(Bug||Sign<0),true);
        for(const auto& Tip:LimbMesh->GetAttachChildren())Tip->SetVisibility(Live&&Bug,true);
    };
    if(S.Body||WakeBody){Limb(WakeBody,-1);Limb(WakeCuff,1);}
}
void AETCharacter::WakeClaw() {
    if(!ETWake::BugPack(*this)||Ride()||!Panel.IsEmpty())return;
    ShotTime=.33f;World->Sound(TEXT("FieldLatch"),.48f,1.3f);
    FVector Eye=Camera->GetComponentLocation();FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(WakeClaw),false,this);
    if(GetWorld()->SweepSingleByChannel(Hit,Eye,Eye+Camera->GetForwardVector()*(Has(TEXT("molt-sense"))?560:380),FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(52),Q)){
        if(auto* Target=Cast<AETAgent>(Hit.GetActor());Target&&Target->WakeHerd<0&&Target->TakeDamage(62*DamageSkill(),FDamageEvent(),GetController(),this)>0)HitTime=.17f;}
}
void AETCharacter::WakeBurrow() {
    if(!Has(TEXT("molt-burrow"))){World->Say(TEXT("Claws ready / learn Burrow in the MOLT skill web [K]"));return;}
    if(ShotTime>0||!GetCharacterMovement()->IsMovingOnGround())return;
    FVector Start=GetActorLocation(),Dir=GetActorForwardVector().GetSafeNormal2D();FVector End=Start+Dir*830;
    const FBox2D Area(FVector2D(Start).ComponentMin(FVector2D(End)),FVector2D(Start).ComponentMax(FVector2D(End)));
    if(World->Geology->DigRestriction(Area)){World->Say(TEXT("This ground is protected. Find open wilderness soil."));return;}
    End.Z=World->SurfaceHeight(End.X,End.Y)+100;FCollisionQueryParams Q(SCENE_QUERY_STAT(WakeBurrow),false,this);
    if(GetWorld()->OverlapBlockingTestByChannel(End,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(35,90),Q)){World->Say(TEXT("The far end of the seam is blocked"));return;}
    SetActorLocation(End);ShotTime=2.5f;EnemyReacquireTime=2.5f;World->Sound(TEXT("JetThrust"),.32f,.6f);World->Say(TEXT("You slip under the soil and emerge along the root seam."));
}
