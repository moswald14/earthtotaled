#include "ETGame.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMemory.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

// Opt-in rendered scenarios, never run in a normal survivor. Story checkpoints stage
// the player beside real actors and require a clear, in-range interaction ray.
// The endurance section uses CharacterMovement, collision, finite supplies and AI.
// Only the weather/day clock is accelerated (13x); movement and survival stay at 1x.
namespace
{
struct FJourney
{
    int32 Checks=0, Failures=0, Waypoint=0, Samples=0, Looted=0, Trades=0;
    int32 MaxRegions=0, MaxActors=0, Drinks=0, CityTransitions=0;
    bool Started=false, Finished=false, LastCity=false, WasDay=false, WasNight=false;
    bool BoundsOK=true, GroundOK=true, UniqueItems=true, VisitedForest=false;
    bool FollowRoad=false,NewCityBearing=false;
    int RoadSamples=0,OnRoadSamples=0;
    double Start=0, Last=0, NextSample=0, NextLoot=0, Distance=0, Stuck=0, HouseWaitUntil=0;
    double MinMemory=DBL_MAX, MaxMemory=0, FirstMemory=0, LastMemory=0;
    float Seconds=720, StartClock=0, MissingGround=0;
    FVector Previous=FVector::ZeroVector;
    double LastProgress=0,JetUntil=0,NextBoost=0,BestRange=DBL_MAX;
    int32 RangeWaypoint=INDEX_NONE;
    TArray<FVector> Route;
    TArray<double> Frames;
    TSet<FIntPoint> VisitedRegions, Cities;
    TSet<int32> WeatherKinds;
    TSet<FString> LootIDs;
    TMap<FString,FIntPoint> LootHomes;
    TSet<FString> UnloadedLoot, RegeneratedLoot;
    FString Report;

    void Check(bool OK,const FString& Label)
    {
        ++Checks; if(!OK) ++Failures;
        Report+=FString::Printf(TEXT("%s %s\n"),OK?TEXT("PASS"):TEXT("FAIL"),*Label);
    }
    AETItem* Find(AETWorld* W,const FString& Id)
    {
        for(TActorIterator<AETItem> It(W->GetWorld());It;++It) if(It->Id==Id) return *It;
        return nullptr;
    }
    bool Use(AETWorld* W,AETCharacter* P,AETItem* I,bool StagePosition)
    {
        if(!IsValid(I)) return false;
        if(!P->Panel.IsEmpty()) P->ClosePanel();
        const FVector Aim=I->Mesh->Bounds.Origin;
        FCollisionQueryParams Params; Params.AddIgnoredActor(P);
        for(int32 Candidate=0;Candidate<(StagePosition?48:1);++Candidate) {
            if(StagePosition) {
                const double Angle=(Candidate%16)*2*PI/16;
                const double Radius=200+(Candidate/16)*60;
                FVector At=I->GetActorLocation()+FVector(FMath::Cos(Angle)*Radius,FMath::Sin(Angle)*Radius,0);
                const double Ground=W->Height(At.X,At.Y);
                FHitResult Floor;
                const FVector Probe(At.X,At.Y,FMath::Max(Ground+240,I->GetActorLocation().Z+210));
                At.Z=W->GetWorld()->LineTraceSingleByChannel(Floor,Probe,FVector(At.X,At.Y,Ground-200),ECC_Visibility,Params)?Floor.ImpactPoint.Z+96:Ground+110;
                if(W->GetWorld()->OverlapBlockingTestByChannel(At,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Params)) continue;
                P->SetActorLocation(At); P->GetCharacterMovement()->StopMovementImmediately(); P->FallSpeed=0;
            }
            const FVector Eye=P->Camera->GetComponentLocation(), Direction=(Aim-Eye).GetSafeNormal();
            if(FVector::Dist(Eye,Aim)>395) continue;
            FHitResult Hit;
            if(W->GetWorld()->LineTraceSingleByChannel(Hit,Eye,Aim+Direction*5,ECC_Visibility,Params)&&Hit.GetActor()==I) {
                P->GetController()->SetControlRotation(Direction.Rotation()); P->Focus=I; P->Interact(); return true;
            }
        }
        return false;
    }
    void UseID(AETWorld* W,AETCharacter* P,const FString& Id)
    {
        Check(Use(W,P,Find(W,Id),true),TEXT("Reach and interact with ")+Id);
    }
    void Story(AETWorld* W,AETCharacter* P)
    {
        auto S=W->State;
        Check(W->Review&&W->SaveSlot==TEXT("EARTHtotaled-QA-")+W->JourneyEnding,TEXT("Scenario is isolated from the survivor slot"));
        Check(P->Panel==TEXT("story")&&S->Cores==0&&S->Ending.IsEmpty(),TEXT("Fresh survivor begins with the opening recording"));
        P->ClosePanel();
        UseID(W,P,TEXT("uplink"));
        Check(P->Panel.IsEmpty()&&S->Ending.IsEmpty(),TEXT("HELIOS rejects an incomplete archive"));
        UseID(W,P,TEXT("core-1"));
        Check(S->Cores==0&&!S->Consumed.Contains(TEXT("core-1")),TEXT("Out-of-order core stays available"));
        UseID(W,P,TEXT("bunker")); P->Action(TEXT("welcome")); P->Action(TEXT("quest")); P->ClosePanel();
        Check(S->Water==5&&S->Quests.FindRef(TEXT("filter"))==1,TEXT("Nora provides welcome water and starts the filter quest"));
        for(int32 Core=0;Core<3;++Core) {
            const FString Id=FString::Printf(TEXT("core-%d"),Core);
            UseID(W,P,Id);
            Check(S->Cores==Core+1&&S->Consumed.Contains(Id)&&P->Panel==TEXT("story"),Id+TEXT(" advances the story once and opens its recording"));
            P->ClosePanel();
            if(Core==0) UseID(W,P,TEXT("filter"));
            if(Core==1) UseID(W,P,TEXT("pump"));
        }
        Check(S->Unlocked.Contains(1)&&S->Unlocked.Contains(2),TEXT("Archive progression unlocks both story weapons"));
        UseID(W,P,TEXT("bunker"));
        const int32 Water=S->Water, Wrappers=S->Gold;
        P->Action(TEXT("quest")); P->Action(TEXT("quest")); P->ClosePanel();
        Check(S->Quests.FindRef(TEXT("filter"))==2&&S->Water==Water+3&&S->Gold==Wrappers+40&&S->Unlocked.Contains(4),TEXT("Filter hand-in rewards water, wrappers and rifle exactly once"));
        UseID(W,P,TEXT("transit-aid")); P->Action(TEXT("quest")); P->Action(TEXT("quest")); P->ClosePanel();
        Check(S->Quests.FindRef(TEXT("pump"))==2,TEXT("Ivo accepts the recovered pump and completes his quest"));
        for(const TCHAR* Id:{TEXT("starter-pantry"),TEXT("starter-tools"),TEXT("starter-electronics")}) UseID(W,P,Id);
        UseID(W,P,TEXT("bunker"));
        const int32 TradeWater=S->Water, Scrap=S->Scrap;
        P->Action(TEXT("supply-run")); P->ClosePanel();
        Check(Scrap>=12&&S->Water==TradeWater+2&&S->Scrap==Scrap-12,TEXT("Scavenged supplies fund a human water reward"));
        UseID(W,P,TEXT("uplink"));
        Check(P->Panel==TEXT("ending"),TEXT("Three cores expose the final choice at HELIOS"));
        P->Action(W->JourneyEnding);
        Check(S->Ending==W->JourneyEnding&&P->Panel==TEXT("story")&&P->PanelTitle==(W->JourneyEnding==TEXT("garden")?TEXT("THE FIRST RAIN"):TEXT("A LIGHT IN THE DARK")),TEXT("Selected ending displays its own epilogue"));
        Check(W->StoryTitle()==P->PanelTitle&&!W->StoryObjective().Contains(TEXT("uplink")),TEXT("Completed-story HUD acknowledges the ending and continued exploration"));
        P->ClosePanel();
        auto PC=Cast<APlayerController>(P->GetController());
        Check(!UGameplayStatics::IsGamePaused(W)&&PC&&!PC->IsLookInputIgnored()&&!PC->bShowMouseCursor,TEXT("Returning from the ending restores play and captured mouse mode"));
        UseID(W,P,TEXT("uplink"));
        Check(P->Panel==TEXT("story"),TEXT("Revisiting HELIOS recalls the completed ending instead of offering a new choice"));
        P->Action(W->JourneyEnding==TEXT("garden")?TEXT("signal"):TEXT("garden"));
        Check(S->Ending==W->JourneyEnding,TEXT("An already completed survivor cannot overwrite their ending"));
        P->ClosePanel();
        UseID(W,P,TEXT("starter-medical"));
        Check(S->Consumed.Contains(TEXT("starter-medical")),TEXT("Scavenging still works after the epilogue"));
        P->Journal(); Check(P->PanelBody.StartsWith(W->StoryTitle()),TEXT("Journal retains the completed ending above ongoing quests")); P->ClosePanel();
        W->Save();
        auto Loaded=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(W->SaveSlot,0));
        Check(Loaded&&Loaded->Ending==W->JourneyEnding&&Loaded->Cores==3&&Loaded->Consumed.Contains(TEXT("core-2"))&&Loaded->Quests.FindRef(TEXT("filter"))==2&&Loaded->Water==S->Water&&Loaded->Gold==S->Gold,TEXT("Real disk save retains ending, archive, quest and rewards"));
    }
    void Resume(AETWorld* W,AETCharacter* P)
    {
        Check(W->State->Ending==W->JourneyEnding&&W->State->Cores==3,TEXT("A new process restores the completed ending"));
        Check(W->State->Quests.FindRef(TEXT("filter"))==2&&W->State->Quests.FindRef(TEXT("pump"))==2,TEXT("Completed human quests survive restarting the game"));
        bool Gone=true; for(const TCHAR* Id:{TEXT("core-0"),TEXT("core-1"),TEXT("core-2"),TEXT("starter-medical")}) Gone&=!Find(W,Id)&&W->State->Consumed.Contains(Id);
        Check(Gone,TEXT("Recovered cores and looted story caches do not respawn after restart"));
        UseID(W,P,TEXT("uplink"));
        Check(P->Panel==TEXT("story")&&P->PanelTitle==(W->JourneyEnding==TEXT("garden")?TEXT("THE FIRST RAIN"):TEXT("A LIGHT IN THE DARK")),TEXT("HELIOS recalls the correct saved epilogue"));
        P->ClosePanel();
    }
    void Finish(AETWorld* W,AETCharacter* P,bool Endurance)
    {
        if(Finished) return; Finished=true;
        const double Duration=FPlatformTime::Seconds()-Start;
        if(Endurance) {
            Check(W->State->Health>0&&W->State->Ending==W->JourneyEnding,TEXT("Survivor remains alive with the same ending after the journey"));
            Check(Distance/100>Seconds*3.5,TEXT("Continuous travel made sustained progress through the world"));
            Check(VisitedForest&&Cities.Num()>0&&CityTransitions>=2,TEXT("Journey crosses forest and city boundaries in both directions"));
            Check(Seconds<700||(WasDay&&WasNight&&W->State->Elapsed-StartClock>=2*ETDay::Period&&WeatherKinds.Num()==5),TEXT("Long run includes two accelerated day/night cycles and all five weather types"));
            Check(BoundsOK&&MaxRegions<=80,TEXT("Region retention stays bounded throughout exploration"));
            Check(GroundOK,TEXT("Terrain collision remains present under the moving character"));
            Check(UniqueItems,TEXT("Active loot IDs stay unique and consumed caches stay empty"));
            Check(Seconds<700||Looted>=2,TEXT("Procedural city looting works during continuous travel"));
            Check(Seconds<700||RegeneratedLoot.Num()>0,TEXT("Looted city regions unload and regenerate without refilling consumed containers"));
            if(FollowRoad) {
                Check(RoadSamples>=12&&OnRoadSamples>=RoadSamples*.9,FString::Printf(TEXT("Walked the charted road on rendered forest tiles / %d of %d samples on pavement"),OnRoadSamples,RoadSamples));
                Check(NewCityBearing&&Looted>0,TEXT("Nearest-city bearing switched to the outlying refuge and the survivor looted the destination city"));
            }
        }
        P->FireStop(); P->SprintStop(); W->Save();
        const FString Header=FString::Printf(TEXT("ET_JOURNEY %s %d checks / %d failures\n"),*W->JourneyEnding,Checks,Failures);
        FFileHelper::SaveStringToFile(Header+Report,*(FPaths::ProjectSavedDir()/TEXT("journey-verification.txt")));
        Frames.Sort(); double Total=0; for(double T:Frames) Total+=T;
        const double Average=Frames.Num()?Total/Frames.Num():0;
        const double P95=Frames.Num()?Frames[FMath::FloorToInt((Frames.Num()-1)*.95)]:0;
        const double P99=Frames.Num()?Frames[FMath::FloorToInt((Frames.Num()-1)*.99)]:0;
        int32 Hitches=0; for(double T:Frames) if(T>100) ++Hitches;
        const FString Json=FString::Printf(TEXT("{\"ending\":\"%s\",\"checks\":%d,\"failures\":%d,\"wall_seconds\":%.3f,\"clock_seconds\":%.3f,\"clock_rate\":13,\"movement_rate\":1,\"distance_m\":%.1f,\"regions_visited\":%d,\"cities_visited\":%d,\"city_transitions\":%d,\"max_loaded_regions\":%d,\"max_actors\":%d,\"weather_types\":%d,\"looted_containers\":%d,\"water_trades\":%d,\"bottles_drunk\":%d,\"frame_samples\":%d,\"average_ms\":%.3f,\"p95_ms\":%.3f,\"p99_ms\":%.3f,\"frames_over_100ms\":%d,\"first_working_set_mb\":%.1f,\"last_working_set_mb\":%.1f,\"peak_working_set_mb\":%.1f,\"scope\":\"Scripted story interactions; continuous character movement with collision; 13x weather clock, normal survival; no manual playthrough\"}"),*W->JourneyEnding,Checks,Failures,Duration,W->State->Elapsed-StartClock,Distance/100,VisitedRegions.Num(),Cities.Num(),CityTransitions,MaxRegions,MaxActors,WeatherKinds.Num(),Looted,Trades,Drinks,Frames.Num(),Average,P95,P99,Hitches,FirstMemory,LastMemory,MaxMemory);
        FFileHelper::SaveStringToFile(Json,*(FPaths::ProjectSavedDir()/TEXT("journey-metrics.json")));
        FPlatformMisc::RequestExitWithStatus(true,Failures?1:0);
    }
};
}

void AETWorld::TickJourneyVerification(float Delta)
{
    static FJourney Q;
    if(Q.Finished) return;
    auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)); if(!P||!P->GetController()) return;
    const double Now=FPlatformTime::Seconds();
    if(!Q.Started) {
        Q.Started=true; Q.Start=Now; Q.Last=Now;
        FParse::Value(FCommandLine::Get(),TEXT("ETJourneySeconds="),Q.Seconds); Q.Seconds=FMath::Clamp(Q.Seconds,30.f,3600.f);
        IFileManager::Get().MakeDirectory(*FPaths::ProjectSavedDir(),true);
        if(JourneyResume) Q.Resume(this,P); else Q.Story(this,P);
        if(JourneyResume||FParse::Param(FCommandLine::Get(),TEXT("ETStoryOnly"))||Q.Failures) {Q.Finish(this,P,false); return;}
        // Start both paths on the edge of the baked district; all subsequent motion is physical.
        const double Sign=JourneyEnding==TEXT("garden")?1:-1;
        Q.FollowRoad=FParse::Param(FCommandLine::Get(),TEXT("ETFollowRoad"));
        FVector Start=Q.FollowRoad?FVector(Sign*15000,2600,0):FVector(Sign*3000,-22000,0);
        Start.Z=Height(Start.X,Start.Y)+100;
        P->SetActorLocation(Start); P->GetCharacterMovement()->StopMovementImmediately(); P->FallSpeed=0;
        auto Add=[&](double X,double Y){Q.Route.Add(FVector(Sign*X,Y,0));};
        if(Q.FollowRoad) {
            const double Other=Sign*ETNav::DistrictSpacing;
            ETNav::Roads(FBox2D({FMath::Min(0.,Other),-3500},{FMath::Max(0.,Other),3500}),[&](const ETNav::FRoad& Road) {
                if(!Road.Highway||!Road.Horizontal()) return;
                for(auto Bend:{Road.A,Road.B}) if(Bend.X*Sign>25000&&Bend.X*Sign<135000) Q.Route.AddUnique(FVector(Bend,0));
            });
            Q.Route.Sort([Sign](FVector A,FVector B){return A.X*Sign<B.X*Sign;});
        } else {Add(40000,-22000); Add(130000,-1700);}
        for(double City:{160000.0,320000.0}) {
            // Use the south entrance of a real city house, then return to its street.
            Add(City-1600,-1700); Add(City-1600,-750); Add(City-1600,40);
            Add(City-1600,-750); Add(City-1600,-1700); Add(City+30000,-1700);
        }
        Add(320000-1600,-1700); Add(320000-1600,-750); Add(320000-1600,40);
        Add(320000-1600,-750); Add(320000-1600,-1700); Add(190000,-1700);
        Add(160000-1600,-1700); Add(160000-1600,-750); Add(160000-1600,40);
        Add(160000-1600,-750); Add(160000-1600,-1700); Add(40000,-22000);
        Q.LastProgress=FPlatformTime::Seconds(); Q.Previous=Start; Q.StartClock=State->Elapsed; Q.Start=FPlatformTime::Seconds(); Q.Last=Q.Start; Q.NextSample=Q.Start+5;
        Q.LastCity=InCity(Start); P->SprintStart();
        FFileHelper::SaveStringToFile(TEXT("seconds,x_m,y_m,clock,regions,actors,working_set_mb,health,water,hydration,weather,waypoint,looted,regenerated_loot\n"),*(FPaths::ProjectSavedDir()/TEXT("journey-samples.csv")));
        return;
    }
    const double Age=Now-Q.Start, Frame=Now-Q.Last; Q.Last=Now;
    if(Age>15) Q.Frames.Add(Frame*1000);
    if(State->Health<=0) {Q.Check(false,TEXT("Survivor died during continuous traversal")); Q.Finish(this,P,true); return;}
    if(Age>=Q.Seconds) {Q.Finish(this,P,true); return;}
    if(!P->Panel.IsEmpty()) P->ClosePanel();
    State->Elapsed+=Delta*12; // Weather and daylight only, explicitly recorded in the report.
    const FVector At=P->GetActorLocation();
    const double Moved=FVector::Dist2D(At,Q.Previous); Q.Distance+=Moved; Q.Previous=At;
    if(Now-Q.LastProgress>=1.0) {
        const double Remaining=FVector::Dist2D(At,Q.Route[Q.Waypoint]);
        if(Q.RangeWaypoint!=Q.Waypoint||Remaining<Q.BestRange-65.) {Q.BestRange=Remaining;Q.RangeWaypoint=Q.Waypoint;Q.Stuck=0;}
        else Q.Stuck+=Now-Q.LastProgress;
        Q.LastProgress=Now;
    }
    if(Q.JetUntil>0&&Now>=Q.JetUntil) {P->JumpRelease(); Q.JetUntil=0;}
    else if(Q.JetUntil>0&&Q.JetUntil-Now<1.50&&P->GetCharacterMovement()->IsFalling()&&!P->AirJumpSpent) {
        P->JumpRelease(); P->JumpPress();
    }
    if(Q.Stuck>25) {Q.Check(false,TEXT("Physical route blocked at ")+At.ToCompactString()+TEXT(" waypoint ")+FString::FromInt(Q.Waypoint)); Q.Finish(this,P,true); return;}
    if(State->Hydration<55&&State->Water>0) {P->Drink(); ++Q.Drinks;}
    if(State->Health<60&&State->Meds>0) P->Heal();
    const bool City=InCity(At); if(City!=Q.LastCity) {++Q.CityTransitions; Q.LastCity=City;}
    Q.VisitedForest|=!City&&FVector::Dist2D(At,CityCenter(At.X,At.Y))>38000;
    if(City&&At.Size2D()>40000) {auto C=CityCenter(At.X,At.Y); Q.Cities.Add(FIntPoint(FMath::RoundToInt(C.X/160000),FMath::RoundToInt(C.Y/160000)));}
    Q.WasDay|=Hour()>=6&&Hour()<19; Q.WasNight|=Hour()<6||Hour()>=19; Q.WeatherKinds.Add(ETClimate::Sample(State->Elapsed,State->Seed).Kind);
    const FIntPoint Tile(FMath::FloorToInt(At.X/RegionSize),FMath::FloorToInt(At.Y/RegionSize)); Q.VisitedRegions.Add(Tile);
    FCollisionQueryParams Params; Params.AddIgnoredActor(P); FHitResult Ground;
    if(GetWorld()->LineTraceSingleByChannel(Ground,At,At-FVector(0,0,P->JetUsed?1800:400),ECC_Visibility,Params)) Q.MissingGround=0;
    else Q.MissingGround+=Delta;
    if(Q.MissingGround>2) Q.GroundOK=false;
    if(Now>=Q.NextSample) {
        Q.NextSample=Now+5; ++Q.Samples;
        if(Q.FollowRoad&&Q.Samples==6) FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/TEXT("blackpine-road.png"),true,false);
        if(Q.FollowRoad&&FMath::Abs(At.X)>39000&&FMath::Abs(At.X)<120000&&Q.Waypoint<=2) {
            ++Q.RoadSamples;
            if(auto* R=Regions.FindRef(Tile);R&&R->RoadMesh&&R->OnHighway(FVector2D(At))) ++Q.OnRoadSamples;
            Q.NewCityBearing|=ETNav::District(FVector2D(At))!=FIntPoint::ZeroValue;
        }
        int32 Actors=0; for(TActorIterator<AActor> It(GetWorld());It;++It) ++Actors;
        Q.MaxActors=FMath::Max(Q.MaxActors,Actors); Q.MaxRegions=FMath::Max(Q.MaxRegions,Regions.Num());
        for(auto It:Regions) Q.BoundsOK&=RegionDistance(It.Key,At)<=State->ViewDistance*100+8000+RegionSize+1000;
        TSet<FString> IDs;
        for(TActorIterator<AETItem> It(GetWorld());It;++It) {Q.UniqueItems&=!IDs.Contains(It->Id)&&!State->Consumed.Contains(It->Id); IDs.Add(It->Id);}
        for(auto It:Q.LootHomes) {
            auto R=Regions.FindRef(It.Value);
            if(!R) Q.UnloadedLoot.Add(It.Key);
            else if(R->Stage==6&&Q.UnloadedLoot.Contains(It.Key)) Q.RegeneratedLoot.Add(It.Key);
        }
        const double Memory=FPlatformMemory::GetStats().UsedPhysical/1048576.0;
        if(Q.Samples==1) Q.FirstMemory=Memory; Q.LastMemory=Memory; Q.MaxMemory=FMath::Max(Q.MaxMemory,Memory); Q.MinMemory=FMath::Min(Q.MinMemory,Memory);
        const FString Row=FString::Printf(TEXT("%.2f,%.1f,%.1f,%.1f,%d,%d,%.1f,%.1f,%d,%.1f,%d,%d,%d,%d\n"),Age,At.X/100,At.Y/100,State->Elapsed,Regions.Num(),Actors,Memory,State->Health,State->Water,State->Hydration,Weather.Kind,Q.Waypoint,Q.Looted,Q.RegeneratedLoot.Num());
        FFileHelper::SaveStringToFile(Row,*(FPaths::ProjectSavedDir()/TEXT("journey-samples.csv")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);
    }
    if(Now>=Q.NextLoot) {
        Q.NextLoot=Now+1;
        TArray<AETItem*> Nearby;
        for(TActorIterator<AETItem> It(GetWorld());It;++It) if(FVector::DistSquared2D(It->GetActorLocation(),At)<400*400&&(It->Kind==TEXT("container")||It->Kind==TEXT("loot")||It->Kind==TEXT("gold")||It->Kind==TEXT("human"))) Nearby.Add(*It);
        for(auto I:Nearby) {
            const FString Id=I->Id, Kind=I->Kind;
            auto Region=Cast<AETRegion>(I->GetOwner());
            if(!Q.Use(this,P,I,false)) continue;
            if(Kind==TEXT("human")) {
                const int Before=State->Water;
                if(State->Scrap>=12) P->Action(TEXT("supply-run"));
                if(State->Water>Before) ++Q.Trades;
                P->ClosePanel();
            } else if((Kind==TEXT("loot")||Kind==TEXT("container"))&&State->Consumed.Contains(Id)&&!Q.LootIDs.Contains(Id)) {
                ++Q.Looted; Q.LootIDs.Add(Id); if(Region) Q.LootHomes.Add(Id,Region->Coord);
            }
        }
    }
    FVector Goal=Q.Route[Q.Waypoint]; Goal.Z=At.Z;
    if(FVector::Dist2D(At,Goal)<110) {
        if(Goal.Y==40) {
            if(Q.HouseWaitUntil==0) Q.HouseWaitUntil=Now+2;
            if(Now<Q.HouseWaitUntil) {P->GetCharacterMovement()->StopMovementImmediately(); Q.Stuck=0; return;}
        }
        Q.HouseWaitUntil=0; Q.Waypoint=(Q.Waypoint+1)%Q.Route.Num(); Goal=Q.Route[Q.Waypoint]; Goal.Z=At.Z;
    }
    // Steer back toward the centerline after avoiding an obstruction on a long straight leg.
    if(Q.FollowRoad&&FMath::IsNearlyEqual(Goal.Y,-1700.,1.)&&FMath::Abs(At.X)>190000.&&FMath::Abs(Goal.X-At.X)>4200.)
        Goal.X=At.X+FMath::Sign(Goal.X-At.X)*4200.;
    FVector Direction=(Goal-At).GetSafeNormal2D();
    // Steer around real tree trunks, walls and wildlife without teleporting through them.
    double Best=-DBL_MAX; FVector Move=Direction;
    const double TerrainZ=SurfaceHeight(At.X,At.Y); const bool OnTerrain=FMath::Abs(At.Z-TerrainZ-92)<150;
    for(double Angle:{0.0,25.0,-25.0,50.0,-50.0,80.0,-80.0,110.0,-110.0,150.0,-150.0,180.0}) {
        FVector Candidate=Direction.RotateAngleAxis(Angle,FVector::UpVector); FHitResult Hit;
        const FVector From=At+FVector(0,0,25);
        FVector Ahead=From+Candidate*650;
        if(OnTerrain) Ahead.Z+=FMath::Clamp(SurfaceHeight(Ahead.X,Ahead.Y)-TerrainZ,-450.,450.);
        const bool Block=GetWorld()->SweepSingleByChannel(Hit,From,Ahead,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(38,65),Params);
        const double Clear=Block?Hit.Time:1.0;
        const double Score=Clear*3+FVector::DotProduct(Candidate,Direction);
        if(Score>Best) {Best=Score; Move=Candidate;}
    }
    if(Q.Stuck>1.8&&Now>=Q.NextBoost&&State->JetFuel>=25) {
        P->JumpPress(); Q.JetUntil=Now+1.75; Q.NextBoost=Now+4;
    } else if(!Q.JetUntil) P->StopJumping();
    if(Q.JetUntil>Now) Move=Direction;
    P->SprintStart(); P->AddMovementInput(Move,1); P->GetController()->SetControlRotation(Move.Rotation());
    // Fight only a threat in range, using the survivor's actual ammunition and reload.
    AETAgent* Threat=nullptr; double Nearest=DBL_MAX;
    for(TActorIterator<AETAgent> It(GetWorld());It;++It) {
        const double D=FVector::DistSquared2D(It->GetActorLocation(),At);
        if(!It->Awake()||It->Kind==TEXT("deer")||D>FMath::Square(It->Kind==TEXT("robot")?3600:1100)||D>=Nearest) continue;
        const FVector Aim=It->Parts.Num()?It->Parts[0]->Bounds.Origin:It->GetActorLocation()+FVector(0,0,60); FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,P->Camera->GetComponentLocation(),Aim,ECC_Visibility,Params)&&Hit.GetActor()==*It) {Nearest=D; Threat=*It;}
    }
    if(Threat) {
        P->GetController()->SetControlRotation((Threat->Parts[0]->Bounds.Origin-P->Camera->GetComponentLocation()).Rotation());
        if(State->Ammo[State->Weapon]==0) {P->FireStop(); P->Reload();} else P->FireStart();
    } else P->FireStop();
}
