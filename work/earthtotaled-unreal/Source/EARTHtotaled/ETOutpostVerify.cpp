#include "ETGame.h"
#include "ETGeology.h"
#include "ETTerrainMath.h"
#include "Camera/CameraComponent.h"
#include "Camera/CameraTypes.h"
#include "Engine/Texture2D.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

void AETWorld::RunOutpostChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    auto* S=State;
    S->ClockOffset=ETDay::At(16.5);S->Elapsed=1080; UpdateNightWatch(P->GetActorLocation());
    P->Panel.Empty(); UGameplayStatics::SetGamePaused(P,false);
    S->Health=1; S->Shield=0; S->Armor=0; S->Hydration=97; S->Water=0;
    P->Drink(); Check(Notice.Contains(TEXT("0 sealed bottles"))&&Notice.Contains(TEXT("97%")),TEXT("Empty pack at 97% reports bottle count separately from hydration"));
    P->Hurt(10,TEXT("Fall impact"));
    Check(P->Panel==TEXT("death")&&P->PanelBody.Contains(TEXT("Fall impact"))&&P->PanelBody.Contains(TEXT("97%"))&&!P->PanelBody.Contains(TEXT("Dehydration")),TEXT("Lethal fall at 97% names the impact and correct hydration"));
    P->Panel.Empty(); UGameplayStatics::SetGamePaused(P,false); S->Health=.01f; S->Hydration=0; P->Tick(.05f);
    Check(P->Panel==TEXT("death")&&P->PanelBody.Contains(TEXT("Dehydration")),TEXT("Zero hydration really can kill and reports dehydration"));
    P->Panel.Empty(); UGameplayStatics::SetGamePaused(P,false); P->Command(TEXT("heal"));
    const int32 Before=S->Inventory.FindRef(TEXT("ironore")); P->Command(TEXT("give iron -500"));
    Check(S->Inventory.FindRef(TEXT("ironore"))==Before&&P->Command(TEXT("status")).Contains(TEXT("hydration")),TEXT("Admin status works and rejects invalid quantities"));
    Check(UETGeology::Grade(1500)==1&&UETGeology::Grade(1800)==2&&UETGeology::Grade(4900)==4,TEXT("Recoverable ore increases every 16 metres to the depth cap"));
    FVector Deep(-62960,41400,0); Deep.Z=SurfaceHeight(-64010,41400)-2300;
    EnsureRegion({-2,1}); EnsureRegion({-3,1}); P->SetActorLocation(Deep); P->GetController()->SetControlRotation(FRotator(0,180,0));
    State->NextPatrol=0; PatrolTick=0; TickPatrols(P,1);
    bool Under=false; for(auto It:LiveThreats) if(IsValid(It.Value)&&It.Value->Kind==TEXT("bulwark")) {
        const FVector At=It.Value->GetActorLocation(); Under|=SurfaceHeight(At.X,At.Y)-At.Z>1400&&Geology->IsOpen(At+FVector(0,0,60));
    }
    Check(Under,TEXT("Armored underground brood actually spawns inside a walkable deep tunnel"));
    int32 DeepIron=0,DeepGold=0; const FVector DeepRig(Deep.X,Deep.Y,SurfaceHeight(Deep.X,Deep.Y)-7200);
    Check(Geology->ExtractNear(DeepRig,DeepIron,DeepGold)&&(DeepIron>=15||DeepGold>=5),TEXT("Mining a real deep deposit pays a higher grade into the hopper"));
    const FVector Field(-15700,50800,0);
    EnsureRegion({-1,1});
    S->Inventory.FindOrAdd(TEXT("ironore"))=45; S->Inventory.FindOrAdd(TEXT("goldore"))=16;
    if(!P->Building) P->ToggleBuild();
    auto Place=[&](int32 Piece,int32 StartX)->AETConstruction* {
        P->BuildPiece=Piece; P->BuildFacing=0;
        for(int32 X=0;X<6;++X) for(int32 Y=0;Y<6;++Y) {
            FVector At(Field.X+StartX+X*400,Field.Y+Y*400,0); At.X=FMath::GridSnap(At.X,400.); At.Y=FMath::GridSnap(At.Y,400.);
            At.Z=SurfaceHeight(At.X,At.Y); P->SetActorLocation(At+FVector(0,0,660)); P->GetController()->SetControlRotation(FRotator(-89.9f,0,0));
            FMinimalViewInfo View; P->Camera->GetCameraView(0,View);
            FETBuild Candidate; FString Why; if(!P->BuildTarget(Candidate,Why)) continue;
            const int32 Old=S->Builds.Num(); P->PlaceBuild();
            if(S->Builds.Num()>Old) for(const auto& It:LiveBuilds) if(auto* A=It.Value.Get();A&&A->Design.Piece==Piece&&A->Design.Anchor.Equals(Candidate.Anchor,5)) return A;
        }
        return nullptr;
    };
    auto* Core=Place(AETConstruction::Core,0);
    auto* Tower=Place(AETConstruction::Turret,400);
    auto* Turtle=Place(AETConstruction::Miner,800);
    P->StopBuilding();
    Check(Core&&Tower&&Turtle&&S->Inventory.FindRef(TEXT("ironore"))==36&&S->Inventory.FindRef(TEXT("goldore"))==13,TEXT("Actual B-tool placements consume nine iron and three gold ore for relay, turret and miner"));
    if(Core&&Tower&&Turtle) {
        const FGuid Relay=Core->Id;
        Check(BaseValue(Relay)>=27&&NearestCore(Tower->Design.Anchor,7500)==Relay,TEXT("Outpost value crosses the raid threshold and relay powers the sentry"));
        auto& Miner=S->Builds[Turtle->Id]; const int32 Mined=S->MinedVeins.Num();
        Miner.Work=29.9f; StepDevices(.4f); const int32 Cargo=Miner.Iron+Miner.GoldOre;
        Check(Cargo>0&&S->MinedVeins.Num()==Mined+1,TEXT("Automatic turtle drill removes a real finite deposit and stores ore"));
        const int32 Pack=S->Inventory.FindRef(TEXT("ironore"))+S->Inventory.FindRef(TEXT("goldore"));
        P->OpenDevice(Turtle); P->Action(TEXT("hopper")); P->Action(TEXT("hopper")); P->ClosePanel();
        Check(!Miner.Iron&&!Miner.GoldOre&&S->Inventory.FindRef(TEXT("ironore"))+S->Inventory.FindRef(TEXT("goldore"))==Pack+Cargo,TEXT("Collect hopper transfers ore exactly once"));
        P->OpenDevice(Core); P->Action(TEXT("plate")); P->Action(TEXT("plate")); P->ClosePanel();
        Check(S->ArmorTier==2&&S->Armor==170&&ThreatTier()>=2,TEXT("Relay armor upgrades raise capacity and unlock stronger raid castes"));
        S->RaidWait=.05f; StepRaid(P,.35f);
        bool Heavy=false,Fly=false; int32 Formation=0;
        for(const auto& It:S->Threats) if(RaidMember(It.Key)) {Heavy|=It.Value.Kind==TEXT("bulwark"); Fly|=It.Value.Kind==TEXT("skimmer"); ++Formation;}
        Check(S->RaidStage==1&&Heavy&&Fly&&Formation>=3,TEXT("Valuable base draws a staged armored formation with a progression-gated flier"));
        const float Remaining=S->RaidTime; const int Serial=S->RaidSerial;
        P->Command(TEXT("time day")); StepRaid(P,45);
        bool Withdrawn=true;
        for(const auto& It:LiveThreats) if(RaidMember(It.Key)&&IsValid(It.Value)) Withdrawn&=It.Value->IsHidden()&&!It.Value->GetActorEnableCollision();
        Check(Withdrawn&&S->RaidTime==Remaining&&S->RaidStage==1&&S->RaidSerial==Serial&&!Muster(Relay),TEXT("Dawn withdraws the raid line and freezes its clock without rewarding a victory"));
        P->Command(TEXT("time night"));
        StepRaid(P,17);
        Check(S->RaidStage==2,TEXT("Muster warning expires and the raid charges"));
        FVector Stand=Tower->Mechanism->GetComponentLocation()+FVector(0,0,10);
        const FVector Spawn=Stand+FVector(380,0,-50);
        auto* Enemy=AddThreat(TEXT("verify-sentry"),TEXT("runner"),Spawn);
        const float HP=Enemy?Enemy->Health:0; S->Builds[Tower->Id].Work=0; StepDevices(.4f);
        Check(Enemy&&Enemy->Health<HP,TEXT("Powered auto turret acquires a visible hostile and deals actual damage"));
        if(Enemy&&IsValid(Enemy)) Enemy->TakeDamage(10000,FDamageEvent(),nullptr,P);
        // Exercise hostile structure damage beside the core while the survivor is out of melee range.
        const FString Siege=FString::Printf(TEXT("raid-%d-probe"),S->RaidSerial);
        auto* Guard=AddThreat(Siege,TEXT("bulwark"),Core->Design.Anchor+FVector(110,0,8));
        P->SetActorLocation(Core->Design.Anchor+FVector(-840,0,105));
        const float Integrity=S->Builds[Relay].Integrity;
        if(Guard) {Guard->AttackTime=0; Guard->Tick(.1f);}
        Check(S->Builds[Relay].Integrity<Integrity,TEXT("Charging carapace creature physically damages the relay"));
        TArray<AETAgent*> Wave; for(auto It:LiveThreats) if(RaidMember(It.Key)) Wave.Add(It.Value);
        for(auto* A:Wave) if(IsValid(A)) A->TakeDamage(10000,FDamageEvent(),nullptr,P);
        const int32 Wins=S->Waves; StepRaid(P,.4f);
        Check(S->RaidStage==0&&S->Waves==Wins+1&&S->RaidWait>0,TEXT("Defeating the attackers pays ore, records a defended wave and starts recovery"));
        P->OpenDevice(Core); P->Action(TEXT("hunt")); P->ClosePanel(); AETAgent* Nest=nullptr;
        for(auto It:LiveThreats) if(IsValid(It.Value)&&It.Value->Kind==TEXT("hive")) Nest=It.Value;
        Check(IsValid(Nest),TEXT("Relay traces a real destructible brood nest after a successful defense"));
        if(IsValid(Nest)) {const FString Id=Nest->Id; const int32 Iron=S->Inventory.FindRef(TEXT("ironore"));
            Nest->TakeDamage(10000,FDamageEvent(),nullptr,P);
            const bool Spilled=S->DropKinds.FindRef(Id+TEXT("-drop"))==TEXT("ironore")&&S->DropKinds.FindRef(Id+TEXT("-drop-1"))==TEXT("goldore");
            if(auto* Ore=LooseActors.FindRef(Id+TEXT("-drop")).Get()) P->TakeLoose(Ore);
            Check(Spilled&&S->Consumed.Contains(Id)&&S->Inventory.FindRef(TEXT("ironore"))==Iron+12,TEXT("Clearing nest awards finite bounty and permanently clears its trace"));}
    }
    const double R1=ETMath::Relief(54200,63300,84371);
    Check(R1==ETMath::Relief(54200,63300,84371)&&R1!=ETMath::Relief(54200,63300,5124),TEXT("Landscape ridges and drainage reproduce by seed and vary between survivors"));
    for(const TCHAR* Name:{TEXT("Ground"),TEXT("Bark"),TEXT("Concrete"),TEXT("FaultShale"),TEXT("FaultIron"),TEXT("FaultGold")}) {
        const FString Tex=FString(TEXT("T_Relief"))+Name+TEXT("_Normal");
        auto* T=LoadObject<UTexture2D>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Tex+TEXT(".")+Tex));
        const int Size=FCString::Strncmp(Name,TEXT("Fault"),5)==0?2048:4096;
        Check(T&&T->GetSizeX()==Size&&T->CompressionSettings==TC_Normalmap&&!T->SRGB,FString(TEXT("Cooked tangent-space relief / "))+Name);
    }
    // A distinct full data profile, including map keys and excavations, exercises portable decoding.
    auto* Profile=NewObject<UETSave>(); Profile->ArmorTier=2; Profile->Armor=159; Profile->Hydration=97; Profile->Gold=149;
    Profile->Seed=67911; Profile->Inventory.Add(TEXT("goldore"),17); Profile->Skills.Add(TEXT("parkour")); Profile->BoreCuts.Add({61,32,-18});
    Profile->SquareCuts.Add({3,6,-8}); Profile->MinedVeins.Add({0,0,1}); Profile->LegacyGround.Add({-3,7});
    FGuid Key=FGuid::NewGuid(); FETBuild Rig; Rig.Piece=6; Rig.Work=26; Rig.Iron=13; Rig.Anchor=FVector(-450,860,-240); Profile->Builds.Add(Key,Rig);
    FETThreat Beast; Beast.Kind=TEXT("skimmer"); Beast.Health=33; Beast.AttackCooldown=1.35f; Beast.Position=FVector(50,100,360); Profile->Threats.Add(TEXT("test-flight"),Beast);
    const FString File=FPaths::ProjectSavedDir()/TEXT("outpost-roundtrip.etsave"); FString Msg;
    Check(P->ExportProfile(Profile,File,Msg),TEXT("Occupied survivor exports to transferable file"));
    auto* Restored=P->ReadProfile(File,Msg);
    Check(Restored&&Restored->Builds.FindRef(Key).Iron==13&&Restored->Builds.FindRef(Key).Anchor==Rig.Anchor&&Restored->Threats.FindRef(TEXT("test-flight")).Health==33&&
          FMath::IsNearlyEqual(Restored->Threats.FindRef(TEXT("test-flight")).AttackCooldown,Beast.AttackCooldown)&&
          Restored->MinedVeins.Contains({0,0,1})&&Restored->LegacyGround.Contains({-3,7})&&Restored->BoreCuts==Profile->BoreCuts&&Restored->SquareCuts==Profile->SquareCuts&&
          Restored->ArmorTier==2&&Restored->Gold==149&&Restored->Hydration==97&&Restored->Skills.Contains(TEXT("parkour"))&&Restored->Inventory.FindRef(TEXT("goldore"))==17,TEXT("Imported world retains GUID machinery, cargo, threats, cuts, mining, old surface, personal gear and wrappers"));
    const FString Bad=FPaths::ProjectSavedDir()/TEXT("outpost-damaged.etsave");
    FString Raw; FFileHelper::LoadFileToString(Raw,*File); FFileHelper::SaveStringToFile(Raw.LeftChop(9),*Bad);
    Check(!P->ReadProfile(Bad,Msg),TEXT("Truncated portable save rejected before a slot write"));
    Profile->ArmorTier=9;
    const bool Rejected=!P->ExportProfile(Profile,File,Msg); Profile->ArmorTier=2;
    Check(Rejected&&P->ReadProfile(File,Msg),TEXT("An unsupported world cannot overwrite a good portable backup"));
    TSet<FString> Keys; bool Indep=true;
    for(int I=1;I<=10;++I) {Profile->Gold=I*7; Keys.Add(SlotName(I)); Indep&=P->StoreSlot(Profile,I);}
    for(int I=1;I<=10;++I) {const auto* Slot=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SlotName(I),0)); Indep&=Slot&&Slot->Gold==I*7;}
    Check(Keys.Num()==10&&Indep,TEXT("All ten namespace-isolated disk slots independently save and reload different survivors"));
    Profile->Gold=999; P->StoreSlot(Profile,7);
    const auto* Old=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SlotName(7)+TEXT("-previous"),0));
    Check(Old&&Old->Gold==49,TEXT("Replacing an occupied slot preserves its exact previous survivor"));
}

