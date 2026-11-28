#include "ETCombat.h"
#include "ETGame.h"
#include "ETDayCycle.h"
#include "ETAgentMovement.h"
#include "Components/BoxComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace {
    AActor* TestBlock(UWorld* World,FVector Center,FVector Extent) {
        auto* Actor=World->SpawnActor<AActor>();
        auto* Box=NewObject<UBoxComponent>(Actor);
        Actor->SetRootComponent(Box);
        Box->SetBoxExtent(Extent);
        Box->SetCollisionProfileName(TEXT("BlockAll"));
        Box->RegisterComponent();
        Actor->SetActorLocation(Center);
        return Actor;
    }
    AETAgent* FindResident(AETWorld& W,AETCharacter& P,bool City,FVector& PlayerGround) {
        FIntPoint Cell(1,0);
        if(City) {
            // A returning survivor may already have reclaimed the eastern town.
            for(int X=1;X<=32&&W.TownRemaining(Cell)==0;++X)Cell=FIntPoint(X+1,0);
            if(!W.TownRemaining(Cell))return nullptr;
            const FVector2D Town=FVector2D(Cell)*ETNav::DistrictSpacing;
            for(int X:{-1,1})for(int Y:{-1,1}){
                const FVector2D Lane=Town+FVector2D(X*4500,Y*6500);
                W.EnsureRegion({FMath::FloorToInt32(Lane.X/W.RegionSize),FMath::FloorToInt32(Lane.Y/W.RegionSize)});
            }
            P.SetActorLocation(FVector(Town,W.SurfaceHeight(Town.X,Town.Y)+94));
            W.TownTick=0;W.TickTowns(&P,1);
        }else W.EnsureRegion({-2,-2});
        for(TActorIterator<AETAgent> It(W.GetWorld());It;++It) {
            if(City?(It->TownIndex<0||It->TownCell!=Cell):
                (It->Kind!=TEXT("wolf")||!It->Id.StartsWith(TEXT("r-2,-2-"))))continue;
            const FVector Feet=It->GetActorLocation();
            FCollisionQueryParams Query(SCENE_QUERY_STAT(ETEncounterLane),false,*It);
            Query.AddIgnoredActor(&P);
            for(int I=0;I<12;++I) {
                FVector Stand=Feet+FRotator(0,I*30.,0).Vector()*730.;
                if(!ETAgentMovement::FindGroundSpawn(W,Stand)||W.SafeSettlement(Stand)||W.InCity(Stand)!=City)continue;
                if(W.GetWorld()->LineTraceTestByChannel(Feet+FVector(0,0,65),Stand+FVector(0,0,86),ECC_Visibility,Query))continue;
                PlayerGround=Stand-FVector(0,0,8);return *It;
            }
        }
        return nullptr;
    }
}

// Opt-in route in a disposable profile. Let the engine schedule AI, movement, projectiles and damage.
void ETCombat::EncounterTrial(AETWorld& W,AETCharacter& P) {
    static int Stage=0,Failures=0;
    static FString Report;
    static FVector Platform,Center,Origin;
    static TWeakObjectPtr<AETAgent> Unit;
    static TWeakObjectPtr<AActor> Wall;
    static float Began=0,LastVitals=0,Damage=0;
    static double Travel=0,Approach=0;
    const TCHAR* Types[]={TEXT("walker"),TEXT("runner"),TEXT("bulwark"),TEXT("spitter"),TEXT("robot"),
        TEXT("drone"),TEXT("skimmer"),TEXT("wolf"),TEXT("boar"),TEXT("bear"),TEXT("runner"),TEXT("robot"),TEXT("hive"),TEXT("wolf"),TEXT("walker")};
    constexpr int Detour=10,DayPatrol=11,Hive=12,Woodland=13,City=14;
    const FString File=FPaths::ProjectSavedDir()/TEXT("encounter-checks.txt");
    auto Result=[&](bool Ok,const FString& Name){
        Failures+=!Ok;Report+=(Ok?TEXT("PASS "):TEXT("FAIL "))+Name+TEXT("\n");FFileHelper::SaveStringToFile(Report,*File);
    };
    auto NextStage=[&](){
        Unit.Reset();
        if(++Stage==UE_ARRAY_COUNT(Types)){
            Report+=FString::Printf(TEXT("TOTAL %d failures\n"),Failures);
            FFileHelper::SaveStringToFile(Report,*File);
            FPlatformMisc::RequestExitWithStatus(false,Failures?2:0);
        }
    };
    if(Stage>=UE_ARRAY_COUNT(Types))return;
    auto& S=*W.State;
    if(Platform.IsZero()) {
        W.Review=true;P.Panel.Empty();UGameplayStatics::SetGamePaused(&W,false);
        Platform={-47600,-46300,0};Platform.Z=W.SurfaceHeight(Platform.X,Platform.Y)+4800;
        TestBlock(W.GetWorld(),Platform-FVector(0,0,30),FVector(3600,1350,30));
        P.GetCharacterMovement()->DisableMovement();S.ArrivalStage=2;S.NextPatrol=100000;
        P.TestInvincible=false;P.EnemyReacquireTime=0;S.Armor=0;
    }
    if(!Unit.IsValid()) {
        const FString Kind=Types[Stage];
        S.ClockOffset=ETDay::At(Stage==DayPatrol||Stage==City?12:22)-S.Elapsed;
        W.LastNight=W.IsNight();W.AtRefuge=false;
        Center=Platform;AETAgent* A=nullptr;
        if(Stage>=Woodland) {
            A=FindResident(W,P,Stage==City,Center);
        }else{
            const bool Flying=Kind==TEXT("drone")||Kind==TEXT("skimmer");
            const double Range=Kind==TEXT("robot")?2375:Kind==TEXT("spitter")?1575:Flying?1700:880;
            A=W.GetWorld()->SpawnActor<AETAgent>(Center+FVector(-Range,0,Flying?210:8),FRotator(0,180,0));
            if(A){A->FrontUnit=Stage==DayPatrol;A->Configure(&W,FString::Printf(TEXT("test-encounter-%d"),Stage),Kind);}
        }
        if(!A){Result(false,FString::Printf(TEXT("No live %s at encounter %d"),*Kind,Stage));NextStage();return;}
        P.SetActorLocation(Center+FVector(0,0,94),false,nullptr,ETeleportType::TeleportPhysics);
        P.RecentEnemy.Reset();P.CombatTime=0;S.Health=P.MaxHealth();S.Shield=S.MaxShield();S.ShieldDelay=100;
        A->UpdatePresence(P.GetActorLocation());A->AttackTime=.3f;Unit=A;Origin=A->GetActorLocation();
        LastVitals=S.Health+S.Shield;Began=S.Elapsed;Travel=0;Approach=0;Damage=0;
        if(Stage==Detour){
            Wall=TestBlock(W.GetWorld(),Center+FVector(-435,0,115),FVector(58,145,115));
            A->PlayerAggro=4;
        }
        return;
    }
    auto* A=Unit.Get();
    Travel=FMath::Max(Travel,FVector::Dist2D(Origin,A->GetActorLocation()));
    Approach=FMath::Max(Approach,FVector::Dist2D(Origin,Center)-FVector::Dist2D(A->GetActorLocation(),Center));
    const float Vitals=S.Health+S.Shield;
    if(P.RecentEnemy.Get()==A)Damage+=FMath::Max(LastVitals-Vitals,0.f);
    LastVitals=Vitals;
    const float Age=S.Elapsed-Began;
    if(Stage==Hive) {
        if(!S.Threats.Contains(A->Id+TEXT("/guard0"))&&Age<4)return;
        const auto* Guard=W.LiveThreats.FindRef(A->Id+TEXT("/guard0"));
        Result(IsValid(Guard)&&FMath::Abs(Guard->GetActorLocation().Z-A->GetActorLocation().Z)<65&&Travel<1,TEXT("Stationary hive calls a live guard on its own floor"));
    }else if(Stage==DayPatrol) {
        if(Age<3)return;
        Result(Travel>45&&Damage==0,TEXT("Daytime front guard patrols without attacking a neutral survivor"));
    }else {
        if((Damage<.5f||Approach<65)&&Age<12)return;
        const FString Name=Stage==City?TEXT("Resident walker on an infested street in daylight"):Stage==Woodland?TEXT("Resident wolf in a streamed forest at night"):
            Stage==Detour?TEXT("Bug detours around solid cover"):FString(Types[Stage])+TEXT(" on raised walkable ground");
        Result(Approach>65&&Damage>.5f&&A->Phase>.2f,FString::Printf(TEXT("%s / approached %.0f cm, dealt %.0f damage"),*Name,Approach,Damage));
    }
    if(Wall.IsValid()){Wall->Destroy();Wall.Reset();}
    for(TActorIterator<AETEnemyBolt> It(W.GetWorld());It;++It)if(It->GetOwner()==A)It->Destroy();
    A->Destroy();NextStage();
}
