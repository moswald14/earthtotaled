#include "ETWakeVehicle.h"
#include "ETGame.h"
#include "ETLandfall.h"
#include "ETSaveDisk.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

void AETCharacter::JumpPress()
{
    if(Ride()){LiftHeld=true;return;}
    if(!Panel.IsEmpty()||Climbing||JumpHeld) return;
    auto* Move=GetCharacterMovement();
    JumpHeld=true; JumpHeldTime=0; ThrustArmed=false;
    if(Move->IsMovingOnGround()) {Jump(); return;}
    if(!Move->IsFalling()) return;
    ThrustArmed=true;
    if(AirJumpSpent) {JumpHeldTime=.26f; return;}
    AirJumpSpent=true;
    Move->Velocity.Z=Move->JumpZVelocity;
    FallSpeed=0;
    if(World) World->Sound(TEXT("JetThrust"),.23f,1.6f);
}
void AETCharacter::JumpRelease()
{
    LiftHeld=false;JumpHeld=Jetting=ThrustArmed=false; JumpHeldTime=JetSoundTime=0; StopJumping();
}
void AETCharacter::TickJet(float D)
{
    UCharacterMovementComponent* Move=GetCharacterMovement();
    float& Fuel=World->State->JetFuel;
    if(JumpHeld) JumpHeldTime+=D;
    const bool Grounded=Move->IsMovingOnGround()&&Move->CurrentFloor.IsWalkableFloor()&&!Climbing;
    if(Grounded) {
        Fuel=FMath::Min(100.f,Fuel+24.f*D);
        if(Fuel>0) FuelLocked=false;
    }
    Jetting=JumpHeld&&ThrustArmed&&AirJumpSpent&&JumpHeldTime>.25f&&!Climbing&&Move->IsFalling()&&!FuelLocked&&Fuel>0;
    if(Jetting) {
        JetUsed=true;
        Move->Velocity.Z=FMath::Min(770.,Move->Velocity.Z+2450.*D);
        Move->AirControl=.65f;
        FallSpeed=FMath::Min(0.,Move->Velocity.Z);
        Fuel=FMath::Max(0.f,Fuel-12.5f*D);
        FuelLocked=Fuel<=0;
        JetSoundTime-=D;
        if(JetSoundTime<=0) {World->Sound(TEXT("JetThrust"),.46f,FMath::Lerp(.88f,1.08f,Fuel/100)); JetSoundTime=.58f;}
    } else {
        Move->AirControl=.25f;
        if(JetUsed&&Move->IsFalling()&&Move->Velocity.Z<-220) {Move->Velocity.Z=-220; FallSpeed=-220;}
    }
}
float AETCharacter::NextGunRank(int32 Rank)
{
    static constexpr float Thresholds[]={0,150,450,1000,2000,4000};
    return Rank>=1&&Rank<UE_ARRAY_COUNT(Thresholds)?Thresholds[Rank]:0;
}
int32 AETCharacter::GunRank(int32 I) const
{
    int32 Rank=1;
    if(!World->State->WeaponDamage.IsValidIndex(I)) return Rank;
    const float Damage=World->State->WeaponDamage[I];
    while(NextGunRank(Rank)>0&&Damage>=NextGunRank(Rank)) ++Rank;
    return Rank;
}
void AETCharacter::TurnTool()
{
    if(!Panel.IsEmpty()) return;
    if(auto* Vehicle=Ride()){Vehicle->StartEngine();return;}
    if(Building) {BuildFacing=(BuildFacing+1)%4; RefreshBuild();}
    else if(Borer) {
        auto& S=*World->State; FurrowStart.Reset();
        if(S.Wake.Body==ETWake::Bug){ToggleBorer();return;}
        if(S.FurrowHead){S.FurrowHead=false;S.SquareBorer=false;}
        else if(S.SquareBorer){S.SquareBorer=false;S.FurrowHead=true;}
        else S.SquareBorer=true;
        World->Say(S.FurrowHead?TEXT("Irrigation / tap fire to mark inlet, then a downstream endpoint"):S.SquareBorer?TEXT("Square head / level floors and straight walls"):TEXT("Round head / fracture pulse"));
        World->Sound(TEXT("Switch"),.35f); World->Save();
    } else Reload();
}
void AETCharacter::Title()
{
    TestInvincible=false;
    OpenPanel(TEXT("title"),TEXT("EARTHtotaled"),TEXT("The Last Signal"));
    if(!World->Review&&!World->FrontierQA&&!World->SlotQA) CheckVersions(true);
}
void AETCharacter::ContinueSurvivor()
{
    // Level travel is deferred. Retire the old run before its autosave can fire.
    Panel=TEXT("travel");
    UGameplayStatics::SetGamePaused(this,false);
    TravelSlot(World->ActiveSlot,true);
}
void AETCharacter::StartFresh()
{
    if(World->Review&&World->JourneyEnding.IsEmpty()) {ContinueSurvivor(); return;}
    auto* Fresh=Cast<UETSave>(UGameplayStatics::CreateSaveGameObject(UETSave::StaticClass()));
    Fresh->RetroDisplay=false; Fresh->ModernDay=true; Fresh->ClockOffset=ETDay::At(9); Fresh->Seed=World->FrontierQA?84371:FMath::RandRange(1,MAX_int32-1);
    Fresh->ArrivalStage=World->Review?-1:0; Fresh->LandscapeEdition=1; Fresh->BuildStock=0;
    Fresh->RetroDisplay=World->State->RetroDisplay; Fresh->RenderScale=World->State->RenderScale; Fresh->HasRingDisplay=true;
    Fresh->ViewDistance=World->State->ViewDistance; Fresh->MusicVolume=World->State->MusicVolume; Fresh->MusicMuted=World->State->MusicMuted; Fresh->JazzSource=World->State->JazzSource; Fresh->JazzTrack=World->State->JazzTrack; Fresh->WeatherVolume=World->State->WeatherVolume;
    Fresh->AlertVolume=World->State->AlertVolume; Fresh->AlertCaptions=World->State->AlertCaptions; Fresh->HitFlashIntensity=World->State->HitFlashIntensity;
    if(!ETSaveDisk::Write(Fresh,World->SaveSlot,true)) {PanelBody=TEXT("Could not start the new save. Check free disk space and try again."); return;}
    ContinueSurvivor();
}
