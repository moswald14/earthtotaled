#include "ETCombat.h"
#include "ETHighwayContactTrial.h"
#include "ETGame.h"
#include "ETAgentMovement.h"
#include "ETDayCycle.h"
#include "ETLandfall.h"
#include "ETSaveDisk.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace {
    struct FAttacker {
        FString Id,Kind;
        bool Front;
    };
    struct FHit {
        FAttacker Attacker;
        float Damage;
    };
    struct FBattleTrace {
        TMap<TWeakObjectPtr<AActor>,FAttacker> Shots;
        TArray<FHit> Hits;
    };
    // Observe only this disposable encounter. A bolt can arrive after its shooter has died.
    FBattleTrace* Trace=nullptr;

    // Find an actual front-line combatant with an unobstructed place for the survivor to stand.
    AETAgent* PickGuard(AETWorld& W,AETCharacter& P,bool Bug,FVector& Stand) {
        TArray<AETAgent*> Guards;
        for(const auto& Entry:W.FrontUnits) {
            auto* A=Entry.Value;
            if(IsValid(A)&&!A->IsHidden()&&A->Health>A->FullHealth*.65f&&(Bug?A->Kind==TEXT("runner")||A->Kind==TEXT("bulwark"):A->Kind==TEXT("robot")))Guards.Add(A);
        }
        Guards.Sort([&](const AETAgent& A,const AETAgent& B){return FVector::DistSquared(A.GetActorLocation(),P.GetActorLocation())<FVector::DistSquared(B.GetActorLocation(),P.GetActorLocation());});
        for(auto* A:Guards) {
            FCollisionQueryParams Q(SCENE_QUERY_STAT(ETContactLane),false,&P);Q.AddIgnoredActor(A);
            for(int I=0;I<16;++I){
                FVector Feet=A->GetActorLocation()+FRotator(0,I*22.5,0).Vector()*1900.;
                if(!ETAgentMovement::FindGroundSpawn(W,Feet)||W.SafeSettlement(Feet))continue;
                Stand=Feet+FVector(0,0,86);
                if(!W.GetWorld()->LineTraceTestByChannel(Stand+FVector(0,0,60),A->AimPoint(),ECC_Visibility,Q)&&
                   !W.GetWorld()->LineTraceTestByChannel(A->GetActorLocation()+FVector(0,0,70),Stand,ECC_Visibility,Q))return A;
            }
        }
        return nullptr;
    }
}

void ETHighwayContactTrial::Shot(AETEnemyBolt& Bolt,const AETAgent& Shooter) {
    if(Trace)Trace->Shots.Add(&Bolt,FAttacker{Shooter.Id,Shooter.Kind,Shooter.FrontUnit});
}
void ETHighwayContactTrial::Hit(AActor* Causer,float AppliedDamage) {
    if(!Trace||!Causer)return;
    if(const auto* A=Cast<AETAgent>(Causer))Trace->Hits.Add(FHit{{A->Id,A->Kind,A->FrontUnit},AppliedDamage});
    else if(const auto* Shooter=Trace->Shots.Find(Causer))Trace->Hits.Add(FHit{*Shooter,AppliedDamage});
}

// Enter through the title's Continue button. Keep the real clock and grace period through level travel.
// Each scenario runs in its own disposable profile with ordinary region streaming, character movement and attacks.
void ETCombat::HighwayContactTrial(AETWorld& W,AETCharacter& P,const FString& Route) {
    static int Stage=0,Shots=0;
    static float Since=0,LastVitals=0,InitialGrace=0,WaitShot=0;
    static FString Log,TargetId;
    static TWeakObjectPtr<AETAgent> Unit;
    static FBattleTrace Observed;
    static bool HitGuard=false,CloseEnough=false;
    const bool Approach=Route.EndsWith(TEXT("close")),Shards=Route.EndsWith(TEXT("shards"));
    const bool Truce=Route.EndsWith(TEXT("truce")),Dark=Route.EndsWith(TEXT("night"));
    auto& S=*W.State;
    const auto File=FPaths::ProjectSavedDir()/TEXT("highway-contact.txt");
    auto Record=[&](const FString& Row){Log+=Row+TEXT("\n");FFileHelper::SaveStringToFile(Log,*File);};
    auto Finish=[&](bool OK,const FString& Message){Trace=nullptr;Record((OK?TEXT("PASS "):TEXT("FAIL "))+Message);Record(OK?TEXT("TOTAL 0 failures"):TEXT("TOTAL 1 failure"));Stage=99;FPlatformMisc::RequestExitWithStatus(false,OK?0:2);};
    if(Stage==99)return;
    if(!Stage){
        if(P.Panel!=TEXT("title"))return;
        if(!W.HadSave){
            const FVector2D Site=ETNav::Site({0,0},0).At(0,-3200);
            S.Position=FVector(Site,W.SurfaceHeight(Site.X,Site.Y)+96);S.Elapsed=60;S.ArrivalStage=2;S.IntroRead=true;
            S.TrenchSides=15;S.Unlocked.AddUnique(ETLandfall::Pistol);S.Weapon=ETLandfall::Pistol;
            if(!ETSaveDisk::Write(&S,W.SaveSlot)){Finish(false,TEXT("Could not prepare the disposable survivor"));return;}
            W.HadSave=true;
        }
        Stage=1;P.Action(TEXT("continue"));return;
    }
    if(Stage==1){
        if(!W.GetWorld()->URL.HasOption(TEXT("ETContinue"))||!P.Panel.IsEmpty())return;
        InitialGrace=P.EnemyReacquireTime;Since=S.Elapsed;
        Record(FString::Printf(TEXT("Continue loaded at %.2f h, position %s, grace %.2f s, shield %.1f, health %.1f"),W.Hour(),*P.GetActorLocation().ToCompactString(),InitialGrace,S.Shield,S.Health));
        if(InitialGrace<=0||P.TestInvincible){Finish(false,TEXT("Expected the restored survivor's normal protection timer"));return;}
        if(S.Hydration<10&&S.Water>0){P.Drink();Record(TEXT("Used one carried water bottle"));}
        Stage=2;return;
    }
    if(!P.Panel.IsEmpty()){Finish(false,TEXT("Encounter interrupted / ")+P.Panel+TEXT(" / ")+P.PanelBody);return;}
    if(Stage==2){
        if(P.EnemyReacquireTime>0)return;
        Record(FString::Printf(TEXT("PASS Spawn protection expired in %.2f seconds"),S.Elapsed-Since));
        // Hold each encounter on its specified side of sunset, including saves made seconds before 19:00.
        S.ClockOffset=ETDay::At(Dark?22.:17.)-S.Elapsed;
        if(Truce)S.Wake.Chapter=ETWake::Triage;
        Stage=3;Since=S.Elapsed;return;
    }
    if(Stage==3){
        FVector Stand;
        auto* A=PickGuard(W,P,Approach,Stand);
        if(!A){
            if(S.Elapsed-Since>20){Finish(false,TEXT("No streamed soldier with a clear approach"));return;}
            if(S.Elapsed-Since>5&&W.FrontUnits.IsEmpty()){
                const FVector2D Site=ETNav::Site(ETNav::District(FVector2D(P.GetActorLocation())),0).At(0,-3200);
                P.SetActorLocation(FVector(Site,W.SurfaceHeight(Site.X,Site.Y)+96));
            }
            return;
        }
        if(A->WakeTruce()!=Truce){Finish(false,TEXT("Soldier allegiance does not match the requested encounter"));return;}
        Unit=A;TargetId=A->Id;
        P.SetActorLocation(Stand,false,nullptr,ETeleportType::TeleportPhysics);
        P.GetCharacterMovement()->StopMovementImmediately();
        if(!Approach&&!Dark){
            if(P.Borer)P.ToggleBorer();
            P.SelectWeapon(ETLandfall::Pistol);P.AimStart();
            if(S.Weapon!=ETLandfall::Pistol){Finish(false,TEXT("This encounter needs a survivor who has received the starter pistol"));return;}
        }
        Record(FString::Printf(TEXT("Streamed %s %s, distance %.1f m, daylight %d, truce %d"),*TargetId,*A->Kind,FVector::Dist2D(Stand,A->GetActorLocation())/100.,!W.IsNight(),A->WakeTruce()));
        Since=S.Elapsed;LastVitals=S.Health+S.Shield+S.Armor;Trace=&Observed;Stage=4;return;
    }
    auto* A=Unit.Get();
    const float Vitals=S.Health+S.Shield+S.Armor;
    if(Truce&&Vitals<LastVitals-.01f){Finish(false,TEXT("The survivor took damage during the daylight truce"));return;}
    for(const FHit& Hit:Observed.Hits){
        const auto& Source=Hit.Attacker;
        const bool FromTarget=Shards?Source.Id.StartsWith(TargetId+TEXT("/shard")):Source.Id==TargetId||(Dark&&Source.Front);
        if(FromTarget){
            if(Truce||(!Approach&&!Dark&&!HitGuard)){Finish(false,TEXT("An unprovoked or allied guard hurt the survivor"));return;}
            Finish(Hit.Damage>0&&Vitals<LastVitals-.01f,FString::Printf(TEXT("%s dealt %.1f damage after %s"),*Source.Kind,Hit.Damage,
                Shards?TEXT("the player's shots split its parent"):Approach?TEXT("an approach without firing"):Dark?TEXT("night contact"):TEXT("the player's pistol hit")));return;
        }
    }
    Observed.Hits.Reset();
    LastVitals=Vitals;
    if(Truce&&HitGuard&&S.Elapsed-WaitShot>=6){Finish(true,TEXT("Shared-story truce protected the survivor after a live shot"));return;}
    if(S.Elapsed-Since>20){Finish(false,FString::Printf(TEXT("%s did not land the expected attack / shots %d, hit %d, close %d, remaining HP %.1f, refuge %d"),*TargetId,Shots,HitGuard,CloseEnough,A?A->Health:0.f,W.AtRefuge));return;}
    if(!A){
        if(!HitGuard&&!Dark)Finish(false,TEXT("Front soldier fell before the encounter finished"));
        return;
    }
    const double Range=FVector::Dist2D(P.GetActorLocation(),A->GetActorLocation());
    const auto Facing=(A->AimPoint()-P.Camera->GetComponentLocation()).Rotation();
    P.GetController()->SetControlRotation(Facing);P.Camera->SetWorldRotation(Facing);
    if(Approach){
        if(Range<520.)CloseEnough=true;
        if(Range>240.)P.AddMovementInput((A->GetActorLocation()-P.GetActorLocation()).GetSafeNormal2D());
        return;
    }
    if(Dark||S.Elapsed-Since<1.2f||(!Shards&&HitGuard)||P.ShotTime>0||P.ReloadTime>0)return;
    if(P.CurrentMagazine()<=0){P.Reload();return;}
    const float Health=A->Health;
    P.Shoot();++Shots;
    if(A->Health<Health){
        if(!HitGuard){
            if(Range<=1200.){Finish(false,TEXT("First shot occurred inside the guard's close defense radius"));return;}
            Record(FString::Printf(TEXT("PASS Equipped pistol hit the selected guard from %.1f m (%.0f to %.0f HP)"),Range/100.,Health,A->Health));
        }
        HitGuard=true;WaitShot=S.Elapsed;
    }
}
