#include "ETCombat.h"
#include "ETGame.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundBase.h"

const ETCombat::FVoice* ETCombat::Voice(const FString& Kind)
{
    for(const auto& Entry : Voices) if(Kind==Entry.Kind) return &Entry;
    return nullptr;
}

FVector2D ETCombat::Bearing(FVector TowardSource,float ViewYaw)
{
    const FVector Local=FRotator(0,ViewYaw,0).UnrotateVector(TowardSource.GetSafeNormal2D());
    return FVector2D(Local.X,Local.Y);
}

const TCHAR* ETCombat::Direction(FVector2D Bearing)
{
    if(Bearing.IsNearlyZero()) return TEXT("nearby");
    if(FMath::Abs(Bearing.X)>=FMath::Abs(Bearing.Y)) return Bearing.X>=0?TEXT("ahead"):TEXT("behind");
    return Bearing.Y>=0?TEXT("right"):TEXT("left");
}

UAudioComponent* AETAgent::UpdateAlert(bool Engaging,bool AgainstPlayer,float Delta)
{
    if(!Engaging) {
        AlertHold=FMath::Max(0.f,AlertHold-Delta);
        return nullptr;
    }
    const bool First=AlertHold<=0;
    AlertHold=1.65f;
    return First?World->EnemyAlert(*this,AgainstPlayer):nullptr;
}

UAudioComponent* AETWorld::EnemyAlert(AETAgent& Agent,bool AgainstPlayer)
{
    const auto* Voice=ETCombat::Voice(Agent.Kind);
    if(!Voice) return nullptr;
    auto* P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    const FVector At=Agent.AimPoint();
    constexpr float Near=340.f, Falloff=9500.f;
    if(!P||FVector::DistSquared(P->GetActorLocation(),At)>=FMath::Square(Near+Falloff)) return nullptr;
    if(!AlertAttenuation) {
        AlertAttenuation=NewObject<USoundAttenuation>(this);
        auto& Settings=AlertAttenuation->Attenuation;
        Settings.bSpatialize=true;
        Settings.AttenuationShapeExtents=FVector(Near);
        Settings.FalloffDistance=Falloff;
    }
    if(AgainstPlayer&&!P->CombatTarget()) P->RememberEnemy(&Agent);
    UAudioComponent* Playing=nullptr;
    if(State->AlertVolume>0)
        if(auto* Cue=Sounds.FindRef(Voice->Cue))
            Playing=UGameplayStatics::SpawnSoundAtLocation(this,Cue,At,FRotator::ZeroRotator,FMath::Clamp(State->AlertVolume,0,100)*.01f,1.f,0,AlertAttenuation);
    // Keep a readable line when several units acquire targets in the same frame.
    if(State->AlertCaptions&&(P->AlertCaptionTime<=0||(AgainstPlayer&&!P->CaptionThreat)||
       (AgainstPlayer==P->CaptionThreat&&P->AlertCaptionTime<.55f))) {
        P->AlertCaption=Voice->Caption;
        P->CaptionOrigin=At;
        P->CaptionThreat=AgainstPlayer;
        P->AlertCaptionTime=1.85f;
    }
    return Playing;
}

namespace {
    bool ValidOpponent(const AETAgent* Agent)
    {
        return IsValid(Agent)&&Agent->Health>0&&!Agent->IsHidden()&&Agent->WakeHerd<0&&
            Agent->Awake()&&!Agent->WakeTruce()&&ETCombat::Voice(Agent->Kind);
    }
}

void AETCharacter::RememberEnemy(AETAgent* Enemy)
{
    if(!ValidOpponent(Enemy)) return;
    RecentEnemy=Enemy;
    CombatTime=4.f;
}

AETAgent* AETCharacter::CombatTarget() const
{
    if(auto* Target=Sighted.Get();ValidOpponent(Target)) return Target;
    if(auto* Target=RecentEnemy.Get();CombatTime>0&&ValidOpponent(Target)) return Target;
    return nullptr;
}
