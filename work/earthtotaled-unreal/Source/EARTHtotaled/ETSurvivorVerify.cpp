#include "ETGame.h"
#include "ETGeology.h"
#include "ETRing.h"
#include "ETRailDistricts.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "ProceduralMeshComponent.h"

void AETWorld::RunSurvivorChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    auto& S=*State;
    bool AllQuarterStock=true,AllModels=true;
    TSet<FString> Businesses,Landmarks;
    for(int City=0;City<2*ETRing::Stops;++City) {
        const auto& Quarter=ETRailDistrict::Get(City);
        Landmarks.Add(Quarter.Landmark);
        const FString Asset=FString(TEXT("/Game/EARTHtotaled/Art/SM_Quarter"))+Quarter.Landmark;
        const auto* Mesh=LoadObject<UStaticMesh>(nullptr,*Asset);
        AllModels&=Mesh&&Mesh->GetBounds().BoxExtent.Z>350;
        for(const auto& Place:Quarter.Places) {
            Businesses.Add(Place.Name); const auto* Entries=LootTables.Find(Place.Stock);
            AllQuarterStock&=Entries&&Entries->Num()>=2;
        }
    }
    Check(AllModels&&Landmarks.Num()==16,TEXT("All sixteen different station landmarks are present in the cooked world"));
    Check(AllQuarterStock&&Businesses.Num()==32,TEXT("Every station has two named businesses with working loot tables"));
    for(const int Line:{ETRing::NorthLink,ETRing::SouthLink}) {
        const double HalfLeg=ETRing::Dwell+(ETRing::ShuttleLeg()-ETRing::Dwell)*.5;
        const auto A=ETRing::ServiceAt(Line,HalfLeg),B=ETRing::ServiceAt(Line,ETRing::ShuttleLeg()+HalfLeg);
        Check(A.Stop==1&&B.Stop==0&&A.Direction==-B.Direction&&FMath::IsNearlyEqual(A.Angle,.5)&&FMath::IsNearlyEqual(B.Angle,.5)
            &&A.Speed==ETRing::Cruise&&B.Speed==A.Speed,TEXT("Straight shuttle crosses the same midpoint in each direction at full speed"));
        const auto Start=ETRing::Dock(Line,0),End=ETRing::Dock(Line,1);
        Check(Start.Y==End.Y&&End.X-Start.X==306400.&&ETRing::ServiceAt(Line,ETRing::ShuttleLeg()).Docked,
            TEXT("Shuttle stops and reverses at a real platform on the other loop"));
    }
    bool RailSupportProtected=false,OpenSpanMinable=false;
    for(int I=0;I<ETRing::Spans&&(!RailSupportProtected||!OpenSpanMinable);I+=2) {
        const auto Pier=ETRing::Position(I*UE_TWO_PI/ETRing::Spans);
        const auto Span=ETRing::Position((I+1)*UE_TWO_PI/ETRing::Spans);
        const TCHAR* Reason=Geology->DigRestriction(FBox2D(Pier-FVector2D(120),Pier+FVector2D(120)));
        RailSupportProtected|=Reason&&FCString::Strstr(Reason,TEXT("Ringline"));
        OpenSpanMinable|=!Geology->DigRestriction(FBox2D(Span-FVector2D(400),Span+FVector2D(400)));
    }
    Check(RailSupportProtected&&OpenSpanMinable,TEXT("Concrete railway footings reject excavation while open spans allow mining"));
    EnsureRegion({1,0});
    auto* Tile=Regions.FindChecked({1,0});
    auto Pulse=[&](FVector At,bool Square,FString& Why) {
        FHitResult Face(Tile,Tile->Terrain,At,FVector::ForwardVector);
        Face.ImpactPoint=At; Face.ImpactNormal=FVector::ForwardVector;
        return Geology->Blast(Face,Why,Square);
    };
    const int Count=S.BoreCuts.Num()+S.SquareCuts.Num(),Veins=S.MinedVeins.Num(),Fallen=S.FallenRock.Num();
    const auto Inventory=S.Inventory;
    FString Why;
    for(bool Square:{false,true}) {
        bool AllProtected=true;
        for(FVector2D At:{FVector2D(0,-4000),FVector2D(46900,2600),FVector2D(46900,3650),ETNav::Site({-1,0},0).Center,ETNav::Site({0,0},0).Center}) {
            AllProtected&=!Pulse(FVector(At,-19000),Square,Why)&&Why.Contains(TEXT("Protected"));
        }
        Check(AllProtected,Square?TEXT("Square tunnels are denied below towns, both faction capitals and the edge of a highway"):TEXT("Round tunnels are denied below towns, both faction capitals and the edge of a highway"));
    }
    const FVector Pad(46900,7200,SurfaceHeight(46900,7200)); const FGuid Key=FGuid::NewGuid();
    for(int Piece:{0,1,2,3,AETConstruction::Core,AETConstruction::Turret,AETConstruction::Miner}) {
        FETBuild B; B.Piece=Piece; B.Facing=1; B.Anchor=Pad; S.Builds.Add(Key,B);
        const auto Center=AETConstruction::Shape(B).GetTranslation();
        Check(!Pulse(Center-FVector(0,0,18000),false,Why)&&Why.Contains(TEXT("foundation")),FString(TEXT("Deep mining respects the saved footprint of "))+AETConstruction::Name(Piece));
    }
    S.Builds.Remove(Key);
    Check(!Geology->DigRestriction(FBox2D(FVector2D(Pad)-FVector2D(300),FVector2D(Pad)+FVector2D(300))),
        TEXT("Removing the last structure frees its wilderness claim"));
    Check(S.BoreCuts.Num()+S.SquareCuts.Num()==Count&&S.MinedVeins.Num()==Veins&&S.FallenRock.Num()==Fallen&&S.Inventory.OrderIndependentCompareEqual(Inventory),
        TEXT("Denied edits never change terrain, loose rock, ore or inventory"));
    P->StopBuilding(); P->Panel.Empty(); P->Command(TEXT("time night")); P->Command(TEXT("heal")); P->SetActorLocation(Pad+FVector(0,0,110));
    const auto Spawn=Pad+FVector(1050,0,0);
    auto* Bug=FrontFighter(TEXT("qa-reaction-bug"),TEXT("runner"),Spawn);
    auto* Tech=FrontFighter(TEXT("qa-reaction-tech"),TEXT("robot"),Spawn+FVector(900,0,0));
    if(Bug&&Tech) {
        Bug->UpdatePresence(Pad); Tech->UpdatePresence(Pad);
        Check(Sounds.FindRef(TEXT("Hit"))&&Sounds.FindRef(TEXT("FieldLatch")),TEXT("Both the shell impact and Supertech metal layer load from the packaged game"));
        for(auto* Unit:{Bug,Tech}) {
            auto* Source=Unit==Bug?Tech:Bug; const float HP=Unit->Health;
            Unit->TakeDamage(12,FDamageEvent(),nullptr,Source);
            const bool Active=Unit->Health==HP-12&&Unit->FlinchTime>0&&!Unit->FlinchVelocity.IsNearlyZero()&&Unit->ImpactCooldown>0&&ImpactAttenuation;
            Unit->TickImpact(.04f);
            Check(Active&&!Unit->Parts[0]->GetRelativeRotation().IsNearlyZero(),FString(Unit==Bug?TEXT("Bug"):TEXT("Supertech"))+TEXT(" takes damage, flinches and starts spatial hit audio when its rival lands a hit"));
            Unit->TickImpact(.4f);
            Check(Unit->Parts[0]->GetRelativeRotation().IsNearlyZero(),TEXT("Combatant returns to its normal pose after a hit"));
        }
        P->TakeDamage(7,FDamageEvent(),nullptr,Tech); P->TickImpact(.02f);
        Check(P->SuitImpactCooldown>0&&P->SuitFlinch>0&&!P->Gun->GetRelativeRotation().IsNearlyZero(),TEXT("The survivor receives suit impact audio and first-person recoil"));
        P->TickImpact(.4f);
        Check(P->SuitFlinch==0&&P->Gun->GetRelativeRotation().IsNearlyZero(),TEXT("First-person recoil settles without changing the survivor's aim"));
    } else Check(false,TEXT("Both faction recipients spawn for hit feedback"));
    S.Level=1;S.XP=90;S.Points=0;S.Skills={TEXT("survivor")};
    P->AddXP(235);
    Check(S.Level==3&&S.XP==25&&S.Points==2&&P->LevelNoticeTime>0,TEXT("XP carries through two level thresholds and grants two spendable points"));
    P->Skills(); P->Action(TEXT("perk:climber2")); P->Action(TEXT("learn-perk"));
    Check(!P->Has(TEXT("climber2"))&&S.Points==2,TEXT("Selecting a distant perk cannot skip its route"));
    P->Action(TEXT("perk:climb")); Check(!P->Has(TEXT("climb"))&&S.Points==2,TEXT("Inspecting Climbing Grip previews it before spending a point"));
    P->Action(TEXT("learn-perk")); P->Action(TEXT("learn-perk"));
    P->Action(TEXT("perk:mantle")); P->Action(TEXT("learn-perk"));
    Check(P->Has(TEXT("climb"))&&P->Has(TEXT("mantle"))&&S.Points==0,TEXT("Connected climbing perks unlock for their listed cost and cannot be bought twice"));
    TArray<uint8> Bytes; auto* Restored=UGameplayStatics::SaveGameToMemory(State,Bytes)?Cast<UETSave>(UGameplayStatics::LoadGameFromMemory(Bytes)):nullptr;
    Check(Restored&&Restored->Level==3&&Restored->XP==25&&Restored->Skills.Contains(TEXT("mantle"))&&Restored->Points==0,TEXT("Experience, level, spent points and perk path survive a save round trip"));
    P->ClosePanel();
}
