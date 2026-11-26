#include "ETGame.h"
#include "ETGeology.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "UnrealClient.h"

void AETWorld::TickBorerRecoveryCheck(AETCharacter* P)
{
    // Run against a copy of a threatened checkpoint. The QA slot stays separate
    // across both real map reloads, including the one initiated by the death menu.
    static int32 Stage=0,Checks=0,Failures=0,Round=0,Square=0,Heading=0,Seed=0,Wallet=0;
    static float InitialHP=0,ResumeClock=0,NextAim=0;
    static bool ProtectedStart=false,RoundDenied=false,SquareDenied=false;
    static double Started=FPlatformTime::Seconds();
    static FVector Start;
    static FString Report;
    static TArray<FIntVector> SavedRound,SavedSquare;
    static TMap<FString,int32> Pack;
    const FString File=FPaths::ProjectSavedDir()/TEXT("borer-recovery.txt");
    auto Check=[&](bool OK,const FString& Message) {
        ++Checks; if(!OK) ++Failures;
        Report+=(OK?FString(TEXT("PASS ")):FString(TEXT("FAIL ")))+Message+TEXT("\n");
        FFileHelper::SaveStringToFile(Report,*File);
    };
    auto Finish=[&]() {
        Stage=5;
        FFileHelper::SaveStringToFile(FString::Printf(TEXT("ET_BORER %d checks / %d failures\n"),Checks,Failures)+Report,*File);
        FPlatformMisc::RequestExitWithStatus(true,Failures?1:0);
    };
    if(Stage==5) return;
    if(FPlatformTime::Seconds()-Started>60) {Check(false,FString::Printf(TEXT("Timed out in stage %d, panel %s, health %.1f"),Stage,*P->Panel,State->Health)); Finish(); return;}
    if(Stage==0) {
        if(P->Panel!=TEXT("title")) return;
        const bool Ready=SaveSlot==TEXT("EARTHtotaled-QA-BorerRecovery")&&HadSave&&State->Health<=25&&State->Threats.Num()>0;
        Check(Ready,TEXT("Isolated title loads a living, critically wounded survivor with a hostile patrol"));
        if(!Ready) {Finish();return;}
        InitialHP=State->Health; Round=State->BoreCuts.Num(); Square=State->SquareCuts.Num(); Start=State->Position;
        Stage=1; P->Action(TEXT("continue")); return;
    }
    if(P->Panel==TEXT("travel")) return;
    if(Stage==1) {
        if(!GetWorld()->URL.HasOption(TEXT("ETContinue"))) return;
        Check(P->Panel.IsEmpty()&&FVector::Dist2D(Start,P->GetActorLocation())<200&&State->Health==InitialHP&&P->EnemyReacquireTime>6,
            FString::Printf(TEXT("Continue restores the actual encounter with control / %.1f HP, %.1fs to react"),State->Health,P->EnemyReacquireTime));
        ResumeClock=State->Elapsed; NextAim=ResumeClock;
        ProtectedStart=Geology->DigRestriction(FBox2D(FVector2D(Start),FVector2D(Start)))!=nullptr;
        P->ToggleBorer(); if(State->SquareBorer) P->TurnTool(); Stage=2; return;
    }
    if(Stage==2) {
        if(P->Panel==TEXT("death")||State->Elapsed-ResumeClock>7.8f) {
            Check(false,FString::Printf(TEXT("Did not finish sculpting / round %d, square %d, panel %s, %s"),State->BoreCuts.Num()-Round,State->SquareCuts.Num()-Square,*P->Panel,*P->PanelBody));
            Finish(); return;
        }
        if(State->BoreCuts.Num()>=Round+2&&!State->SquareBorer) {P->TurnTool(); NextAim=State->Elapsed;}
        if(State->SquareCuts.Num()>=Square+2||(RoundDenied&&SquareDenied)) {
            P->FireStop();
            if(ProtectedStart)
                Check(RoundDenied&&SquareDenied&&State->Health==InitialHP&&State->BoreCuts.Num()==Round&&State->SquareCuts.Num()==Square&&P->Panel.IsEmpty(),
                    TEXT("Both borer modes report protected ground and preserve the wounded survivor and previous excavations"));
            else
                Check(State->Health==InitialHP&&State->BoreCuts.Num()>=Round+2&&P->Panel.IsEmpty(),
                    FString::Printf(TEXT("Held fire makes real round and square cuts from the saved position / %.1f HP"),State->Health));
            SavedRound=State->BoreCuts; SavedSquare=State->SquareCuts; Pack=State->Inventory; Wallet=State->Gold; Seed=State->Seed;
            if(!IsNight()) {
                Check(State->Health==InitialHP,TEXT("Testing the borer in daylight leaves the survivor alive with the saved hostiles dormant"));
                const float GraceElapsed=State->Elapsed-ResumeClock;
                P->Command(TEXT("time night")); ResumeClock=State->Elapsed-GraceElapsed;
            }
            Stage=3; return;
        }
        if(State->Elapsed<NextAim) return;
        NextAim=State->Elapsed+.72f;
        const FVector Eye=P->Camera->GetComponentLocation();
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ETBorerReplay),false,P);
        for(int32 I=0;I<24;++I) {
            const FRotator View(-16.-(Heading%3)*3,Heading*37.,0);
            ++Heading;
            FHitResult Hit;
            if(!GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+View.Vector()*ETRock::ToolRange,ECC_Visibility,Query)||
               Hit.Distance<400||!Geology->CanBore(Hit)) continue;
            P->GetController()->SetControlRotation(View); FMinimalViewInfo POV; P->Camera->GetCameraView(0,POV);
            if(ProtectedStart) {
                P->FireBorer();
                if(Notice.Contains(TEXT("Protected"))) {
                    if(State->SquareBorer) SquareDenied=true;
                    else {RoundDenied=true;P->TurnTool();}
                }
            } else P->FireStart();
            break;
        }
        return;
    }
    if(Stage==3) {
        if(P->Panel!=TEXT("death")) return;
        Check(P->EnemyReacquireTime<=0&&State->Elapsed-ResumeClock>=7.8f&&
            (P->PanelBody.Contains(TEXT("Cause of death: Supertech"))||P->PanelBody.Contains(TEXT("Cause of death: Bug"))),
            FString::Printf(TEXT("Hostiles can still kill after the reaction window / %.1fs, %s"),State->Elapsed-ResumeClock,*P->PanelBody));
        Stage=4; P->Action(TEXT("shelter-rescue")); return;
    }
    if(!GetWorld()->URL.HasOption(TEXT("ETRecoverShelter"))||AtRecoveryShelter) return;
    const auto* Disk=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot,0));
    Check(P->Panel.IsEmpty()&&SafeSettlement(P->GetActorLocation())&&State->Health>=50&&Disk&&Disk->Health>=50&&
        FVector::Dist2D(Disk->Position,GetDefault<UETSave>()->Position)<1700,TEXT("Recovery opens gameplay on a supported shelter landing and saves the living survivor"));
    Check(Disk&&Disk->Seed==Seed&&Disk->Gold==Wallet&&Disk->Inventory.OrderIndependentCompareEqual(Pack)&&
        Disk->BoreCuts==SavedRound&&Disk->SquareCuts==SavedSquare,TEXT("Recovery preserves the excavated world, ore and wrapper balance"));
    Finish();
}

void AETWorld::RunGeologyChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    if(!P->Panel.IsEmpty()) P->ClosePanel();
    auto* PC=CastChecked<APlayerController>(P->GetController());
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ETGeologyCheck),false,P);
    auto Trace=[&](FVector A,FVector B,FHitResult& Hit){return GetWorld()->LineTraceSingleByChannel(Hit,A,B,ECC_Visibility,Params);};
    auto Prepare=[&](FVector Pos){
        const FIntPoint C(FMath::FloorToInt(Pos.X/RegionSize),FMath::FloorToInt(Pos.Y/RegionSize));
        EnsureRegion(C); return Regions.FindRef(C);
    };
    auto Face=[&](FVector Eye,FVector Aim){
        P->SetActorLocation(Eye-FVector(0,0,72),false,nullptr,ETeleportType::TeleportPhysics);
        PC->SetControlRotation((Aim-Eye).Rotation());
        FMinimalViewInfo View; P->Camera->GetCameraView(0,View);
    };
    Prepare({1000,-2750,0}); Prepare({-2000,-4400,0});
    Check(Material(TEXT("FieldDisplay"))&&State->RetroDisplay&&P->Camera->PostProcessSettings.WeightedBlendables.Array.Num()>0,TEXT("PS1 scene display is cooked and active on the player camera"));
    P->Pause(); P->Action(TEXT("field-display"));
    Check(!State->RetroDisplay&&P->Camera->PostProcessSettings.WeightedBlendables.Array.IsEmpty(),TEXT("Pause display control restores native scene rendering"));
    P->Action(TEXT("field-display")); P->ClosePanel();
    P->ToggleBorer();
    Check(P->Borer&&P->Gun->GetStaticMesh()&&P->Gun->GetStaticMesh()->GetFName()==TEXT("SM_SurveyBorer"),TEXT("Player can equip the original survey borer on an existing arsenal"));
    FHitResult Hit; FString Reward;
    Check(Trace({3370,-2750,-800},{3370,-2750,-1550},Hit)&&Geology->CanBore(Hit)&&Hit.ImpactPoint.Z<-1100,TEXT("Starter adit has a collidable underground floor"));
    FHitResult AditRoof,AditSurface;
    const bool HasRoof=Trace({3370,-2750,-800},{3370,-2750,300},AditRoof)&&Geology->CanBore(AditRoof);
    const bool HasSurface=Trace({3370,-2750,200},{3370,-2750,-800},AditSurface)&&Geology->CanBore(AditSurface);
    Check(HasRoof&&HasSurface&&AditRoof.ImpactNormal.Z<-.4&&AditSurface.ImpactNormal.Z>.7&&
        AditSurface.ImpactPoint.Z-AditRoof.ImpactPoint.Z>100,
        FString::Printf(TEXT("Starter chamber has separate collidable ceiling and surface / thickness %.0f cm"),AditSurface.ImpactPoint.Z-AditRoof.ImpactPoint.Z));
    bool Walkable=true; int32 RampX=0;
    for(int32 X=1030;X<=3110;X+=80) {
        const double Line=300-(X-770)*1060./2350.;
        RampX=X;
        if(!Trace({double(X),-2750,Line},{double(X),-2750,Line-650},Hit)||Hit.ImpactNormal.Z<P->GetCharacterMovement()->GetWalkableFloorZ()) {Walkable=false; break;}
        const FVector Stand=Hit.ImpactPoint+FVector(0,0,112);
        if(GetWorld()->OverlapBlockingTestByChannel(Stand,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Params)) {Walkable=false; break;}
    }
    Check(Walkable,Walkable?TEXT("Starter ramp has a walkable slope and full survivor headroom down to the chamber"):
        FString::Printf(TEXT("Starter ramp blocked at x=%d / floor normal Z %.3f / floor %.1f"),RampX,Hit.ImpactNormal.Z,Hit.ImpactPoint.Z));
    Check(Geology->Survey({3550,-2750,-835},FVector::ForwardVector).Contains(TEXT("NO RETURN")),TEXT("The survey omits inaccessible deposits under the shelter"));
    const int32 Balance=State->Gold,Water=State->Water,Ammo=State->Ammo[State->Weapon];
    Face({3550,-2750,-840},{4200,-2750,-840});
    P->Shoot();
    Check(!State->MinedVeins.Contains({0,0,1})&&Notice.Contains(TEXT("Protected")),TEXT("A lateral shot from inside the existing shelter mine cannot remove rock or gold"));
    auto MineOutcrop=[&](double X) {
        FVector Seam(X,-15300,SurfaceHeight(X,-15300)); Prepare(Seam); Prepare(Seam+FVector(-200,0,0));
        Face(Seam+FVector(0,0,230),Seam-FVector(0,0,300)); P->Shoot();
    };
    MineOutcrop(32000);
    Check(State->MinedVeins.Contains({0,0,3})&&State->Inventory.FindRef(TEXT("goldore"))>=3,TEXT("Firing the equipped borer into a wilderness outcrop adds real gold ore"));
    Check(State->Gold==Balance&&State->Water==Water&&State->Ammo[State->Weapon]==Ammo,TEXT("Boring spends no firearm ammo and awards neither wrappers nor water"));
    MineOutcrop(33000);
    Check(State->MinedVeins.Contains({0,0,4})&&State->Inventory.FindRef(TEXT("ironore"))>=5,TEXT("Borer extracts the separate iron vein in the forest"));
    const int32 BeforeIron=State->Inventory.FindRef(TEXT("ironore")),BeforeAlloy=State->Inventory.FindRef(TEXT("alloy"));
    P->OpenPanel(TEXT("bench"),TEXT("Workbench"),TEXT("")); P->Craft(TEXT("refine-iron")); P->ClosePanel();
    Check(State->Inventory.FindRef(TEXT("ironore"))==BeforeIron-2&&State->Inventory.FindRef(TEXT("alloy"))==BeforeAlloy+1,TEXT("Workbench consumes two iron ore to make one usable alloy sheet"));
    const int32 BeforeGold=State->Inventory.FindRef(TEXT("goldore"));
    P->ContactPanel(TEXT("SALT-7")); P->Action(TEXT("sell-ore")); P->ClosePanel();
    Check(State->Inventory.FindRef(TEXT("goldore"))==BeforeGold-1&&State->Gold==Balance+12,TEXT("SALT-7 exchanges one mined gold ore for twelve existing currency wrappers"));
    FHitResult Protected;
    Check(Trace({150,-700,120},{150,-300,120},Protected)&&!Geology->Blast(Protected,Reward),TEXT("Borer respects normal contact and building collision"));

    // The pulse hits the baked asphalt itself. Remaining pavement on both sides must still support a survivor.
    const FVector Road(-3200,-4400,0);
    const bool Street=Trace(Road+FVector(0,0,230),Road-FVector(0,0,100),Hit)&&Hit.GetComponent()->ComponentHasTag(TEXT("ET_Paving"));
    Check(Street&&!Geology->Blast(Hit,Reward)&&Reward.Contains(TEXT("Protected")),TEXT("Baked city pavement rejects an excavation pulse with a reason"));
    Check(Trace(Road+FVector(0,0,160),Road-FVector(0,0,1200),Hit)&&Hit.ImpactPoint.Z>-20,TEXT("City street collision remains solid after an attempted blast"));
    Check(Trace(Road+FVector(800,0,160),Road+FVector(800,0,-100),Hit)&&Hit.GetComponent()->ComponentHasTag(TEXT("ET_Paving")),TEXT("Uncut city pavement beside a crater retains collision"));
    const FVector Outlying(160000,-1700,CityCenter(160000,0).Z);
    Prepare(Outlying); Prepare(Outlying+FVector(0,2500,0));
    bool City=Trace(Outlying+FVector(0,0,250),Outlying-FVector(0,0,80),Hit)&&!Geology->Blast(Hit,Reward,true);
    Check(City&&Trace(Outlying+FVector(0,0,140),Outlying-FVector(0,0,1300),Hit)&&Hit.ImpactPoint.Z>Outlying.Z-20,TEXT("Square cuts in another procedural city leave the street solid"));

    // Start on a negative region boundary, then drill across a vertical chunk boundary and sideways under a roof.
    FVector Shaft(-64010,41400,0); Shaft.Z=SurfaceHeight(Shaft.X,Shaft.Y);
    auto* West=Prepare(Shaft); Prepare(Shaft+FVector(300,0,0));
    const FVector Top=Shaft+FVector(0,0,350),Bottom=Shaft-FVector(0,0,8000);
    bool Dug=true; FHitResult First;
    for(int32 I=0;I<9;++I) {
        if(!Trace(Top,Bottom,Hit)||!Geology->CanBore(Hit)) {Dug=false; break;}
        if(I==0) First=Hit;
        if(!Geology->Blast(Hit,Reward)) {Dug=false; break;}
    }
    const int32 Cuts=State->BoreCuts.Num();
    Check(Dug&&Trace(Top,Bottom,Hit)&&Hit.ImpactPoint.Z<Shaft.Z-3000,TEXT("Deep mining crosses vertical chunk levels beyond the old fall-recovery threshold"));
    Check(!Geology->Blast(First,Reward)&&State->BoreCuts.Num()==Cuts,TEXT("Repeating the exact same pulse cannot duplicate an edit or mineral yield"));
    const FVector Lateral=Shaft-FVector(0,0,2300);
    for(int32 I=0;I<6;++I) {
        if(!Trace(Lateral,Lateral+FVector(6000,0,0),Hit)||!Geology->Blast(Hit,Reward)) {Dug=false; break;}
    }
    Check(Dug&&!GetWorld()->SweepSingleByChannel(Hit,Lateral,Lateral+FVector(1200,0,0),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Params),TEXT("A character capsule can cross the region seam through a player-made horizontal tunnel"));
    const FVector Inside=Lateral+FVector(1050,0,0);
    Check(Trace(Inside,Inside+FVector(0,0,2000),Hit)&&Hit.ImpactPoint.Z<Inside.Z+650,TEXT("Tunnelling sideways leaves solid rock overhead"));
    Check(Geology->IsOpen(Inside)&&ResolveStart(Inside).Equals(Inside),TEXT("Underground spawn resolver keeps a survivor in excavated air"));
    if(West) {West->WantedTerrainGrid=West->TerrainGrid==80?160:80; while(!West->BuildTerrainSlice()) {}}
    Check(Trace(Top,Bottom,Hit)&&Hit.ImpactPoint.Z<Shaft.Z-3000,TEXT("Switching terrain detail does not seal a shaft"));
    for(FIntPoint C:{FIntPoint(-3,1),FIntPoint(-2,1)}) if(auto* R=Regions.FindRef(C)) {R->Destroy(); Regions.Remove(C); EnsureRegion(C);}
    Check(Trace(Top,Bottom,Hit)&&Hit.ImpactPoint.Z<Shaft.Z-3000&&!Trace(Lateral,Inside,Hit),TEXT("Forest cuts reconstruct on both sides after unloading and regenerating the regions"));

    // Discover a sealed, seeded gallery through previously unaltered ground. Its far branch is outside every pulse.
    FVector Gallery(69600,-16800,0); Gallery.Z=SurfaceHeight(Gallery.X,Gallery.Y); Prepare(Gallery);
    bool Found=false;
    for(int32 I=0;I<5;++I) {
        if(!Trace(Gallery+FVector(0,0,150),Gallery-FVector(0,0,3500),Hit)||!Geology->Blast(Hit,Reward)) break;
        for(int32 Side=0;Side<16;++Side) for(int32 Depth:{660,850,1030}) {
            const double A=Side*2*PI/16;
            FVector Point=Gallery+FVector(970*FMath::Cos(A),970*FMath::Sin(A),-Depth);
            if(Geology->IsOpen(Point)&&Trace(Point,Point-FVector(0,0,850),Hit)&&Geology->CanBore(Hit)) Found=true;
        }
        if(Found) break;
    }
    Check(Found,TEXT("Breaking into seeded wilderness geology exposes an explorable natural gallery beyond the blast radius"));
    P->SetActorLocation(Inside); State->WinchAnchor=Shaft+FVector(-1700,0,115);
    P->Winch();
    Check(P->GetActorLocation().Z>Shaft.Z-150&&P->GetActorLocation().Z< Shaft.Z+1800,TEXT("Winch finds a supported surface and retrieves the survivor from a deep drift"));
    P->ToggleBorer(); Check(!P->Borer&&P->Gun->GetStaticMesh()!=nullptr,TEXT("Stowing the borer restores the selected firearm"));
    if(FParse::Param(FCommandLine::Get(),TEXT("ETWriteMineQA"))) {
        State->Position=Inside; State->View=FRotator(0,0,0); State->Health=100; State->Hydration=100;
        Check(UGameplayStatics::SaveGameToSlot(State,TEXT("EARTHtotaled-QA-Faultline"),0),TEXT("Wrote the isolated underground restart fixture"));
    }
}

void AETWorld::CheckMineRestart(AETCharacter* P)
{
    static int32 Frames=0;
    ++Frames;
    const FString Image=FPaths::ProjectSavedDir()/TEXT("faultline-restart.png");
    if(Frames==1) {P->ToggleBorer(); return;}
    if(Frames==45) FScreenshotRequest::RequestScreenshot(Image,true,false);
    if(Frames<46||(!IFileManager::Get().FileExists(*Image)&&Frames<210)) return;
    const FVector Here=P->GetActorLocation(),Saved=State->Position;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ETMineResume),false,P);
    FHitResult Floor,Roof;
    const bool FloorOK=GetWorld()->LineTraceSingleByChannel(Floor,Here,Here-FVector(0,0,700),ECC_Pawn,Params)&&Geology->CanBore(Floor);
    const bool RoofOK=GetWorld()->LineTraceSingleByChannel(Roof,Here,Here+FVector(0,0,800),ECC_Visibility,Params)&&Geology->CanBore(Roof);
    const bool OK=FVector::Dist2D(Here,Saved)<100&&Here.Z<SurfaceHeight(Here.X,Here.Y)-1400&&Geology->IsOpen(Here)&&
        FloorOK&&RoofOK&&State->MinedVeins.Contains({0,0,3})&&State->MinedVeins.Contains({0,0,4})&&State->BoreCuts.Num()>12&&State->Inventory.FindRef(TEXT("goldore"))>=2;
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("ET_MINE_RESTART %s\nDepth %.1f m / floor %d / roof %d / saved edits %d / depleted veins %d / gold ore %d\n"),
        OK?TEXT("PASS"):TEXT("FAIL"),(SurfaceHeight(Here.X,Here.Y)-Here.Z)*.01,FloorOK,RoofOK,State->BoreCuts.Num(),State->MinedVeins.Num(),State->Inventory.FindRef(TEXT("goldore"))),
        *(FPaths::ProjectSavedDir()/TEXT("faultline-restart.txt")));
    FPlatformMisc::RequestExitWithStatus(true,OK?0:1);
}

void AETWorld::WalkMineCheck(AETCharacter* P)
{
    // Drive the same movement component as WASD in a disposable rendered review.
    static const double Start=FPlatformTime::Seconds();
    static double AtFace=0, MinZ=0;
    static bool Photographed=false, Returning=false;
    const FVector Here=P->GetActorLocation();
    const double Now=FPlatformTime::Seconds();
    MinZ=FMath::Min(MinZ,Here.Z);
    const FString Image=FPaths::ProjectSavedDir()/TEXT("survey-drift.png");
    if(!AtFace&&Here.X>=3430&&Here.Z<-700) AtFace=Now;
    if(AtFace&&!Returning) {
        P->GetController()->SetControlRotation((FVector(3990,-2735,-835)-P->Camera->GetComponentLocation()).Rotation());
        if(Now-AtFace>.8&&!Photographed) {FScreenshotRequest::RequestScreenshot(Image,true,false); Photographed=true;}
        Returning=Photographed&&(IFileManager::Get().FileExists(*Image)||Now-AtFace>4);
    }
    if(!AtFace||Returning) {
        const FVector Direction=Returning?-FVector::ForwardVector:FVector::ForwardVector;
        P->GetController()->SetControlRotation(FRotator(Returning?8:-14,Returning?180:0,0));
        P->AddMovementInput(Direction,1);
    }
    const bool Done=Returning&&Here.X<=480&&Here.Z>40;
    if(!Done&&Now-Start<48) return;
    FFileHelper::SaveStringToFile(FString::Printf(TEXT("ET_MINE_WALK %s\n%.1f s / reached chamber %d / returned %d / deepest %.2f m / final %.0f %.0f %.0f\n"),
        Done?TEXT("PASS"):TEXT("FAIL"),Now-Start,AtFace>0,Done,-MinZ*.01,Here.X,Here.Y,Here.Z),*(FPaths::ProjectSavedDir()/TEXT("mine-walk.txt")));
    FPlatformMisc::RequestExitWithStatus(true,Done?0:1);
}
