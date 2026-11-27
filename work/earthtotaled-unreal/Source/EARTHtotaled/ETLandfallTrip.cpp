#include "ETLandfall.h"
#include "ETGame.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "UnrealClient.h"
#include "Misc/FileHelper.h"
#include "Kismet/GameplayStatics.h"

void ETLandfall::DriveTest(AETWorld& W,AETCharacter& P)
{
    static int Stage=0,Shots=0;static double Last=0,Start=FPlatformTime::Seconds();static bool Voiced=false;
    const double Now=FPlatformTime::Seconds();
    const auto Saved=FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir());
    auto End=[&](bool Good,const FString& Reason){FFileHelper::SaveStringToFile(Reason,*(Saved/TEXT("landfall-trip.txt")));FPlatformMisc::RequestExitWithStatus(true,Good?0:1);Stage=99;};
    if(Stage==99) return;
    if(Now-Start>105) {End(false,FString::Printf(TEXT("Timed out stage %d shots %d mask %d / %s"),Stage,Shots,W.State->TrenchSides,*W.Notice));return;}
    if(Stage==0&&P.Panel==TEXT("title")) {
        if(W.HadSave) {End(false,TEXT("Opening validation requires an empty test slot."));return;}
        P.Action(TEXT("new"));Stage=1;return;
    }
    if(P.Panel==TEXT("arrival")) {
        if(Stage==1&&P.FlightSeconds>8.4f){FScreenshotRequest::RequestScreenshot(Saved/TEXT("descent.png"),true,false);Stage=2;}
        if(Stage==2&&P.FlightSeconds>15.5f){Voiced=P.FlightAudio!=nullptr;FScreenshotRequest::RequestScreenshot(Saved/TEXT("crash.png"),true,false);Stage=3;}
        return;
    }
    if(Stage>=3&&Stage<=6&&P.Panel.IsEmpty()&&Now-Last>.9) {
        Last=Now;int Side=NextBank(W.State->TrenchSides);
        if(Side==INDEX_NONE){End(false,TEXT("Completed before the expected bank."));return;}
        if(!P.Borer) P.ToggleBorer();
        const auto XY=Stake(Side);const auto Out=(XY-FVector2D(0,-480)).GetSafeNormal();
        const FVector2D Stand=XY+Out*560;
        P.SetActorLocation(FVector(Stand,W.SurfaceHeight(Stand.X,Stand.Y)+100));
        const FVector Aim(XY,W.SurfaceHeight(XY.X,XY.Y)+179);
        P.GetController()->SetControlRotation((Aim-P.Camera->GetComponentLocation()).Rotation());P.Camera->SetWorldRotation(P.GetControlRotation());
        P.FireBorer();++Shots;
        if(W.State->TrenchSides&(1<<Side))++Stage;
        if(Shots>12){End(false,FString::Printf(TEXT("Borer missed %s stake / %s"),Bank(Side),*W.Notice));return;}
        if(Stage==7) {P.OpenPanel(TEXT("pause"),TEXT("Pause"),TEXT(""));P.Action(TEXT("main-menu"));P.Action(TEXT("continue"));return;}
    }
    if(Stage==7&&P.Panel.IsEmpty()&&W.HadSave&&W.State->ArrivalStage==2) {
        bool Capture=GEngine&&GEngine->GameViewport&&GEngine->GameViewport->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
        if(!Voiced||!Capture||W.State->Weapon!=Pistol||W.State->TrenchSides!=15){End(false,TEXT("Opening voice, restored input or quest handoff failed."));return;}
        P.Pause();P.Action(TEXT("updates"));Stage=8;return;
    }
    if(Stage==8&&P.Panel==TEXT("updates")&&!P.CheckingVersions) {
        if(!P.ReleaseTags.Contains(TEXT("v1.12.0"))) {End(false,P.UpdateNote);return;}
        const auto Old=P.ReleaseTags.Last();
        while(P.ReleaseIndex+1<P.ReleaseTags.Num()) P.Action(TEXT("next"));
        P.Action(TEXT("choose"));const bool Confirm=P.ConfirmVersion&&P.ReleaseTags[P.ReleaseIndex]==Old;
        P.Action(TEXT("later"));
        End(Confirm&&P.Panel==TEXT("pause"),FString::Printf(TEXT("Crash and recorded voice played. Four actual borer hits in %d attempts. Pistol and full trench restored with mouse look. GitHub offered %d versions; choosing %s and declining returned to pause."),Shots,P.ReleaseTags.Num(),*Old));
    }
}
