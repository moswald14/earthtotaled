#include "ETGame.h"
#include "ETGeology.h"
#include "Camera/CameraComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"

void AETWorld::RunHomesteadChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    using Site=AETConstruction;
    auto* S=State; auto* PC=CastChecked<APlayerController>(P->Controller);
    P->Panel.Empty(); UGameplayStatics::SetGamePaused(this,false); P->Command(TEXT("heal"));
    P->StopBuilding(); if(P->Borer) P->ToggleBorer();
    auto Aim=[&](FVector Eye,FVector Target) {
        P->SetActorLocation(Eye-FVector(0,0,72)); PC->SetControlRotation((Target-Eye).Rotation());
        FMinimalViewInfo V; P->Camera->GetCameraView(0,V);
    };
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETHomestead),false,P);
    auto Trace=[&](FVector Start,FVector End,FHitResult& H){return GetWorld()->LineTraceSingleByChannel(H,Start,End,ECC_Visibility,Q);};
    P->Title(); P->Action(TEXT("controls")); P->Action(TEXT("guide-build"));
    Check(P->Panel==TEXT("controls")&&P->GuidePage==1&&PC->bShowMouseCursor,TEXT("Title opens the clickable building guide"));
    P->Pause(); Check(P->Panel==TEXT("title")&&UGameplayStatics::IsGamePaused(this),TEXT("Escape returns the field guide to its originating main menu"));
    P->Panel.Empty(); P->Pause(); P->Controls(); P->Action(TEXT("guide-back"));
    Check(P->Panel==TEXT("pause"),TEXT("Guide opened from pause returns to settings")); P->ClosePanel();
    P->Controls(); P->Action(TEXT("guide-survive")); P->Controls();
    Check(P->Panel.IsEmpty()&&!PC->bShowMouseCursor&&!PC->IsLookInputIgnored()&&!UGameplayStatics::IsGamePaused(this),TEXT("Field guide returns to captured mouse look and live survival"));

    const FVector Pad(4000,-6400,0); EnsureRegion({0,-1}); P->SetActorLocation(Pad+FVector(0,0,110));
    S->ClockOffset=ETDay::At(16.5); S->Elapsed=599; UpdateNightWatch(Pad);
    auto* Beast=AddThreat(TEXT("verify-watch"),TEXT("spitter"),Pad+FVector(940,0,8));
    auto* Deer=GetWorld()->SpawnActor<AETAgent>(Pad+FVector(730,300,0),FRotator::ZeroRotator);
    Deer->Configure(this,TEXT("verify-day-deer"),TEXT("deer")); AtRefuge=false;
    S->NextPatrol=0; const int Patrol=S->PatrolNumber; PatrolTick=0; TickPatrols(P,1);
    const float HP=Beast->Health; Beast->Tick(1);
    Check(!IsNight()&&Beast->IsHidden()&&!Beast->GetActorEnableCollision()&&!Beast->IsActorTickEnabled()&&
        Beast->TakeDamage(25,FDamageEvent(),PC,P)==0&&Beast->Health==HP&&!Deer->IsHidden()&&Patrol==S->PatrolNumber,TEXT("Before 19:00 hostiles have no collision, combat or new patrols and deer stay out"));
    S->Elapsed=600; UpdateNightWatch(Pad);
    Check(IsNight()&&Beast->Awake()&&Beast->GetActorEnableCollision()&&!Beast->IsHidden()&&Beast->IsActorTickEnabled(),TEXT("At exactly 19:00 the same hostile emerges and can attack"));
    Beast->TakeDamage(11,FDamageEvent(),PC,P);
    auto* Bolt=GetWorld()->SpawnActor<AETEnemyBolt>(Beast->GetActorLocation()+FVector(0,0,70),FRotator::ZeroRotator);
    Bolt->Launch(Beast,P->GetActorLocation(),true);
    S->Elapsed=1919.9f; Check(IsNight(),TEXT("Hostiles remain active immediately before 06:00"));
    S->Elapsed=1920; UpdateNightWatch(Pad); UpdateVisibility(Pad);
    Check(!IsNight()&&!IsValid(Bolt)&&Beast->IsHidden()&&!Beast->GetActorEnableCollision()&&!Deer->IsHidden(),TEXT("At 06:00 hostile shots clear and streaming cannot reveal sleeping enemies"));
    S->Elapsed=5040; UpdateNightWatch(Pad);
    Check(Beast->Awake()&&Beast->Health==HP-11,TEXT("Next night restores the same wounded encounter without resetting health"));
    Deer->Destroy(); Beast->Destroy(); LiveThreats.Remove(TEXT("verify-watch")); S->Threats.Remove(TEXT("verify-watch"));
    P->Command(TEXT("time day"));

    const FIntPoint Forest(2,0); EnsureRegion(Forest);
    auto* Region=Regions.FindChecked(Forest); FVector Stump=FVector::ZeroVector; bool Found=false;
    for(auto* H:Region->Instances) {
        if(Found||!H->GetStaticMesh()||H->GetStaticMesh()->GetFName()!=TEXT("SM_Trunk")) continue;
        for(int I=0;I<H->GetInstanceCount()&&!Found;++I) {
            FTransform T; H->GetInstanceTransform(I,T,true); const FVector Root=T.GetLocation(),Eye=Root+FVector(-140,0,90);
            FHitResult Hit;
            Found=Trace(Eye,Root+FVector(0,0,90),Hit)&&TreeRoot(Hit,Stump)&&Stump.Equals(Root,1);
            if(Found) Aim(Eye,Root+FVector(0,0,90));
        }
    }
    const int Wood=S->Inventory.FindRef(TEXT("wood"));
    if(Found) {P->Tick(.01f); P->Interact(); P->Tick(.01f); P->Interact();}
    const FString Tree=TreeId(Stump);
    Check(Found&&S->Consumed.Contains(Tree)&&S->Inventory.FindRef(TEXT("wood"))==Wood+8,TEXT("Actual forest trunk interaction grants eight wood once"));
    if(Found) {
        Regions.Remove(Forest); Region->Destroy(); EnsureRegion(Forest);
        bool Returned=false;
        for(auto* H:Regions.FindChecked(Forest)->Instances) if(H->GetStaticMesh()&&IsTree(H->GetStaticMesh()->GetFName()))
            for(int I=0;I<H->GetInstanceCount();++I) {FTransform T; H->GetInstanceTransform(I,T,true); Returned|=T.GetLocation().Equals(Stump,1);}
        Check(!Returned,TEXT("Regenerating the forest restores neither crown nor trunk collider"));
    }
    const FVector Quarry(32400,-17600,SurfaceHeight(32400,-17600)); EnsureRegion({1,-1});
    const int Stone=S->Inventory.FindRef(TEXT("stone"));
    Aim(Quarry+FVector(-150,0,245),Quarry); P->ToggleBorer(); P->FireBorer(); P->ToggleBorer();
    Check(S->Inventory.FindRef(TEXT("stone"))==Stone+2,TEXT("A successful equipped borer shot adds two building stone"));

    FString Why; FETBuild Frame; P->ToggleBuild(); P->BuildPiece=1; P->BuildMaterial=0;
    Aim(Pad+FVector(-430,0,250),Pad); P->RefreshBuild();
    S->Inventory.Add(TEXT("wood"),8); S->Inventory.Add(TEXT("stone"),4); S->Inventory.Add(TEXT("ironore"),2);
    FHitResult Earth; const bool HaveEarth=Trace(Pad+FVector(0,0,60),Pad-FVector(0,0,70),Earth)&&Geology->CanBore(Earth);
    const bool Valid=P->BuildTarget(Frame,Why); P->PlaceBuild();
    FGuid Id;
    for(const auto& B:S->Builds) if(B.Value.Anchor.Equals(Frame.Anchor,2)&&B.Value.Piece==1) Id=B.Key;
    Check(Valid&&Id.IsValid()&&S->Inventory.FindRef(TEXT("wood"))==4,TEXT("Wooden floor uses four wood through the normal preview and placement / ")+Why);
    if(Id.IsValid()) {
        const FVector Deck=Frame.Anchor+FVector(0,0,14);
        Aim(Deck+FVector(-80,0,220),Deck); P->UpgradeFrame();
        auto* Floor=LiveBuilds.FindRef(Id).Get();
        Check(S->Builds[Id].MaterialTier==1&&S->Builds[Id].Integrity==320&&S->Inventory.FindRef(TEXT("stone"))==0&&
            Floor->Mesh->GetMaterial(0)==Material(TEXT("Brick")),TEXT("Stone upgrade spends four stone and changes the same floor's surface and integrity"));
        P->UpgradeFrame(); P->UpgradeFrame();
        Check(S->Builds[Id].MaterialTier==2&&S->Builds[Id].Integrity==600&&S->Inventory.FindRef(TEXT("ironore"))==0&&
            Floor->Mesh->GetMaterial(0)==Material(TEXT("Steel")),TEXT("Final metal upgrade spends two iron once and supplies 600 integrity"));
        const FString Portable=FPaths::ProjectSavedDir()/TEXT("homestead-roundtrip.etsave"); FString Note;
        auto* Loaded=P->ExportProfile(S,Portable,Note)?P->ReadProfile(Portable,Note):nullptr;
        Check(Loaded&&Loaded->Builds.FindRef(Id).MaterialTier==2&&!Loaded->Builds.FindRef(Id).FromKit&&
            Loaded->Inventory.FindRef(TEXT("wood"))==4&&Loaded->Consumed.Contains(Tree),TEXT("Portable save preserves upgrades, timber balance and harvested tree identity / ")+Note);
        P->SalvageBuild(); Check(!S->Builds.Contains(Id)&&S->Inventory.FindRef(TEXT("ironore"))==2,TEXT("Salvaging metal returns the paid metal instead of producing a wooden kit"));
    }
    P->CycleMaterial(); P->CycleMaterial(); P->CycleMaterial();
    Check(P->BuildMaterial==0,TEXT("Build material selection cycles wood, stone, metal, wood")); P->StopBuilding();

    auto Build=[&](FVector Anchor,int Piece,int Facing) {
        FETBuild B; B.Anchor=Anchor; B.Piece=Piece; B.Facing=Facing; const FGuid Key=FGuid::NewGuid();
        S->Builds.Add(Key,B); SpawnBuild(Key,B); return Key;
    };
    const FVector Foundation(Pad.X,Pad.Y,4);
    const auto Wall=Build(Foundation,0,0),Bridge=Build(Foundation+FVector(0,0,280),1,0);
    const auto FarFoot=Build(Foundation,0,2),Rig=Build(Foundation+FVector(0,0,297),Site::Turret,0);
    const auto Neighbor=Build(Foundation+FVector(1200,0,0),1,0);
    // The walls support opposite sides of the deck. Removing one leaves the other bearing edge.
    HitBuild(LiveBuilds.FindRef(FarFoot).Get(),10000);
    Check(S->Builds.Contains(Wall)&&S->Builds.Contains(Bridge)&&S->Builds.Contains(Rig),TEXT("A surviving full edge holds the upper deck and turret after an alternate support is removed"));
    auto* Barrel=Cast<AStaticMeshActor>(Static(TEXT("SM_Barrel"),TEXT("Rust"),Pad+FVector(15,0,0)));
    auto* Cache=Item(TEXT("verify-undermined-cache"),TEXT("loot"),TEXT("Test cache"),Pad+FVector(45,0,30));
    auto* Quest=Item(TEXT("verify-safe-quest"),TEXT("quest"),TEXT("Test relay module"),Pad+FVector(-55,-30,25));
    FString Reward; const bool Cut=HaveEarth&&Geology->Blast(Earth,Reward,true);
    Check(!Cut&&S->Builds.Contains(Wall)&&S->Builds.Contains(Bridge)&&S->Builds.Contains(Rig)&&S->Builds.Contains(Neighbor),
        TEXT("Excavation leaves the final footing, upper deck and powered machinery intact"));
    Check(Barrel->GetStaticMeshComponent()->IsVisible()&&IsValid(Cache)&&IsValid(Quest)&&!S->Settled.Contains(Quest->Id),
        TEXT("A denied cut leaves scenery, loot and mission items in place"));
    HitBuild(LiveBuilds.FindRef(Wall).Get(),10000);
    Check(!S->Builds.Contains(Bridge)&&!S->Builds.Contains(Rig)&&S->Builds.Contains(Neighbor),TEXT("Destroying the last support still collapses the attached deck and machine"));
    const FVector Rock(152000,-26750,SurfaceHeight(152000,-26750)); EnsureRegion({4,-1}); FHitResult H;
    auto CutDown=[&](FVector XY) {
        return Trace(XY+FVector(0,0,260),XY-FVector(0,0,950),H)&&Geology->Blast(H,Reward,true);
    };
    bool Ring=true;
    for(FVector Side:{FVector(-400,0,0),FVector(400,0,0),FVector(0,-400,0),FVector(0,400,0)}) Ring&=CutDown(Rock+Side);
    Ring&=CutDown(Rock+FVector(-400,0,0));
    Check(Ring&&Trace(Rock+FVector(0,0,160),Rock-FVector(0,0,850),H)&&H.ImpactPoint.Z>Rock.Z-80,TEXT("A ring of cuts leaves a stone pillar standing while it still reaches bedrock"));
    const int Fallen=S->FallenRock.Num();
    const bool Undercut=Trace(Rock+FVector(-400,0,-400),Rock+FVector(400,0,-400),H)&&Geology->Blast(H,Reward,true);
    Check(Undercut&&S->FallenRock.Num()>Fallen&&Trace(Rock+FVector(0,0,160),Rock-FVector(0,0,1100),H)&&H.ImpactPoint.Z<Rock.Z-350&&
        Geology->IsOpen(Rock-FVector(0,0,55)),TEXT("Undercutting the final rock connection removes the isolated pillar from the rendered terrain and collision"));
    TArray<uint8> Bytes;
    auto* Saved=UGameplayStatics::SaveGameToMemory(S,Bytes)?Cast<UETSave>(UGameplayStatics::LoadGameFromMemory(Bytes)):nullptr;
    bool Reopened=Saved&&Saved->FallenRock.Num()==S->FallenRock.Num()&&Saved->FallenRock.Num()>0;
    if(Reopened) {
        auto* Rebuilt=NewObject<UETGeology>(this);
        State=Saved; Rebuilt->Initialize(this);
        Reopened=Rebuilt->IsOpen(Rock-FVector(0,0,55)); State=S;
    }
    Check(Reopened,TEXT("Loading saved excavation data rebuilds the open space left by fallen rock"));
    // Use a real labelled sign from the fixed shelter district.
    // The travel tests unload the shelter. Its placard and the ground under it stream in separate tiles.
    EnsureRegion({0,0}); EnsureRegion({-1,-1});
    const FVector Sign(-700,-700,0);
    UTextRenderComponent* Letters=nullptr;
    for(TActorIterator<AActor> It(GetWorld());It;++It) if(It->GetAttachParentActor()&&It->GetAttachParentActor()->GetActorLocation().Equals(Sign,1))
        if(auto* T=It->FindComponentByClass<UTextRenderComponent>()) {Letters=T; break;}
    const bool FirstPost=CutDown(Sign+FVector(150,-85,0));
    auto FootOpen=[&](double Offset){const double X=Sign.X+Offset; return Geology->IsOpen(FVector(X,Sign.Y,SurfaceHeight(X,Sign.Y)-12));};
    auto PostState=[&](bool CutSucceeded) {
        return FString::Printf(TEXT(" / cut %d, label %s, open footings %d,%d, hit %s"),int(CutSucceeded),
            !Letters?TEXT("missing"):Letters->IsVisible()?TEXT("visible"):TEXT("removed"),
            int(FootOpen(-180)),int(FootOpen(180)),*GetNameSafe(H.GetComponent()));
    };
    Check(!FirstPost&&!FootOpen(-180)&&!FootOpen(180)&&Letters&&Letters->IsVisible(),TEXT("The first shelter signpost and its label resist excavation")+PostState(FirstPost));
    const bool BothPosts=CutDown(Sign+FVector(-150,-85,0));
    Check(!BothPosts&&!FootOpen(-180)&&!FootOpen(180)&&Letters&&Letters->IsVisible(),TEXT("The second signpost and its label remain supported")+PostState(BothPosts));
}
