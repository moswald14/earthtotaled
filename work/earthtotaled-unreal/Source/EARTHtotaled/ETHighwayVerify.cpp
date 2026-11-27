#include "ETGame.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void AETWorld::RunHighwayChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    const auto Machine=ETNav::Site({0,0},0),Hive=ETNav::Site({-1,0},0);
    Check(Hive.Bug&&!Machine.Bug&&Hive.Id()!=Machine.Id()&&Hive.Center.X<0&&Machine.Center.X>0,
        TEXT("H7 has different faction settlements east and west of Shelter 09"));
    for(const auto& S:{Machine,Hive}) {
        const auto Box=FBox2D(S.Center-FVector2D(1),S.Center+FVector2D(1));
        int Copies=0; ETNav::Sites(Box,[&](const ETNav::FSite& R){if(R.Id()==S.Id()&&R.Center.Equals(S.Center)) ++Copies;});
        Check(Copies==1,FString(S.Name())+TEXT(" has a stable unique position on the atlas"));
        const FVector2D Approach=S.At(0,-4750);
        const FIntPoint Tile(FMath::FloorToInt32(Approach.X/RegionSize),FMath::FloorToInt32(Approach.Y/RegionSize));
        P->SetActorLocation(FVector(Approach,SurfaceHeight(Approach.X,Approach.Y)+105)); EnsureRegion(Tile);
        const FIntPoint Heart(FMath::FloorToInt32(S.Center.X/RegionSize),FMath::FloorToInt32(S.Center.Y/RegionSize));
        EnsureRegion(Heart);
        auto* R=Regions.FindChecked(Heart); AStaticMeshActor* Landmark=nullptr; AETItem* Vault=nullptr;
        for(auto* A:R->Residents) if(IsValid(A)) {
            if(auto* Building=Cast<AStaticMeshActor>(A);Building&&FVector2D(Building->GetActorLocation()).Equals(S.Center,2)) Landmark=Building;
            if(auto* Item=Cast<AETItem>(A);Item&&Item->Id==S.Id()+TEXT("/vault")) Vault=Item;
        }
        Check(Landmark&&Vault&&Landmark->GetStaticMeshComponent()->Bounds.BoxExtent.Z>1600,
            FString(S.Name())+TEXT(" streams a monumental collidable interior and a working salvage cache"));
        if(Landmark) {
            const auto Tr=Landmark->GetActorTransform(); FHitResult Hit;
            FCollisionQueryParams Q(SCENE_QUERY_STAT(ETHighwayDoor),true);
            const bool Open=!Landmark->GetStaticMeshComponent()->LineTraceComponent(Hit,Tr.TransformPosition({0,-2400,100}),Tr.TransformPosition({0,0,100}),Q);
            Check(Open,FString(S.Name())+TEXT(" has a clear street-level passage through the landmark to its cache"));
        }
        const auto Chart=ETNav::Atlas({1600,900},S.Center,55000);
        P->UseMapPoint(Chart.Plot(S.Center),Chart,false);
        Check(State->MapPins.IsValidIndex(State->TrackedPin)&&State->MapPins[State->TrackedPin].Equals(S.Center,.1),
            FString(S.Name())+TEXT(" can be selected directly on the M map"));
    }
    P->Command(TEXT("time day")); AtRefuge=false;
    const FVector Pad(10000,23000,18000);
    AETAgent* Bug=FrontFighter(TEXT("front/verify/bug"),TEXT("spitter"),Pad);
    AETAgent* Tech=FrontFighter(TEXT("front/verify/tech"),TEXT("robot"),Pad+FVector(1050,0,0));
    if(!Bug||!Tech) {Check(false,TEXT("Both highway companies spawn"));return;}
    Bug->UpdatePresence(Pad); Tech->UpdatePresence(Pad);
    Check(!IsNight()&&Bug->FindRival(1)==Tech&&Tech->FindRival(1)==Bug,TEXT("Both factions acquire their opposing unit in daylight"));
    const int BeforeXP=State->XP,BeforeKills=State->Kills,BeforeDrops=State->DropPositions.Num(),BeforeTech=SupertechHits;
    const FVector From=Tech->AimPoint()+FVector(-150,0,0);
    if(auto* Bolt=GetWorld()->SpawnActor<AETEnemyBolt>(From,FRotator::ZeroRotator)) {
        Bolt->Launch(Tech,Bug->AimPoint(),false); const float HP=Bug->Health;
        for(int N=0;N<80&&IsValid(Bolt);++N) Bolt->Tick(.016f);
        Check(Bug->Health<HP&&SupertechHits>BeforeTech,TEXT("A Supertech projectile applies real damage to a Bug in daylight"));
    }
    const auto BugId=Bug->Id,TechId=Tech->Id;
    Tech->TakeDamage(1000,FDamageEvent(),nullptr,Bug);
    Check(State->XP==BeforeXP&&State->Kills==BeforeKills&&State->DropPositions.Num()>BeforeDrops&&State->FrontLosses.Contains(TechId),
        TEXT("A rival kill records a casualty and spills salvage without granting survivor XP or kills"));
    Check(FrontUnits.Contains(TechId+TEXT("/shard0"))&&FrontUnits.Contains(TechId+TEXT("/shard1")),
        TEXT("A destroyed Supertech front-line hull launches two fighting drones"));
    Check(!FrontFighter(TechId,TEXT("robot"),Pad),TEXT("Defeated highway soldier waits for reinforcements"));
    const auto Retake=State->FrontLosses.FindRef(TechId); State->Elapsed=Retake+1;
    Check(FrontFighter(TechId,TEXT("robot"),Pad+FVector(1100,0,0))!=nullptr,TEXT("A fresh reinforcement takes the vacant position on the campaign clock"));
    P->SetActorLocation({0,-1500,115}); FrontClock=0; TickWarFronts(P,1);
}

void AETWorld::TickHighwayTour(AETCharacter* P)
{
    static int Stage=0,DayBug=0,DayTech=0,NightBug=0,NightTech=0;
    static float StartingHP=0,StartingArmor=0;
    static double Start=FPlatformTime::Seconds(),PhaseStart=Start;
    static bool DayPassed=false,DayUnharmed=false,RoadView=false;
    if(Stage>=4) return;
    const double Now=FPlatformTime::Seconds();
    FString Scene; FParse::Value(FCommandLine::Get(),TEXT("ETReview="),Scene);
    const auto S=ETNav::Site(Scene==TEXT("bug")?FIntPoint(-1,0):FIntPoint::ZeroValue,0);
    auto Picture=[&](const FString& Name){FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/(Name+TEXT(".png")),true,false);};
    if(Stage==0) {
        WeatherOverride=ETClimate::Clear; State->IntroRead=true; DayBug=BugHits; DayTech=SupertechHits;
        StartingHP=State->Health; StartingArmor=State->Armor; ++Stage; return;
    }
    if(Stage==1&&Now-Start>3.) {Picture(Scene); ++Stage;return;}
    if(Stage==2&&!RoadView&&Now-Start>4.5) {
        RoadView=true;
        const auto At=S.Exit-S.Inward*1680-S.Along*3800;
        const FVector Focus(S.Exit+S.Inward*2500,SurfaceHeight(S.Exit.X,S.Exit.Y)+450);
        P->SetActorLocation(FVector(At,SurfaceHeight(At.X,At.Y)+104)); P->GetCharacterMovement()->StopMovementImmediately();
        P->GetController()->SetControlRotation((Focus-P->GetActorLocation()).Rotation());
    }
    if(Stage==2&&((Now-Start>8&&BugHits>DayBug&&SupertechHits>DayTech)||Now-Start>33)) {
        DayUnharmed=State->Armor==StartingArmor&&State->Health==StartingHP;
        DayPassed=BugHits>DayBug&&SupertechHits>DayTech&&DayUnharmed;
        Picture(Scene+TEXT("-highway")); NightBug=BugHits; NightTech=SupertechHits;
        PhaseStart=Now; ++Stage;return;
    }
    if(Stage!=3) return;
    if(!IsNight()&&Now-PhaseStart>.6) {
        P->Command(TEXT("time night"));
        const auto At=S.Exit-S.Inward*2550-S.Along*1900;
        P->SetActorLocation(FVector(At,SurfaceHeight(At.X,At.Y)+105)); P->GetCharacterMovement()->StopMovementImmediately();
        P->GetController()->SetControlRotation(FVector(S.Center-At,1250).Rotation());
    }
    P->EnemyReacquireTime=8;
    if(Now-PhaseStart<35&&!(Now-PhaseStart>5&&BugHits>NightBug&&SupertechHits>NightTech)) return;
    const bool Pass=DayPassed&&IsNight()&&BugHits>NightBug&&SupertechHits>NightTech;
    FString Report=FString::Printf(TEXT("ET_FRONT %s %s\nDaytime Bug hits %d, Supertech hits %d, survivor unharmed %d\nNighttime Bug hits %d, Supertech hits %d\nActive fighters %d\n"),*Scene,Pass?TEXT("PASS"):TEXT("FAIL"),NightBug-DayBug,NightTech-DayTech,DayUnharmed?1:0,BugHits-NightBug,SupertechHits-NightTech,FrontUnits.Num());
    if(!Pass) for(const auto& Entry:FrontUnits) {
        if(auto* Fighter=Entry.Value;IsValid(Fighter)) {
            const auto* Rival=Fighter->Rival.Get();
            Report+=FString::Printf(TEXT("%s %s HP %.1f at %s, rival %s, separation %.1fm\n"),*Fighter->Id,*Fighter->Kind,Fighter->Health,
                *Fighter->GetActorLocation().ToCompactString(),Rival?*Rival->Id:TEXT("none"),Rival?FVector::Distance(Fighter->GetActorLocation(),Rival->GetActorLocation())/100.:0.);
        }
    }
    FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("front-tour.txt")));
    ++Stage; FPlatformMisc::RequestExitWithStatus(true,Pass?0:1);
}
