#include "ETGame.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"

void AETCharacter::OpenDevice(AETConstruction* A)
{
    const auto* B=A?World->State->Builds.Find(A->Id):nullptr;
    if(!B||B->Piece<AETConstruction::Core) return;
    DeviceId=A->Id; auto S=World->State;
    FString Info=FString::Printf(TEXT("Integrity %.0f / %.0f     Pack: %d iron, %d gold ore\n\n"),B->Integrity,A->Durability(B->Piece),S->Inventory.FindRef(TEXT("ironore")),S->Inventory.FindRef(TEXT("goldore")));
    if(B->Piece==A->Core) {
        Info+=FString::Printf(TEXT("Outpost value %d / raids begin at 27. Turret power reaches 75 m.\nRaids held: %d. Armor tier %d / 3."),World->BaseValue(A->Id),S->Waves,S->ArmorTier);
        if(S->ArmorTier<3) Info+=FString::Printf(TEXT("\nNext armor tier: %d iron + %d gold ore. Raises capacity and damage absorption."),(S->ArmorTier+1)*4,S->ArmorTier+1);
        Info+=TEXT("\nValuable outposts draw Bug raiders after 19:00. A raid pauses at dawn. Repel the attackers to trace their nest.");
    } else if(B->Piece==A->Miner) {
        Info+=FString::Printf(TEXT("Hopper: %d iron, %d gold ore. Capacity: 80 before the next load.\n"),B->Iron,B->GoldOre);
        Info+=B->Work<0?TEXT("Local deposits exhausted. Salvage the rig and relocate it to fresh ground."):TEXT("Drilling local deposits. One cycle takes 30 seconds while this region is active. Underground sites yield richer ore.");
    } else Info+=World->NearestCore(B->Anchor,7500).IsValid()?TEXT("Powered. Tracks Supertech and Bug fighters in a 34 m radius. Keep its line of fire clear."):TEXT("Offline. Build a relay within 75 m.");
    OpenPanel(TEXT("device"),A->Name(B->Piece),Info);
}
void AETCharacter::ServiceDevice(const FString& Id)
{
    auto* A=World->LiveBuilds.FindRef(DeviceId).Get();
    auto* S=World->State;
    auto* B=S->Builds.Find(DeviceId);
    if(!A||!B) {ClosePanel(); return;}
    FString Note;
    if(Id==TEXT("hopper")&&B->Piece==A->Miner) {
        S->Inventory.FindOrAdd(TEXT("ironore"))+=B->Iron; S->Inventory.FindOrAdd(TEXT("goldore"))+=B->GoldOre;
        Note=FString::Printf(TEXT("Collected %d iron and %d gold ore."),B->Iron,B->GoldOre); B->Iron=B->GoldOre=0;
    } else if(Id==TEXT("plate")&&B->Piece==A->Core) {
        const int32 Tier=S->ArmorTier+1;
        if(Tier>3) Note=TEXT("Armor plating fully upgraded.");
        else if(S->Inventory.FindRef(TEXT("ironore"))<Tier*4||S->Inventory.FindRef(TEXT("goldore"))<Tier) Note=TEXT("More iron and gold ore needed.");
        else {S->Inventory.FindOrAdd(TEXT("ironore"))-=Tier*4; S->Inventory.FindOrAdd(TEXT("goldore"))-=Tier; S->ArmorTier=Tier; S->Armor=S->ArmorLimit(); Note=TEXT("Plating fitted. Suit fully repaired.");}
    } else if(Id==TEXT("mend")) {
        if(B->Integrity>=A->Durability(B->Piece)) Note=TEXT("Integrity at maximum.");
        else if(S->Inventory.FindRef(TEXT("ironore"))<1) Note=TEXT("Repairs need one iron ore.");
        else {--S->Inventory.FindOrAdd(TEXT("ironore")); B->Integrity=FMath::Min(A->Durability(B->Piece),B->Integrity+100); Note=TEXT("Restored up to 100 integrity.");}
    } else if(Id==TEXT("hunt")&&B->Piece==A->Core) {
        if(S->Waves<1) Note=TEXT("Repel a raid to trace its signal.");
        else {
            const FString Nest=FString::Printf(TEXT("nest-%s-%d"),*DeviceId.ToString(),S->Waves);
            if(S->Consumed.Contains(Nest)) Note=TEXT("This nest is cleared. Another raid will leave a fresh trail.");
            else {
                if(!S->Threats.Contains(Nest)) {
                    const FVector Origin=B->Anchor; FRandomStream R(GetTypeHash(DeviceId)+S->Waves);
                    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETNestSite),false);
                    for(int32 I=0;I<24;++I) {
                        FVector Site=Origin+FRotator(0,R.FRandRange(0,360),0).Vector()*R.FRandRange(5800,8600);
                        Site.Z=World->SurfaceHeight(Site.X,Site.Y);
                        FHitResult Ground;
                        if(!GetWorld()->LineTraceSingleByChannel(Ground,Site+FVector(0,0,170),Site-FVector(0,0,150),ECC_Visibility,Q)||Ground.ImpactNormal.Z<.65||FMath::Abs(Ground.ImpactPoint.Z-Site.Z)>70||World->SafeSettlement(Site)) continue;
                        Site=Ground.ImpactPoint+FVector(0,0,8);
                        if(!GetWorld()->OverlapBlockingTestByChannel(Site+FVector(0,0,135),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(105,120),Q)) {World->AddThreat(Nest,TEXT("hive"),Site); break;}
                    }
                }
                if(auto* Mark=S->Threats.Find(Nest)) {
                    const FVector To=Mark->Position-B->Anchor;
                    Note=FString::Printf(TEXT("Nest signal: %.0f m, bearing %03d degrees (east = 000). Tracker active on your HUD."),To.Size2D()/100,(FMath::RoundToInt(To.Rotation().Yaw)+360)%360);
                } else Note=TEXT("Signal obstructed. Scan again from open ground.");
            }
        }
    } else return;
    World->Save(); OpenDevice(A); PanelBody+=TEXT("\n\n")+Note;
}

