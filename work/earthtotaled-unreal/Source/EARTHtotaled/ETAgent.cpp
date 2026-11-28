#include "ETGame.h"
#include "ETGeology.h"
#include "ETAgentMovement.h"
#include "ETCombat.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

namespace {
    constexpr double GuardRadius=1200.;
    constexpr double NightGuardRadius=2200.;
    constexpr float ReturnFireTime=16.f;
    bool SeesSurvivor(AETAgent& Agent,const AETCharacter& Player) {
        FCollisionQueryParams Q(SCENE_QUERY_STAT(ETGuardSight),false,&Agent);
        Q.AddIgnoredActor(&Player);
        return !Agent.GetWorld()->LineTraceTestByChannel(Agent.AimPoint(),Player.GetActorLocation(),ECC_Visibility,Q);
    }
}

bool ETCombat::HostileInDaylight(const AETAgent& Agent) {
    return Agent.WakeEnemy()||(Agent.FrontUnit&&Agent.PlayerAggro>0);
}

AETAgent::AETAgent()
{
    PrimaryActorTick.bCanEverTick=true; Pivot=CreateDefaultSubobject<USceneComponent>(TEXT("Body")); SetRootComponent(Pivot);
}
bool AETAgent::IsMachine() const {return Kind==TEXT("robot")||Kind==TEXT("drone");}
bool AETAgent::Awake() const {return World&&(WakeEnemy()||FrontUnit||Kind==TEXT("walker")||Kind==TEXT("deer")||World->IsNight());}
void AETAgent::UpdatePresence(FVector Viewer)
{
    const double Distance=FVector::DistSquared2D(GetActorLocation(),Viewer);
    const bool Present=Awake()&&Distance<FMath::Square(World->State->ViewDistance*100.+5000.);
    if(!Present)BearWindup=0;
    if(!Present||Distance>=FMath::Square(14000.)) AlertHold=0;
    SetActorHiddenInGame(!Present);
    SetActorEnableCollision(Present);
    const float Interval=Distance<FMath::Square(9000.)?.04f:Distance<FMath::Square(14000.)?.14f:.85f;
    SetActorTickInterval(Interval);
    SetActorTickEnabled(Present);
}
bool AETAgent::IsBrood() const {return Kind==TEXT("runner")||Kind==TEXT("spitter")||Kind==TEXT("bulwark")||Kind==TEXT("skimmer")||Kind==TEXT("hive");}
void AETAgent::Snapshot(FETThreat& Record) const
{
    Record.Position=GetActorLocation();
    Record.Health=Health;
    Record.AttackCooldown=FMath::Max(0.f,AttackTime);
}
void AETAgent::Configure(AETWorld* InWorld,const FString& InId,const FString& InKind)
{
    World=InWorld; Id=InId; Kind=InKind; Home=GetActorLocation();
    if(World->State->Consumed.Contains(Id)) {Destroy(); return;}
    if(Kind==TEXT("robot")) Health=120;
    else if(Kind==TEXT("drone")) Health=30;
    else if(Kind==TEXT("hive")) Health=480;
    else if(Kind==TEXT("bulwark")) Health=168;
    else if(Kind==TEXT("skimmer")) Health=62;
    else if(Kind==TEXT("spitter")) Health=78;
    else if(Kind==TEXT("runner")) Health=52;
    else if(Kind==TEXT("bear")) Health=360;
    else Health=Kind==TEXT("boar")?85:60;
    FullHealth=Health;
    if(const float* Saved=World->State->AgentHealth.Find(Id)) Health=*Saved;
    FString Asset=Kind==TEXT("robot")?TEXT("SM_SupertechWarden"):Kind==TEXT("deer")?TEXT("SM_Deer"):Kind==TEXT("boar")?TEXT("SM_Boar"):TEXT("SM_Wolf");
    if(Kind==TEXT("walker")) Asset=TEXT("SM_HollowWalker");
    if(Kind==TEXT("bear")) Asset=TEXT("SM_WoodlandBear");
    if(Kind==TEXT("drone")) Asset=TEXT("SM_SupertechScout");
    if(Kind==TEXT("hive")) {Asset=TEXT("SM_MoltBulwark"); SetActorScale3D(FVector(2.5,2.7,2.4));}
    if(Kind==TEXT("bulwark")) Asset=TEXT("SM_MoltBulwark");
    if(Kind==TEXT("skimmer")) Asset=TEXT("SM_MoltSkimmer");
    if(Kind==TEXT("runner")) Asset=TEXT("SM_MoltRunner");
    if(Kind==TEXT("spitter")) Asset=TEXT("SM_MoltSpitter");
    auto M=NewObject<UStaticMeshComponent>(this); M->SetupAttachment(Pivot); M->RegisterComponent();
    M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Asset+TEXT(".")+Asset)));
    World->DressMesh(M,Kind==TEXT("robot")?TEXT("Rust"):TEXT("Hide")); M->SetCollisionProfileName(TEXT("BlockAll")); Parts.Add(M);
    if(Kind==TEXT("bear"))AttachBearLegs();
    SetActorTickInterval(.04f);
    UpdatePresence(UGameplayStatics::GetPlayerPawn(this,0)?UGameplayStatics::GetPlayerPawn(this,0)->GetActorLocation():World->State->Position);
}
void AETAgent::Fly(float D,AETCharacter* Player,AETAgent* Enemy)
{
    const bool Wing=Kind==TEXT("skimmer");
    const bool Siege=World->RaidMember(Id);
    const auto* Relay=Siege?World->State->Builds.Find(World->State->RaidCore):nullptr;
    const FVector Here=GetActorLocation(),Target=Relay?Relay->Anchor+FVector(0,0,100):Enemy?Enemy->AimPoint():Player->GetActorLocation()+FVector(0,0,35);
    const FVector To=Target-Here;
    const bool InRange=To.SizeSquared()<=FMath::Square(8500.);
    const bool CanEngage=Relay||Enemy||Player->EnemyReacquireTime<=0;
    UpdateAlert(InRange&&CanEngage,!Relay&&!Enemy,D);
    if(!InRange){ETAgentMovement::Patrol(*this,D);return;}
    const FVector Radial=To.GetSafeNormal2D();
    const double Hand=Id.EndsWith(TEXT("1"))?-1.:1.;
    const FVector Orbit(-Radial.Y*Hand,Radial.X*Hand,0);
    const FVector Goal=Target-Radial*690+Orbit*(210+90*FMath::Sin(Phase))+FVector(0,0,95+65*FMath::Sin(Phase*2));
    FVector FlightGoal=Goal;
    if(Wing&&Here.Z>=World->SurfaceHeight(Here.X,Here.Y)-50) {
        const auto Next=Here+(Goal-Here).GetClampedToMaxSize(610.*D);const double Soil=World->SurfaceHeight(Next.X,Next.Y);
        if(World->Geology->SurfaceOpening(Next.X,Next.Y)<45) FlightGoal.Z=FMath::Max(FlightGoal.Z,Soil+100);
    }
    ETAgentMovement::FlyTo(*this,FlightGoal,Siege&&World->State->RaidStage==1?65:Wing?610:485,D);
    ETAgentMovement::Face(*this,To,D);
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETDrone),false,this);
    FHitResult H;
    if((!Relay&&!Enemy&&Player->EnemyReacquireTime>0)||(Siege&&World->State->RaidStage==1)||AttackTime>0||To.SizeSquared()>FMath::Square(2700.)) return;
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(H,Here,Target,ECC_Visibility,Q);
    if(Blocked&&H.GetActor()!=(Enemy?static_cast<AActor*>(Enemy):Player)&&!Cast<AETConstruction>(H.GetActor())) return;
    if(FrontUnit&&!Relay&&!Enemy&&PlayerAggro>0)PlayerAggro=FMath::Max(PlayerAggro,4.f);
    if(auto* Bolt=GetWorld()->SpawnActor<AETEnemyBolt>(Here,To.Rotation())) Bolt->Launch(this,Target,Wing);
    AttackTime=1.55f; World->Sound(TEXT("ShardChirp"),.25f,1.25f);
}
void AETAgent::Tick(float D)
{
    Super::Tick(D); if(!Awake()||Health<=0) return;
    auto Player=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)); if(!Player||!Player->Panel.IsEmpty()) return;
    TickImpact(D);
    Phase+=D; AttackTime-=D;
    if(TickWakeDeer(D,*Player))return;
    if(Kind==TEXT("hive")) {
        const bool Calling=!WakeTruce()&&!World->AtRefuge&&FVector::DistSquared(Player->GetActorLocation(),GetActorLocation())<FMath::Square(4100.);
        UpdateAlert(Calling,true,D);
        if(AttackTime<=0&&Calling) {
            AttackTime=14;
            for(int32 I=0;I<3;++I) {
                const FString Guard=Id+FString::Printf(TEXT("/guard%d"),I);
                if(World->State->Threats.Contains(Guard)) continue;
                FVector Spot=GetActorLocation()+FRotator(0,I*120,0).Vector()*240;
                FHitResult Floor;
                FCollisionQueryParams GroundQuery(SCENE_QUERY_STAT(ETHiveFloor),false,this);
                if(!GetWorld()->LineTraceSingleByChannel(Floor,Spot+FVector(0,0,90),Spot-FVector(0,0,300),ECC_Visibility,GroundQuery)||
                    Floor.ImpactNormal.Z<.62||Cast<AETAgent>(Floor.GetActor())||Cast<APawn>(Floor.GetActor()))continue;
                const bool Flier=I==2&&World->ThreatTier()>=2;
                Spot.Z=Floor.ImpactPoint.Z+(Flier?120:9);
                World->AddThreat(Guard,Flier?TEXT("skimmer"):TEXT("runner"),Spot); break;
            }
        }
        return;
    }
    const bool Siege=World->RaidMember(Id);
    PlayerAggro=FMath::Max(0.f,PlayerAggro-D);
    if(!Siege&&FVector::DistSquared2D(Player->GetActorLocation(),GetActorLocation())>FMath::Square(14000.)){
        AlertHold=0;ETAgentMovement::Patrol(*this,D);return;
    }
    const bool Truce=WakeTruce(),Ready=Player->EnemyReacquireTime<=0;
    const double PlayerD2=FVector::DistSquared(GetActorLocation(),Player->GetActorLocation());
    const bool Night=World->IsNight();
    if(FrontUnit&&Ready&&!Truce&&!World->AtRefuge&&PlayerD2<FMath::Square(Night?NightGuardRadius:GuardRadius)&&SeesSurvivor(*this,*Player)){
        if(PlayerAggro<=0)AttackTime=FMath::Max(AttackTime,.55f);
        PlayerAggro=FMath::Max(PlayerAggro,4.f);
    }
    const bool Hostile=!Truce&&!World->AtRefuge&&(Night||Kind==TEXT("walker")||ETCombat::HostileInDaylight(*this));
    const bool FocusPlayer=Hostile&&Ready&&(PlayerAggro>0||PlayerD2<FMath::Square(600.));
    AETAgent* Enemy=Siege||Kind==TEXT("walker")||FocusPlayer?nullptr:FindRival(D);
    if(Enemy&&Hostile&&Ready&&PlayerD2<FMath::Square(3800.)&&PlayerD2<FVector::DistSquared(GetActorLocation(),Enemy->GetActorLocation())*.64&&SeesSurvivor(*this,*Player))
        Enemy=nullptr;
    if(!Enemy&&(Truce||(!Siege&&Kind!=TEXT("deer")&&!Hostile))) {
        BearWindup=0;AlertHold=0;ETAgentMovement::Patrol(*this,D);return;
    }
    if(Kind==TEXT("drone")||Kind==TEXT("skimmer")) {Fly(D,Player,Enemy); return;}
    AActor* Target=Enemy?static_cast<AActor*>(Enemy):Player;
    if(Siege) {
        auto* Relay=World->LiveBuilds.FindRef(World->State->RaidCore).Get();
        if(!Relay) {UpdateAlert(false,false,D);return;}
        if(World->State->RaidStage==1||FVector::DistSquared(Player->GetActorLocation(),GetActorLocation())>FMath::Square(470.)) Target=Relay;
    }
    FVector P=GetActorLocation(), To=Target->GetActorLocation()-P; To.Z=0; float Distance=To.Size();
    const bool Staging=Siege&&World->State->RaidStage==1;
    if((Staging&&Distance<1750)||Distance>14000) {UpdateAlert(false,false,D);return;}
    FCollisionQueryParams Params; Params.AddIgnoredActor(this); FHitResult Sight;
    const bool Robot=Kind==TEXT("robot"), Flee=Kind==TEXT("deer"), Brood=IsBrood(), Spitter=Kind==TEXT("spitter");
    const FVector Eye=P+FVector(0,0,Robot?120:65);
    const FVector Aim=Enemy?Enemy->AimPoint():Target->GetActorLocation();
    bool Clear=!GetWorld()->LineTraceSingleByChannel(Sight,Eye,Aim,ECC_Visibility,Params)||Sight.GetActor()==Target;
    auto* Cover=Cast<AETConstruction>(Sight.GetActor());
    const bool Bear=Kind==TEXT("bear");
    const bool Responding=FrontUnit&&Target==Player&&PlayerAggro>0;
    if(Responding&&Clear&&Distance<8000)PlayerAggro=FMath::Max(PlayerAggro,4.f);
    const float Engage=Responding?14000:Bear?(PlayerAggro>0?6500:2900):Kind==TEXT("walker")?9500:Robot?6000:Brood?5200:Flee?2200:1800;
    const bool Heard=!Flee&&Target==Player&&(PlayerAggro>0||Distance<1200);
    const bool Pursuing=Distance<Engage&&(Clear||Heard||Managed||TownIndex!=INDEX_NONE||Cover||Enemy);
    const bool CanEngage=Target!=Player||Player->EnemyReacquireTime<=0;
    UpdateAlert(!Flee&&CanEngage&&(Pursuing||(Bear&&BearWindup>0)),Target==Player,D);
    if(Bear&&BearSwipe(*Player,D,Clear))return;
    if(Flee&&Parts.Num()) {
        const bool Browsing=!Pursuing&&FMath::Fmod(Phase,15.f)<7.f;
        Parts[0]->SetRelativeRotation(FRotator(Browsing?-3.-2.*FMath::Sin(Phase*1.4):0.,0.,0.));
        if(Browsing)return;
    }
    FVector Direction;
    if(Pursuing) Direction=To.GetSafeNormal()*(Flee?-1:1);
    else Direction=(Home+FVector(FMath::Cos(Phase*.15f)*800,FMath::Sin(Phase*.15f)*800,0)-P).GetSafeNormal2D();
    if(Brood&&!Spitter&&Distance>460) Direction=(Direction+FVector(-Direction.Y,Direction.X,0)*.29*FMath::Sin(Phase+double(GetTypeHash(Id)%9))).GetSafeNormal2D();
    const float Speed=Bear?(Clear&&Distance<Engage?510:132):Staging?55:Kind==TEXT("walker")?170:Kind==TEXT("bulwark")?245:Robot?215:Flee?(Pursuing?475:95):Spitter?230:Brood?395:260;
    if((Distance>(Robot?1800:Spitter?850:145)||Flee)||!Clear)
    {
        ETAgentMovement::Walk(*this,Direction,Speed,D,Target);
    }
    else if(Pursuing)ETAgentMovement::Face(*this,To,D);
    if(Bear){
        BearStride+=FVector::Dist2D(P,GetActorLocation())*.028f;
        PoseBear(BearStride,0);return;
    }
    if(Staging||(Target==Player&&Player->EnemyReacquireTime>0)) return;
    const double AttackDistance=Cover?FVector::Dist(Eye,Sight.ImpactPoint):FVector::Dist(Eye,Aim);
    if(!Flee&&(Clear||Cover)&&AttackDistance<(Robot?4300:Spitter?2700:200)&&AttackTime<=0)
    {
        AttackTime=Robot?1.8f:1.1f;
        World->Sound(Robot?TEXT("Robot"):Brood?TEXT("BroodCall"):TEXT("Animal"),FMath::Clamp(1-Distance/5000,.08f,.6f));
        // Allied Greenline suppresses hostile robots near the shelter.
        if(Robot&&World->State->Allied&&P.Size2D()<6500) {TakeDamage(55,FDamageEvent(),nullptr,Player); World->Say(TEXT("GREENLINE / SALT-7 covering your approach"));}
        else if(Spitter||Robot) {
            if(auto* Bolt=GetWorld()->SpawnActor<AETEnemyBolt>(Eye,(Aim-Eye).Rotation())) Bolt->Launch(this,Aim,Spitter);
            if(Spitter) AttackTime=2.5f;
        } else if(Enemy&&Clear) Enemy->TakeDamage(Kind==TEXT("bulwark")?28:16,FDamageEvent(),nullptr,this);
        else if(Cover||Target!=Player) World->HitBuild(Cover?Cover:Cast<AETConstruction>(Target),Kind==TEXT("bulwark")?25:14);
        else Player->TakeDamage(Kind==TEXT("bulwark")?23:Kind==TEXT("boar")?19:Brood?10:14,FDamageEvent(),nullptr,this);
    }
}
float AETAgent::TakeDamage(float Amount,const FDamageEvent& Event,AController*,AActor* Causer)
{
    if(WakeHerd>=0)return 0;
    if(!Awake()||Health<=0||Amount<=0) return 0;
    const float Applied=FMath::Min(Health,Amount*(Kind==TEXT("bulwark")?.72f:1.f)); Health-=Applied;
    ReceiveImpact(Applied,Event,Causer);
    const auto* Bolt=Cast<AETEnemyBolt>(Causer);
    const auto* Fighter=Cast<AETAgent>(Causer);
    if((Fighter&&Opposes(Fighter))||Bolt) {
        if(Bolt?Bolt->BugBolt:Fighter->IsBrood()) ++World->BugHits; else ++World->SupertechHits;
    }
    const bool Credited=Cast<AETCharacter>(Causer)||Cast<AETConstruction>(Causer);
    if(auto* InstigatorPawn=Cast<AETCharacter>(Causer)) {
        InstigatorPawn->RememberEnemy(this);
        if(!WakeTruce()&&!World->AtRefuge)PlayerAggro=FrontUnit?ReturnFireTime:3.5f;
        World->State->ShieldDelay=10.f;
    }
    if(Managed) {if(auto* R=World->State->Threats.Find(Id)) R->Health=Health;}
    else if(!FrontUnit) World->State->AgentHealth.Add(Id,Health);
    if(Health<=0)
    {
        auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
        if(FrontUnit) {
            World->FrontUnits.Remove(Id); World->State->FrontLosses.Add(Id,World->State->Elapsed+32.f+GetTypeHash(Id)%19);
        } else if(Managed) {World->State->Threats.Remove(Id); World->LiveThreats.Remove(Id);}
        else {if(TownIndex==INDEX_NONE) World->State->Consumed.Add(Id); World->State->AgentHealth.Remove(Id);}
        if(TownIndex!=INDEX_NONE) World->WalkerFell(this);
        const bool Robot=Kind==TEXT("robot"),Hive=Kind==TEXT("hive");
        if(Credited&&P) {
            if(Robot) ++World->State->Kills;
            P->AddXP(Hive?95:Kind==TEXT("bear")?65:Robot?35:Kind==TEXT("drone")?8:20);
        }
        if(P) P->RecordHostileKill(*this,Credited);
        if(Robot) World->SplitRobot(this);
        World->SpillLoot(this,Credited);
        if(Hive) {World->State->Consumed.Add(Id); World->State->RaidWait+=90; World->Say(TEXT("Bug nest cleared / ore spilled from the chamber"));}
        if(Credited||TownIndex!=INDEX_NONE) World->Save(); Destroy();
    }
    return Applied;
}
