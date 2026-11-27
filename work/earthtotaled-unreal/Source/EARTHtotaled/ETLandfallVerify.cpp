#include "ETGame.h"
#include "ETLandfall.h"
#include "ETGeology.h"
#include "Camera/CameraActor.h"
#include "Components/StaticMeshComponent.h"
#include "ProceduralMeshComponent.h"
#include "Kismet/GameplayStatics.h"

void AETWorld::RunLandfallChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    auto& S=*State; S.ArrivalStage=0;S.TrenchSides=0;S.EquippedRelic=INDEX_NONE;S.Builds.Empty();S.Relics.Reset();
    Geology->Initialize(this);Check(!Geology->IsOpen(FVector(3250,-2750,-750)),TEXT("Fresh Shelter 09 has a solid floor over the old survey tunnel"));
    P->StartArrival();const auto View=Cast<APlayerController>(P->GetController());
    Check(P->Panel==TEXT("arrival")&&P->FlightCamera&&View&&View->GetViewTarget()==P->FlightCamera,TEXT("Opening flight takes control of the camera"));
    P->ArrivalTime=FPlatformTime::Seconds()-11;P->TickArrival();
    Check(P->FlightCue==2&&P->FlightCamera->GetActorLocation().Z<12000,TEXT("The cockpit descends over the actual shelter"));
    P->ArrivalTime=FPlatformTime::Seconds()-16;P->TickArrival();
    Check(P->FlightCue==4&&IFileManager::Get().FileSize(*(FPaths::ProjectContentDir()/TEXT("Data/arrival-line.wav")))>44100,TEXT("The crashed pilot delivers the opening line"));
    P->ArrivalTime=FPlatformTime::Seconds()-20;P->TickArrival();
    Check(S.ArrivalStage==1&&S.IntroRead&&P->Panel.IsEmpty()&&S.Weapon==ETLandfall::Pistol&&S.Unlocked.Contains(ETLandfall::Pistol)&&!S.Unlocked.Contains(0),TEXT("Landfall hands the survivor an unlimited-reserve pistol and the first watch quest"));
    MakeTrenchStakes();FString Why;AETItem* Stake=nullptr;
    for(auto* Actor:StoryActors) if(auto* A=Cast<AETItem>(Actor);IsValid(A)&&A->Kind==TEXT("stake")&&A->Amount==0) Stake=A;
    bool Blasted=false;
    if(Stake) {FHitResult Face(Stake,Stake->Mesh,Stake->GetActorLocation(),FVector::UpVector);Blasted=Geology->Blast(Face,Why);}
    Check(Blasted&&S.TrenchSides==1&&Geology->SurfaceOpening(0,-2280)<0,TEXT("The player's borer opens physical ground along the marked bank"));
    Check(!Geology->BugCanStep(FVector(0,-2650,8),FVector(0,-1920,8))&&Geology->BugCanStep(FVector(0,-2800,8),FVector(110,-2800,8)),TEXT("Ground Bugs stop at the trench edge and still move on the bank"));
    const int Water=S.Water;for(int Side=1;Side<4;++Side) Geology->CutTrench(Side,Why);
    Check(S.ArrivalStage==2&&S.TrenchSides==15&&Water+3==S.Water&&!Geology->CutTrench(3,Why),TEXT("Four connected banks finish the quest and award supplies once"));
    const FIntPoint Town(1,0);S.Towns.FindOrAdd(Town).Claimed=true;
    const FVector Inside(FVector2D(Town)*ETNav::DistrictSpacing+FVector2D(19000,9000),0);
    Check(SettledCity(Inside)&&!AddThreat(TEXT("landfall-patrol"),TEXT("runner"),Inside)&&!FrontFighter(TEXT("landfall-front"),TEXT("robot"),Inside),TEXT("Neither patrols nor faction units can spawn across a settled district"));
    FRandomStream LootRng(2253);int Rare=0;bool WildlifeClear=true;
    for(int I=0;I<80000;++I){Rare+=ETLandfall::RollDrop(LootRng,true)>0;WildlifeClear&=!ETLandfall::RollDrop(LootRng,false);}
    Check(Rare>128&&Rare<275&&WildlifeClear,FString::Printf(TEXT("Rare roll yielded %d guns in 80000 hostile rolls and none from deer"),Rare));
    const auto From=P->GetActorLocation();
    auto* Drop=LooseItem(TEXT("landfall-drop"),TEXT("relic.731"),1,From,From,false);
    Check(Drop&&P->TakeLoose(Drop)&&S.Relics.Num()==1&&!S.DropPositions.Contains(TEXT("landfall-drop")),TEXT("A rolled gun is a persistent pickup and enters the rack once"));
    P->EquipRelic(0);auto Before=P->CurrentGun();auto& G=S.Relics[0];const ETLandfall::FRoll Roll(G.Seed),Other(834);
    Check(Roll.First!=Roll.Second&&Roll.Name!=Other.Name&&Before.Name==Roll.Name&&P->CurrentRank()==1,TEXT("Recovered guns have two distinct perks and their own rolled stats"));
    const float BaseXP=S.WeaponDamage[S.Weapon];P->CurrentGunDamage()=Roll.Next(1)+1;
    Check(P->CurrentRank()==2&&S.WeaponDamage[S.Weapon]==BaseXP&&P->CurrentThreshold(2)>0,TEXT("Each specimen earns its own rank along the rolled upgrade path"));
    P->CurrentMagazine()=0;P->Reload();P->Tick(P->ReloadTime+.01f);
    Check(P->CurrentMagazine()==P->CurrentGun().Mag,TEXT("Unique gun reloads its own magazine with unlimited reserves"));
    const auto Portable=FPaths::ProjectSavedDir()/TEXT("landfall.etsave");FString Message;
    const bool Stored=P->ExportProfile(State,Portable,Message);const auto* Copy=Stored?P->ReadProfile(Portable,Message):nullptr;
    Check(Copy&&Copy->TrenchSides==15&&Copy->EquippedRelic==0&&Copy->Relics.Num()==1&&Copy->Relics[0].DamageDealt==G.DamageDealt,TEXT("Portable slots preserve the trench, equipped gun, magazine and individual progression"));
}
