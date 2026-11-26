#include "ETGame.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

namespace {
    FVector Incoming(const FDamageEvent& Event,const AActor* Source,const AActor& Target)
    {
        if(Event.IsOfType(FPointDamageEvent::ClassID)) return static_cast<const FPointDamageEvent&>(Event).ShotDirection.GetSafeNormal();
        if(const auto* Bolt=Cast<AETEnemyBolt>(Source)) return Bolt->Velocity.GetSafeNormal();
        return Source?(Target.GetActorLocation()-Source->GetActorLocation()).GetSafeNormal():-Target.GetActorForwardVector();
    }
    FVector2D Lean(const FVector& WorldDirection,const FRotator& Facing)
    {
        const auto Local=Facing.UnrotateVector(WorldDirection);
        return FVector2D(Local.X,Local.Y);
    }
}

void AETWorld::ImpactSound(FVector Position,bool Metal,bool FirstPerson)
{
    const float Pitch=FirstPerson?.85f:Metal?1.36f:1.f;
    if(FirstPerson) {Sound(TEXT("Hit"),.66f,Pitch);return;}
    if(!ImpactAttenuation) {
        ImpactAttenuation=NewObject<USoundAttenuation>(this);
        auto& A=ImpactAttenuation->Attenuation; A.bSpatialize=true;
        A.AttenuationShapeExtents=FVector(350); A.FalloffDistance=5100;
    }
    if(auto* Cue=Sounds.FindRef(TEXT("Hit"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,Position,.85f,Pitch,0,ImpactAttenuation);
    if(Metal) if(auto* Ring=Sounds.FindRef(TEXT("FieldLatch"))) UGameplayStatics::PlaySoundAtLocation(this,Ring,Position,.25f,1.6f,0,ImpactAttenuation);
}

void AETAgent::ReceiveImpact(float Damage,const FDamageEvent& Event,AActor* Source)
{
    const auto Direction=Incoming(Event,Source,*this);
    FlinchLean=Lean(Direction,GetActorRotation());
    FlinchTime=.21f;
    const double Weight=Kind==TEXT("hive")?0.:Kind==TEXT("bulwark")?.35:IsMachine()?.65:1.;
    FlinchVelocity=(FlinchVelocity+Direction.GetSafeNormal2D()*FMath::Clamp(5.f*Damage,60.f,230.f)*Weight).GetClampedToMaxSize(235.);
    if(ImpactCooldown>0&&Health>0) return;
    ImpactCooldown=.105f;
    FVector At=AimPoint();
    if(Event.IsOfType(FPointDamageEvent::ClassID)) At=static_cast<const FPointDamageEvent&>(Event).HitInfo.ImpactPoint;
    World->ImpactSound(At,IsMachine(),false);
    if(auto* FX=GetWorld()->SpawnActor<AETBoreBurst>(At,FRotator::ZeroRotator))
        FX->Ignite(World,-Direction,false,IsMachine()?TEXT("CircuitLight"):TEXT("ResinLight"));
}

void AETAgent::TickImpact(float D)
{
    ImpactCooldown=FMath::Max(0.f,ImpactCooldown-D);
    if(FlinchTime<=0) return;
    FlinchTime=FMath::Max(0.f,FlinchTime-D);
    const float T=FlinchTime/.21f;
    if(Parts.Num()) Parts[0]->SetRelativeRotation(FRotator(-11.f*FlinchLean.X*T,0,12.f*FlinchLean.Y*T));
    const double Decay=FMath::Exp(-12.*D);
    FVector Step=FlinchVelocity*((1.-Decay)/12.);
    FlinchVelocity*=Decay;
    if(Step.IsNearlyZero(.08)) return;
    const auto From=GetActorLocation(); const auto Hip=From+FVector(0,0,45);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETImpactRecoil),false,this); FHitResult Hit;
    if(GetWorld()->SweepSingleByChannel(Hit,Hip,Hip+Step,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(24),Q))
        Step*=FMath::Max(0.f,Hit.Time-.03f);
    FVector Next=From+Step;
    if(Kind!=TEXT("drone")&&Kind!=TEXT("skimmer")) {
        if(!GetWorld()->LineTraceSingleByChannel(Hit,Next+FVector(0,0,50),Next-FVector(0,0,65),ECC_Visibility,Q)||
           Hit.ImpactNormal.Z<.58||Cast<AETAgent>(Hit.GetActor())||Cast<AETCharacter>(Hit.GetActor())) return;
        Next.Z=Hit.ImpactPoint.Z+8;
    }
    SetActorLocation(Next);
}

void AETCharacter::ReceiveImpact(float Damage,const FDamageEvent& Event,AActor* Source)
{
    const auto Direction=Incoming(Event,Source,*this);
    SuitLean=Lean(Direction,GetControlRotation()); SuitFlinch=.24f;
    if(SuitImpactCooldown>0) return;
    World->ImpactSound(GetActorLocation(),false,true); SuitImpactCooldown=.105f;
    if(!Climbing&&World->State->Health>0) GetCharacterMovement()->AddImpulse(Direction.GetSafeNormal2D()*FMath::Clamp(Damage*2.f,16.f,72.f),true);
}

void AETCharacter::TickImpact(float D)
{
    SuitImpactCooldown=FMath::Max(0.f,SuitImpactCooldown-D);
    if(SuitFlinch<=0) return;
    SuitFlinch=FMath::Max(0.f,SuitFlinch-D);
    const float T=SuitFlinch/.24f;
    Gun->SetRelativeRotation(FRotator(-5.f*T,1.5f*SuitLean.Y*T,5.f*SuitLean.Y*T));
    Gun->SetRelativeLocation(FVector(78-3*T,19+SuitLean.Y*1.5f*T,-22-T));
}
