#include "ETGame.h"
#include "ETRing.h"
#include "ETGeology.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "InputKeyEventArgs.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"

namespace
{
    void AimAt(AETCharacter* P,FVector Eye,FVector Target)
    {
        P->SetActorLocation(Eye-FVector(0,0,72));
        P->GetController()->SetControlRotation((Target-Eye).Rotation());
        FMinimalViewInfo View; P->Camera->GetCameraView(0,View);
    }
    // Find a clear surface in a generated forest. Use the same world collision as the borer and player.
    bool ClearPad(AETWorld* W,AETCharacter* P,FVector& Center)
    {
        W->EnsureRegion({1,0});
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ETFieldPad),false,P);
        for(int32 X=36000;X<=41200;X+=400) for(int32 Y=5600;Y<=8800;Y+=400) {
            FVector At(X,Y,W->SurfaceHeight(X,Y)); FHitResult H;
            if(W->GetWorld()->LineTraceSingleByChannel(H,At+FVector(0,0,1100),At-FVector(0,0,160),ECC_Visibility,Q)&&
               W->Geology->CanBore(H)&&H.ImpactNormal.Z>.96&&
               !W->GetWorld()->OverlapBlockingTestByChannel(H.ImpactPoint+FVector(0,0,470),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeBox(FVector(310,310,385)),Q)) {
                Center=H.ImpactPoint; return true;
            }
        }
        return false;
    }
}
void AETWorld::RunFrontierChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    State->ClockOffset=ETDay::At(16.5);State->Elapsed=1080; UpdateNightWatch(P->GetActorLocation());
    Check(SaveSlot==TEXT("EARTHtotaled-QA-Preview"),TEXT("Transient review sessions never address the normal survivor slot"));
    if(!P->Panel.IsEmpty()) P->ClosePanel();
    if(P->Borer) P->ToggleBorer();
    auto* PC=CastChecked<APlayerController>(P->GetController());
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETFieldTests),false,P);
    State->Unlocked.AddUnique(0); P->SelectWeapon(0); const auto Gun=Weapons[0];
    State->Reserve[0]=0; State->Ammo[0]=0; P->Reload();
    Check(P->ReloadTime>0&&State->Ammo[0]==0,TEXT("Empty gun with zero reserve still waits for a timed reload"));
    P->Tick(P->ReloadTime+.01f);
    Check(State->Ammo[0]==Gun.Mag&&State->Reserve[0]==0,TEXT("Unlimited reserve fills exactly one magazine"));
    const FVector Eye(40000,6000,10000); P->SetActorLocation(Eye-FVector(0,0,72));
    auto* Bot=AddThreat(TEXT("qa-hull"),TEXT("robot"),Eye+FVector(360,0,-65));
    FVector Aim=Bot->Parts[0]->Bounds.Origin;
    AimAt(P,Eye,Aim);
    const float OldXP=State->WeaponDamage[0]=148;
    Bot->Health=9;
    P->Shoot();
    Check(State->WeaponDamage[0]==OldXP+9&&P->GunRank(0)==2&&P->GunRank(1)==1,TEXT("A lethal hit credits only nine remaining HP and automatically upgrades just the firing gun"));
    auto* Drone=LiveThreats.FindRef(TEXT("qa-hull/shard0"));
    Check(IsValid(Drone)&&LiveThreats.Contains(TEXT("qa-hull/shard1"))&&!State->Threats.Contains(TEXT("qa-hull")),TEXT("Destroyed hull replaced by exactly two tracked hostile shard drones"));
    if(IsValid(Drone)) {
        Drone->AttackTime=0; AtRefuge=false; Drone->Tick(.04f);
        AETEnemyBolt* Bolt=nullptr; for(TActorIterator<AETEnemyBolt> It(GetWorld());It;++It) if(It->GetOwner()==Drone) Bolt=*It;
        Check(Bolt!=nullptr,TEXT("A detached drone independently pursues and fires a physical projectile"));
        if(Bolt) {
            const float Before=State->Health+State->Armor+State->Shield;
            for(int32 I=0;I<35&&IsValid(Bolt);++I) Bolt->Tick(.02f);
            Check(State->Health+State->Armor+State->Shield<Before,TEXT("Shard fire lands against the survivor capsule"));
        }
        Drone->TakeDamage(999,FDamageEvent(),PC,P);
        Check(!State->Threats.Contains(TEXT("qa-hull/shard0"))&&!State->Threats.Contains(TEXT("qa-hull/shard0/shard0")),TEXT("Drone death is final and never recursively splits"));
    }
    AimAt(P,Eye,Eye+FVector(0,0,900)); const float BeforeMiss=State->WeaponDamage[0]; P->Shoot();
    Check(BeforeMiss==State->WeaponDamage[0],TEXT("Firing into open sky awards zero mastery"));
    auto* Target=AddThreat(TEXT("qa-uprank"),TEXT("spitter"),Eye+FVector(360,0,-55));
    AimAt(P,Eye,Target->Parts[0]->Bounds.Origin); P->Shoot();
    Check(FMath::IsNearlyEqual(78-Target->Health,Gun.Damage*1.15f,.1f),TEXT("Mk 2 changes actual damage on the next enemy"));
    // Isolate the survivor patrol exercise from the preceding live-fire target.
    State->Threats.Remove(Target->Id); LiveThreats.Remove(Target->Id); Target->Destroy();
    // A saved patrol must reacquire a returning survivor, then fire a dodgeable
    // bolt from its actual muzzle at the survivor's position at firing time.
    const FString PatrolId=TEXT("qa-restored-robot");
    const FVector BotSite=Eye+FVector(0,950,-120);
    auto* Patrol=AddThreat(PatrolId,TEXT("robot"),BotSite);
    const float BeforeSuit=State->Health+State->Armor+State->Shield;
    auto FindBolt=[&]()->AETEnemyBolt* {
        for(TActorIterator<AETEnemyBolt> It(GetWorld());It;++It) if(It->GetOwner()==Patrol) return *It;
        return nullptr;
    };
    const FVector Standing=P->GetActorLocation();
    AtRefuge=false; P->EnemyReacquireTime=8; Patrol->AttackTime=0; Patrol->Tick(.02f);
    Check(!FindBolt()&&BeforeSuit==State->Health+State->Armor+State->Shield,TEXT("A ready patrol cannot fire on the survivor during checkpoint re-entry"));
    if(auto* Stray=GetWorld()->SpawnActor<AETEnemyBolt>(BotSite+FVector(0,0,120),FRotator::ZeroRotator)) {
        Stray->Launch(Patrol,Standing,false);
        for(int I=0;I<40&&IsValid(Stray);++I) Stray->Tick(.02f);
        Check(!IsValid(Stray)&&BeforeSuit==State->Health+State->Armor+State->Shield,TEXT("A stray patrol bolt that reaches the returning survivor cannot bypass checkpoint protection"));
    }
    P->EnemyReacquireTime=0; Patrol->Tick(.02f);
    auto* Incoming=FindBolt();
    const bool InFlight=Incoming&&BeforeSuit==State->Health+State->Armor+State->Shield;
    P->SetActorLocation(Standing+FVector(550,0,0));
    if(Incoming) for(int32 I=0;I<40&&IsValid(Incoming);++I) Incoming->Tick(.02f);
    Check(InFlight&&BeforeSuit==State->Health+State->Armor+State->Shield,TEXT("A HELIOS bolt travels visibly and misses a survivor who steps out of its path"));
    if(IsValid(Incoming)) Incoming->Destroy();
    P->SetActorLocation(Standing); Patrol->AttackTime=0; Patrol->Tick(.02f); Incoming=FindBolt();
    if(Incoming) for(int32 I=0;I<40&&IsValid(Incoming);++I) Incoming->Tick(.02f);
    Check(FMath::IsNearlyEqual(BeforeSuit-State->Health-State->Armor-State->Shield,13.f,.01f),TEXT("An unobstructed HELIOS bolt still deals its thirteen damage on impact"));
    Patrol->AttackTime=1.375f; Patrol->Snapshot(State->Threats[PatrolId]);
    Patrol->Destroy(); LiveThreats.Remove(PatrolId); Patrol=AddThreat(PatrolId,TEXT("robot"),BotSite);
    Check(Patrol&&FMath::IsNearlyEqual(Patrol->AttackTime,1.375f),TEXT("Restoring a live patrol preserves its remaining attack cooldown"));
    if(Patrol) Patrol->Destroy(); State->Threats.Remove(PatrolId); LiveThreats.Remove(PatrolId);
    FVector Pad; bool HasPad=ClearPad(this,P,Pad);
    Check(HasPad,TEXT("Located an unobstructed forest site")); if(!HasPad) return;
    P->SetActorLocation(Pad+FVector(0,0,110)); State->NextPatrol=0; State->PatrolNumber=0; PatrolTick=0; TickPatrols(P,1);
    Check(State->PatrolNumber==1&&State->Threats.Contains(TEXT("patrol-0-0")),TEXT("A forest patrol arrives through exploration even on a cleared save"));
    P->SetActorLocation(FVector(155000,1000,SurfaceHeight(155000,1000)+100)); EnsureRegion({4,0}); EnsureRegion({5,0});
    State->NextPatrol=0; PatrolTick=0; TickPatrols(P,1);
    bool Spitter=false,Runner=false; for(auto It:LiveThreats) if(IsValid(It.Value)&&It.Key.StartsWith(TEXT("patrol-1"))) {Spitter|=It.Value->Kind==TEXT("spitter"); Runner|=It.Value->Kind==TEXT("runner");}
    Check(Spitter&&Runner,TEXT("Travel into a city brings a living Bug pack with both castes"));
    P->SetActorLocation(FVector(0,-1500,110)); State->NextPatrol=0; PatrolTick=0; const int32 Serial=State->PatrolNumber; TickPatrols(P,1);
    Check(AtRefuge&&State->PatrolNumber==Serial,TEXT("Patrols do not spawn in the starting human sanctuary"));
    FHitResult H; FString Ore;
    const FVector Top=Pad+FVector(0,0,500), Bottom=Pad-FVector(0,0,2400);
    bool Square=true; for(int32 I=0;I<3;++I) {
        if(!GetWorld()->LineTraceSingleByChannel(H,Top,Bottom,ECC_Visibility,Q)||!Geology->Blast(H,Ore,true)) {Square=false;break;}
    }
    if(Square) {
        FVector C=FVector(State->SquareCuts.Last())*ETRock::Step; FHitResult A,B,Side;
        Square=GetWorld()->LineTraceSingleByChannel(A,C,C-FVector(0,0,470),ECC_Visibility,Q)&&
            GetWorld()->LineTraceSingleByChannel(B,C+FVector(95,0,0),C+FVector(95,0,-470),ECC_Visibility,Q)&&
            GetWorld()->LineTraceSingleByChannel(Side,C,C+FVector(500,0,0),ECC_Visibility,Q)&&
            FMath::Abs(A.ImpactPoint.Z-B.ImpactPoint.Z)<8&&A.ImpactNormal.Z>.96&&FMath::Abs(Side.ImpactNormal.X)>.94;
    }
    Check(Square,TEXT("Three square cuts produce a collidable level floor and a vertical wall in generated terrain"));
    // A separate undisturbed tile for a normal preview, placement and salvage.
    Pad.X+=1200;
    P->ToggleBuild(); P->BuildPiece=1;
    FETBuild Site; FString Why,FirstObstruction; bool Valid=false;
    // The new landscape changes the first clearing, so inspect neighboring bays for
    // the undisturbed site instead of assuming that the adjacent bay is always bare.
    for(int32 I=0;I<20&&!Valid;++I) {
        FVector Bay=Pad+FVector((I%5)*400,(I/5)*400,0); Bay.Z=SurfaceHeight(Bay.X,Bay.Y);
        AimAt(P,Bay+FVector(-490,0,290),Bay); P->RefreshBuild(); Valid=P->BuildTarget(Site,Why);
        if(!I&&!Valid) FirstObstruction=Why;
    }
    if(!FirstObstruction.IsEmpty()) Why+=TEXT(" / first bay: ")+FirstObstruction;
    Check(Valid&&P->Preview&&P->Preview->Mesh->GetCollisionEnabled()==ECollisionEnabled::NoCollision,TEXT("Floor hologram previews a real footing without blocking aim / ")+Why);
    int32 Stock=State->BuildStock; int32 Count=State->Builds.Num();
    P->PlaceBuild();
    Check(State->Builds.Num()==Count+1&&State->BuildStock==Stock-1,TEXT("Placing a completed floor consumes one kit"));
    if(State->Builds.Num()>Count) {
        FGuid Id; for(auto It:State->Builds) if(It.Value.Anchor.Equals(Site.Anchor,1)) Id=It.Key;
        const FVector C=Site.Anchor;
        Check(GetWorld()->LineTraceSingleByChannel(H,C+FVector(0,0,190),C-FVector(0,0,20),ECC_Pawn,Q)&&Cast<AETConstruction>(H.GetActor()),TEXT("Finished deck supports real pawn collision"));
        FIntPoint Cell(FMath::FloorToInt(C.X/RegionSize),FMath::FloorToInt(C.Y/RegionSize)); Regions[Cell]->Destroy(); Regions.Remove(Cell); EnsureRegion(Cell); RestoreDrops(Regions[Cell]);
        int32 Copies=0; for(TActorIterator<AETConstruction> It(GetWorld());It;++It) if(It->Id==Id) ++Copies;
        Check(Copies==1,TEXT("Built frames return once after terrain streams out and back"));
        AimAt(P,C+FVector(-330,0,190),C+FVector(0,0,15)); P->BuildPiece=2; P->BuildFacing=0;
        FETBuild RampSite; FString RampReason; P->BuildTarget(RampSite,RampReason); P->PlaceBuild();
        Check(State->Builds.Num()==Count+2,TEXT("A sloped ramp snaps onto the aimed-at deck / ")+RampReason);
        FHitResult Ramp; const bool OnRamp=GetWorld()->LineTraceSingleByChannel(Ramp,C+FVector(80,0,400),C+FVector(80,0,30),ECC_Pawn,Q);
        Check(OnRamp&&Cast<AETConstruction>(Ramp.GetActor())&&Ramp.ImpactNormal.Z>.71&&Ramp.ImpactNormal.Z<.92,TEXT("Placed ramp has a walkable inclined collision surface"));
        AimAt(P,C+FVector(75,0,480),C+FVector(75,0,140)); P->AimStart();
        Check(State->Builds.Num()==Count+1&&State->BuildStock==Stock-1,TEXT("Right mouse dismantles only the aimed-at survivor frame and recovers its kit"));
        AimAt(P,C+FVector(-360,0,215),C+FVector(0,0,12));
        P->BuildPiece=0; P->BuildFacing=3; P->TurnTool(); P->PlaceBuild();
        Check(State->Builds.Num()==Count+2&&P->BuildFacing==0,TEXT("Rotated wall snaps to the far edge of an existing floor"));
        AimAt(P,C+FVector(-180,0,205),C+FVector(200,0,230));
        P->BuildPiece=3; P->PlaceBuild();
        Check(State->Builds.Num()==Count+3&&IsSheltered(C+FVector(0,0,145)),TEXT("A peaked roof on the wall provides actual weather shelter"));
        AETConstruction* Wall=nullptr;
        for(const auto& It:LiveBuilds) {
            auto* Built=It.Value.Get();
            if(Built&&Built->Design.Piece==0&&Built->Design.Anchor.Equals(C,4)&&Built->Design.Facing==0) {Wall=Built; break;}
        }
        FHitResult Outside;
        GetWorld()->SweepSingleByChannel(Outside,C+FVector(620,0,105),P->GetActorLocation(),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(6),Q);
        bool Stopped=false;
        if(Wall) {
            // Exercise the two faces of this actual placed wall. In a forest, a
            // shot starting several metres away can strike the hillside first.
            const FVector Face=Wall->GetActorForwardVector(),Center=Wall->GetActorLocation();
            P->SetActorLocation(Center-Face*110);
            auto* Foe=AddThreat(TEXT("qa-cover-spitter"),TEXT("spitter"),Center+Face*210);
            auto* Glob=GetWorld()->SpawnActor<AETEnemyBolt>(Center+Face*90,FRotator::ZeroRotator);
            const float Suit=State->Armor,HP=State->Health,Before=State->Builds[Wall->Id].Integrity;
            if(Foe&&Glob) {
                Glob->Launch(Foe,P->GetActorLocation(),true);
                for(int32 I=0;I<25&&IsValid(Glob);++I) Glob->Tick(.02f);
                Stopped=FMath::IsNearlyEqual(Before-State->Builds[Wall->Id].Integrity,11.f)&&HP==State->Health&&Suit==State->Armor;
            }
        }
        Check(Stopped,TEXT("Built cover intercepts a spitter projectile and takes its exact damage / forest approach: ")+GetNameSafe(Outside.GetComponent()));
    }
    P->StopBuilding();
}

void AETWorld::TickFrontierJourney(AETCharacter* P,float Delta)
{
    // Static state spans real level travel. The second pass starts a new OS process.
    static int32 Step=0,Checks=0,Failures=0;
    static const bool Cold=FParse::Param(FCommandLine::Get(),TEXT("ETFieldRead"));
    static const bool Legacy=FParse::Param(FCommandLine::Get(),TEXT("ETFieldLegacy"));
    static FString Report;
    static FVector Shaft;
    static double Started=FPlatformTime::Seconds(),LiftStart=0,BottomZ=0;
    static float TakeoffFuel=0;
    static bool Done=false;
    if(Done) return;
    const FString File=FPaths::ProjectSavedDir()/(Legacy?TEXT("fieldworks-upgrade.txt"):Cold?TEXT("fieldworks-restart.txt"):TEXT("fieldworks-play.txt"));
    auto Check=[&](bool OK,FString Name) {
        ++Checks; if(!OK) ++Failures;
        Report+=(OK?TEXT("PASS "):TEXT("FAIL "))+Name+TEXT("\n");
        FFileHelper::SaveStringToFile(Report,*File);
    };
    auto Finish=[&]() {
        Done=true;
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("ET_FIELD %d checks / %d failures\n"),Checks,Failures)+Report,*File);
        FPlatformMisc::RequestExitWithStatus(true,Failures?1:0);
    };
    auto* PC=Cast<APlayerController>(P->Controller); if(!PC) return;
    static double KeySent=0;
    auto KeyPress=[&](FKey Key) {
        auto* V=GetWorld()->GetGameViewport();
        const auto Device=FInputDeviceId::CreateFromInternalId(0);
        V->InputKey(FInputKeyEventArgs(V->Viewport,Device,Key,IE_Pressed,FPlatformTime::Cycles64()));
        V->InputKey(FInputKeyEventArgs(V->Viewport,Device,Key,IE_Released,FPlatformTime::Cycles64()));
        KeySent=FPlatformTime::Seconds();
    };
    if(FPlatformTime::Seconds()-Started>120) {Check(false,FString::Printf(TEXT("Timed out at stage %d, panel %s"),Step,*P->Panel)); Finish(); return;}
    auto Capture=[&]() {
        auto* V=GetWorld()->GetGameViewport();
        return V&&!PC->bShowMouseCursor&&!PC->IsLookInputIgnored()&&V->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
    };
    if(Step==0) {
        if(P->Panel!=TEXT("title")) return;
        Check(PC->bShowMouseCursor&&UGameplayStatics::IsGamePaused(this),TEXT("Launch opens a paused clickable main menu"));
        Check(UGameUserSettings::GetGameUserSettings()->GetFullscreenMode()!=EWindowMode::Windowed,TEXT("Windows executable starts in fullscreen"));
        if(Legacy) {
            Check(HadSave,TEXT("Title recognizes the copied survivor from a previous release")); P->Action(TEXT("continue")); Step=9;
        } else if(Cold) {
            Check(HadSave,TEXT("Returning process finds the previous survivor")); P->Action(TEXT("continue")); Step=6;
        } else {
            Check(!HadSave,TEXT("First-run menu detects a new profile")); P->Action(TEXT("new")); Step=1;
        }
        return;
    }
    if(Step==9) {
        if(!GetWorld()->URL.HasOption(TEXT("ETContinue"))) return;
        if(P->Panel==TEXT("story")) P->ClosePanel();
        const auto* Old=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot,0));
        Check(Old&&State->Gold==Old->Gold&&State->Water==Old->Water&&State->Cores==Old->Cores&&State->Ending==Old->Ending&&State->Unlocked==Old->Unlocked&&State->Consumed.Num()==Old->Consumed.Num()&&State->BoreCuts==Old->BoreCuts,TEXT("Returning survivor keeps currency, water, guns, story, loot and old mine edits"));
        const FString Transfer=FPaths::ProjectSavedDir()/TEXT("returning-survivor.etsave"); FString Note;
        const bool Wrote=P->ExportProfile(State,Transfer,Note);
        const auto* Portable=Wrote?P->ReadProfile(Transfer,Note):nullptr;
        Check(Portable&&Portable->Gold==State->Gold&&Portable->Water==State->Water&&Portable->BoreCuts==State->BoreCuts&&Portable->LegacyGround.Num()==State->LegacyGround.Num(),TEXT("The existing survivor can export and import with resources and excavations intact / ")+Note);
        Check(State->WeaponDamage.Num()==Weapons.Num()&&State->BuildStock>=0&&Capture(),TEXT("Returning survivor can use the new equipment and captured mouse"));
        const float Before=PC->GetControlRotation().Yaw;
        P->Turn(9); PC->UpdateRotation(.016f);
        Check(FMath::Abs(FMath::FindDeltaAngleDegrees(Before,PC->GetControlRotation().Yaw))>.1f,TEXT("Unheld mouse-look axis rotates the camera after loading the returning survivor"));
        P->Pause(); P->Action(TEXT("main-menu"));
        Check(P->Panel==TEXT("title")&&HadSave,TEXT("The upgraded copy saves and returns to the title"));
        Finish(); return;
    }
    if(Step==1) {
        if(!P->Panel.IsEmpty()||State->ArrivalStage!=1) return;
        Check(State->BuildStock==0&&State->Inventory.FindRef(TEXT("wood"))==0,TEXT("Fresh survivor starts ready to harvest their first wooden frame"));
        Check(Capture(),TEXT("Leaving the opening restores permanent mouse capture"));
        const FVector Birch(-1320,-1950,0);
        AimAt(P,Birch+FVector(0,-165,82),Birch+FVector(0,0,82)); P->Tick(.01f); P->Interact();
        Check(State->Inventory.FindRef(TEXT("wood"))==8,TEXT("Use on the starter birch supplies two wooden pieces"));
        P->ToggleBuild(); KeyPress(EKeys::C); Step=11; return;
    }
    if(Step==11) {
        if(P->BuildMaterial!=1&&FPlatformTime::Seconds()-KeySent<2) return;
        Check(P->BuildMaterial==1,TEXT("The C key switches the live wooden preview to stone"));
        P->BuildMaterial=0; KeyPress(EKeys::F2); Step=12; return;
    }
    if(Step==12) {
        if(P->Panel!=TEXT("controls")&&FPlatformTime::Seconds()-KeySent<2) return;
        Check(P->GuidePage==1&&P->Panel==TEXT("controls"),TEXT("The F2 key opens the construction guide directly from the builder"));
        if(P->Panel!=TEXT("controls")) {Finish(); return;}
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("building-guide.png"),true,false);
        Step=10; return;
    }
    if(Step==10) {
        if(FScreenshotRequest::IsScreenshotRequested()) return;
        KeyPress(EKeys::F2); Step=13; return;
    }
    if(Step==13) {
        if(!P->Panel.IsEmpty()&&FPlatformTime::Seconds()-KeySent<2) return;
        Check(P->Panel.IsEmpty()&&Capture(),TEXT("F2 closes the paused guide and recaptures the mouse"));
        if(!P->Panel.IsEmpty()) {Finish(); return;}
        P->ToggleBuild();
        KeyPress(EKeys::M); Step=14; return;
    }
    if(Step==14) {
        if(FPlatformTime::Seconds()-KeySent<.35) return;
        if((!P->MapInput.IsValid()||FSlateApplication::Get().GetKeyboardFocusedWidget()!=P->MapInput)&&FPlatformTime::Seconds()-KeySent<3) return;
        Check(P->Panel==TEXT("map")&&P->MapInput.IsValid()&&PC->bShowMouseCursor&&UGameplayStatics::IsGamePaused(this),TEXT("M opens a paused fullscreen atlas with a free pointer"));
        if(!P->MapInput.IsValid()) {Finish();return;}
        CheckAtlasInput(P,Check);
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("field-atlas.png"),true,false);
        Step=15; return;
    }
    if(Step==15) {
        if(FScreenshotRequest::IsScreenshotRequested()) return;
        auto& Slate=FSlateApplication::Get(); const FKeyEvent Key(EKeys::M,FModifierKeysState(),0u,false,0,0);
        Slate.ProcessKeyDownEvent(Key); Slate.ProcessKeyUpEvent(Key);
        Check(P->Panel.IsEmpty()&&!P->MapInput.IsValid()&&Capture()&&!P->Firing,TEXT("M closes the atlas, returns permanent mouse look and does not fire"));
        if(!P->Panel.IsEmpty()) {Finish();return;}
        P->Pause(); P->Action(TEXT("aim-assist")); P->ClosePanel();
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("navigation-hud.png"),true,false);
        Step=16; return;
    }
    if(Step==16) {
        if(FScreenshotRequest::IsScreenshotRequested()) return;
        P->Skills(); P->Action(TEXT("perk:climb"));
        Check(P->Panel==TEXT("skills")&&P->SelectedPerk==TEXT("climb")&&PC->bShowMouseCursor,TEXT("Skill web exposes a selectable climbing perk in the fullscreen game"));
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("survivor-skill-web.png"),true,false);
        Step=17;return;
    }
    if(Step==17) {
        if(FScreenshotRequest::IsScreenshotRequested()) return;
        P->Skills(); Check(P->Panel.IsEmpty()&&Capture(),TEXT("Closing the skill web recaptures the mouse"));
        if(!ClearPad(this,P,Shaft)) {Check(false,TEXT("No open forest pad")); Finish(); return;}
        // Place a ground wall through the build controls, then drill a different bay for the flight.
        P->ToggleBuild(); P->BuildPiece=0; P->BuildFacing=1;
        FString WallReason; FETBuild Wall;
        for(int32 I=0;I<24&&State->Builds.IsEmpty();++I) {
            FVector Bay=Shaft+FVector(800+(I%6)*400,(I/6)*400,0); Bay.Z=SurfaceHeight(Bay.X,Bay.Y);
            AimAt(P,Bay+FVector(-510,0,300),Bay); P->RefreshBuild(); if(P->BuildTarget(Wall,WallReason)) P->PlaceBuild();
        }
        Check(State->Builds.Num()==1,TEXT("Normal build controls place a saved field wall / ")+WallReason);
        P->ToggleBuild(); P->ToggleBorer(); P->TurnTool();
        const FVector Top=Shaft+FVector(0,0,450);
        for(int32 I=0;I<5;++I) {AimAt(P,Top,Shaft-FVector(0,0,2500)); P->FireBorer();}
        Check(State->SquareCuts.Num()>=5,TEXT("Equipped square borer cuts a five-pulse shaft"));
        P->ToggleBorer();
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ETJetShaft),false,P); FHitResult H;
        if(!GetWorld()->LineTraceSingleByChannel(H,Top,Shaft-FVector(0,0,3500),ECC_Visibility,Q)) {Check(false,TEXT("No shaft floor")); Finish(); return;}
        const FVector Feet=H.ImpactPoint+FVector(0,0,100);
        P->SetActorLocation(Feet); P->GetCharacterMovement()->StopMovementImmediately(); P->GetCharacterMovement()->SetMovementMode(MOVE_Falling);
        State->JetFuel=62; BottomZ=Feet.Z; LiftStart=FPlatformTime::Seconds(); Step=21; return;
    }
    auto Space=[&](EInputEvent Event) {
        auto* V=GetWorld()->GetGameViewport();
        V->InputKey(FInputKeyEventArgs(V->Viewport,FInputDeviceId::CreateFromInternalId(0),EKeys::SpaceBar,Event,FPlatformTime::Cycles64()));
    };
    if(Step==21) {
        if(!P->GetCharacterMovement()->IsMovingOnGround()||State->JetFuel<67) return;
        Check(State->JetFuel>62&&!P->AirJumpSpent,TEXT("Shaft floor restores fuel and a free air jump"));
        TakeoffFuel=State->JetFuel; Space(IE_Pressed); LiftStart=FPlatformTime::Seconds(); Step=22; return;
    }
    if(Step==22) {
        if(FPlatformTime::Seconds()-LiftStart<.32) return;
        Check(P->GetCharacterMovement()->IsFalling()&&!P->Jetting&&!P->AirJumpSpent&&State->JetFuel>=TakeoffFuel-.1f,TEXT("Holding the first jump takes off without spending jet fuel"));
        TakeoffFuel=State->JetFuel;
        Space(IE_Released); Space(IE_Pressed); LiftStart=FPlatformTime::Seconds(); Step=23; return;
    }
    if(Step==23) {
        if(FPlatformTime::Seconds()-LiftStart<.13) return;
        Check(P->AirJumpSpent&&P->GetVelocity().Z>0&&!P->Jetting&&FMath::IsNearlyEqual(State->JetFuel,TakeoffFuel,.03f),TEXT("Second Space press adds an upward kick for free"));
        Space(IE_Released); LiftStart=FPlatformTime::Seconds(); Step=24; return;
    }
    if(Step==24) {
        if(FPlatformTime::Seconds()-LiftStart<.22) return;
        Check(P->GetCharacterMovement()->IsFalling()&&FMath::IsNearlyEqual(State->JetFuel,TakeoffFuel,.03f),TEXT("A released pack cannot refill in midair"));
        Space(IE_Pressed); LiftStart=FPlatformTime::Seconds(); Step=2; return;
    }
    if(Step==2) {
        if(P->GetActorLocation().Z<Shaft.Z+200&&FPlatformTime::Seconds()-LiftStart<16) return;
        Check(P->GetActorLocation().Z>Shaft.Z+160&&Shaft.Z-BottomZ>800,
            FString::Printf(TEXT("Held jump flew the capsule out of a %.1f m shaft / rise %.1f m"),(Shaft.Z-BottomZ)/100,(P->GetActorLocation().Z-BottomZ)/100));
        Check(State->JetFuel<TakeoffFuel-4,TEXT("Powered ascent after the double jump consumes jet fuel"));
        Space(IE_Released);
        State->WeaponDamage[0]=479; State->Ammo[0]=3;
        AddThreat(TEXT("field-test/shard0"),TEXT("drone"),Shaft+FVector(550,330,260));
        P->Pause(); P->Action(TEXT("main-menu"));
        Check(P->Panel==TEXT("title")&&HadSave,TEXT("Escape menu saves and returns to title"));
        P->Action(TEXT("new")); P->Pause();
        Check(P->Panel==TEXT("title")&&State->Builds.Num()==1,TEXT("Canceling restart retains the current survivor and construction"));
        Step=3; P->Action(TEXT("continue")); return;
    }
    if(Step==3||Step==6) {
        if(!P->Panel.IsEmpty()||!GetWorld()->URL.HasOption(TEXT("ETContinue"))) return;
        Check(Capture(),TEXT("Continue restores game-only mouse look"));
        Check(State->MapPins.Num()==2&&State->TrackedPin==1&&State->MapPins[1].Equals(ETRing::Station(4),1.)&&!State->LightAimAssist,
            TEXT("Continue restores the hand-placed pin, tracked Ringline station and aim assist preference"));
        Check(State->Builds.Num()==1&&LiveBuilds.Num()>0&&State->SquareCuts.Num()>=5&&State->SquareBorer,TEXT("Saved wall and square excavations reload into the world"));
        Check(State->Ammo[0]==3&&P->GunRank(0)==3&&State->Threats.Contains(TEXT("field-test/shard0")),TEXT("Partial magazine, gun upgrades and surviving drone reload"));
        if(!Cold) {Finish(); return;}
        P->Pause(); P->Action(TEXT("main-menu")); P->Action(TEXT("new"));
        Check(P->Panel==TEXT("confirm-new"),TEXT("A new game with existing progress requires confirmation"));
        Step=7; SaveTime=21; P->Action(TEXT("confirm"));
        Check(!Save(),TEXT("A pending autosave from the departing world cannot overwrite the fresh survivor")); return;
    }
    if(Step==7&&P->Panel.IsEmpty()&&State->ArrivalStage==1) {
        const auto* Old=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot+TEXT("-previous"),0));
        Check(Old&&Old->Builds.Num()==1&&Old->SquareCuts.Num()>=5&&Old->WeaponDamage[0]==479,TEXT("Previous survivor retained in an intact backup slot"));
        Check(State->SquareCuts.IsEmpty()&&State->Builds.IsEmpty()&&State->Threats.IsEmpty()&&P->GunRank(0)==1&&State->JetFuel==100&&State->Ammo[0]==Weapons[0].Mag,TEXT("Confirmed restart returns to the opening with fresh terrain, gear and enemies"));
        P->Die(); P->Action(TEXT("main-menu"));
        Check(P->Panel==TEXT("title")&&PC->bShowMouseCursor,TEXT("Death screen also returns to the main menu"));
        Finish();
    }
}
