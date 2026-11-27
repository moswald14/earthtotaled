#include "ETGame.h"
#include "ETLandfall.h"
#include "ETGeology.h"
#include "Camera/CameraComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Engine/StaticMesh.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Materials/MaterialInterface.h"

void AETCharacter::ApplyFieldDisplay()
{
    auto* Pass=World->Material(TEXT("FieldDisplay"));
    if(!Pass) return;
    if(World->State->RetroDisplay) Camera->AddOrUpdateBlendable(Pass);
    else Camera->RemoveBlendable(Pass);
}
void AETCharacter::ToggleBorer()
{
    if(!World||!Panel.IsEmpty()||Climbing) return;
    StopBuilding();
    if(Borer) {EquipFrame(World->State->Weapon); return;}
    LampBeforeBorer=Flashlight->IsVisible();
    Borer=true; Firing=Aiming=false; ReloadTime=0; ShotTime=.15f;
    Gun->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EARTHtotaled/Art/SM_SurveyBorer.SM_SurveyBorer")));
    World->DressMesh(Gun,TEXT("Steel"));
    Flashlight->SetVisibility(true); World->Sound(TEXT("Switch"),.32f,.72f);
    World->Say(TEXT("Survey borer / R switches round or square cuts / hold right mouse to find ore"));
}
void AETCharacter::FireBorer()
{
    if(!Borer||!Panel.IsEmpty()||Climbing) return;
    ShotTime=.64f;
    const FVector Eye=Camera->GetComponentLocation(),Forward=Camera->GetForwardVector();
    FHitResult Hit; FCollisionQueryParams Params(SCENE_QUERY_STAT(ETBorePulse),false,this);
    if(!GetWorld()->LineTraceSingleByChannel(Hit,Eye,Eye+Forward*ETRock::ToolRange,ECC_Visibility,Params)) {
        World->Say(TEXT("No surface in range / move within 27 m")); return;
    }
    FString Reward;
    if(!World->Geology->Blast(Hit,Reward,World->State->SquareBorer)) {World->Say(Reward.IsEmpty()?TEXT("Aim at fresh earth or a rock face in the wilderness"):Reward); return;}
    MuzzleTime=.09f; Muzzle->SetVisibility(true);
    World->Sound(TEXT("BorerPulse"),.66f,FMath::FRandRange(.93f,1.05f));
    if(auto* Burst=GetWorld()->SpawnActor<AETBoreBurst>(Hit.ImpactPoint+Hit.ImpactNormal*18,FRotator::ZeroRotator))
        Burst->Ignite(World,Hit.ImpactNormal,!Reward.IsEmpty());
    AddXP(Reward.IsEmpty()?2:12);
    if(!Reward.IsEmpty()) {World->Sound(TEXT("OreChime"),.45f); Reward+=TEXT("   /   ");}
    World->State->Inventory.FindOrAdd(TEXT("stone"))+=2;
    World->Say(Reward+TEXT("+2 stone"));
    World->Save();
}
void AETCharacter::Winch()
{
    if(!Borer||!World||!Panel.IsEmpty()||Climbing) return;
    const FVector Here=GetActorLocation();
    if(Here.Z>World->SurfaceHeight(Here.X,Here.Y)+15) {World->Say(TEXT("Surface winch is for underground use")); return;}
    // Work from the current overhead ground if the original anchor has streamed out.
    FVector Anchor=World->State->WinchAnchor;
    if(FVector::Dist2D(Anchor,Here)>9000) Anchor=Here;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ETWinch),false,this);
    for(int32 Ring=0;Ring<=8;++Ring) for(int32 Side=0;Side<(Ring?12:1);++Side) {
        const double Angle=Side*2*PI/12;
        FVector Spot=Anchor+FVector(FMath::Cos(Angle)*Ring*330,FMath::Sin(Angle)*Ring*330,0);
        Spot.Z=World->SurfaceHeight(Spot.X,Spot.Y);
        FHitResult Floor;
        if(!GetWorld()->LineTraceSingleByChannel(Floor,Spot+FVector(0,0,1200),Spot-FVector(0,0,32),ECC_Visibility,Params)||Floor.ImpactNormal.Z<.72) continue;
        const FVector Exit=Floor.ImpactPoint+FVector(0,0,100);
        if(GetWorld()->OverlapBlockingTestByChannel(Exit,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Params)) continue;
        if(!TeleportTo(Exit,GetActorRotation())) continue;
        GetCharacterMovement()->StopMovementImmediately(); FallSpeed=0; Firing=false;
        World->State->WinchAnchor=Exit; World->Say(TEXT("Winch retrieved / back on the surface")); World->Save(); return;
    }
    World->Say(TEXT("No clear anchor overhead / move along the tunnel and try again"));
}

AETBoreBurst::AETBoreBurst()
{
    PrimaryActorTick.bCanEverTick=true;
    Chips=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("Chips")); SetRootComponent(Chips);
    Chips->SetCollisionEnabled(ECollisionEnabled::NoCollision); Chips->SetCastShadow(false);
    Flash=CreateDefaultSubobject<UPointLightComponent>(TEXT("Impact")); Flash->SetupAttachment(Chips);
    Flash->SetCastShadows(false); Flash->SetAttenuationRadius(950);
}
void AETBoreBurst::Ignite(AETWorld* W,FVector Normal,bool Ore,const TCHAR* ContactMaterial)
{
    Chips->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EARTHtotaled/Art/SM_Rock.SM_Rock")));
    Contact=ContactMaterial!=nullptr;
    Chips->SetMaterial(0,W->Material(Contact?ContactMaterial:Ore?TEXT("FaultGold"):TEXT("FaultShale")));
    Flash->SetLightColor(Ore||(Contact&&FCString::Strcmp(ContactMaterial,TEXT("ResinLight"))==0)?FLinearColor(1,.65f,.25f):FLinearColor(.58f,.84f,1));
    Flash->SetIntensity(Contact?1100:9500);
    for(int32 I=0;I<(Contact?6:14);++I) {
        Velocities.Add((Normal*.65+FMath::VRand())*FMath::FRandRange(160.f,450.f));
        Chips->AddInstance(FTransform(FRotator::ZeroRotator,FVector::ZeroVector,FVector(.035)));
    }
    SetLifeSpan(Contact?.22f:.48f);
}
void AETBoreBurst::Tick(float D)
{
    Super::Tick(D); Age+=D;
    Flash->SetIntensity((Contact?1100:9500)*FMath::Max(0.f,1-Age/(Contact?.07f:.16f)));
    for(int32 I=0;I<Velocities.Num();++I) {
        FVector P=Velocities[I]*Age-FVector(0,0,490*Age*Age);
        Chips->UpdateInstanceTransform(I,FTransform(FRotator(Age*(I+1)*65,I*32,0),P,FVector(.048f*FMath::Max(.05f,1-Age/.48f))),false,I==Velocities.Num()-1,true);
    }
}
