#include "ETCombat.h"
#include "ETGame.h"
#include "ETDayCycle.h"
#include "ETWake.h"
#include "ETWakeVehicle.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundWave.h"
#include "UnrealClient.h"

namespace {
    void OpenField(AETWorld& W,AETCharacter& P) {
        P.Panel.Empty();
        UGameplayStatics::SetGamePaused(&W,false);
        P.SetMouseMode(false);
        W.State->ArrivalStage=2;
        W.State->ClockOffset=ETDay::At(20.1)-W.State->Elapsed;
        W.AtRefuge=false;
        P.EnemyReacquireTime=0;
    }

    AETAgent* Spawn(AETWorld& W,const FVector& At,const FString& Kind) {
        auto* A=W.GetWorld()->SpawnActor<AETAgent>(At,FRotator(0,180,0));
        if(A) A->Configure(&W,FGuid::NewGuid().ToString(),Kind);
        return A;
    }

    void CheckCombat(AETWorld& W,AETCharacter& P) {
        int Fail=0;FString Report;
        auto Check=[&](bool OK,const FString& Label) {
            if(!OK)++Fail;
            Report+=(OK?TEXT("PASS "):TEXT("FAIL "))+Label+TEXT("\n");
        };
        OpenField(W,P);
        auto& S=*W.State;
        Check(S.AlertVolume==75&&S.HitFlashIntensity==75&&S.AlertCaptions&&S.Schema==1,TEXT("Defaults load on a fresh survivor"));
        const FVector Position=P.GetActorLocation()+FVector(360,120,0);
        TSet<USoundWave*> Assets;
        for(const auto& Voice:ETCombat::Voices) {
            auto* A=Spawn(W,Position,Voice.Kind);
            if(!A){Check(false,Voice.Name);continue;}
            auto* Wave=Cast<USoundWave>(W.Sounds.FindRef(Voice.Cue));
            Check(Wave&&!Assets.Contains(Wave)&&Wave->NumChannels==1&&!Wave->IsLooping()&&Wave->GetDuration()>.18f,
                FString(Voice.Name)+TEXT(" has its own cooked mono one-shot"));
            Assets.Add(Wave);
            P.AlertCaptionTime=0;
            auto* Audio=A->UpdateAlert(true,true,.04f);
            Check(Audio&&Audio->IsPlaying()&&Audio->Sound==Wave&&Audio->bAllowSpatialization&&
                Audio->AttenuationSettings==W.AlertAttenuation&&Audio->GetComponentLocation().Equals(A->AimPoint(),1.0)&&
                FMath::IsNearlyEqual(Audio->VolumeMultiplier,.75f)&&P.AlertCaption==Voice.Caption,
                FString(Voice.Name)+TEXT(" plays spatially at the source with the correct caption and independent volume"));
            Check(!A->UpdateAlert(true,false,.04f)&&!A->UpdateAlert(false,false,.5f)&&
                !A->UpdateAlert(true,true,.04f),FString(Voice.Name)+TEXT(" stays quiet through pursuit, target swap and brief cover"));
            A->UpdateAlert(false,false,2);
            auto* Again=A->UpdateAlert(true,true,.04f);
            Check(Again!=nullptr,FString(Voice.Name)+TEXT(" sounds again after disengaging"));
            if(Audio)Audio->Stop(); if(Again)Again->Stop();
            A->Managed=true; A->AttackTime=8; A->RivalScan=5;
            A->UpdateAlert(false,false,2);P.AlertCaptionTime=0;
            A->Tick(.04f);
            Check(A->AlertHold>0&&P.AlertCaptionTime>0&&P.AlertCaption==Voice.Caption,
                FString(Voice.Name)+TEXT(" enters alert through its live AI decision"));
            P.AlertCaptionTime=0;A->Tick(.04f);
            Check(P.AlertCaptionTime==0,FString(Voice.Name)+TEXT(" stays quiet on its next AI tick"));
            A->Destroy();
        }
        Check(W.AlertAttenuation&&W.AlertAttenuation->Attenuation.bSpatialize&&W.AlertAttenuation->Attenuation.bAttenuate,TEXT("Alerts use positional attenuation"));
        auto* Bear=Spawn(W,Position,TEXT("bear"));auto* Wolf=Spawn(W,Position+FVector(0,210,0),TEXT("wolf"));
        auto* Deer=Spawn(W,Position,TEXT("deer"));
        if(Bear&&Wolf&&Deer) {
            Bear->Managed=true;Bear->AttackTime=8;
            P.AlertCaptionTime=0;Bear->Tick(.04f);
            Check(Bear->AlertHold>0&&P.AlertCaption==TEXT("Bear roars"),TEXT("The live bear pursuit enters alert without waiting for its swipe"));
            P.AlertCaptionTime=0;Bear->Tick(.04f);
            Check(P.AlertCaptionTime==0,TEXT("A second AI tick does not reannounce the bear"));
            Deer->Tick(.04f);Check(Deer->AlertHold==0,TEXT("Fleeing deer never raise enemy alerts"));
            Bear->TakeDamage(35,FDamageEvent(),nullptr,&P);
            P.Sighted=Deer;
            Check(P.CombatTarget()==Bear&&Bear->Health==325&&Bear->FullHealth==360,TEXT("Fighting memory retains the wounded bear while sight passes over a deer"));
            P.Sighted=Wolf;Check(P.CombatTarget()==Wolf,TEXT("Sight takes priority over the previous combatant"));
            P.Sighted.Reset();P.CombatTime=0;Check(!P.CombatTarget(),TEXT("An expired encounter clears its health bar"));
            auto* Returned=W.GetWorld()->SpawnActor<AETAgent>(Position,FRotator::ZeroRotator);
            if(Returned){S.AgentHealth.Add(TEXT("combat-check-wounded"),87.f);Returned->Configure(&W,TEXT("combat-check-wounded"),TEXT("bear"));}
            Check(Returned&&Returned->Health==87&&Returned->FullHealth==360,TEXT("A wounded returning bear retains the correct maximum"));
            S.Shield=28;S.Health=81;S.Armor=0;P.TestInvincible=false;
            P.TakeDamage(6,FDamageEvent(),nullptr,Wolf);
            Check(S.Shield==22&&S.Health==81&&!P.HealthWasHit&&P.DamageFlash>0&&P.CombatTarget()==Wolf&&
                FVector::DotProduct(P.HitBearing,(Wolf->GetActorLocation()-P.GetActorLocation()).GetSafeNormal2D())>.99,TEXT("Shield-only impact tracks its actual attacker and source bearing"));
            P.TakeDamage(30,FDamageEvent(),nullptr,Bear);
            Check(S.Shield==0&&S.Health==73&&P.HealthWasHit&&P.ShieldBreakTime>0,TEXT("Shield overflow produces a break and a health hit"));
            P.Hurt(51,TEXT("test hazard"));
            Check(S.Health==22&&P.HitBearing.IsNearlyZero(),TEXT("Low-health environmental damage has no fabricated direction"));
            P.RememberEnemy(Wolf);Wolf->Destroy();Check(!P.CombatTarget(),TEXT("Destroyed opponents clear immediately"));
            P.Sighted=Deer;Check(!P.CombatTarget(),TEXT("Neutral wildlife never receives an enemy bar"));
            S.AlertVolume=0;S.AlertCaptions=true;P.AlertCaptionTime=0;
            Check(!W.EnemyAlert(*Bear,false)&&P.AlertCaptionTime>0&&!P.CaptionThreat,TEXT("Muted alerts retain optional captions for nearby faction combat"));
            P.AlertCaptionTime=0;S.AlertCaptions=false;
            W.EnemyAlert(*Bear,true);Check(P.AlertCaptionTime==0,TEXT("Disabled captions stay quiet"));
            if(Returned)Returned->Destroy();Deer->Destroy();Bear->Destroy();
        } else Check(false,TEXT("Encounter actors available"));
        auto* Warden=Spawn(W,Position+FVector(2200,0,0),TEXT("robot"));
        auto* Runner=Spawn(W,Position+FVector(2420,0,0),TEXT("runner"));
        if(Warden&&Runner){
            P.Sighted.Reset();P.CombatTime=0;P.AlertCaptionTime=0;S.AlertCaptions=true;
            Warden->Rival=Runner;Warden->RivalScan=5;Warden->AttackTime=8;
            Warden->Tick(.04f);
            Check(Warden->AlertHold>0&&P.AlertCaptionTime>0&&!P.CaptionThreat&&!P.CombatTarget(),
                TEXT("Nearby faction combat makes a call without marking the survivor as its opponent"));
            S.Wake.Retuned=15;P.Sighted=Warden;Warden->Rival.Reset();Warden->RivalScan=5;
            P.AlertCaptionTime=0;Warden->Tick(.04f);
            Check(!P.CombatTarget()&&Warden->AlertHold==0&&P.AlertCaptionTime==0,
                TEXT("A friendly faction contact cannot target the player or occupy the enemy bar"));
            S.Wake.Retuned=0;S.AlertCaptions=false;Warden->Destroy();Runner->Destroy();
        }else Check(false,TEXT("Faction encounter actors available"));
        Check(FString(ETCombat::Direction(ETCombat::Bearing(FVector(1,0,0),90)))==TEXT("left")&&
            FString(ETCombat::Direction(ETCombat::Bearing(FVector(-1,0,0),0)))==TEXT("behind"),TEXT("Left and rear bearings rotate with the camera"));
        P.Pause();P.Action(TEXT("alert-volume"));P.Action(TEXT("alert-captions"));
        S.HitFlashIntensity=100;P.Action(TEXT("hit-intensity"));
        Check(S.AlertVolume==25&&S.AlertCaptions&&S.HitFlashIntensity==0,TEXT("Pause controls independently adjust the three combat preferences"));
        TArray<uint8> Bytes;UGameplayStatics::SaveGameToMemory(&S,Bytes);
        auto* Loaded=Cast<UETSave>(UGameplayStatics::LoadGameFromMemory(Bytes));
        Check(Loaded&&Loaded->AlertVolume==25&&Loaded->AlertCaptions&&Loaded->HitFlashIntensity==0,TEXT("The Unreal save format restores combat preferences"));
        Report+=FString::Printf(TEXT("TOTAL %d failures"),Fail);
        FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("combat-checks.txt")));
        FPlatformMisc::RequestExitWithStatus(true,Fail?2:0);
    }

    void Capture(AETWorld& W,AETCharacter& P) {
        static int Scene=0;static double Start=0;static bool Requested=false;
        static TWeakObjectPtr<AETAgent> Subject;
        const TCHAR* Names[]={TEXT("shield-hit"),TEXT("low-hp"),TEXT("roadster"),TEXT("survey-plane"),TEXT("feedback-settings")};
        const auto File=FPaths::ProjectSavedDir()/(FString(Names[Scene])+TEXT(".png"));
        const double Now=FPlatformTime::Seconds();
        if(!Start) {
            OpenField(W,P);P.TestInvincible=true;auto& S=*W.State;S.Wake.Chapter=ETWake::Flight;W.RefreshWake();
            W.WeatherOverride=ETClimate::Clear;S.ClockOffset=ETDay::At(10.5)-S.Elapsed;
            if(Scene==0) {
                const FVector At=W.WakeSpot(TEXT("garage"));
                W.StreamAround(At,true);
                const FVector Here=At+FVector(400,-1350,0);
                P.GetCharacterMovement()->DisableMovement();
                P.SetActorLocation(FVector(Here.X,Here.Y,W.SurfaceHeight(Here.X,Here.Y)+100));
                const FVector There=P.GetActorLocation()+FVector(720,150,-38);
                auto* Bear=Spawn(W,There,TEXT("bear"));
                if(!Bear){FPlatformMisc::RequestExitWithStatus(true,2);return;}
                Bear->FrontUnit=true;Bear->SetActorHiddenInGame(false);Bear->SetActorTickEnabled(false);
                Bear->SetActorRotation(FRotator(0,-134,0));Bear->Health=275;Subject=Bear;
                P.Controller->SetControlRotation((Bear->AimPoint()-P.Camera->GetComponentLocation()).Rotation());
            }
            if(Scene==2||Scene==3) {
                auto* Vehicle=Scene==2?W.WakeCar:W.WakePlane;
                if(!Vehicle){FPlatformMisc::RequestExitWithStatus(true,2);return;}
                P.EnterRide(Vehicle,true);P.Controller->SetControlRotation(FRotator(-3,Vehicle->State().Heading,0));
                Vehicle->StartEngine();P.Forward(1);P.LiftHeld=Scene==3;
                for(int Step=0;Step<(Scene==3?180:45);++Step) Vehicle->Drive(P,.025f);
                if(!Vehicle->State().Started||Vehicle->State().Speed<100||(Scene==3&&!Vehicle->State().Flying)){
                    FPlatformMisc::RequestExitWithStatus(true,2);return;
                }
                S.Shield=S.MaxShield();S.Health=P.MaxHealth();
            }
            if(Scene==4)P.Pause();
            Start=Now;Requested=false;
        }
        if((Scene==2||Scene==3)&&P.Ride()) {
            P.Forward(.65f);P.LiftHeld=Scene==3;
            P.Ride()->Drive(P,.015f);
        }
        if(Scene<2&&Subject.IsValid()){
            auto* Bear=Subject.Get();Bear->SetActorTickEnabled(false);Bear->SetActorHiddenInGame(false);
            P.RememberEnemy(Bear);P.AlertCaption=TEXT("Bear roars");P.AlertCaptionTime=1;P.CaptionOrigin=Bear->GetActorLocation();
            P.World->State->ShieldDelay=10;P.World->State->Shield=Scene?0:31;P.World->State->Health=Scene?22:90;
            P.DamageFlash=.30f;P.HealthWasHit=Scene!=0;P.ShieldBreakTime=Scene?.8f:0;
            const auto Rot=FRotator(0,P.GetControlRotation().Yaw,0);
            P.HitBearing=Rot.RotateVector(Scene?FVector(1,0,0):FVector(0,1,0));
        }
        if(!Requested&&Now-Start>4.5){FScreenshotRequest::RequestScreenshot(File,true,false);Requested=true;return;}
        if(Requested&&(IFileManager::Get().FileSize(*File)>2048||Now-Start>22)){
            if(IFileManager::Get().FileSize(*File)<=2048){FPlatformMisc::RequestExitWithStatus(true,2);return;}
            if(++Scene==5){FPlatformMisc::RequestExitWithStatus(true,0);return;}
            if(Scene==2&&Subject.IsValid()){Subject->Destroy();P.Sighted.Reset();P.RecentEnemy.Reset();P.DamageFlash=0;P.ShieldBreakTime=0;P.AlertCaptionTime=0;}
            Start=0;
        }
    }
}

void ETCombat::Trial(AETWorld& World,AETCharacter& Player,bool Visual)
{
    if(Visual)Capture(World,Player);
    else {static bool Done=false;if(!Done){Done=true;CheckCombat(World,Player);}}
}
