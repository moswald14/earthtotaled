#include "ETGame.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

void AETItem::Toss(FVector From,FVector Landing)
{
    SpillOrigin=From; Destination=Landing; SpillAge=0;
    SetActorLocation(From); SetActorTickEnabled(true);
}
void AETItem::Tick(float Delta)
{
    Super::Tick(Delta); SpillAge=FMath::Min(1.f,SpillAge+Delta/.48f);
    SetActorLocation(FMath::Lerp(SpillOrigin,Destination,SpillAge)+FVector(0,0,240.f*SpillAge*(1-SpillAge)));
    if(SpillAge>=1) SetActorTickEnabled(false);
}
FString AETWorld::DropLabel(const FString& Kind) const
{
    if(const auto* Find=ModFind(Kind)) return Find->Name;
    if(Kind==TEXT("gold")) return TEXT("Candybar wrappers");
    if(Kind==TEXT("chitin")) return TEXT("Bug shell");
    if(Kind==TEXT("harvest")) return TEXT("Salvaged hide");
    if(Kind==TEXT("ironore")) return TEXT("Iron ore");
    if(Kind==TEXT("goldore")) return TEXT("Gold ore");
    if(Kind==TEXT("gunparts")) return TEXT("Mechanical parts");
    if(Kind==TEXT("medkit")) return TEXT("Field medkit");
    if(Kind==TEXT("cloth")) return TEXT("Cloth roll");
    return Kind;
}
AETItem* AETWorld::LooseItem(const FString& Id,const FString& Kind,int32 Count,FVector From,FVector Ground,bool Animate)
{
    if(State->DropPositions.Contains(Id)||State->Consumed.Contains(Id)) return nullptr;
    auto* A=Item(Id,Kind,DropLabel(Kind),Ground,Count);
    if(!A) return nullptr;
    State->DropPositions.Add(Id,Ground); State->DropAmounts.Add(Id,Count); State->DropKinds.Add(Id,Kind); LooseActors.Add(Id,A);
    A->Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    const FIntPoint Cell(FMath::FloorToInt(Ground.X/RegionSize),FMath::FloorToInt(Ground.Y/RegionSize));
    if(auto* Region=Regions.FindRef(Cell)) {A->SetOwner(Region); Region->Residents.AddUnique(A);}
    if(Animate) A->Toss(From,Ground); return A;
}
void AETWorld::SpillLoot(AETAgent* Victim,bool Credited)
{
    auto* P=Cast<AETCharacter>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!P) return;
    // Repeating battlefield soldiers share one unclaimed pile per kind and firing position.
    const FString Base=Victim->Id+TEXT("-drop");
    FRandomStream R(GetTypeHash(Base)^State->Seed); int Slot=0;
    const auto From=Victim->GetActorLocation()+FVector(0,0,58);
    auto Spill=[&](const FString& Kind,int N) {
        const FString Id=Slot==0?Base:Base+FString::Printf(TEXT("-%d"),Slot); ++Slot;
        const float Yaw=R.FRandRange(0.f,360.f),Radius=R.FRandRange(48.f,115.f);
        FVector End=From+FRotator(0,Yaw,0).Vector()*Radius; FHitResult Hit;
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ETLootSpill),false,Victim); Q.AddIgnoredActor(P);
        if(GetWorld()->LineTraceSingleByChannel(Hit,From,End,ECC_Visibility,Q)) End=Hit.Location-FRotator(0,Yaw,0).Vector()*12.;
        FVector Land=End-FVector(0,0,48);
        if(GetWorld()->LineTraceSingleByChannel(Hit,End+FVector(0,0,6),End-FVector(0,0,2400),ECC_Visibility,Q)) Land=Hit.ImpactPoint+FVector(0,0,7);
        LooseItem(Id,Kind,N,From,Land,true);
    };
    const bool Walker=Victim->Kind==TEXT("walker"),Hive=Victim->Kind==TEXT("hive");
    const FString Source=Walker?TEXT("zombie"):Victim->IsMachine()?TEXT("supertech"):Victim->IsBrood()?TEXT("bug"):TEXT("wildlife");
    if(Hive) {Spill(TEXT("ironore"),12);Spill(TEXT("goldore"),6);}
    else if(Victim->Kind==TEXT("robot")) {Spill(TEXT("gold"),Credited?FMath::RoundToInt(16*P->LootRate()):5);Spill(TEXT("gunparts"),1);}
    else if(Victim->IsMachine()) {Spill(TEXT("gunparts"),1);Spill(TEXT("scrap"),1);}
    else if(Walker) {Spill(TEXT("gold"),R.RandRange(3,7));Spill(R.RandRange(0,4)?TEXT("cloth"):TEXT("medkit"),1);}
    else {Spill(Victim->IsBrood()?TEXT("chitin"):TEXT("harvest"),1);Spill(TEXT("scrap"),2);}
    for(const auto& Pack:Mods) if(Pack.Enabled) for(const auto& Find:Pack.Finds)
        if(Find.Source==Source&&R.RandRange(1,100)<=Find.Chance) Spill(Find.Key,1);
}
void AETCharacter::TickShield(float D)
{
    auto& S=*World->State;
    const float Quiet=FMath::Max(0.f,D-S.ShieldDelay);
    S.ShieldDelay=FMath::Max(0.f,S.ShieldDelay-D);
    S.Shield=FMath::Min(S.MaxShield(),S.Shield+Quiet*2.f);
}
void AETCharacter::CrouchLoot()
{
    if(!Panel.IsEmpty()||Climbing||SweepHeld) return;
    Crouch(); SweepHeld=true; SweepArmed=false; SweepHold=SweepClock=0;
}
void AETCharacter::ReleaseLoot()
{
    SweepHeld=SweepArmed=false;SweepHold=SweepClock=0;UnCrouch();
}
bool AETCharacter::TakeLoose(AETItem* A)
{
    if(!IsValid(A)||!World->State->DropPositions.Contains(A->Id)) return false;
    auto& S=*World->State; const auto Kind=S.DropKinds.FindRef(A->Id); const int N=FMath::Max(1,S.DropAmounts.FindRef(A->Id));
    if(Kind==TEXT("gold")) S.Gold+=N;
    else if(Kind==TEXT("medkit")) S.Meds+=N;
    else if(Kind==TEXT("scrap")) S.Scrap+=N;
    else if(Kind==TEXT("chitin")) {S.BuildStock+=N; AddXP(10);}
    else if(Kind==TEXT("harvest")) {S.Scrap+=4; S.Inventory.FindOrAdd(TEXT("cloth"))+=2; AddXP(15);}
    else S.Inventory.FindOrAdd(Kind)+=N;
    if(!A->Id.StartsWith(TEXT("front/"))&&!A->Id.StartsWith(TEXT("patrol-"))&&!A->Id.StartsWith(TEXT("town/"))) S.Consumed.Add(A->Id);
    S.DropPositions.Remove(A->Id); S.DropAmounts.Remove(A->Id); S.DropKinds.Remove(A->Id); S.Settled.Remove(A->Id);
    World->LooseActors.Remove(A->Id); if(Focus==A) Focus=nullptr;
    World->Say(FString::Printf(TEXT("+%d %s"),N,*World->DropLabel(Kind))); A->Destroy(); return true;
}
void AETCharacter::TickSweep(float D)
{
    if(!SweepHeld||!bIsCrouched) return;
    if(SweepArmed) SweepHold=FMath::Min(5.f,SweepHold+D);
    SweepClock-=D;if(SweepClock>0) return; SweepClock=.12f;
    const double Radius=250.+(3600.-250.)*SweepHold/5.;
    const auto Feet=GetActorLocation(),Eye=Feet+FVector(0,0,28);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETSalvageSweep),false,this);
    TArray<AETItem*> Nearby;
    for(auto It=World->LooseActors.CreateIterator();It;++It) {
        auto* A=It.Value().Get(); if(!A) {It.RemoveCurrent();continue;}
        if(A->SpillAge<1||FVector::DistSquared(Feet,A->GetActorLocation())>Radius*Radius) continue;
        FHitResult Hit;
        if(!GetWorld()->LineTraceSingleByChannel(Hit,Eye,A->GetActorLocation()+FVector(0,0,8),ECC_Visibility,Q)||Hit.GetActor()==A) Nearby.Add(A);
    }
    if(Nearby.IsEmpty()) return;
    SweepArmed=true;
    for(auto* A:Nearby) TakeLoose(A);
    World->Sound(TEXT("Pickup"),.4f); World->Save();
}
