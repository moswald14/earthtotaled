#include "ETGame.h"
#include "ETGeology.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

using Site=AETConstruction;

FIntPoint Site::Cost(int32 Piece)
{
    if(Piece==Core) return {2,1};
    if(Piece==Turret) return {3,1};
    if(Piece==Miner) return {4,1};
    return {0,0};
}
const TCHAR* Site::MaterialName(int32 T) {return T==2?TEXT("Metal"):T==1?TEXT("Stone"):TEXT("Wood");}
const TCHAR* Site::Resource(int32 T) {return T==2?TEXT("ironore"):T==1?TEXT("stone"):TEXT("wood");}
int32 Site::FrameCost(int32 T) {return T==2?2:4;}
float Site::Durability(int32 P,int32 T) {return P==Core?450:P==Turret?230:P==Miner?200:T==2?600:T==1?320:160;}
FString Site::Recipe(int32 P,int32 T)
{
    const auto C=Cost(P);
    return P<Core?FString::Printf(TEXT("%d %s"),FrameCost(T),T==2?TEXT("iron ore"):T==1?TEXT("stone"):TEXT("wood")):FString::Printf(TEXT("%d iron + %d gold ore"),C.X,C.Y);
}
FGuid AETWorld::NearestCore(FVector Position,double Radius) const
{
    FGuid Result; double Best=Radius*Radius;
    for(const auto& It:State->Builds) if(It.Value.Piece==Site::Core) {
        const double D=FVector::DistSquared(Position,It.Value.Anchor);
        if(D<=Best) {Best=D; Result=It.Key;}
    }
    return Result;
}
int32 AETWorld::BaseValue(FGuid Core) const
{
    const auto* Relay=State->Builds.Find(Core);
    if(!Relay||Relay->Piece!=Site::Core) return 0;
    int32 Value=0;
    for(const auto& It:State->Builds) {
        const auto& B=It.Value;
        if(FVector::DistSquared(B.Anchor,Relay->Anchor)>FMath::Square(7500.)) continue;
        Value+=(B.Piece==Site::Miner?17:B.Piece==Site::Turret?12:B.Piece==Site::Core?10:2)+FMath::Min(8,(B.Iron+B.GoldOre)/4);
    }
    return Value;
}
int32 AETWorld::ThreatTier() const
{
    int32 Score=State->ArmorTier+FMath::Min(3,State->Waves)+FMath::Min(2,(State->Level-1)/4);
    Score+=State->Skills.Contains(TEXT("parkour"))||State->Skills.Contains(TEXT("pressure"))?1:0;
    for(float Damage:State->WeaponDamage) if(Damage>=800) {++Score; break;}
    return FMath::Clamp(Score,0,6);
}
bool AETWorld::RaidMember(const FString& Id) const
{
    return State->RaidStage>0&&Id.StartsWith(FString::Printf(TEXT("raid-%d-"),State->RaidSerial));
}
void AETWorld::HitBuild(Site* Target,float Amount)
{
    if(!IsValid(Target)) return;
    if(auto* B=State->Builds.Find(Target->Id)) {
        B->Integrity=FMath::Max(0.f,B->Integrity-Amount);
        if(B->Integrity<=0) RemoveBuild(Target);
    }
}
void AETWorld::StepDevices(float D)
{
    TArray<Site*> Turrets;
    for(auto It=LiveBuilds.CreateIterator();It;++It) {
        auto* A=It.Value().Get();
        if(!A) {It.RemoveCurrent(); continue;}
        auto* B=State->Builds.Find(It.Key());
        if(!B) continue;
        if(B->Piece==Site::Miner) {
            if(B->Iron+B->GoldOre>=80) continue;
            B->Work+=D;
            A->Mechanism->AddLocalRotation(FRotator(0,200*D,0));
            if(B->Work>=30) {const bool Found=Geology->ExtractNear(B->Anchor,B->Iron,B->GoldOre); B->Work=Found?0:-30;}
        } else if(B->Piece==Site::Turret&&NearestCore(B->Anchor,7500).IsValid()) {
            B->Work=FMath::Max(0.f,B->Work-D); if(B->Work<=0) Turrets.Add(A);
        }
    }
    if(Turrets.IsEmpty()) return;
    TArray<AETAgent*> Targets;
    for(TActorIterator<AETAgent> It(GetWorld());It;++It) if(!It->WakeTruce()&&(It->Kind==TEXT("walker")||It->WakeEnemy()||(IsNight()&&(It->IsMachine()||It->IsBrood())))) Targets.Add(*It);
    for(auto* Turret:Turrets) {
        FCollisionQueryParams Trace(SCENE_QUERY_STAT(ETSentry),false,Turret);
        auto Origin=Turret->Mechanism->GetComponentLocation()+FVector(0,0,10);
        AETAgent* Target=nullptr; FVector Aim; double Best=FMath::Square(3400.);
        for(auto* A:Targets) {
            if(!IsValid(A)||A->Health<=0) continue;
            const FVector Point=A->GetActorLocation()+FVector(0,0,A->Kind==TEXT("drone")?0:A->Kind==TEXT("robot")?100:51);
            const double D2=FVector::DistSquared(Origin,Point);
            if(D2>Best) continue;
            FHitResult H;
            if(GetWorld()->LineTraceSingleByChannel(H,Origin,Point,ECC_Visibility,Trace)&&H.GetActor()!=A) continue;
            Best=D2; Aim=Point; Target=A;
        }
        if(!Target) continue;
        Turret->Mechanism->SetWorldRotation((Aim-Origin).Rotation());
        if(auto* B=State->Builds.Find(Turret->Id)) B->Work=.78f;
        if(auto* TraceActor=GetWorld()->SpawnActor<AETEnemyBolt>(Origin,FRotator::ZeroRotator)) TraceActor->Beam(this,Aim);
        Target->TakeDamage(19,FDamageEvent(),nullptr,Turret);
        if(auto* Cue=Sounds.FindRef(TEXT("ShardChirp"))) UGameplayStatics::PlaySoundAtLocation(this,Cue,Origin,.20f,.78f);
    }
}
bool AETWorld::Muster(FGuid Core)
{
    const auto* B=State->Builds.Find(Core);
    if(State->Wake.Winner()==ETWake::Bug||!IsNight()||State->RaidStage||!B||B->Piece!=Site::Core) return false;
    const FVector At=B->Anchor;
    const int32 Tier=ThreatTier(),Count=FMath::Clamp(4+Tier+BaseValue(Core)/55,4,13);
    FRandomStream R(State->Seed^((State->RaidSerial+1)*3517));
    const double Bearing=R.FRandRange(0,360);
    TArray<FVector> Line;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETMuster),false);
    for(int32 Side=0;Side<12&&Line.Num()<3;++Side) {
        Line.Reset();
        const FVector Out=FRotator(0,Bearing+Side*30,0).Vector(),Along(-Out.Y,Out.X,0);
        for(int32 I=0;I<Count;++I) {
            FVector P=At+Out*(2750+(I%2)*85)+Along*((I-(Count-1)*.5)*220);
            P.Z=SurfaceHeight(P.X,P.Y);
            FHitResult Floor;
            if(!GetWorld()->LineTraceSingleByChannel(Floor,P+FVector(0,0,320),P-FVector(0,0,280),ECC_Visibility,Q)||Floor.ImpactNormal.Z<.64||FMath::Abs(Floor.ImpactPoint.Z-P.Z)>90||Cast<AETAgent>(Floor.GetActor())||Cast<AETConstruction>(Floor.GetActor())||Cast<AETItem>(Floor.GetActor())) continue;
            P=Floor.ImpactPoint+FVector(0,0,8);
            if(!GetWorld()->OverlapBlockingTestByChannel(P+FVector(0,0,84),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(47,70),Q)) Line.Add(P);
        }
    }
    if(Line.Num()<3) return false;
    ++State->RaidSerial; State->RaidCore=Core; State->RaidStage=1; State->RaidTime=16;
    int32 Spawned=0;
    for(int32 I=0;I<Line.Num();++I) {
        FString Kind=I%4==0?TEXT("bulwark"):I%3==0?TEXT("spitter"):TEXT("runner");
        if(Tier>=2&&I==Line.Num()-1) {Kind=TEXT("skimmer"); Line[I].Z+=320;}
        const FString Id=FString::Printf(TEXT("raid-%d-%d"),State->RaidSerial,I);
        if(auto* A=AddThreat(Id,Kind,Line[I])) {A->SetActorRotation((At-Line[I]).Rotation()); ++Spawned;}
    }
    if(!Spawned) {State->RaidStage=0; State->RaidCore.Invalidate(); return false;}
    Say(TEXT("RELAY ALARM / carapace line forming. Turrets tracking.")); Sound(TEXT("BroodCall"),.85f,.7f); return true;
}
void AETWorld::EndRaid(bool Won)
{
    TArray<FString> Withdraw;
    for(const auto& It:State->Threats) if(RaidMember(It.Key)) Withdraw.Add(It.Key);
    for(const auto& Id:Withdraw) {
        if(auto* A=LiveThreats.FindRef(Id);IsValid(A)) A->Destroy();
        LiveThreats.Remove(Id); State->Threats.Remove(Id);
    }
    if(Won) {
        ++State->Waves;
        const int32 Reward=2+FMath::Min(8,State->Waves);
        State->Gold+=14+Reward*3; State->Inventory.FindOrAdd(TEXT("goldore"))+=Reward;
        Say(FString::Printf(TEXT("Relay held / +%d gold ore and %d candybar wrappers"),Reward,14+Reward*3));
    } else Say(TEXT("The siege ended / check the relay and rebuild damaged machinery"));
    State->RaidCore.Invalidate(); State->RaidStage=0; State->RaidTime=0; State->RaidWait=170;
    Save();
}
void AETWorld::StepRaid(AETCharacter* Player,float D)
{
    if(!IsNight()) return;
    if(State->RaidStage) {
        const auto* Relay=State->Builds.Find(State->RaidCore);
        if(!Relay) {EndRaid(false); return;}
        if(FVector::DistSquared(Relay->Anchor,Player->GetActorLocation())>FMath::Square(10500.)) return;
        bool Alive=false; for(const auto& It:State->Threats) if(RaidMember(It.Key)) {Alive=true; break;}
        if(!Alive) {EndRaid(true); return;}
        State->RaidTime-=D;
        if(State->RaidTime<=0) {
            if(State->RaidStage==1) {State->RaidStage=2; State->RaidTime=165; Say(TEXT("The carapace line is charging your relay!")); Sound(TEXT("BroodCall"),.85f,1.15f);}
            else EndRaid(false);
        }
        return;
    }
    FGuid Best; int32 Value=27;
    for(const auto& It:State->Builds) if(It.Value.Piece==Site::Core&&FVector::DistSquared(It.Value.Anchor,Player->GetActorLocation())<FMath::Square(9000.)) {
        const int32 V=BaseValue(It.Key);
        if(V>=Value) {Best=It.Key; Value=V;}
    }
    if(!Best.IsValid()) return;
    State->RaidWait-=D;
    if(State->RaidWait<=0&&!Muster(Best)) State->RaidWait=6;
}
void AETWorld::TickOutposts(AETCharacter* Player,float Delta)
{
    OutpostClock+=Delta;
    if(OutpostClock<.32f) return;
    const float D=OutpostClock; OutpostClock=0;
    const auto P=Player->GetActorLocation();
    const float Depth=float((SurfaceHeight(P.X,P.Y)-P.Z)/100.);
    if(Depth>=16&&FMath::FloorToInt(Depth/16)>FMath::FloorToInt(State->MaxDepth/16)) {
        State->NextPatrol=FMath::Min(State->NextPatrol,4.f); Say(TEXT("DEEP SEAM / ore grade increased. Movement inside the rock."));
    }
    State->MaxDepth=FMath::Max(State->MaxDepth,Depth);
    if(State->Builds.Num()||State->RaidStage) {StepDevices(D); StepRaid(Player,D);}
}

