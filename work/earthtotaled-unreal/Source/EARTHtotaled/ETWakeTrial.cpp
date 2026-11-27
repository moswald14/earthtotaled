#include "UnrealClient.h"
#include "HAL/FileManager.h"
#include "ETDayCycle.h"
#include "Camera/CameraComponent.h"
#include "ETSaveDisk.h"
#include "ETWake.h"
#include "ETUnderstory.h"
#include "ETCombat.h"
#include "ETWakeVehicle.h"
#include "ETGame.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

// Runs only with a named test route and a separate Unreal user directory.
void ETWake::Trial(AETWorld& W,AETCharacter& P) {
    static bool Ran=false;if(Ran||W.WakeLayout==INDEX_NONE||!P.Controller)return;
    FString Route;FParse::Value(FCommandLine::Get(),TEXT("ETWakeTest="),Route);
    if(Route=="combat"||Route=="combat-view"){ETCombat::Trial(W,P,Route=="combat-view");return;}
    if(Route=="deer-view"){ETFlora::DeerView(W,P);return;}
    if(Route=="woodland-view"){
        static int Scene=0;static double Since=0;static bool Captured=false;
        static TWeakObjectPtr<AETAgent> Subject;static FVector Home,Eye,Look;
        const double Now=FPlatformTime::Seconds();
        if(!Since){
            P.TestInvincible=true;W.State->ArrivalStage=2;W.WeatherOverride=ETClimate::Clear;P.Flashlight->SetVisibility(Scene==0);
            if(!P.Panel.IsEmpty()){P.Panel.Empty();UGameplayStatics::SetGamePaused(&W,false);P.SetMouseMode(false);}
            if(Scene==0){
                int X=0,Y=0;FParse::Value(FCommandLine::Get(),TEXT("DenX="),X);FParse::Value(FCommandLine::Get(),TEXT("DenY="),Y);
                W.State->ClockOffset=ETDay::At(19.03)-W.State->Elapsed;W.EnsureRegion({X,Y});
                if(auto* Den=W.Regions.FindRef(FIntPoint(X,Y)))for(auto* A:Den->Residents)if(auto* B=Cast<AETAgent>(A);IsValid(B)&&B->Kind=="bear"){Subject=B;break;}
                if(!Subject.IsValid()){FPlatformMisc::RequestExitWithStatus(true,2);return;}
                Home=Subject->GetActorLocation();Look=Home+FVector(0,0,89);
                for(int K=0;K<12;++K){
                    const auto Offset=FRotator(0,40+K*30.,0).Vector()*565.;Eye=Home+Offset;Eye.Z=W.SurfaceHeight(Eye.X,Eye.Y)+118;
                    FCollisionQueryParams Q;Q.AddIgnoredActor(Subject.Get());Q.AddIgnoredActor(&P);
                    if(!W.GetWorld()->LineTraceTestByChannel(Eye,Look,ECC_Visibility,Q))break;
                }
                W.StreamAround(Home,true);P.GetCharacterMovement()->DisableMovement();
            }else{
                W.State->ClockOffset=ETDay::At(9.4)-W.State->Elapsed;
                TArray<UInstancedStaticMeshComponent*> Plants;W.GetComponents(Plants);double Best=DBL_MAX;
                for(auto* C:Plants)if(C->GetStaticMesh()&&C->GetStaticMesh()->GetFName()=="SM_AmberCaps")
                    for(int I=0;I<C->GetInstanceCount();++I){FTransform T;C->GetInstanceTransform(I,T,true);
                        const double Range=FVector::DistSquared(Home,T.GetLocation());if(Range<Best){Best=Range;Look=T.GetLocation()+FVector(0,0,14);}}
                if(Best==DBL_MAX){FPlatformMisc::RequestExitWithStatus(true,2);return;}
                Eye=Look+FVector(105,-215,110);
            }
            Since=Now;Captured=false;
        }
        if(Scene==0&&Subject.IsValid()){
            W.State->ClockOffset=ETDay::At(19.03)-W.State->Elapsed;
            auto* Bear=Subject.Get();Bear->SetActorTickEnabled(false);Bear->SetActorLocation(Home);Bear->SetActorRotation(FRotator(0,(Eye-Home).Rotation().Yaw+48,0));Bear->PoseBear(.45f,0);
        }
        P.SetActorLocation(Eye-FVector(0,0,72));P.Controller->SetControlRotation((Look-Eye).Rotation());
        const FString Path=FPaths::ProjectSavedDir()/(Scene?TEXT("forest-floor.png"):TEXT("woodland-bear.png"));
        if(!Captured&&Now-Since>7){
            FScreenshotRequest::RequestScreenshot(Path,true,false);Captured=true;return;}
        if(Captured&&(IFileManager::Get().FileSize(*Path)>2048||Now-Since>22)){
            if(IFileManager::Get().FileSize(*Path)<=2048){Ran=true;FPlatformMisc::RequestExitWithStatus(true,2);return;}
            if(++Scene==2){
                W.Speak(Clue(PawSign).Cue);
                const bool Playing=W.DialogueAudio&&W.DialogueAudio->IsPlaying()&&W.SubtitleSpeaker==TEXT("SALT-7")&&W.HasSubtitle();
                FFileHelper::SaveStringToFile(Playing?TEXT("PASS SALT-7 field recording playing with matching subtitles"):TEXT("No active audio device or matching subtitle"),
                    *(FPaths::ProjectSavedDir()/TEXT("woodland-voice.txt")));
                Ran=true;FPlatformMisc::RequestExitWithStatus(true,0);return;
            }Since=0;
        }
        return;
    }
    if(Route=="scenes"){
        if(W.ActiveSlot!=7){P.TravelSlot(7,true);return;}
        static int Scene=0;static double Since=0;static bool Captured=false;
        const TCHAR* Names[]={TEXT("vault"),TEXT("aircraft"),TEXT("journal")};
        const double Now=FPlatformTime::Seconds();const FString Image=FPaths::ProjectSavedDir()/FString::Printf(TEXT("megawake-%s.png"),Names[Scene]);
        if(!Since){
            P.TestInvincible=true;W.State->ClockOffset=ETDay::At(10.75)-W.State->Elapsed;
            if(!P.Panel.IsEmpty()){P.Panel.Empty();UGameplayStatics::SetGamePaused(&W,false);P.SetMouseMode(false);}
            if(Scene<2){
                const FVector Origin=Scene==0?W.WakeSpot("vault"):W.WakePlane->GetActorLocation();
                const FVector Eye=Origin+(Scene==0?FVector(-390,-92,171):FVector(730,-605,290));
                W.StreamAround(Eye,true);P.GetCharacterMovement()->DisableMovement();P.SetActorLocation(Eye-FVector(0,0,72));
                const FVector Look=Origin+(Scene==0?FVector(180,15,102):FVector(0,0,35));P.Controller->SetControlRotation((Look-Eye).Rotation());
            }else P.Journal();
            Since=Now;Captured=false;return;
        }
        if(!Captured&&Now-Since>2.5){FScreenshotRequest::RequestScreenshot(Image,true,false);Captured=true;return;}
        const bool Saved=IFileManager::Get().FileSize(*Image)>2048;
        if(Captured&&(Saved||Now-Since>15)){
            if(!Saved||++Scene==3){Ran=true;FPlatformMisc::RequestExitWithStatus(true,Saved?0:2);return;}Since=0;
        }
        return;
    }
    if(Route.EndsWith(TEXT("resume"))){
        const int Expected=Route.EndsWith(TEXT("airresume"))?8:Route.EndsWith(TEXT("formresume"))?9:1;
        if(W.ActiveSlot!=Expected){P.TravelSlot(Expected,true);return;}
    }
    Ran=true;
    const FString Report=FPaths::ProjectSavedDir()/TEXT("megawake-journey.txt");FString Results;int Failures=0;
    auto Check=[&](bool Ok,FString Label){Failures+=!Ok;Results+=(Ok?TEXT("PASS "):TEXT("FAIL "))+Label+TEXT("\n");FFileHelper::SaveStringToFile(Results,*Report);};
    auto Finish=[&](){Results+=FString::Printf(TEXT("TOTAL %d failures\n"),Failures);FFileHelper::SaveStringToFile(Results,*Report);FPlatformMisc::RequestExitWithStatus(false,Failures?2:0);};
    auto& S=W.State->Wake;
    if(Route=="deer"||Route=="deer-legacy"){
        if(Route=="deer-legacy")Check(W.HadSave,TEXT("Opened the existing survivor before streaming wildlife"));
        ETFlora::DeerTrial(W,P,Check);Finish();return;
    }
    if(Route==TEXT("legacy")){
        Check(W.HadSave&&S.Chapter==Missing&&S.Body==Human&&!S.HasCar()&&S.Restored()==0&&S.Valid(),TEXT("Previous release survivor opened with an unstarted MegaWAKE campaign"));
        Check(W.WakeObjects.FindRef("thrum")&&W.Weapons.Num()>0&&W.State->Skills.Contains(TEXT("survivor")),TEXT("Existing skills, arsenal, and new story contact loaded together"));
        auto Survey=S.FieldSurvey;const FVector2D Original=Field(0,S);FETBuild LegacyBase;LegacyBase.Anchor=FVector(Original,0);
        const auto Id=FGuid::NewGuid();W.State->Builds.Add(Id,LegacyBase);S.FieldSurvey=FIntPoint(-1,-1);SurveyFields(W);
        Check(Field(0,S)!=Original&&FVector2D::Distance(Field(0,S),Original)>10000,TEXT("An occupied legacy airfield site moves the new runway clear of the old base"));
        W.State->Builds.Remove(Id);S.FieldSurvey=Survey;
        Finish();return;}
    if(Route==TEXT("woodland")){
        ETFlora::Trial(W,P,Check);Finish();return;}
    if(Route==TEXT("trail-legacy")){
        Check(W.State->AlertVolume==75&&W.State->HitFlashIntensity==75&&W.State->AlertCaptions,
            TEXT("A 1.16 survivor receives readable combat defaults alongside existing settings"));
        const int Known=(1<<CaseFold)|(1<<DashFold)|(1<<Carrier);
        Check(W.HadSave&&S.Chapter==Home&&S.Rescued()&&S.EndingHerds>0&&S.Valid(),TEXT("An original 1.16 completed survivor opens with Kit safe and their chosen ending intact"));
        Check(S.TrailFound==Known&&S.TrailHeard==Known,TEXT("Previously visited case, car and relay enter the journal without ambushing the survivor with old audio"));
        bool Reachable=true;for(FName Id:{FName("tracks"),FName("fernfold"),FName("paws"),FName("liftfold")}){
            const auto* Site=W.WakeObjects.FindRef(Id);Reachable&=IsValid(Site)&&!Site->IsHidden();}
        Check(Reachable,TEXT("A returning survivor can still find every optional physical sign after the finale"));
        Finish();return;}
    if(Route.EndsWith(TEXT("airresume"))){
        Check(W.HadSave&&P.Ride()==W.WakePlane&&S.Aircraft.Flying&&S.Aircraft.Started&&S.Aircraft.Speed>1000,FString::Printf(TEXT("Fresh process restores the airborne engine and pilot seat / slot %d chapter %d seat %d speed %.0f"),W.ActiveSlot,S.Chapter,S.Seat,S.Aircraft.Speed));
        Check(W.WakePlane&&FVector::Dist(P.GetActorLocation(),W.WakePlane->SeatPosition())<10,TEXT("Survivor returns to the cockpit at the saved altitude"));Finish();return;
    }
    if(Route.EndsWith(TEXT("formresume"))){
        const bool Organic=Route.StartsWith(TEXT("bug"));Check(W.HadSave&&S.Body==(Organic?Bug:Frame)&&S.Chapter==Passage,TEXT("Fresh process keeps the vault decision"));
        Check(P.Has(Organic?TEXT("molt"):TEXT("override"))&&CanBuild(S,Organic?AETConstruction::Nursery:AETConstruction::Fabricator)&&
            W.State->Inventory.FindRef(TEXT("food"))==(Organic?3:2),TEXT("Pack, upgrade path and building catalog restored"));Finish();return;
    }
    if(Route.EndsWith(TEXT("resume"))){
        Check(W.HadSave&&S.Chapter==Home&&S.Rescued(),TEXT("A fresh executable restores the rescued family and finale"));
        Check(S.TrailFound==(Route.StartsWith(TEXT("crossed"))?((1<<CaseFold)|(1<<DashFold)|(1<<Carrier)):(1<<ClueCount)-1),
            TEXT("Fresh executable restores the survivor's actual discoveries"));
        Check(W.WakeCar&&W.WakePlane&&S.Airfields==3,TEXT("Parked vehicles and discovered airstrips restored"));
        Check(S.EndingHerds==S.Restored()&&P.Has(S.Body==Bug?TEXT("molt"):TEXT("override")),TEXT("Herds, body, political outcome and chosen skill web restored"));
        Finish();return;
    }
    const int Sponsor=Route=="frame"?Frame:Bug;
    const int Body=Route=="crossed"?Frame:Sponsor;
    auto Close=[&](){if(P.Panel=="arrival")P.FinishArrival();else if(P.Panel=="title"){P.Panel.Empty();UGameplayStatics::SetGamePaused(&W,false);}else if(!P.Panel.IsEmpty())P.Action(TEXT("resume"));};
    Close();if(P.Panel=="arrival")Close();
    W.State->ArrivalStage=2;W.State->IntroRead=true;P.TestInvincible=true;
    auto Go=[&](FVector At){P.GetCharacterMovement()->StopMovementImmediately();P.SetActorLocation(At);P.Focus=nullptr;P.FocusBuild.Reset();P.TreeInReach=false;};
    auto Use=[&](FName Key){
        Close();auto* O=W.WakeObjects.FindRef(Key);
        if(!IsValid(O)||O->IsHidden()){Check(false,TEXT("Missing active site ")+Key.ToString());return;}
        const auto Center=O->GetActorLocation();const FVector Aim=O->Mesh->Bounds.Origin;
        W.EnsureRegion({FMath::FloorToInt32(Center.X/W.RegionSize),FMath::FloorToInt32(Center.Y/W.RegionSize)});
        bool Reached=false;
        for(int Side=0;Side<12&&!Reached;++Side){
            FVector Spot=Center+FRotator(0,180.+30.*Side,0).Vector()*245.;Spot.Z=W.SurfaceHeight(Spot.X,Spot.Y)+105;
            Go(Spot);const FVector Eye=P.Camera->GetComponentLocation();P.GetController()->SetControlRotation((Aim-Eye).Rotation());
            FMinimalViewInfo View;P.Camera->GetCameraView(0,View);P.Tick(.001f);
            if(P.Focus==O||(!IsValid(P.Focus)&&P.NearbyWake()==O)){P.Interact();Reached=true;}
        }
        if(!Reached){Check(false,TEXT("No walking-range sightline to ")+Key.ToString());}
    };
    auto HerdTo=[&](int I){auto Target=W.WakeSpot("glade",I);Go(Target+FVector(0,0,100));
        for(int Step=0;Step<140;++Step)for(auto* A:W.WakeDeer)if(IsValid(A)&&A->WakeHerd==I)A->TickWakeDeer(.05f,P);};
    auto Board=[&](AETWakeVehicle* V){Close();Go(V->GetActorLocation()+FVector(0,-170,45));P.Interact();};
    auto RoundTrip=[&](const TCHAR* Stage){
        W.Save();auto* Native=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(W.SaveSlot,0));
        FString Note;const auto Path=FPaths::ProjectSavedDir()/TEXT("wake-trial.etsave");
        bool Exported=P.ExportProfile(W.State,Path,Note);auto* Imported=Exported?P.ReadProfile(Path,Note):nullptr;
        for(auto* A:{Native,Imported}){const auto* T=A?&A->Wake:nullptr;
            Check(T&&T->Chapter==S.Chapter&&T->Body==S.Body&&T->TaskHerd==S.TaskHerd&&T->TaskSide==S.TaskSide&&T->ReadSource==S.ReadSource&&T->Seat==S.Seat&&
                T->Silenced==S.Silenced&&T->Retuned==S.Retuned&&T->WarningSeen==S.WarningSeen&&T->Airfields==S.Airfields&&T->VaultLine==S.VaultLine&&T->TrailFound==S.TrailFound&&T->TrailHeard==S.TrailHeard&&
                T->Roadster.Position.Equals(S.Roadster.Position,1)&&T->Aircraft.Position.Equals(S.Aircraft.Position,1)&&T->EndingHerds==S.EndingHerds&&T->FieldSurvey==S.FieldSurvey&&T->Valid(),
                FString(Stage)+(A==Native?TEXT(" / native save"):TEXT(" / portable save"))+(A?TEXT(""):TEXT(" / ")+Note));}
    };
    const bool Signs=Route!="crossed";
    auto FindSign=[&](FName Key,int Index){
        Use(Key);Check(P.WakePanelNode=="trail"&&S.TrailFound&(1<<Index)&&P.PanelBody.Contains(W.VoiceLines.FindRef(Clue(Index).Cue).Text)&&W.VoiceId==Clue(Index).Cue,
            TEXT("E examines and plays the field recording / ")+Key.ToString());Close();
    };
    Use("thrum");Use("band");Close();
    Check(S.TrailFound==1&&!(S.TrailHeard&1),TEXT("The red case records Kit's habit behind the wristband dialogue"));
    W.SubtitleUntil=0;W.TickWake(P,.5f);Check(W.VoiceId==Clue(CaseFold).Cue&&S.TrailHeard==1,TEXT("The radio waits for the quest speaker and delivers the case clue once"));
    if(Signs)FindSign("tracks",Hooves);
    Use("herd");Close();
    Check(S.Chapter==FirstHerd&&S.SharedLead,TEXT("Mara follows Kit's transfer clue and starts the joint rescue"));
    if(Signs)FindSign("fernfold",FernFold);
    HerdTo(0);Use("glade0");Check(S.Chapter==Garage&&S.Count(Bug)+S.Count(Frame)==0,TEXT("Walked the herd to safety; shared work awards no faction marks"));
    if(W.WakeCar)Go(W.WakeCar->GetActorLocation()+FVector(0,-175,45));
    Check(W.WakeCar&&!W.WakeCar->Boardable(P),TEXT("Roadside repair car is present before the ignition handoff"));
    Use("garage");if(!W.WakeCar){Check(false,TEXT("Roadster unlocked"));Finish();return;}
    Board(W.WakeCar);P.TurnTool();P.Forward(1);auto* CarUnit=P.Ride();Check(CarUnit==W.WakeCar&&S.Roadster.Started,TEXT("E boards and R starts the roadster"));if(!CarUnit){Finish();return;}
    for(int N=0;N<700&&S.Roadster.Position.X<13300;++N)CarUnit->Drive(P,.02f);
    P.Forward(-1);for(int N=0;N<100;++N)CarUnit->Drive(P,.02f);RoundTrip(TEXT("Behind the wheel"));P.Interact();
    Check(S.Chapter==Dispatch&&S.Seat==Foot,FString::Printf(TEXT("Drove through the road gate, braked and exited at dispatch / X %.0f"),S.Roadster.Position.X));
    if(S.Chapter!=Dispatch){Finish();return;}Use("dispatch");Close();
    auto Job=[&](int Side) {
        Close();P.Journal();P.Action(Side==Bug?TEXT("job-bug"):TEXT("job-frame"));
        if(S.Count(Side)==3)Check(P.PanelBody.Contains(TEXT("FOURTH MARK")),TEXT("The fourth job warns before acceptance"));
        P.Action(TEXT("accept"));int I=S.TaskHerd;if(I<=0){Check(false,TEXT("Job accepted"));return;}
        Use(FName(*FString::Printf(TEXT("source%d"),I)));Close();
        if(Side==Bug)HerdTo(I);
        Use(FName(*FString::Printf(TEXT("glade%d"),I)));
        if(Side==Frame){const int Before=S.Count(Side);P.Action(FString(TEXT("tune-"))+(FString(Frequency(I))==TEXT("Cyan")?TEXT("Amber"):TEXT("Cyan")));
            Check(S.Count(Side)==Before,TEXT("Wrong frequency leaves herd glitched"));P.Action(TEXT("tune-")+FString(Frequency(I)));}
        Check(S.TaskSide==0&&((S.Silenced|S.Retuned)&(1<<(I-1)))!=0,FString(TEXT("Restored "))+Herd(I).Name);
    };
    for(int I=0;I<3;++I)Job(Sponsor);
    Check(!S.Winner()&&S.WarningSeen!=0&&P.PanelBody.Contains(TEXT("FACTION WARNING")),TEXT("Third completed job displays a warning while rival remains neutral"));
    Job(Sponsor);Check(S.Winner()==Sponsor&&S.Chapter==Runway&&P.PanelBody.Contains(TEXT("Kit is alive")),TEXT("Fourth job changes faction hostility and reveals fraudCORP's hostage"));
    if(S.Chapter!=Runway){Finish();return;}
    if(Sponsor==Frame)for(int N=0;N<4;++N)Job(Sponsor);
    RoundTrip(TEXT("Faction record"));
    if(Signs)FindSign("paws",PawSign);
    if(W.WakePlane)Go(W.WakePlane->GetActorLocation()+FVector(0,-175,45));
    Check(W.WakePlane&&!W.WakePlane->Boardable(P),TEXT("The damaged plane is present before the runway and repair tasks"));
    Use("runway");Use("hangar");Close();if(!W.WakePlane){Check(false,TEXT("Survey plane unlocked"));Finish();return;}
    Board(W.WakePlane);auto* Aircraft=P.Ride();if(!Aircraft||Aircraft!=W.WakePlane){Check(false,TEXT("Boarded the aircraft"));Finish();return;}
    P.TurnTool();P.Forward(1);P.JumpPress();
    for(int N=0;N<300;++N)Aircraft->Drive(P,.025f);
    P.JumpRelease();Check(S.TookOff&&S.Aircraft.Flying,TEXT("Actual runway roll and takeoff on the repaired plane"));RoundTrip(TEXT("In flight"));
    Check(ETSaveDisk::Write(W.State,W.SlotName(8)),TEXT("Saved an airborne slot for a separate-process cockpit check"));
    for(int N=0;N<2600&&S.Aircraft.Position.X<Field(1,S).X-13400;++N){
        const bool FinalApproach=S.Aircraft.Position.X>Field(1,S).X-20500;
        auto To=Field(1,S)-(FinalApproach?FVector2D::ZeroVector:FVector2D(19300,0))-FVector2D(S.Aircraft.Position);
        const float Target=FMath::RadiansToDegrees(FMath::Atan2(To.Y,To.X));
        P.Right(FMath::Clamp(FMath::FindDeltaAngleDegrees(S.Aircraft.Heading,Target)/12.f,-1.f,1.f));
        Aircraft->Drive(P,.025f);}
    P.Right(0);P.Forward(-1);P.CrouchLoot();for(int N=0;N<450;++N)Aircraft->Drive(P,.025f);
    P.ReleaseLoot();P.Interact();Check(S.Chapter==Vault&&S.Airfields==3&&S.Seat==Foot,FString::Printf(TEXT("Flew across the map, landed, braked and exited at Crown / %.0f %.0f"),S.Aircraft.Position.X,S.Aircraft.Position.Y));
    if(S.Chapter!=Vault){Finish();return;}
    Use("vault");for(int N=0;N<4;++N)P.Action(TEXT("wake-next"));RoundTrip(TEXT("Shaking vault / both offers"));Check(ETSaveDisk::Write(W.State,W.SlotName(7)),TEXT("Both offers persist at the vault before committing"));
    W.State->Inventory.FindOrAdd(TEXT("food"))=3;W.State->Meds=4;
    Use(Body==Bug?"juice":"frame");P.Action(TEXT("transform"));Check(S.Body==Body&&S.Chapter==Passage&&P.Has(Body==Bug?TEXT("molt"):TEXT("override")),TEXT("Confirmed physical transformation and opened the corresponding skill web"));
    Close();
    const float Moisture=W.State->Hydration,Health=W.State->Health;W.State->Hydration=0;P.TestInvincible=false;P.Tick(.1f);
    Check(P.Panel.IsEmpty()&&W.State->Health==Health,TEXT("Zero hydration cannot hurt the transformed survivor"));P.TestInvincible=true;W.State->Hydration=Moisture;
    P.UseSupply(TEXT("food"));Check(W.State->Inventory.FindRef(TEXT("food"))==(Body==Bug?3:2),TEXT("Ordinary inventory is sealed for Bug and usable in frame"));RoundTrip(TEXT("Transformed survivor"));Check(ETSaveDisk::Write(W.State,W.SlotName(9)),TEXT("Saved transformed survivor in an independent slot"));
    const FString Perk=Body==Bug?TEXT("molt-shell"):TEXT("override-fabricate");
    P.Skills();P.Action(TEXT("perk:")+Perk);P.Action(TEXT("learn-perk"));Close();Check(P.Has(Perk),TEXT("New faction skill learned through the existing web"));
    const auto Stock=W.State->Inventory;const int Previous=W.State->Builds.Num();P.ToggleBuild();
    P.BuildPiece=Body==Bug?AETConstruction::Nursery:AETConstruction::Fabricator;W.EnsureRegion({1,-1});FGuid Placed;FString Why;
    for(int X=0;X<6&&!Placed.IsValid();++X)for(int Y=0;Y<6&&!Placed.IsValid();++Y){
        FVector At(37600+X*400.,-7600+Y*400.,0);At.Z=W.SurfaceHeight(At.X,At.Y);
        Go(At+FVector(0,0,690));P.GetController()->SetControlRotation(FRotator(-89.9,0,0));FMinimalViewInfo View;P.Camera->GetCameraView(0,View);
        FETBuild B;if(!P.BuildTarget(B,Why))continue;P.RefreshBuild();P.RefreshBuild();P.PlaceBuild();
        for(const auto& It:W.State->Builds)if(It.Value.Piece==P.BuildPiece&&It.Value.Anchor.Equals(B.Anchor,2))Placed=It.Key;
    }
    P.StopBuilding();Check(Placed.IsValid()&&Previous+1==W.State->Builds.Num()&&Stock.OrderIndependentCompareEqual(W.State->Inventory),TEXT("Placed unlocked faction architecture without consuming the old pack / ")+Why);
    if(auto* Device=W.LiveBuilds.FindRef(Placed).Get()){
        const int Meds=W.State->Meds;W.State->Health=50;P.OpenDevice(Device);
        Check(Body==Bug?W.State->Health==85:W.State->Meds==Meds+1,TEXT("The built nursery heals or the fabrication hall prints supplies"));Close();}
    Board(W.WakePlane);Check(P.Ride()==W.WakePlane,TEXT("Plane boards after transformation without an inventory item"));P.LeaveRide();
    Board(W.WakeCar);Check(P.Ride()==W.WakeCar,TEXT("Roadster boards after transformation without an inventory item"));P.LeaveRide();
    Use(Body==Bug?"burrow":"gate");
    if(Signs){
        FindSign("liftfold",LiftFold);P.Journal();P.Action(TEXT("trail-log"));P.Action(TEXT("trail-prev"));P.Action(TEXT("trail-next"));
        Check(P.TrailPage==LiftFold&&P.PanelBody.Contains(TEXT("Three knocks")),TEXT("Field journal browses and replays the custody recording"));
        Close();
    }
    Use("kit");Check(!S.Rescued()&&S.Chapter==Escort,TEXT("Reached custody by the chosen physical route and freed Kit"));
    Close();Go(W.WakeSpot("evac")+FVector(0,100,100));for(int N=0;N<160;++N)W.TickWake(P,.05f);
    Use("evac");Check(S.Rescued()&&S.Chapter==Strike,TEXT("Kit followed to the lift before Sunseed became available"));Use("strike");P.Action(TEXT("sunseed"));for(int T=0;T<40;++T)W.TickWake(P,.1f);
    Check(S.Chapter==Home&&S.EndingHerds==(Sponsor==Bug?5:9)&&W.WakeObjects.FindRef("home")&&!W.WakeObjects["home"]->IsHidden(),TEXT("Detonated Sunseed, brought Kit home and populated the ending with the restored herds"));
    RoundTrip(TEXT("Homecoming"));Finish();
}
