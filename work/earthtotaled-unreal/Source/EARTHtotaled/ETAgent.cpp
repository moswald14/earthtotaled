#include "ETGame.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"

AETAgent::AETAgent()
{
    PrimaryActorTick.bCanEverTick=true; Pivot=CreateDefaultSubobject<USceneComponent>(TEXT("Body")); SetRootComponent(Pivot);
}
void AETAgent::Configure(AETWorld* InWorld,const FString& InId,const FString& InKind)
{
    World=InWorld; Id=InId; Kind=InKind; Home=GetActorLocation();
    if(World->State->Consumed.Contains(Id)) {Destroy(); return;}
    Health=World->State->AgentHealth.Contains(Id)?World->State->AgentHealth[Id]:(Kind==TEXT("robot")?120.f:Kind==TEXT("boar")?85.f:60.f);
    FString Asset=Kind==TEXT("robot")?TEXT("SM_Robot"):Kind==TEXT("deer")?TEXT("SM_Deer"):Kind==TEXT("boar")?TEXT("SM_Boar"):TEXT("SM_Wolf");
    auto M=NewObject<UStaticMeshComponent>(this); M->SetupAttachment(Pivot); M->RegisterComponent();
    M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Asset+TEXT(".")+Asset)));
    World->DressMesh(M,Kind==TEXT("robot")?TEXT("Rust"):TEXT("Hide")); M->SetCollisionProfileName(TEXT("BlockAll")); Parts.Add(M);
    SetActorTickInterval(.04f);
}
void AETAgent::Tick(float D)
{
    Super::Tick(D); if(!World||Health<=0) return;
    auto Player=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)); if(!Player||!Player->Panel.IsEmpty()) return;
    Phase+=D; AttackTime-=D;
    FVector P=GetActorLocation(), To=Player->GetActorLocation()-P; To.Z=0; float Distance=To.Size();
    if(Distance>14000) return;
    FCollisionQueryParams Params; Params.AddIgnoredActor(this); FHitResult Sight;
    bool Clear=!GetWorld()->LineTraceSingleByChannel(Sight,P+FVector(0,0,120),Player->GetActorLocation(),ECC_Visibility,Params)||Sight.GetActor()==Player;
    bool Robot=Kind==TEXT("robot"), Flee=Kind==TEXT("deer");
    FVector Direction;
    if(Distance<(Flee?2200:Robot?4800:1500)&&Clear) Direction=To.GetSafeNormal()*(Flee?-1:1);
    else Direction=(Home+FVector(FMath::Cos(Phase*.15f)*800,FMath::Sin(Phase*.15f)*800,0)-P).GetSafeNormal2D();
    const float Speed=Robot?165:Flee?410:260;
    if((Distance>(Robot?1800:160)||Flee)||!Clear)
    {
        FVector Next=P+Direction*Speed*D; Next.Z=World->Height(Next.X,Next.Y)+8;
        FHitResult Obstacle;
        bool Blocked=GetWorld()->LineTraceSingleByChannel(Obstacle,P+FVector(0,0,65),Next+Direction*90+FVector(0,0,65),ECC_Visibility,Params);
        if(!Blocked&&FMath::Abs(Next.Z-P.Z)<90) SetActorLocation(Next);
        else Home=P+FVector(-Direction.Y,Direction.X,0)*600;
        SetActorRotation(Direction.Rotation());
    }
    if(!Flee&&Clear&&Distance<(Robot?4300:180)&&AttackTime<=0)
    {
        AttackTime=Robot?1.8f:1.1f;
        World->Sound(Robot?TEXT("Robot"):TEXT("Animal"),FMath::Clamp(1-Distance/5000,.08f,.6f));
        // Allied Greenline suppresses hostile robots near the shelter.
        if(Robot&&World->State->Allied&&P.Size2D()<6500) {TakeDamage(55,FDamageEvent(),nullptr,Player); World->Say(TEXT("GREENLINE / SALT-7 covering your approach"));}
        else Player->TakeDamage(Robot?13:Kind==TEXT("boar")?19:14,FDamageEvent(),nullptr,this);
    }
}
float AETAgent::TakeDamage(float Amount,const FDamageEvent&,AController*,AActor*)
{
    if(!World||Health<=0) return 0; Health=FMath::Max(0.f,Health-Amount); World->State->AgentHealth.Add(Id,Health);
    if(Health<=0)
    {
        auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)); World->State->Consumed.Add(Id);
        bool Robot=Kind==TEXT("robot"); if(Robot) ++World->State->Kills;
        if(P) P->AddXP(Robot?35:20);
        const FString DropId=Id+TEXT("-drop"); const FVector DropPosition=GetActorLocation()+FVector(0,0,20);
        const FString DropKind=Robot?TEXT("gold"):TEXT("harvest"); const int32 DropAmount=Robot?FMath::RoundToInt(16*(P?P->LootRate():1)):0;
        World->State->DropPositions.Add(DropId,DropPosition); World->State->DropAmounts.Add(DropId,DropAmount); World->State->DropKinds.Add(DropId,DropKind);
        auto Drop=World->Item(DropId,DropKind,Robot?TEXT("Collect candybar wrappers"):TEXT("Salvage animal remains"),DropPosition,DropAmount);
        if(auto Region=Cast<AETRegion>(GetOwner())) if(Drop) Region->Residents.Add(Drop);
        World->Save(); Destroy();
    }
    return Amount;
}
