#include "ETRingLine.h"
#include "ETGame.h"
#include "HAL/FileManager.h"
#include "Components/BoxComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/GameViewportClient.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

// Called only in an explicitly isolated packaged test run. The entire lap uses normal based movement.
void AETRingLine::TestRide(AETCharacter& P)
{
    static int Stage=0,Stops=0,Loaded=0;
    static double Start=0,StageStart=0,Peak=0,Drift=0;
    static FVector Seat;
    static bool Airborne=false,Saved=false,JumpedAtSpeed=false,StationPhoto=false;
    static FString Report;
    const double Now=FPlatformTime::Seconds();
    const auto Folder=FPaths::ProjectSavedDir();
    auto Log=[&](const FString& S) {
        Report+=S+TEXT("\n");
        FFileHelper::SaveStringToFile(Report,*(Folder/TEXT("ring-ride.txt")));
    };
    auto Finish=[&](bool OK,const FString& S) {
        Log((OK?FString(TEXT("PASS ")):FString(TEXT("FAIL ")))+S);
        FPlatformMisc::RequestExitWithStatus(true,OK?0:1);
    };
    auto Next=[&](int N){Stage=N;StageStart=Now;};
    if(!Start) {Start=StageStart=Now;}
    const bool Resume=FParse::Param(FCommandLine::Get(),TEXT("ETRingReturn"));
    if(P.Panel==TEXT("title")) {
        if(Resume!=Zone->HadSave) {Finish(false,TEXT("Unexpected survivor slot at the title screen"));return;}
        P.Action(Resume?TEXT("continue"):TEXT("new"));
        return;
    }
    if(Resume) {
        static double MountedAt=-1;
        static FVector From;
        if(!P.Panel.IsEmpty()) {Finish(false,TEXT("Continue did not return to gameplay"));return;}
        if(MountedAt<0) {MountedAt=Now;From=P.GetActorLocation();return;}
        if(Now-MountedAt>3.) {
            const double Offset=FVector::Dist(P.GetActorLocation(),SavedPosition());
            const bool Resumed=Zone->HadSave&&Zone->State->OnRing&&Riding(P)&&Phase.Speed>0&&Offset<650&&FVector::Dist2D(From,P.GetActorLocation())>7000;
            Finish(Resumed,Resumed?FString(TEXT("Saved survivor resumed on the moving freight deck")):
                FString::Printf(TEXT("Resume: save %d, passenger %d, floor %s, speed %.1f, offset %.1f cm"),
                    Zone->HadSave,Zone->State->OnRing,*GetNameSafe(P.GetMovementBaseObject()),Phase.Speed,Offset));
        }
        return;
    }
    if(Now-Start>600||(Stage!=3&&Now-StageStart>16)) {
        Finish(false,FString::Printf(TEXT("Stage %d timed out, player %s, floor %s, car speed %.1f"),Stage,*P.GetActorLocation().ToString(),*GetNameSafe(P.GetMovementBaseObject()),Phase.Speed));
        return;
    }
    const FTransform Home=Platform(0);
    const FVector Across=Home.GetRotation().GetAxisY();
    const auto L=Home.InverseTransformPosition(P.GetActorLocation());
    if(Stage==0) {
        if(P.Panel==TEXT("arrival")) P.FinishArrival();
        if(!P.Panel.IsEmpty()) P.ClosePanel();
        P.TestInvincible=true;
        Zone->WeatherOverride=ETClimate::Clear;
        auto StartPoint=Home.TransformPosition(FVector(0,615,-3600))+FVector(0,0,104);
        P.SetActorLocation(StartPoint,false,nullptr,ETeleportType::TeleportPhysics);
        Zone->State->RingClock=ETRing::Schedule().Arrival[1]; PositionCar(Zone->State->RingClock);
        P.Interact();
        if(Home.InverseTransformPosition(P.GetActorLocation()).Z<0) {
            Finish(false,FString::Printf(TEXT("Street lift missed the platform, panel %s, local position %s"),
                *P.Panel,*Home.InverseTransformPosition(P.GetActorLocation()).ToString()));return;
        }
        P.Interact();
        if(!Phase.Docked||Phase.Stop!=0) {Finish(false,TEXT("Station failed to call the train"));return;}
        P.GetController()->SetControlRotation(Home.GetRotation().RotateVector(FVector(550,-680,-130)).Rotation());
        Log(TEXT("Street lift and station wait reached the open train at Shelter 09"));Next(1);return;
    }
    if(Stage==1) {
        if(Now-StageStart<1.4) return;
        if(!StationPhoto) {FScreenshotRequest::RequestScreenshot(Folder/TEXT("station.png"),true,false);StationPhoto=true;}
        if(!P.GetCharacterMovement()->IsMovingOnGround()) return;
        P.AddMovementInput(-Across);
        if(L.Y<490) {P.JumpPress();Next(2);}
        return;
    }
    if(Stage==2) {
        Airborne|=P.GetCharacterMovement()->IsFalling();
        if(Now-StageStart>.24) P.JumpRelease();
        if(L.Y>65) P.AddMovementInput(-Across);
        else if(Riding(P)) {
            if(!Airborne){Finish(false,TEXT("Boarding never left station floor"));return;}
            P.GetCharacterMovement()->StopMovementImmediately();
            Seat=Deck->GetComponentTransform().InverseTransformPosition(P.GetActorLocation());
            P.GetController()->SetControlRotation(FRotator(-12,Deck->GetComponentRotation().Yaw,0));
            FScreenshotRequest::RequestScreenshot(Folder/TEXT("boarded.png"),true,false);
            Log(TEXT("Jumped from the station and landed on the deck"));Next(3);
        }
        return;
    }
    if(Stage==3) {
        if(!Riding(P)) {Finish(false,FString::Printf(TEXT("Lost deck at %.1f km/h, clock %.2f, offset %s"),Phase.Speed*.036,Zone->State->RingClock,*Deck->GetComponentTransform().InverseTransformPosition(P.GetActorLocation()).ToString()));return;}
        const auto Local=Deck->GetComponentTransform().InverseTransformPosition(P.GetActorLocation());
        Drift=FMath::Max(Drift,FVector::Dist2D(Seat,Local));
        Peak=FMath::Max(Peak,Phase.Speed);
        if(Phase.Speed>ETRing::Cruise*.99&&!Saved) {
            // Keep the real checkpoint before the rest of the lap produces later autosaves.
            const auto Slot=Folder/TEXT("SaveGames")/(Zone->SaveSlot+TEXT(".sav"));
            Saved=Zone->Save()&&Zone->State->OnRing&&
                IFileManager::Get().Copy(*(Folder/TEXT("cruise-checkpoint.sav")),*Slot)==COPY_OK;
            FScreenshotRequest::RequestScreenshot(Folder/TEXT("cruising.png"),true,false);
            if(!Saved) {Finish(false,TEXT("Moving checkpoint could not be saved"));return;}
            P.JumpPress();Next(6);return;
        }
        if(!Phase.Docked) return;
        const int Mask=1<<Phase.Stop;
        if(!(Stops&Mask)) {
            Stops|=Mask;
            Log(FString::Printf(TEXT("Docked at %s, clock %.1f, maximum deck drift %.2f cm"),ETRing::Name(Phase.Stop),Zone->State->RingClock,Drift));
        }
        const auto Here=P.GetActorLocation();
        if(const auto* R=Zone->Regions.FindRef({FMath::FloorToInt32(Here.X/Zone->RegionSize),FMath::FloorToInt32(Here.Y/Zone->RegionSize)}))
            if(R->Stage==6) Loaded|=Mask;
        if(Stops==255&&Phase.Stop==0&&Peak>0) {
            Log(FString::Printf(TEXT("One complete lap, peak %.0f km/h, deck drift %.2f cm, streamed station mask %d"),Peak*.036,Drift,Loaded));
            if(Drift>200||Loaded!=255) {Finish(false,TEXT("Passenger or city streaming incomplete"));return;}
            Next(4);
        }
        return;
    }
    if(Stage==4) {
        P.AddMovementInput(Across);
        if(L.Y>215&&Riding(P)) {P.JumpPress();Next(5);}
        return;
    }
    if(Stage==5) {
        if(Now-StageStart>.22) P.JumpRelease();
        if(L.Y<620) P.AddMovementInput(Across);
        if(L.Y>408&&P.GetCharacterMovement()->IsMovingOnGround()&&!Riding(P))
            Finish(true,TEXT("Passenger jumped off onto Shelter 09 station after eight cities and a full circuit"));
        return;
    }
    if(Stage==6) {
        JumpedAtSpeed|=P.GetCharacterMovement()->IsFalling();
        if(Now-StageStart>.2) P.JumpRelease();
        if(JumpedAtSpeed&&Riding(P)&&Now-StageStart>1.4) {
            Seat=Deck->GetComponentTransform().InverseTransformPosition(P.GetActorLocation());
            Log(TEXT("Jumped at cruise speed and landed back on the moving deck"));Next(3);
        }
    }
}
