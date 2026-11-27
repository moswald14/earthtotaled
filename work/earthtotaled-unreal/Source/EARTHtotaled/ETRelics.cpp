#include "ETGame.h"
#include "ETLandfall.h"

namespace {
    const TCHAR* Makers[]={TEXT("Meridian"),TEXT("Nightline"),TEXT("Bellwether"),TEXT("Ashriver"),TEXT("Vesper"),TEXT("Kiln")};
    const TCHAR* Calls[]={TEXT("Moth"),TEXT("Stranger"),TEXT("Comet"),TEXT("Jackal"),TEXT("Courier"),TEXT("Sparrow"),TEXT("Cinder"),TEXT("Latch")};
    const TCHAR* Traits[]={TEXT("Hot load (+14% damage)"),TEXT("Short throw (15% faster reload)"),TEXT("Deep well (+30% magazine)"),TEXT("Match barrel (30% tighter spread)"),TEXT("Light bolt (12% faster cycling)")};
    const TCHAR* Paths[]={TEXT("Impact / +14% damage per rank"),TEXT("Cycler / faster fire and reload per rank"),TEXT("Surveyor / steadier aim, longer reach and larger magazine per rank")};
}
ETLandfall::FRoll::FRoll(int Seed)
{
    FRandomStream R(Seed);
    Frame=R.RandRange(0,9); Path=R.RandRange(0,2); First=R.RandRange(0,4);
    Second=(First+R.RandRange(1,4))%5; Pace=R.RandRange(85,130);
    Power=R.FRandRange(.93f,1.24f); Cycle=R.FRandRange(.86f,1.12f); Accuracy=R.FRandRange(.76f,1.1f);
    const int A=R.RandRange(0,5),B=R.RandRange(0,7);
    Name=FString::Printf(TEXT("%s %s %03X"),Makers[A],Calls[B],Seed&4095);
}
float ETLandfall::FRoll::Next(int R) const {return AETCharacter::NextGunRank(R)*Pace/100.f;}
int ETLandfall::FRoll::Rank(float Damage) const {int R=1;while(Next(R)>0&&Damage>=Next(R)) ++R;return R;}
FETWeapon ETLandfall::FRoll::Build(const FETWeapon& Base,int Rank) const
{
    FETWeapon W=Base; W.Name=Name; W.Damage*=Power;W.Rate*=Cycle;W.Spread*=Accuracy;
    for(int Perk:{First,Second}) switch(Perk) {
        case 0:W.Damage*=1.14f;break; case 1:W.Reload*=.85f;break;
        case 2:W.Mag=FMath::CeilToInt(W.Mag*1.3f);break; case 3:W.Spread*=.7f;break; case 4:W.Rate*=.88f;break;
    }
    int Steps=Rank-1;
    if(Path==0) W.Damage*=1+.14f*Steps;
    if(Path==1) {W.Rate*=1-.06f*Steps; W.Reload*=1-.065f*Steps;}
    if(Path==2) {W.Spread*=1-.095f*Steps; W.Range*=1+.1f*Steps; W.Mag+=2*Steps;}
    return W;
}
FString ETLandfall::FRoll::Details(int R) const {
    return FString::Printf(TEXT("%s\n%s\n\nUpgrade path   %s\nMk %d of 6"),Traits[First],Traits[Second],Paths[Path],R);
}
FETWeapon AETCharacter::CurrentGun() const {
    auto& S=*World->State;
    if(S.Relics.IsValidIndex(S.EquippedRelic)) {
        const auto& Specimen=S.Relics[S.EquippedRelic];const ETLandfall::FRoll Roll(Specimen.Seed);
        return Roll.Build(World->Weapons[Roll.Frame],Roll.Rank(Specimen.DamageDealt));
    }
    return World->Weapons[S.Weapon];
}
int32& AETCharacter::CurrentMagazine() {auto& S=*World->State;return S.Relics.IsValidIndex(S.EquippedRelic)?S.Relics[S.EquippedRelic].Loaded:S.Ammo[S.Weapon];}
float& AETCharacter::CurrentGunDamage() {auto& S=*World->State;return S.Relics.IsValidIndex(S.EquippedRelic)?S.Relics[S.EquippedRelic].DamageDealt:S.WeaponDamage[S.Weapon];}
int32 AETCharacter::CurrentRank() const {const auto& S=*World->State;if(S.Relics.IsValidIndex(S.EquippedRelic)){const auto& G=S.Relics[S.EquippedRelic];return ETLandfall::FRoll(G.Seed).Rank(G.DamageDealt);}return GunRank(S.Weapon);}
float AETCharacter::CurrentThreshold(int R) const {const auto& S=*World->State;return S.Relics.IsValidIndex(S.EquippedRelic)?ETLandfall::FRoll(S.Relics[S.EquippedRelic].Seed).Next(R):NextGunRank(R);}
void AETCharacter::EquipRelic(int I) {
    auto& S=*World->State;if(!S.Relics.IsValidIndex(I)) return; S.EquippedRelic=I; EquipFrame(ETLandfall::FRoll(S.Relics[I].Seed).Frame);
}
void AETCharacter::RelicRack() {
    auto& S=*World->State;RackIndex=FMath::Clamp(RackIndex,0,FMath::Max(0,S.Relics.Num()-1));
    if(!S.Relics.IsValidIndex(RackIndex)){OpenPanel(TEXT("relics"),TEXT("FIELD ARMORY"),TEXT("Enemies occasionally drop a unique gun. Approach the weapon and hold Ctrl to collect it."));return;}
    const auto& G=S.Relics[RackIndex];const ETLandfall::FRoll R(G.Seed);const int Rank=R.Rank(G.DamageDealt);
    const auto W=R.Build(World->Weapons[R.Frame],Rank);
    const FString Next=R.Next(Rank)>0?FString::Printf(TEXT("%.0f / %.0f damage to next rank"),G.DamageDealt,R.Next(Rank)):TEXT("Path complete");
    OpenPanel(TEXT("relics"),W.Name,FString::Printf(TEXT("%d / %d     %s%s\n\n%.1f damage    %.0f RPM    %d rounds    %.2f s reload\n\n%s\n%s"),RackIndex+1,S.Relics.Num(),*World->Weapons[R.Frame].Name,S.EquippedRelic==RackIndex?TEXT("  /  equipped"):TEXT(""),W.Damage,60.f/W.Rate,W.Mag,W.Reload,*R.Details(Rank),*Next));
}
