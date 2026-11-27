#include "ETGame.h"
#include "Engine/DamageEvents.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "InputCoreTypes.h"

void AETCharacter::HiddenTools()
{
    if(Panel==TEXT("devtools")) {ClosePanel();return;}
    const auto* PC=Cast<APlayerController>(Controller);
    if(!PC||!Panel.IsEmpty()||
        !(PC->IsInputKeyDown(EKeys::LeftControl)||PC->IsInputKeyDown(EKeys::RightControl))||
        !(PC->IsInputKeyDown(EKeys::LeftShift)||PC->IsInputKeyDown(EKeys::RightShift))) return;
    OpenPanel(TEXT("devtools"),TEXT("SURVIVOR TEST TOOLS"),TEXT("Changes apply to this survivor. Use a spare save slot for experiments."));
}
void AETCharacter::ReclamationAction(const FString& Id)
{
    auto& S=*World->State;
    if(Id==TEXT("draw-water")&&Panel.IsEmpty()&&IsValid(Focus)&&Focus->Kind==TEXT("tower")) {
        const auto Cell=FIntPoint(int32(Focus->Destination.X),int32(Focus->Destination.Y));
        if(S.Elapsed<S.TowerRefills.FindRef(Cell)) {World->Say(TEXT("Tap is dry until the next dawn."));return;}
        const int N=3+World->SeedFor(Cell,717)%3;S.Water+=N;
        double Until=ETDay::Phase(ETDay::At(6)-ETDay::Phase(S.ClockOffset+S.Elapsed));
        S.TowerRefills.Add(Cell,S.Elapsed+(Until<1.?ETDay::Period:Until));
        World->Sound(TEXT("Drink"));World->Say(FString::Printf(TEXT("Hill cistern / +%d bottles. Refills at dawn."),N));World->Save();return;
    }
    if(Id==TEXT("back")) {ClosePanel();return;}
    const auto Here=ETNav::District(FVector2D(GetActorLocation()));
    auto Infested=[&](FIntPoint Start) {
        FIntPoint Best=Start; double Distance=1.e30;
        for(int Y=-4;Y<=4;++Y) for(int X=-4;X<=4;++X) {const auto C=Start+FIntPoint(X,Y);
            if(World->TownRemaining(C)<=0) continue;
            const double D=FVector2D::DistSquared(FVector2D(GetActorLocation()),ETNav::Refuge(C));if(D<Distance) {Distance=D;Best=C;}
        }return Best;
    };
    if(Panel==TEXT("devtools")) {
        if(Id==TEXT("cheat-shield")) {TestInvincible=!TestInvincible;PanelBody=TestInvincible?TEXT("Invincibility enabled for this play session."):TEXT("Invincibility off.");}
        else if(Id==TEXT("cheat-supply")) {S.Gold+=200;S.Meds+=3;S.Water+=4;S.Inventory.FindOrAdd(TEXT("ironore"))+=25;S.Inventory.FindOrAdd(TEXT("goldore"))+=15;PanelBody=TEXT("Added 200 wrappers, 3 medkits, 4 water, 25 iron ore and 15 gold ore.");}
        else if(Id==TEXT("cheat-day")||Id==TEXT("cheat-night")) PanelBody=Command(Id==TEXT("cheat-day")?TEXT("time day"):TEXT("time night"));
        else if(Id==TEXT("cheat-tower")||Id==TEXT("cheat-town")) {
            FVector P=Id==TEXT("cheat-tower")?World->TowerLocation(Here)+FVector(440,0,0):FVector(ETNav::Refuge(Infested(Here))+FVector2D(500,-300),0);
            World->StreamAround(P,true);P.Z=World->SurfaceHeight(P.X,P.Y)+110;
            GetCharacterMovement()->StopMovementImmediately();SetActorLocation(P,false,nullptr,ETeleportType::TeleportPhysics);
            ClosePanel();return;
        } else if(Id==TEXT("cheat-clear")) {
            World->TownTick=0;World->TickTowns(this,1);TArray<TWeakObjectPtr<AETAgent>> Targets;World->TownUnits.GenerateValueArray(Targets);
            for(auto Target:Targets) if(Target.IsValid()&&Target->TownCell==Here) Target->TakeDamage(10000,FDamageEvent(),Controller,this);
            PanelBody=World->TownStatus(Here);
        } else if(Id==TEXT("cheat-loot")) {
            const auto P=GetActorLocation();const FString Batch=TEXT("test-loot-")+FGuid::NewGuid().ToString();
            for(int I=0;I<6;++I) {auto Land=P+GetActorForwardVector()*(130.+I*440.);Land.Z=World->SurfaceHeight(Land.X,Land.Y)+12;World->LooseItem(Batch+FString::FromInt(I),TEXT("gold"),1,Land+FVector(0,0,100),Land,true);}
            PanelBody=TEXT("Six wrappers scattered ahead. Stand by one and hold Ctrl.");
        }
        return;
    }
    if(Panel!=TEXT("town")) return;
    if(Id==TEXT("mark-tower")||Id==TEXT("mark-town")) {
        const FVector2D At=Id==TEXT("mark-tower")?FVector2D(World->TowerLocation(BoardCell)):ETNav::Refuge(Infested(BoardCell));
        const int Existing=S.MapPins.IndexOfByPredicate([&](FVector2D P){return FVector2D::DistSquared(At,P)<100.;});
        if(Existing!=INDEX_NONE) S.TrackedPin=Existing;
        else if(S.MapPins.Num()<ETNav::MaxPins) S.TrackedPin=S.MapPins.Add(At);
        else {PanelBody=TEXT("Atlas full. Remove a pin on the field map and try again.");return;}
        World->Save();PanelBody=TEXT("Marked and tracked on your compass and field map.");return;
    }
    auto* T=S.Towns.Find(BoardCell);if(!T||!T->Claimed) return;
    int Price=0;
    if(Id==TEXT("deposit")) Price=20;
    else if(Id==TEXT("walls")&&T->Defenses==0) Price=48;
    else if(Id==TEXT("sentries")&&T->Defenses==1) Price=90;
    else if(Id==TEXT("repair-town")&&T->Defenses>0) Price=12;
    if(!Price) return;
    if(S.Gold<Price) {PanelBody=FString::Printf(TEXT("You need %d candybar wrappers. You have %d."),Price,S.Gold);return;}
    S.Gold-=Price;
    if(Id==TEXT("deposit")) T->Treasury+=Price;
    else {if(Id!=TEXT("repair-town")) ++T->Defenses;World->TownFortifications(BoardCell,T->Defenses,Id==TEXT("repair-town"));}
    World->Save();TownPanel(BoardCell);
}
