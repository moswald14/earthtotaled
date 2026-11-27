#include "ETGame.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformMisc.h"
#include "Materials/MaterialInterface.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Sound/SoundWave.h"
#include "HAL/PlatformTime.h"
#include "EngineUtils.h"
#include "Engine/StaticMesh.h"
#include "Engine/Texture2D.h"
#include "HAL/IConsoleManager.h"
#include "ProceduralMeshComponent.h"

// Opt-in cooked-game integration checks. A fresh transient save is used; player saves are untouched.
void AETWorld::RunSelfTest()
{
    int32 Count=0,Failed=0; FString Report;
    auto Check=[&](bool OK,const FString& Label) {
        ++Count; if(!OK) ++Failed;
        Report+=FString::Printf(TEXT("%s %s\n"),OK?TEXT("PASS"):TEXT("FAIL"),*Label);
    };
    auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    Check(P!=nullptr,TEXT("Native first-person character spawned"));
    if(!P) {FPlatformMisc::RequestExitWithStatus(false,1); return;}
    RunAtmosphereChecks(Check);
    Check(Weapons.Num()==10&&Perks.Num()==18&&Quests.Num()==5,TEXT("Ten weapons, original perk web and quests loaded in game"));
    int32 Baked=0; for(TActorIterator<AActor> It(GetWorld());It;++It) if(It->ActorHasTag(TEXT("ET_BakedStart"))) ++Baked;
    Check(Baked>=250,TEXT("Pregenerated starting architecture is present in the cooked map"));
    Check(Supplies.Num()==11&&Recipes.Num()==6&&LootTables.Num()==6,TEXT("Supplies, mined ores, workbench recipes and container tables loaded"));
    Check(VoiceLines.Num()==20,TEXT("Twenty spoken character lines have subtitle data"));
    Check(Speak(TEXT("nora"))&&HasSubtitle()&&SubtitleSpeaker==TEXT("Nora"),TEXT("Human dialogue starts its matching speaker subtitle"));
    auto VoiceComponent=DialogueAudio;
    Check(!Speak(TEXT("nora")),TEXT("Repeated dialogue click does not immediately restart the same clip"));
    Check(Speak(TEXT("salt"))&&DialogueAudio==VoiceComponent&&DialogueAudio&&DialogueAudio->bIsUISound&&!DialogueAudio->bAutoDestroy,TEXT("Robot reply reuses one audio channel that works during paused conversations"));
    bool Prepared=true; for(auto It:Regions) Prepared&=It.Value->Stage==6;
    Check(Prepared&&Regions.Num()>9,TEXT("Starting view is fully populated before player control"));
    auto* Scale=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"));
    Check(Scale->GetFloat()==125,TEXT("New survivors start at 125 percent resolution"));
    P->OpenPanel(TEXT("pause"),TEXT("test"),TEXT("test"));
    for(int Value:{150,175,200,100,125}) {
        P->Action(TEXT("render"));
        Check(State->RenderScale==Value&&Scale->GetFloat()==Value,FString::Printf(TEXT("Pause applies %d percent rendering"),Value));
    }
    P->ClosePanel();
    for(const TCHAR* Surface:{TEXT("Ground"),TEXT("Concrete"),TEXT("Bark"),TEXT("Asphalt"),TEXT("Steel"),TEXT("Carapace"),TEXT("AlloyHull")})
        for(const TCHAR* Channel:{TEXT("Normal"),TEXT("BaseColor"),TEXT("Roughness")}) {
            auto* T=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/EARTHtotaled/Art/T_Relief%s_%s"),Surface,Channel));
            Check(T&&T->GetSizeX()==4096&&T->GetSizeY()==4096,FString::Printf(TEXT("Cooked 4K surface / %s %s"),Surface,Channel));
        }
    auto Fine=Regions.FindRef(FIntPoint(-1,-1)); auto Coarse=Regions.FindRef(FIntPoint(-2,-1));
    Check(Fine&&Fine->TerrainGrid==160&&Fine->Terrain->GetProcMeshSection(0)->ProcIndexBuffer.Num()==153600,TEXT("Nearby terrain has 51200 triangles per tile, sixteen times original detail"));
    Check(Coarse&&Coarse->TerrainGrid==80,TEXT("Distant terrain retains four times original detail with lower cost"));
    if(Fine&&Coarse) {
        bool Seam=true; auto F=Fine->Terrain->GetProcMeshSection(0); auto C=Coarse->Terrain->GetProcMeshSection(0);
        for(int32 Y=0;Y<=80;++Y) Seam&=FMath::IsNearlyEqual(F->ProcVertexBuffer[Y*2*161].Position.Z,C->ProcVertexBuffer[Y*81+80].Position.Z,.01);
        Check(Seam,TEXT("High and lower terrain detail meet without cracks"));
    }
    for(int32 I=0;I<10;++I) {
        const FString Asset=FString::Printf(TEXT("/Game/EARTHtotaled/Art/SM_Weapon%d.SM_Weapon%d"),I,I);
        auto Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);
        Check(Mesh&&Mesh->GetNumTriangles(0)>3000&&Mesh->GetStaticMaterials().Num()>=3,FString::Printf(TEXT("Weapon %d retains detailed geometry and separate material surfaces"),I));
    }
    for(const TCHAR* Name:{TEXT("SM_PineB"),TEXT("SM_Birch"),TEXT("SM_Fern"),TEXT("SM_Shrub"),TEXT("SM_FallenLog"),TEXT("SM_Stump"),TEXT("SM_Rubble"),TEXT("SM_WallRuin"),TEXT("SM_Rebar")}) {
        auto M=LoadObject<UStaticMesh>(nullptr,*(FString(TEXT("/Game/EARTHtotaled/Art/"))+Name+TEXT(".")+Name));
        Check(M&&M->GetNumTriangles(0)>20,FString(Name)+TEXT(" environmental detail is present in the cooked game"));
    }
    for(const TCHAR* Name:{TEXT("Ground"),TEXT("Concrete"),TEXT("Steel"),TEXT("Wood")}) {
        auto T=LoadObject<UTexture2D>(nullptr,*(FString(TEXT("/Game/EARTHtotaled/Art/T_"))+Name+TEXT("_BaseColor.T_")+Name+TEXT("_BaseColor")));
        Check(T&&T->GetSizeX()==2048&&T->GetSizeY()==2048,FString(Name)+TEXT(" surface uses a 2048px texture"));
    }
    for(const TCHAR* Name:{TEXT("Concrete"),TEXT("Bark"),TEXT("Pine"),TEXT("Steel"),TEXT("Wood"),TEXT("Rust"),TEXT("DarkMetal"),TEXT("Rubber"),TEXT("Glass"),TEXT("Skin")}) {
        auto M=Material(Name);
        Check(M&&M->CheckMaterialUsage_Concurrent(MATUSAGE_Nanite)&&M->CheckMaterialUsage_Concurrent(MATUSAGE_InstancedStaticMeshes),FString(Name)+TEXT(" material supports Nanite and instanced rendering"));
    }
    FCollisionQueryParams Params; Params.AddIgnoredActor(P); FHitResult Hit;
    Check(!GetWorld()->SweepSingleByChannel(Hit,FVector(0,-750,110),FVector(0,-290,110),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Params),TEXT("Player capsule fits through actual imported doorway"));
    Check(GetWorld()->LineTraceSingleByChannel(Hit,FVector(150,-700,120),FVector(150,-300,120),ECC_Visibility,Params),TEXT("House wall blocks collision"));
    auto PC=Cast<APlayerController>(P->GetController());
    for(const TCHAR* Panel:{TEXT("bench"),TEXT("skills"),TEXT("contact"),TEXT("journal"),TEXT("pause"),TEXT("inventory")}) {
        P->OpenPanel(Panel,Panel,TEXT("verification")); P->OpenPanel(Panel,Panel,TEXT("refresh"));
        Check(PC->bShowMouseCursor&&PC->IsLookInputIgnored(),FString(Panel)+TEXT(" exposes cursor"));
        const float Hydration=State->Hydration,Time=State->Elapsed; P->Tick(10);
        Check(State->Hydration==Hydration&&State->Elapsed==Time,FString(Panel)+TEXT(" pauses survival and time"));
        P->ClosePanel();
        Check(!PC->bShowMouseCursor&&!PC->IsLookInputIgnored(),FString(Panel)+TEXT(" restores mouse look after repeated opens"));
        auto V=GetWorld()->GetGameViewport();
        Check(V&&V->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown,FString(Panel)+TEXT(" restores permanent capture"));
    }
    State->Hydration=100; State->Water=3; State->Elapsed=1; P->Tick(1000);
    Check(FMath::IsNearlyEqual(State->Hydration,45.f,.01f),TEXT("Water drains during exploration"));
    P->Drink(); Check(State->Water==2&&FMath::IsNearlyEqual(State->Hydration,90.f),TEXT("Drinking consumes a bottle and restores hydration"));
    State->Hydration=0; State->Health=100; State->Armor=40; P->Tick(2);
    Check(State->Health<100&&State->Armor==40,TEXT("Dehydration threatens life and bypasses armor"));
    State->Elapsed=0; float StartHour=Hour(); State->Elapsed=ETDay::Period;
    Check(FMath::IsNearlyEqual(StartHour,Hour(),.001f),TEXT("74 minute cycle returns to same daylight hour"));
    State->ClockOffset=ETDay::At(16.5); State->Elapsed=1080; Check(Hour()>=22&&Hour()<=24,TEXT("Night follows dusk"));
    int32 WaterBefore=State->Water;
    auto Robot=GetWorld()->SpawnActor<AETAgent>(FVector(2500,500,40),FRotator::ZeroRotator); Robot->Configure(this,TEXT("verify-robot"),TEXT("robot")); Robot->TakeDamage(1000,FDamageEvent(),PC,P);
    Check(State->Water==WaterBefore&&State->DropKinds.FindRef(TEXT("verify-robot-drop"))==TEXT("gold"),TEXT("Robot kill drops candybar wrappers and never water"));
    AETItem* WrapperDrop=nullptr;
    for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Id==TEXT("verify-robot-drop")) {WrapperDrop=*It; break;}
    Check(WrapperDrop&&WrapperDrop->Mesh->GetStaticMesh()&&WrapperDrop->Mesh->GetStaticMesh()->GetName()==TEXT("SM_Wrappers"),TEXT("Legacy currency drop kind displays the folded wrapper pickup"));
    auto DropRegion=Regions.FindRef(FIntPoint(0,0));
    Check(WrapperDrop&&DropRegion&&WrapperDrop->GetOwner()==DropRegion&&DropRegion->Residents.Contains(WrapperDrop),TEXT("Unowned story guard assigns its wrappers to the current terrain region"));
    if(DropRegion) {DropRegion->Destroy(); Regions.Remove(FIntPoint(0,0)); EnsureRegion(FIntPoint(0,0));}
    if(auto RestoredRegion=Regions.FindRef(FIntPoint(0,0))) RestoreDrops(RestoredRegion);
    int32 DropCopies=0; WrapperDrop=nullptr;
    for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Id==TEXT("verify-robot-drop")) {++DropCopies; WrapperDrop=*It;}
    Check(DropCopies==1&&WrapperDrop&&WrapperDrop->GetOwner()==Regions.FindRef(FIntPoint(0,0)),TEXT("Uncollected story-guard wrappers return exactly once after unload, regeneration and repeated restore"));
    State->Gold=32;
    if(WrapperDrop) {
        P->Focus=WrapperDrop;P->Interact();
        Check(State->Gold==32&&IsValid(WrapperDrop),TEXT("Standing interaction leaves defeated-enemy loot on the ground"));
        const FVector Last=P->GetActorLocation();P->SetActorLocation(WrapperDrop->GetActorLocation()+FVector(0,0,68));
        P->CrouchLoot();P->bIsCrouched=true;P->TickSweep(.2f);P->ReleaseLoot();P->bIsCrouched=false;P->SetActorLocation(Last);
    }
    Check(State->Gold==48&&State->Consumed.Contains(TEXT("verify-robot-drop"))&&State->Water==WaterBefore,TEXT("Robot wrappers add to an existing balance without adding water"));
    // A roaming encounter can cross the boundary of the region that spawned it.
    auto OldHome=Regions.FindRef(FIntPoint(0,0)); auto CurrentHome=Regions.FindRef(FIntPoint(1,0));
    auto Roaming=GetWorld()->SpawnActor<AETAgent>(FVector(33000,500,Height(33000,500)+30),FRotator::ZeroRotator);
    Roaming->SetOwner(OldHome); if(OldHome) OldHome->Residents.Add(Roaming);
    Roaming->Configure(this,TEXT("verify-roaming-robot"),TEXT("robot")); Roaming->TakeDamage(1000,FDamageEvent(),PC,P);
    AETItem* RoamingDrop=nullptr; for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Id==TEXT("verify-roaming-robot-drop")) {RoamingDrop=*It; break;}
    Check(RoamingDrop&&CurrentHome&&RoamingDrop->GetOwner()==CurrentHome&&CurrentHome->Residents.Contains(RoamingDrop),TEXT("Roaming robot drop belongs to the death location rather than its old home region"));
    if(OldHome) {OldHome->Destroy(); Regions.Remove(FIntPoint(0,0)); EnsureRegion(FIntPoint(0,0));}
    Check(IsValid(RoamingDrop),TEXT("Unloading a robot's old home does not remove loot from the still-loaded destination"));
    P->Contact=TEXT("SALT-7"); const int32 StockBefore=State->BuildStock; P->Action(TEXT("kits"));
    Check(State->Gold==28&&State->BuildStock==StockBefore+6,TEXT("Greenline exchanges twenty saved wrappers for six usable building kits"));
    State->Gold=19; P->Action(TEXT("kits"));
    Check(State->Gold==19&&State->BuildStock==StockBefore+6,TEXT("Insufficient wrappers cannot buy building kits or overdraw the balance"));
    P->Contact=TEXT("Refuge Keeper"); State->Scrap=24; State->Water=0;
    P->Action(TEXT("supply-run")); P->Action(TEXT("supply-run"));
    Check(State->Water==4&&State->Scrap==0,TEXT("Repeatable human supply runs exchange scavenged scrap for water"));
    EnsureRegion(FIntPoint(5,0)); EnsureRegion(FIntPoint(-51,47));
    Check(Regions.Contains(FIntPoint(5,0))&&Regions.Contains(FIntPoint(-51,47)),TEXT("New regions generate far beyond starting map in positive and negative directions"));
    Check(InCity(FVector(160000,0,0))&&!InCity(FVector(70000,0,0)),TEXT("Forest separates recurring city districts"));
    int32 Floors=0;
    for(auto A:Regions[FIntPoint(5,0)]->Residents) if(auto ItemActor=Cast<AETItem>(A)) if(ItemActor->Label.Contains(TEXT("Upstairs"))||ItemActor->Label.Contains(TEXT("Top floor"))) ++Floors;
    Check(Floors>0,TEXT("Procedural city has upper-floor loot"));
    for(const auto& It:Sounds) {
        auto Sound=Cast<USoundWave>(It.Value);
        Check(Sound&&Sound->GetDuration()>0,It.Key+TEXT(" sound is present and playable in packaged content"));
        if(It.Key==TEXT("ForestDay")||It.Key==TEXT("ForestNight")||It.Key==TEXT("CityWind")) Check(Sound&&Sound->bLooping,It.Key+TEXT(" ambience is configured to loop"));
    }
    State->Inventory.Add(TEXT("cloth"),4); State->Inventory.Add(TEXT("chemicals"),2); State->Scrap=0;
    P->OpenPanel(TEXT("bench"),TEXT("test"),TEXT("test")); P->Craft(TEXT("bandage"));
    Check(State->Inventory.FindRef(TEXT("bandage"))==1&&State->Inventory.FindRef(TEXT("cloth"))==2&&State->Inventory.FindRef(TEXT("chemicals"))==1,TEXT("Crafting consumes exact ingredients and produces usable item"));
    P->Craft(TEXT("armorplate")); Check(State->Inventory.FindRef(TEXT("armorplate"))==0&&State->Scrap==0,TEXT("Insufficient ingredients cannot produce free items"));
    P->ClosePanel(); P->Inventory(); State->Health=40; const int OldMeds=State->Meds;
    P->Action(TEXT("use:bandage")); ++State->Inventory[TEXT("bandage")]; P->Action(TEXT("use:bandage"));
    Check(State->Health==40&&State->Meds==OldMeds+1&&State->Inventory.FindRef(TEXT("bandage"))==0,TEXT("Two bandages make one medkit and do not directly heal"));
    P->ClosePanel();
    const int32 LootWater=State->Water; Loot(P,TEXT("verify-v11-cache"),TEXT("relief")); const int32 LootScrap=State->Scrap;
    const int32 OnceWater=State->Water; Loot(P,TEXT("verify-v11-cache"),TEXT("relief"));
    Check(State->Scrap==LootScrap&&State->Water==OnceWater&&State->Consumed.Contains(TEXT("verify-v11-cache")),TEXT("Looting the same stable container ID cannot duplicate rewards"));
    State->Unlocked={0,6,7,8,9};
    for(int32 I=6;I<10;++I) {P->SelectWeapon(I); const int32 Ammo=State->Ammo[I]; P->Shoot(); Check(P->Gun->GetStaticMesh()&&State->Ammo[I]==Ammo-1,Weapons[I].Name+TEXT(" equips its own mesh and fires"));}
    P->SelectWeapon(6); P->NextWeapon(); Check(State->Weapon==7,TEXT("Weapon wheel advances through unlocked guns"));
    P->PreviousWeapon(); Check(State->Weapon==6,TEXT("Weapon wheel returns through unlocked guns"));
    Check(RegionDistance(FIntPoint(-1,-1),FVector(-1,-1,0))==0&&RegionDistance(FIntPoint(0,0),FVector(-300,-400,0))==500,TEXT("Distance streaming handles negative coordinates and diagonal tile bounds"));
    State->ViewDistance=350; const FVector Travel(160000,-6400,CityCenter(160000,0).Z+110);
    P->SetActorLocation(Travel); double MaxSlice=0;
    for(int32 I=0;I<220;++I) {const double T=FPlatformTime::Seconds(); StreamAround(Travel); MaxSlice=FMath::Max(MaxSlice,FPlatformTime::Seconds()-T);}
    UpdateVisibility(Travel);
    Check(Regions.Contains(FIntPoint(5,-1))&&Regions[FIntPoint(5,-1)]->Stage==6,TEXT("Exploration streams and completes the city around the player's position"));
    bool Bounded=true; for(auto It:Regions) Bounded&=RegionDistance(It.Key,Travel)<=35000+8000+RegionSize;
    Check(Bounded&&!Regions.Contains(FIntPoint(-51,47)),TEXT("Distant regions unload with a retention buffer"));
    AETItem* Cache=nullptr; for(auto A:Regions[FIntPoint(5,-1)]->Residents) if(auto I=Cast<AETItem>(A)) if(I->Kind==TEXT("container")) {Cache=I; break;}
    Check(Cache!=nullptr,TEXT("Streamed city landmark has a specialized loot container"));
    if(Cache) {
        const FString Id=Cache->Id; P->Focus=Cache; P->Interact(); Regions[FIntPoint(5,-1)]->Destroy(); Regions.Remove(FIntPoint(5,-1)); EnsureRegion(FIntPoint(5,-1));
        bool Returned=false; for(auto A:Regions[FIntPoint(5,-1)]->Residents) if(auto I=Cast<AETItem>(A)) Returned|=I->Id==Id;
        Check(!Returned,TEXT("Loot stays empty after region unload and regeneration"));
    }
    Report+=FString::Printf(TEXT("INFO Largest generation slice in this run: %.1f ms\n"),MaxSlice*1000);
    const FVector TestP(1000000,0,0); auto Building=Static(TEXT("SM_Tenement"),TEXT("Concrete"),TestP);
    // Upper-floor approaches start on the balcony, inside its protective railing.
    for(int32 Floor=0;Floor<3;++Floor) Check(!GetWorld()->SweepSingleByChannel(Hit,TestP+FVector(0,Floor==0?-850:-610,110+380*Floor),TestP+FVector(0,-290,110+380*Floor),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Params),FString::Printf(TEXT("Imported apartment floor %d entrance fits capsule"),Floor));
    for(int32 Floor=0;Floor<2;++Floor) for(int32 I=0;I<20;++I) {
        const float Top=16+Floor*380+(I+1)*19;
        FVector Tread=TestP+FVector(Floor==0?780:1000,Floor==0?-475+I*50:475-I*50,Top);
        Check(!GetWorld()->LineTraceSingleByChannel(Hit,Tread+FVector(0,0,40),Tread+FVector(0,0,215),ECC_Visibility,Params),FString::Printf(TEXT("Stair flight %d tread %d has headroom"),Floor,I));
    }
    Building->Destroy();
    for(const TCHAR* Mesh:{TEXT("SM_Shop"),TEXT("SM_Warehouse")}) {
        auto Shop=Static(Mesh,TEXT("Concrete"),TestP); float Front=FString(Mesh)==TEXT("SM_Shop")?-500:-900;
        Check(!GetWorld()->SweepSingleByChannel(Hit,TestP+FVector(0,Front-200,110),TestP+FVector(0,Front+200,110),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Params),FString(Mesh)+TEXT(" imported entrance admits player capsule")); Shop->Destroy();
    }
    State->Consumed.Add(TEXT("verify-loot")); TArray<uint8> Bytes; bool Written=UGameplayStatics::SaveGameToMemory(State,Bytes);
    auto Loaded=Written?Cast<UETSave>(UGameplayStatics::LoadGameFromMemory(Bytes)):nullptr;
    Check(Loaded&&Loaded->Consumed.Contains(TEXT("verify-loot"))&&Loaded->Water==State->Water&&Loaded->Elapsed==State->Elapsed,TEXT("Save roundtrip preserves supplies, time and emptied caches"));
    Check(Loaded&&Loaded->Gold==19,TEXT("Wrapper balance retains the legacy serialized currency field"));
    Check(Loaded&&Loaded->MusicVolume==25&&Loaded->WeatherVolume==50,TEXT("Save roundtrip preserves separate music and weather volume choices"));
    Check(Loaded&&ETClimate::Sample(Loaded->Elapsed,Loaded->Seed).Kind==ETClimate::Sample(State->Elapsed,State->Seed).Kind,TEXT("Restoring saved time restores the same weather front"));
    Check(Loaded&&Loaded->Inventory.FindRef(TEXT("cloth"))==State->Inventory.FindRef(TEXT("cloth"))&&Loaded->Ammo.Num()==10&&Loaded->ViewDistance==350&&Loaded->Unlocked.Contains(9),TEXT("Save roundtrip preserves expanded inventory, guns and view distance"));
    RunGeologyChecks(P,Check);
    RunFrontierChecks(P,Check);
    RunOutpostChecks(P,Check);
    RunHomesteadChecks(P,Check);
    RunNavigationChecks(P,Check);
    RunHighwayChecks(P,Check);
    RunSurvivorChecks(P,Check);
    RunReclamationChecks(P,Check);
    RunLandfallChecks(P,Check);
    FString Result=FString::Printf(TEXT("ET_VERIFY %d checks / %d failures\n"),Count,Failed);
    FFileHelper::SaveStringToFile(Result+Report,*(FPaths::ProjectSavedDir()/TEXT("native-verification.txt")));
    UE_LOG(LogTemp,Display,TEXT("%s%s"),*Result,*Report);
    FPlatformMisc::RequestExitWithStatus(true,Failed?1:0);
}
