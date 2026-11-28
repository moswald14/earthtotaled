#include "ETField.h"
#include "ETWake.h"
#include "ETGeology.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Sound/SoundWave.h"
#include "Engine/Texture2D.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "UnrealClient.h"
#include "Engine/DamageEvents.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"

void AETWorld::RunFieldChecks(AETCharacter& P,bool Resume){
    int Fail=0;FString Report;
    const auto File=FPaths::ProjectSavedDir()/TEXT("dry-season-checks.txt");
    auto Check=[&](bool OK,const FString& What){if(!OK)++Fail;Report+=(OK?TEXT("PASS "):TEXT("FAIL "))+What+TEXT("\n");FFileHelper::SaveStringToFile(Report,*File);};
    auto Finish=[&](){Report+=FString::Printf(TEXT("TOTAL %d failures\n"),Fail);FFileHelper::SaveStringToFile(Report,*File);FPlatformMisc::RequestExitWithStatus(false,Fail?2:0);};
    auto& S=*State;
    FString Mode;FParse::Value(FCommandLine::Get(),TEXT("ETWakeTest="),Mode);
    if(Mode==TEXT("dry-habitat")){
        static int Scene=0;static double Since=0;static bool Playing[3]={},Land=false,Hatch=false,FrameShot=false;
        if(Scene>=3)return;
        const double Now=FPlatformTime::Seconds();
        if(!Since){
            Review=true;P.TestInvincible=true;P.Panel.Empty();UGameplayStatics::SetGamePaused(this,false);
            if(!Scene){
                S.ClockOffset=ETDay::At(15)-S.Elapsed;WeatherOverride=ETClimate::Clear;
                for(int X=1;X<=4&&(!Land||!Hatch);++X){
                    EnsureRegion({X,0});auto* R=Regions.FindRef({X,0});if(!R)continue;
                    for(auto* Actor:R->Residents){
                        if(auto* I=Cast<AETItem>(Actor);I&&I->Kind==TEXT("siltcache"))Hatch=true;
                        auto* A=Cast<AStaticMeshActor>(Actor);if(!A||!A->GetStaticMeshComponent()->GetStaticMesh())continue;
                        const auto Mesh=A->GetStaticMeshComponent()->GetStaticMesh()->GetFName();
                        if(Land||(Mesh!=TEXT("SM_DryCulvert")&&Mesh!=TEXT("SM_StrandedDock")))continue;
                        FVector At=A->GetActorLocation();Land=DryDepth(At.X,At.Y)>120;
                        FVector Eye=At+A->GetActorForwardVector()*610+A->GetActorRightVector()*425;
                        Eye.Z=SurfaceHeight(Eye.X,Eye.Y)+225;
                        P.SetActorLocation(Eye-FVector(0,0,72),false,nullptr,ETeleportType::TeleportPhysics);
                        P.GetCharacterMovement()->DisableMovement();P.GetController()->SetControlRotation((At+FVector(0,0,46)-Eye).Rotation());
                    }
                }
            }
            P.Pause();P.Action(TEXT("field-music"));if(!S.JazzSource)P.Action(TEXT("field-source"));
            if(S.MusicMuted)P.Action(TEXT("field-mute"));
            for(int I=0;I<3&&S.JazzTrack!=Scene;++I)P.Action(TEXT("field-next"));
            P.ClosePanel();UpdateAtmosphereAudio(0,true);Since=Now;
        }
        if(Now-Since>1.3){
            const int I=Scene+3;Playing[Scene]=Playing[Scene]||(Music.IsValidIndex(I)&&Music[I]->IsPlaying()&&Music[I]->VolumeMultiplier>0&&MusicFor(P.GetActorLocation())==I);
            if(!Scene&&!FrameShot){FrameShot=true;FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("dry-watercourse.png"),false,false);}
        }
        if(Now-Since>3.2){
            Since=0;
            if(++Scene==3){
                Check(Land&&Hatch,TEXT("The streamed dry watercourse has stranded infrastructure and a lootable buried hatch"));
                for(int I=0;I<3;++I)Check(Playing[I],FString(ETField::Track(I))+TEXT(" entered active engine playback with nonzero music gain"));
                Finish();
            }
        }
        return;
    }
    if(Mode==TEXT("dry-legacy")){
        Check(HadSave&&S.DrySeason&&S.Skills.Num()>0,TEXT("Opened an actual earlier Windows survivor slot"));
        bool Migrated=true;for(const auto& Id:S.Skills)Migrated&=S.SkillRanks.FindRef(Id)==1;
        Check(Migrated,TEXT("Every previously learned ID migrated to rank one"));
        Check(!S.DryPreserves.IsEmpty(),TEXT("Legacy terrain datum marked around occupied ground"));
        auto Copy=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot,0));
        Check(Copy&&Copy->Skills==S.Skills&&Copy->Gold==S.Gold&&Copy->Inventory.OrderIndependentCompareEqual(S.Inventory)&&Copy->Builds.Num()==S.Builds.Num()&&Copy->Wake.Chapter==S.Wake.Chapter,TEXT("Story, inventory, wrappers, builds and learned keys match the old slot"));
        Finish();return;
    }
    if(Resume){
        Check(HadSave&&S.Rank(TEXT("marksman"))==5&&S.MusicMuted&&S.JazzSource&&S.JazzTrack==2,TEXT("New process reloads ranks and music preferences"));
        Check(S.Furrows.Num()>=2&&FlowingFurrows().Num()==S.Furrows.Num(),TEXT("Saved water network rehydrates with correct direction"));
        bool Soil=false,Clinic=false;for(const auto& B:S.Builds){Soil|=B.Value.Piece==AETConstruction::Bed&&B.Value.Farm.Fertility>0;Clinic|=B.Value.Piece==AETConstruction::Clinic&&B.Value.Farm.Stock==1;}
        Check(Soil&&Clinic&&S.Run.Completed==1,TEXT("Soil, clinic inventory and completed run survived process restart"));
        Check(S.Run.Type==2&&S.Run.Remaining.Num()==2,TEXT("Unfinished second contract reloads with its original target IDs"));
        S.ClockOffset=ETDay::At(21)-S.Elapsed;
        for(int Type:{2,3}){
            if(Type==3)Check(P.StartThreatRun(Type),TEXT("East Dispatch also offers a repeatable hive contract"));
            const auto Keys=S.Run.Remaining.Array();
            for(const auto& Key:Keys)if(const auto* T=S.Threats.Find(Key))if(auto* A=AddThreat(Key,T->Kind,T->Position))
                A->TakeDamage(1900,FDamageEvent(),P.GetController(),&P);
            Check(!S.Run.Type&&S.Run.Completed==Type,TEXT("Marked machine or hive group clears through real enemy deaths"));
        }
        Finish();return;
    }
    P.TestInvincible=true;P.Panel.Empty();UGameplayStatics::SetGamePaused(this,false);S.ArrivalStage=2;S.Points=150;
    P.Skills();
    auto Buy=[&](const TCHAR* Id,int Count){P.Action(FString(TEXT("perk:"))+Id);for(int I=0;I<Count;++I)P.Action(TEXT("learn-perk"));};
    Buy(TEXT("marksman"),7);
    Check(S.Rank(TEXT("marksman"))==5&&S.Points==141&&FMath::IsNearlyEqual(P.DamageSkill(),1.3f),TEXT("Skill web buys five Clean Shot ranks, charges each price, rejects excess"));
    Buy(TEXT("shieldharvest"),3);Buy(TEXT("vitality"),5);Buy(TEXT("parkour"),5);Buy(TEXT("momentum"),5);Buy(TEXT("hydration"),5);Buy(TEXT("reload"),5);Buy(TEXT("hunterscut"),5);
    Check(P.MaxHealth()==150&&S.Rank(TEXT("hydration"))==5&&!P.Has(TEXT("climb")),TEXT("Health and hydration ranks preserve separate climbing access"));
    P.ClosePanel();S.Shield=0;S.Elapsed=400;
    P.CurrentMagazine()=0;P.ReloadTime=0;P.Reload();
    Check(FMath::IsNearlyEqual(P.ReloadTime,P.CurrentGun().Reload/1.3f),TEXT("Real weapon reload timer receives the five-rank bonus"));
    P.ReloadTime=0;S.Wake.Body=ETWake::Bug;P.Reload();Check(P.ReloadTime==0,TEXT("Living claws cannot start a weapon reload"));
    S.Wake.Body=ETWake::Human;P.TestInvincible=false;S.Hydration=50;P.Tick(1.f);
    Check(FMath::IsNearlyEqual(S.Hydration,50.f-.055f*.70f,.0002f)&&FMath::IsNearlyEqual(P.GetCharacterMovement()->MaxWalkSpeed,516.f,.01f),TEXT("The running survivor applies Water Wise and Light Footed"));
    P.TestInvincible=true;S.Shield=0;
    auto* Target=GetWorld()->SpawnActor<AETAgent>();Target->Configure(this,TEXT("field/qa"),TEXT("drone"));
    for(int I=0;I<5;++I)P.RecordHostileKill(*Target,true);
    Check(S.Streak==5&&S.Shield==30&&FMath::IsNearlyEqual(P.DamageSkill(),1.43f,.0001f)&&FMath::IsNearlyEqual(P.MoveSkill(),1.32f,.0001f),FString::Printf(TEXT("Five credited hostiles / streak %d, shield %.1f, move %.6f, damage %.6f"),S.Streak,S.Shield,P.MoveSkill(),P.DamageSkill()));
    for(int Body:{ETWake::Bug,ETWake::Frame}){S.Wake.Body=Body;Check(FMath::IsNearlyEqual(P.MoveSkill(),1.32f,.0001f)&&FMath::IsNearlyEqual(P.DamageSkill(),1.43f,.0001f),TEXT("Transformed body inherits speed, damage and momentum"));}
    S.Wake.Body=ETWake::Human;S.Elapsed+=9;S.ClockOffset=ETDay::At(20)-S.Elapsed;Check(FMath::IsNearlyEqual(P.DamageSkill(),1.3f),TEXT("Momentum expires after eight seconds"));
    Target->Kind=TEXT("deer");P.RecordHostileKill(*Target,true);Target->Destroy();Check(S.Streak==5,TEXT("Deer do not advance hunting rewards"));
    Check(P.StartThreatRun(1),TEXT("Dispatch spawns a repeatable drone contract independently of the campaign"));
    auto Targets=S.Run.Remaining.Array();for(const auto& Key:Targets){
        auto* T=S.Threats.Find(Key);if(!T)continue;
        if(auto* A=AddThreat(Key,T->Kind,T->Position))A->TakeDamage(900,FDamageEvent(),P.GetController(),&P);
    }
    Check(!S.Run.Type&&S.Run.Completed==1&&S.Wake.Chapter==ETWake::Missing,TEXT("Real enemy deaths complete the contract and leave both story routes open"));
    Check(P.StartThreatRun(2)&&S.Run.Remaining.Num()==2,TEXT("Second contract stays active for reload"));
    auto Place=[&](int Piece,FVector XY,int Style){FETBuild B;B.Piece=Piece;B.Anchor=XY;B.Anchor.Z=SurfaceHeight(XY.X,XY.Y)+4;B.Farm.Style=Style;
        const auto Id=FGuid::NewGuid();S.Builds.Add(Id,B);return Id;};
    const FVector Center(-4400,-4400,0);EnsureRegion({-1,-1});
    FVector Start{},End{};FString Why;bool Cut=false;
    for(int Y=-6400;Y<=-2400&&!Cut;Y+=400)for(int X=-6400;X<=-2400&&!Cut;X+=400){
        Start=FVector(X,Y,0);End=Start+FVector(700,0,0);
        if(FarmSite(FVector2D(Start))&&FarmSite(FVector2D(End+FVector(700,0,0)))&&
           !Geology->DigRestriction(FBox2D(FVector2D(Start)-FVector2D(170),FVector2D(End)+FVector2D(870,170)),true))
            Cut=Geology->CarveFurrow(Start,End,Why);
    }
    Check(Cut,TEXT("Shallow channel carved in sheltered allotment: ")+Why);
    if(!Cut){Finish();return;}
    const auto First=S.Furrows[0];const FVector Middle=(First.Inlet+First.Outlet)*.5;
    Check(Geology->IsOpen(Middle-FVector(0,0,18))&&!Geology->IsOpen(Middle-FVector(0,0,200)),TEXT("Channel physically removes shallow soil and keeps the deep floor"));
    Check(Geology->DigRestriction(FBox2D(FVector2D(Start),FVector2D(Start)))!=nullptr,TEXT("Deep city excavation remains protected"));
    StreamAround(Start,true);UpdateVisibility(Start);
    auto Aim=[&](FVector XY){
        XY.Z=SurfaceHeight(XY.X,XY.Y);P.SetActorLocation(XY+FVector(0,-340,330),false,nullptr,ETeleportType::TeleportPhysics);
        const auto Rot=(XY-P.Camera->GetComponentLocation()).Rotation();P.GetController()->SetControlRotation(Rot);FMinimalViewInfo View;P.Camera->GetCameraView(0,View);
    };
    P.Borer=false;S.SquareBorer=false;S.FurrowHead=false;P.ToggleBorer();P.TurnTool();P.TurnTool();
    Aim(End);P.FireBorer();const FString InletNotice=Notice;Aim(End+FVector(700,0,0));P.FireBorer();
    Check(S.Furrows.Num()==2&&S.FurrowHead,TEXT("The actual Borer selects irrigation, strikes terrain, snaps the outlet and extends the slope: ")+InletNotice+TEXT(" / ")+Notice);
    P.ToggleBorer();
    if(S.Furrows.Num()!=2){Finish();return;}
    bool Placed=false;P.ToggleBuild();P.BuildPiece=AETConstruction::Bed;
    for(int Y=-1;Y<=1&&!Placed;++Y)for(int X=0;X<3&&!Placed;++X){
        Aim(Start+FVector(400*X,-1600+Y*400,0));FETBuild Candidate;
        if(!P.BuildTarget(Candidate,Why))continue;
        const int Before=S.Builds.Num();P.PlaceBuild();Placed=S.Builds.Num()==Before+1;
    }
    Check(Placed,TEXT("The standard building tool places a prepared bed on actual terrain: ")+Why);P.StopBuilding();
    const auto Tank=Place(AETConstruction::Cistern,Start-FVector(0,530,0),ETWake::Human);
    const auto Bed=Place(AETConstruction::Bed,End+FVector(700,350,0),ETWake::Bug);
    const auto Clinic=Place(AETConstruction::Clinic,End+FVector(700,1000,0),ETWake::Frame);
    Check(FlowingFurrows().IsEmpty(),TEXT("Cistern without a pump keeps both channels dry"));
    Place(AETConstruction::Pump,Start-FVector(400,530,0),ETWake::Frame);
    StepFarms(90);Check(S.Builds[Tank].Farm.Stock==1&&WetFurrows.Num()==2,TEXT("Pump and cistern fill once and send water along both furrows"));
    UpdateFlowView(Middle);
    Check(FlowSurface&&FlowSurface->GetInstanceCount()==2&&FlowRipples&&FlowRipples->GetInstanceCount()>2,TEXT("Only connected furrows render surfaces, moving ripples and splashes"));
    if(auto* Machine=SpawnBuild(Bed,S.Builds[Bed])){P.OpenDevice(Machine);Check(P.Panel==TEXT("farm")&&P.DeviceId==Bed,TEXT("Built bed uses the ordinary contextual device menu"));}
    P.DeviceId=Bed;P.FarmPanel();P.FieldAction(TEXT("field-seeds"));P.FieldAction(TEXT("field-plant1"));StepFarms(70);
    auto* CropActor=LiveBuilds.FindRef(Bed).Get();Check(CropActor&&CropActor->Mechanism->GetStaticMesh()&&CropActor->Mechanism->GetStaticMesh()->GetFName()==TEXT("SM_Cinderbean"),TEXT("Mature crop model appears on the streamed bed"));
    auto& Soil=S.Builds[Bed].Farm;Check(Soil.Growth==1&&Soil.Moisture>50,TEXT("Prepared bed grows watered Cinderbean to its harvest stage"));
    P.FieldAction(TEXT("field-harvest"));Check(Soil.Fertility==1&&S.FarmStores.FindRef(TEXT("Cinderbean"))==4&&S.Inventory.FindRef(TEXT("food"))>=1,TEXT("Cinderbean returns food, seed and soil fertility"));
    P.FieldAction(TEXT("field-plant2"));StepFarms(45);Check(ETField::Stage(Soil)==2,TEXT("Saltroot visibly advances more slowly than Cinderbean"));
    StepFarms(150);P.FieldAction(TEXT("field-harvest"));P.FieldAction(TEXT("field-plant3"));StepFarms(130);P.FieldAction(TEXT("field-harvest"));
    Check(S.FarmStores.FindRef(TEXT("leaves"))==2,TEXT("Mendleaf supplies two clinic ingredients"));
    P.DeviceId=Clinic;P.FarmPanel();S.Scrap+=2;P.FieldAction(TEXT("field-order"));StepFarms(75);Check(S.Builds[Clinic].Farm.Stock==1&&S.Builds[Clinic].Farm.Orders==0,TEXT("Clinic actually consumes Mendleaf and scrap, then delivers a medpack after 75 seconds"));
    StepFarms(1200);Check(S.Builds[Tank].Farm.Stock==8,TEXT("Cistern production stops at eight without daily inputs"));
    const auto DryBed=Place(AETConstruction::Bed,Center+FVector(4000,4000,0),0);auto& Dry=S.Builds[DryBed].Farm;Dry.Crop=1;Dry.Moisture=0;
    StepFarms(100);Check(Dry.Crop==1&&Dry.Growth==0,TEXT("Disconnected dry crops remain alive and dormant"));
    const int OldSeeds=S.FarmStores.FindRef(TEXT("Cinderbean"));StepFarms(float(2*ETDay::Period));
    Check(!Dry.Crop&&S.FarmStores.FindRef(TEXT("Cinderbean"))==OldSeeds+1,TEXT("Only prolonged drought withers a crop and it leaves viable seed"));
    Check(LoadObject<UTexture2D>(nullptr,TEXT("/Game/EARTHtotaled/Art/T_FieldKit.T_FieldKit"))&&Material(TEXT("DryGround")),TEXT("Cooked field-kit icon and dry-ground surface load"));
    P.ClosePanel();P.Pause();P.Action(TEXT("field-music"));P.Action(TEXT("field-source"));P.Action(TEXT("field-next"));P.Action(TEXT("field-next"));P.Action(TEXT("field-mute"));
    Check(S.JazzSource&&S.JazzTrack==2&&S.MusicMuted&&MusicFor(Center)==5&&Music[5]->VolumeMultiplier==0,TEXT("Pause menu selects, changes and mutes the offline jazz channel"));
    for(int I=3;I<Music.Num();++I){const auto* Wave=Cast<USoundWave>(Music[I]->Sound);Check(Wave&&Wave->bLooping&&Wave->GetDuration()>24,TEXT("A cooked stereo Doodle Jazz track is independently loopable"));}
    P.ClosePanel();FString Msg;const auto Path=FPaths::ProjectSavedDir()/TEXT("field-profile.etsave");
    UETSave* Imported=P.ExportProfile(&S,Path,Msg)?P.ReadProfile(Path,Msg):nullptr;
    Check(Imported&&Imported->Furrows.Num()==S.Furrows.Num()&&Imported->JazzTrack==2&&Imported->Rank(TEXT("marksman"))==5,TEXT("Export/import preserves new and preexisting data: ")+Msg);
    Check(Save(),TEXT("Writes the slot used by the next process"));Finish();
}
