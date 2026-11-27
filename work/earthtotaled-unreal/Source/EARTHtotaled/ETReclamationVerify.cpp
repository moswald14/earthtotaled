#include "ETGame.h"
#include "ETGeology.h"
#include "ETSpawnSafety.h"
#include "Components/BoxComponent.h"
#include "UnrealClient.h"
#include "Engine/GameViewportClient.h"
#include "InputKeyEventArgs.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "HAL/FileManager.h"

void AETWorld::TickReclamationPhoto(AETCharacter* P)
{
    const double Now=FPlatformTime::Seconds();const FString File=FPaths::ProjectSavedDir()/(TEXT("view-")+PhotoScene+TEXT(".png"));
    if(PhotoScene==TEXT("keys")) {
        const auto* PC=Cast<APlayerController>(P->GetController());auto* V=GetWorld()->GetGameViewport();if(!V||!PC) return;
        auto Key=[&](FKey K,EInputEvent Event){V->InputKey(FInputKeyEventArgs(V->Viewport,FInputDeviceId::CreateFromInternalId(0),K,Event,FPlatformTime::Cycles64()));};
        static bool Crouched=false,Menu=false,Supplied=false;
        if(!PhotoTime) {PhotoTime=Now;return;} if(Now-PhotoTime<.45) return;PhotoTime=Now;
        if(PhotoStage==0) {P->TestInvincible=true;Key(EKeys::LeftControl,IE_Pressed);}
        if(PhotoStage==1) {Crouched=P->SweepHeld&&P->bIsCrouched;Key(EKeys::LeftShift,IE_Pressed);}
        if(PhotoStage==2) Key(EKeys::F10,IE_Pressed);
        if(PhotoStage==3) {
            Menu=P->Panel==TEXT("devtools")&&PC->bShowMouseCursor;const auto Before=State->Gold;
            if(Menu) {P->Action(TEXT("cheat-supply"));Supplied=State->Gold==Before+200;FScreenshotRequest::RequestScreenshot(File,true,false);}
            Key(EKeys::F10,IE_Released);Key(EKeys::LeftShift,IE_Released);Key(EKeys::LeftControl,IE_Released);
        }
        if(PhotoStage==4&&Menu) P->Action(TEXT("back"));
        if(PhotoStage==5) {
            const bool Capture=P->Panel.IsEmpty()&&!PC->bShowMouseCursor&&!PC->IsLookInputIgnored()&&V->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown;
            const bool OK=Crouched&&Menu&&Supplied&&Capture;
            FFileHelper::SaveStringToFile(FString::Printf(TEXT("ET_KEYS %s\nCrouched %d / hidden menu %d / grant %d / gameplay capture %d\n"),OK?TEXT("PASS"):TEXT("FAIL"),Crouched,Menu,Supplied,Capture),*(FPaths::ProjectSavedDir()/TEXT("reclamation-keys.txt")));
            FPlatformMisc::RequestExitWithStatus(true,OK?0:1);
        }
        ++PhotoStage;return;
    }
    if(PhotoStage==0) {
        WeatherOverride=ETClimate::Clear;P->TestInvincible=true;State->IntroRead=true;
        if(!P->Panel.IsEmpty()) {P->Panel.Empty();UGameplayStatics::SetGamePaused(this,false);}
        const bool Tank=PhotoScene==TEXT("cistern"),Town=PhotoScene==TEXT("infected");
        const auto Cell=FIntPoint(1,0);FVector Target=Tank?TowerLocation(Cell):Town?FVector(ETNav::Refuge(Cell),0):FVector(44200,4000,0);
        FVector Stand=Target+(Tank?FVector(1700,-1300,0):Town?FVector(-3400,-2500,0):FVector(540,-540,0));
        StreamAround(Stand,true);Stand.Z=SurfaceHeight(Stand.X,Stand.Y)+110;P->SetActorLocation(Stand);
        Target.Z=SurfaceHeight(Target.X,Target.Y)+(Tank?700:75);P->GetController()->SetControlRotation((Target-Stand-FVector(0,0,65)).Rotation());
        if(PhotoScene==TEXT("workshop")) {P->Pause();P->Workshop();}
        PhotoTime=Now;PhotoStage=1;return;
    }
    if(PhotoStage==1&&Now-PhotoTime>3) {
        if(PhotoScene==TEXT("infected")) for(auto& Entry:TownUnits) if(auto* Z=Entry.Value.Get()) {
            const auto Target=Z->GetActorLocation()+FVector(0,0,100);FVector Stand=Target+FVector(-520,-470,0);
            Stand.Z=SurfaceHeight(Stand.X,Stand.Y)+110;P->SetActorLocation(Stand);P->GetController()->SetControlRotation((Target-Stand-FVector(0,0,60)).Rotation());break;
        }
        PhotoTime=Now;PhotoStage=2;return;
    }
    if(PhotoStage==2&&Now-PhotoTime>1.5) {FScreenshotRequest::RequestScreenshot(File,true,false);PhotoTime=Now;PhotoStage=3;return;}
    if(PhotoStage==3&&(IFileManager::Get().FileSize(*File)>1024||Now-PhotoTime>10)) FPlatformMisc::RequestExitWithStatus(true,IFileManager::Get().FileSize(*File)>1024?0:1);
}

void AETWorld::RunReclamationChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    auto& S=*State; P->Panel.Empty();UGameplayStatics::SetGamePaused(this,false);
    S.ClockOffset=ETDay::At(9);S.Elapsed=0;S.Hydration=100;S.Health=72;S.Shield=60;S.Armor=0;
    Check(ETDay::At(19)-ETDay::At(6)==3120&&ETDay::Period-3120==1320,TEXT("Sun stays up for 52 real minutes and night lasts 22"));
    bool Clock=true;for(double Old:{0.,310.,1441.,5600.,25000.}) Clock&=FMath::IsNearlyEqual(ETDay::Hour(Old+ETDay::FromOld(Old)),FMath::Fmod(16.5+Old/120.,24.),.0001);
    Check(Clock,TEXT("An earlier survivor keeps the same local time across clock migration"));
    P->Hurt(45,TEXT("test"));P->TickShield(9);
    Check(S.Health==72&&S.Shield==15&&S.ShieldDelay==1,TEXT("Shield intercepts hits and waits ten seconds out of combat"));
    P->TickShield(6);P->Hurt(35,TEXT("test"));P->TickShield(40);
    const bool Slow=S.Health==62&&S.Shield==FMath::Min(60.f,S.MaxShield());P->TickShield(60);
    Check(Slow&&S.Health==62&&S.Shield==S.MaxShield(),TEXT("Overflow wounds health and slow shield regeneration never heals it"));
    S.Meds=1;P->Heal();Check(S.Health>62&&S.Meds==0,TEXT("Medkit actively heals a wounded survivor"));
    int Broken=0,Complete=0;bool Stable=true;
    for(int I=-960;I<960;++I) {const int A=ETNav::HighwayDamage(S.Seed,0,I,-2);Broken+=A!=0;Complete+=A==3;Stable&=A==ETNav::HighwayDamage(S.Seed,0,I,-2);}
    Check(Stable&&Broken>90&&Broken<1050&&Complete>0,TEXT("Highway has reproducible torn shoulders and missing spans at negative and positive coordinates"));
    double PeakGrade=0.;
    for(const int Sign:{-1,1}) for(const int Axis:{0,1}) {
        auto RoadZ=[&](int Along) {return Axis?Height(3200.,Sign*Along):Height(Sign*Along,-1700.);};
        for(int Along=191000;Along<287000;Along+=320) PeakGrade=FMath::Max(PeakGrade,FMath::Abs(RoadZ(Along+320)-RoadZ(Along))/320.);
    }
    Check(PeakGrade<.24,TEXT("Highway cuts stay walkable through intercity ridges in both directions and axes"));
    const FIntPoint BeforeCut(FMath::FloorToInt32(271200./6400.),-1);
    const bool AlreadyKept=S.OriginalRoadTerrain.Contains(BeforeCut);S.OriginalRoadTerrain.Add(BeforeCut);
    const bool Preserved=GradeHighway(271200.,-1700.,2468.)==2468.&&GradeHighway(271200.,12000.,1357.)==1357.;
    if(!AlreadyKept) S.OriginalRoadTerrain.Remove(BeforeCut);
    Check(Preserved,TEXT("Existing road excavations and natural hills outside the verge keep their original terrain"));
    FVector BlockAt(52500,63300,0);BlockAt.Z=SurfaceHeight(BlockAt.X,BlockAt.Y)+5000.;
    auto* TestRoof=GetWorld()->SpawnActor<AActor>();
    auto* Solid=NewObject<UBoxComponent>(TestRoof);TestRoof->SetRootComponent(Solid);Solid->SetBoxExtent(FVector(130,130,70));
    Solid->SetCollisionProfileName(TEXT("BlockAll"));Solid->RegisterComponent();TestRoof->SetActorLocation(BlockAt);
    FVector Nearby,Unchanged;
    const bool Recovered=ETSpawn::FindClear(*this,BlockAt,Nearby)&&FVector::Dist(Nearby,BlockAt)>90.&&FVector::Dist(Nearby,BlockAt)<230.;
    Check(Recovered&&ETSpawn::FindClear(*this,Nearby,Unchanged)&&Nearby.Equals(Unchanged),TEXT("A checkpoint embedded in a roof finds nearby standing room and a clear checkpoint stays exact"));
    TestRoof->Destroy();
    bool Art=true;
    for(const TCHAR* M:{TEXT("SM_MoltRunner"),TEXT("SM_MoltSpitter"),TEXT("SM_MoltBulwark"),TEXT("SM_MoltSkimmer"),TEXT("SM_SupertechWarden"),TEXT("SM_SupertechScout"),TEXT("SM_BladePatch"),TEXT("SM_RoadWeed"),TEXT("SM_HollowWalker"),TEXT("SM_HillCistern")}) {
        const FString Path=FString(TEXT("/Game/EARTHtotaled/Art/"))+M+TEXT(".")+M;const auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Path);Art&=Mesh&&Mesh->GetNumTriangles(0)>100;
    }
    Check(Art&&Material(TEXT("Carapace"))&&Material(TEXT("AlloyHull")),TEXT("Ten detailed cooked meshes and both faction surface materials load from the package"));
    const FIntPoint Cell(1,0);const auto Center=FVector2D(Cell)*ETNav::DistrictSpacing;
    auto LoadAt=[&](FVector2D Pos){const FIntPoint R(FMath::FloorToInt(Pos.X/RegionSize),FMath::FloorToInt(Pos.Y/RegionSize));EnsureRegion(R);};
    for(int Y:{-1,1}) for(int X:{-1,1}) LoadAt(Center+FVector2D(X*4500,Y*6500));
    const auto Refuge=ETNav::Refuge(Cell);LoadAt(Refuge);P->SetActorLocation(FVector(Refuge,SurfaceHeight(Refuge.X,Refuge.Y)+100));
    const auto Tank=TowerLocation(Cell);LoadAt(FVector2D(Tank));const int Quota=TownPopulation(Cell);
    Check(TownRemaining({0,0})==0&&Quota>=5&&Quota<=8&&Tank==TowerLocation(Cell)&&FVector2D::Distance(Center,FVector2D(Tank))>30000,TEXT("Shelter 09 stays safe and the neighboring infested town has a seeded hill cistern"));
    S.Towns.Remove(Cell);TownTick=0;TickTowns(P,1);
    AETItem* Board=nullptr;AETItem* Tap=nullptr;
    for(TActorIterator<AETItem> I(GetWorld());I;++I) {if(I->Kind==TEXT("townboard")&&ETNav::District(FVector2D(I->GetActorLocation()))==Cell) Board=*I;if(I->Kind==TEXT("tower")&&FVector::Dist2D(Tank,I->GetActorLocation())<1000) Tap=*I;}
    Check(Board&&Tap&&TownUnits.Num()>=Quota,TEXT("The streamed district contains its board, water tap and actual zombie population"));
    if(Tap) {
        const int Bottles=S.Water;P->Focus=Tap;P->Interact();const int Taken=S.Water;P->Focus=Tap;P->Interact();
        Check(Taken>=Bottles+3&&S.Water==Taken&&S.TowerRefills.FindRef(Cell)>S.Elapsed,TEXT("Cistern dispenses finite bottles then waits for dawn"));
    }
    TArray<TWeakObjectPtr<AETAgent>> Zombies;TownUnits.GenerateValueArray(Zombies);
    for(auto Unit:Zombies) if(Unit.IsValid()&&Unit->TownCell==Cell) Unit->TakeDamage(4000,FDamageEvent(),nullptr,P);
    Check(S.Towns.FindRef(Cell).Claimed&&TownRemaining(Cell)==0,TEXT("Defeating the last real zombie reopens the district"));
    P->TownPanel(Cell);S.Gold=200;P->Action(TEXT("deposit"));P->Action(TEXT("walls"));P->Action(TEXT("sentries"));
    P->Action(TEXT("mark-tower"));P->ClosePanel();
    const auto& T=S.Towns.FindChecked(Cell);int Sentries=0,Walls=0;AETConstruction* Gun=nullptr;
    for(const auto& B:S.Builds) if(FVector2D::Distance(FVector2D(B.Value.Anchor),Refuge)<2500) {
        if(B.Value.Piece==AETConstruction::Turret) {++Sentries;Gun=LiveBuilds.FindRef(B.Key).Get();}
        if(B.Value.Piece==0) ++Walls;
    }
    Check(T.Defenses==2&&T.Treasury==20&&S.Gold==42&&Walls==24&&Sentries==2&&S.MapPins.IsValidIndex(S.TrackedPin)&&S.MapPins[S.TrackedPin].Equals(FVector2D(Tank),2),TEXT("Town board buys 24 wall sections and two sentries with wrappers and pins the exact cistern"));
    if(Gun) {
        FVector At=Gun->Mechanism->GetComponentLocation()+FVector(320,0,-40);auto* Zombie=GetWorld()->SpawnActor<AETAgent>(At,FRotator::ZeroRotator);
        Zombie->Configure(this,TEXT("verify-sentry-zombie"),TEXT("walker"));const float Before=Zombie->Health;
        StepDevices(1);Check(Zombie->Health<Before,TEXT("Powered reclaimed-town sentry hits a living zombie in daylight"));Zombie->Destroy();
    }
    S.Elapsed=T.Upkeep+.1f;TownTick=0;TickTowns(P,1);Check(S.Towns.FindRef(Cell).Treasury==12&&TownRemaining(Cell)==0,TEXT("Paid watch consumes exactly eight wrappers per shift"));
    const FString File=FPaths::ProjectSavedDir()/TEXT("reclamation.etsave");FString Why;
    const bool Stored=P->ExportProfile(State,File,Why);const auto* Read=Stored?P->ReadProfile(File,Why):nullptr;
    Check(Read&&Read->Towns.FindRef(Cell).Treasury==12&&Read->TowerRefills.Contains(Cell)&&Read->Shield==S.Shield,TEXT("Portable save round trip retains watch funds, tower refill and shield"));
    S.Elapsed=S.Towns[Cell].Upkeep+721;TownTick=0;TickTowns(P,1);Check(!S.Towns[Cell].Claimed&&S.Towns[Cell].Wave==1&&TownRemaining(Cell)>0,TEXT("An unpaid watch allows another infestation"));
    FString Raw;const auto Kit=FPaths::ProjectContentDir()/TEXT("ModWorkshop");
    FETModPack Candidate;bool Starter=FFileHelper::LoadFileToString(Raw,*(Kit/TEXT("starter.earthmod.json")))&&ParseMod(Raw,Candidate,Why);
    Check(Starter&&Candidate.Finds.Num()==2&&IFileManager::Get().FileExists(*(Kit/TEXT("mod.schema.json"))),TEXT("Shipped starter and author schema are readable through the installed workshop"));
    if(Starter) {
        const auto Id=TEXT("qa-")+FGuid::NewGuid().ToString(EGuidFormats::Digits).Left(12).ToLower();Raw.ReplaceInline(TEXT("siltworks"),*Id);
        Check(InstallMod(Raw,Why)&&!InstallMod(Raw,Why),TEXT("Workshop imports a valid mod and rejects a duplicate without replacing it"));
        const auto Key=TEXT("mod.")+Id+TEXT(".cistern-seal");const auto Qty=S.Inventory.FindRef(Key);FString Totals;ModCache(TEXT("qa-silt-cache"),Totals);
        Check(S.Inventory.FindRef(Key)==Qty+1&&ModFind(Key),TEXT("Installed cache find uses its authored drop rule"));
        P->Panel.Empty();P->Pause();P->Action(TEXT("mods"));P->ModPage=Mods.IndexOfByPredicate([&](const FETModPack& M){return M.Id==Id;});
        const int Wallet=S.Gold;P->Action(TEXT("redeem"));P->Action(TEXT("toggle"));ModCache(TEXT("another-cache"),Totals);
        Check(S.Gold==Wallet+6&&S.Inventory.FindRef(Key)==0,TEXT("Actual workshop redeems the find and disabling its pack stops new drops"));
        P->Action(TEXT("back"));Check(P->Panel==TEXT("pause"),TEXT("Workshop returns to the pause menu"));P->ClosePanel();
        IFileManager::Get().Delete(*(ModFolder()/(Id+TEXT(".disabled"))));IFileManager::Get().Delete(*(ModFolder()/(Id+TEXT(".earthmod.json"))));LoadMods();
        Check(!ParseMod(Raw.Replace(TEXT("\"scrap\""),TEXT("\"water\"")),Candidate,Why)&&!ParseMod(TEXT("{\"schema\":1}"),Candidate,Why),TEXT("Malformed packs and water from a Supertech drop are rejected"));
    }
    const FVector Spot(281000,31000,7000);P->SetActorLocation(Spot);P->Panel.Empty();P->CrouchLoot();
    const bool Original=P->bIsCrouched;P->bIsCrouched=true;const int Gold=S.Gold;
    auto* Far=LooseItem(TEXT("qa-sweep-far"),TEXT("gold"),3,Spot+FVector(3000,0,0),Spot+FVector(3000,0,0),false);
    P->TickSweep(6);Check(Far&&S.Gold==Gold&&!P->SweepArmed,TEXT("Crouching far from loose loot cannot start a pickup sweep"));
    auto* Near=LooseItem(TEXT("qa-sweep-near"),TEXT("gold"),2,Spot+FVector(90,0,0),Spot+FVector(90,0,0),false);
    P->TickSweep(.2);P->TickSweep(2);const bool Partial=P->SweepArmed&&S.Gold==Gold+2&&IsValid(Far);
    P->TickSweep(3);Check(Near&&Partial&&S.Gold==Gold+5&&!S.DropPositions.Contains(TEXT("qa-sweep-far")),TEXT("Holding Ctrl beside a drop grows the sweep to 36 m and collects distant loot once"));
    P->ReleaseLoot();P->bIsCrouched=Original;
}
