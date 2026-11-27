#include "ETGame.h"
#include "ETGeology.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

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
    SetActorHiddenInGame(!Present);
    SetActorEnableCollision(Present);
    SetActorTickEnabled(Present&&Distance<FMath::Square(14000.));
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
    if(To.SizeSquared()>FMath::Square(8500.)) return;
    const FVector Radial=To.GetSafeNormal2D();
    const double Hand=Id.EndsWith(TEXT("1"))?-1.:1.;
    const FVector Orbit(-Radial.Y*Hand,Radial.X*Hand,0);
    const FVector Goal=Target-Radial*690+Orbit*(210+90*FMath::Sin(Phase))+FVector(0,0,95+65*FMath::Sin(Phase*2));
    FVector Step=(Goal-Here).GetClampedToMaxSize((Siege&&World->State->RaidStage==1?65:Wing?610:485)*D);
    if(Wing&&Here.Z>=World->SurfaceHeight(Here.X,Here.Y)-50) {
        const auto Next=Here+Step;const double Soil=World->SurfaceHeight(Next.X,Next.Y);
        if(World->Geology->SurfaceOpening(Next.X,Next.Y)<45) Step.Z=FMath::Max(Step.Z,Soil+100-Here.Z);
    }
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETDrone),false,this);
    FHitResult H;
    if(GetWorld()->SweepSingleByChannel(H,Here,Here+Step,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(22),Q)) {
        Step=FVector::VectorPlaneProject(Step,H.ImpactNormal)+Orbit*80*D;
        if(GetWorld()->SweepSingleByChannel(H,Here,Here+Step,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(22),Q)) Step*=H.Time*.85;
    }
    SetActorLocation(Here+Step); SetActorRotation(FRotator(5*FMath::Sin(Phase*4),To.Rotation().Yaw,11*FMath::Sin(Phase*3)));
    if((!Relay&&!Enemy&&Player->EnemyReacquireTime>0)||(Siege&&World->State->RaidStage==1)||AttackTime>0||To.SizeSquared()>FMath::Square(2700.)) return;
    const bool Blocked=GetWorld()->LineTraceSingleByChannel(H,Here,Target,ECC_Visibility,Q);
    if(Blocked&&H.GetActor()!=(Enemy?static_cast<AActor*>(Enemy):Player)&&!Cast<AETConstruction>(H.GetActor())) return;
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
        if(AttackTime<=0&&FVector::DistSquared(Player->GetActorLocation(),GetActorLocation())<FMath::Square(4100.)) {
            AttackTime=14;
            for(int32 I=0;I<3;++I) {
                const FString Guard=Id+FString::Printf(TEXT("/guard%d"),I);
                if(World->State->Threats.Contains(Guard)) continue;
                FVector Spot=GetActorLocation()+FRotator(0,I*120,0).Vector()*240;
                Spot.Z=World->SurfaceHeight(Spot.X,Spot.Y)+9;
                World->AddThreat(Guard,I==2&&World->ThreatTier()>=2?TEXT("skimmer"):TEXT("runner"),Spot); break;
            }
        }
        return;
    }
    const bool Siege=World->RaidMember(Id);
    PlayerAggro=FMath::Max(0.f,PlayerAggro-D);
    AETAgent* Enemy=Siege||Kind==TEXT("walker")?nullptr:FindRival(D);
    if((World->IsNight()||WakeEnemy())&&!WakeTruce()&&!World->AtRefuge&&Player->EnemyReacquireTime<=0&&
       (PlayerAggro>0||FVector::DistSquared(Player->GetActorLocation(),GetActorLocation())<FMath::Square(600.))) Enemy=nullptr;
    if(!Enemy&&WakeTruce())return;
    if(!Enemy&&!Siege&&Kind!=TEXT("deer")&&Kind!=TEXT("walker")&&(World->AtRefuge||(!World->IsNight()&&!WakeEnemy()))) {BearWindup=0;return;}
    if(Kind==TEXT("drone")||Kind==TEXT("skimmer")) {Fly(D,Player,Enemy); return;}
    AActor* Target=Enemy?static_cast<AActor*>(Enemy):Player;
    if(Siege) {
        auto* Relay=World->LiveBuilds.FindRef(World->State->RaidCore).Get();
        if(!Relay) return;
        if(World->State->RaidStage==1||FVector::DistSquared(Player->GetActorLocation(),GetActorLocation())>FMath::Square(470.)) Target=Relay;
    }
    FVector P=GetActorLocation(), To=Target->GetActorLocation()-P; To.Z=0; float Distance=To.Size();
    const bool Staging=Siege&&World->State->RaidStage==1;
    if(Staging&&Distance<1750) return;
    if(Distance>14000) return;
    FCollisionQueryParams Params; Params.AddIgnoredActor(this); FHitResult Sight;
    const bool Robot=Kind==TEXT("robot"), Flee=Kind==TEXT("deer"), Brood=IsBrood(), Spitter=Kind==TEXT("spitter");
    const FVector Eye=P+FVector(0,0,Robot?120:65);
    const FVector Aim=Enemy?Enemy->AimPoint():Target->GetActorLocation();
    bool Clear=!GetWorld()->LineTraceSingleByChannel(Sight,Eye,Aim,ECC_Visibility,Params)||Sight.GetActor()==Target;
    auto* Cover=Cast<AETConstruction>(Sight.GetActor());
    const bool Bear=Kind==TEXT("bear");
    if(Bear&&BearSwipe(*Player,D,Clear))return;
    const float Engage=Bear?(PlayerAggro>0?6500:2900):Kind==TEXT("walker")?9500:Robot?6000:Brood?5200:Flee?2200:1800;
    FVector Direction;
    if(Distance<Engage&&(Clear||Managed||TownIndex!=INDEX_NONE||Cover||Enemy)) Direction=To.GetSafeNormal()*(Flee?-1:1);
    else Direction=(Home+FVector(FMath::Cos(Phase*.15f)*800,FMath::Sin(Phase*.15f)*800,0)-P).GetSafeNormal2D();
    if(Brood&&!Spitter&&Distance>460) Direction=(Direction+FVector(-Direction.Y,Direction.X,0)*.29*FMath::Sin(Phase+double(GetTypeHash(Id)%9))).GetSafeNormal2D();
    const float Speed=Bear?(Clear&&Distance<Engage?510:132):Staging?55:Kind==TEXT("walker")?170:Kind==TEXT("bulwark")?245:Robot?215:Flee?410:Spitter?230:Brood?395:260;
    if((Distance>(Robot?1800:Spitter?850:145)||Flee)||!Clear)
    {
        FVector Next=P+Direction*Speed*D; Next.Z=World->Height(Next.X,Next.Y)+8;
        if(Brood&&!World->Geology->BugCanStep(P,Next)) {Home=P-Direction*650;return;}
        const auto Rock=ETRock::Chunk(Next);
        if(FrontUnit||World->Geology->ReplacesSurface({Rock.X,Rock.Y})) {
            FHitResult Ground;
            if(GetWorld()->LineTraceSingleByChannel(Ground,FVector(Next.X,Next.Y,P.Z+80),FVector(Next.X,Next.Y,P.Z-160),ECC_Visibility,Params)&&Ground.ImpactNormal.Z>.65)
                Next.Z=Ground.ImpactPoint.Z+8;
            else {
                // Stay on the bank. If the survivor excavates directly underneath,
                // settle onto the uppermost remaining floor instead of hovering.
                if(!Brood&&World->Geology->IsOpen(P-FVector(0,0,28))&&GetWorld()->LineTraceSingleByChannel(Ground,P+FVector(0,0,16),P-FVector(0,0,6000),ECC_Visibility,Params))
                    SetActorLocation(Ground.ImpactPoint+FVector(0,0,8));
                Home=P-Direction*650;
                return;
            }
        }
        FHitResult Obstacle;
        bool Blocked=GetWorld()->LineTraceSingleByChannel(Obstacle,P+FVector(0,0,65),Next+Direction*90+FVector(0,0,65),ECC_Visibility,Params);
        if(!Blocked&&FMath::Abs(Next.Z-P.Z)<90) SetActorLocation(Next);
        else {
            const float Hand=GetTypeHash(Id)%2?1.f:-1.f;
            FVector Side=FVector(-Direction.Y,Direction.X,0)*Hand;
            FVector Around=P+Side*Speed*D;
            FHitResult Bank;
            if((!Brood||World->Geology->BugCanStep(P,Around))&&!GetWorld()->LineTraceTestByChannel(P+FVector(0,0,60),Around+Side*70+FVector(0,0,60),ECC_Visibility,Params)&&
               GetWorld()->LineTraceSingleByChannel(Bank,Around+FVector(0,0,68),Around-FVector(0,0,85),ECC_Visibility,Params)&&Bank.ImpactNormal.Z>.62)
                SetActorLocation(Bank.ImpactPoint+FVector(0,0,8));
            Home=P+Side*600;
        }
        SetActorRotation(Direction.Rotation());
    }
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
    if(Cast<AETCharacter>(Causer)) {PlayerAggro=3.5f; World->State->ShieldDelay=10.f;}
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
        if(Robot) World->SplitRobot(this);
        World->SpillLoot(this,Credited);
        if(Hive) {World->State->Consumed.Add(Id); World->State->RaidWait+=90; World->Say(TEXT("Bug nest cleared / ore spilled from the chamber"));}
        if(Credited||TownIndex!=INDEX_NONE) World->Save(); Destroy();
    }
    return Applied;
}
