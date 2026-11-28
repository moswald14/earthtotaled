#include "ETField.h"

namespace {
    struct FSkill {const TCHAR* Id; int Cap,Step; const TCHAR* Effect;};
    const FSkill Skills[]={
        {TEXT("parkour"),5,4,TEXT("+%d%% movement speed")},
        {TEXT("climber2"),3,15,TEXT("+%d%% climbing speed")},
        {TEXT("marksman"),5,6,TEXT("+%d%% gun and claw damage")},
        {TEXT("reload"),5,6,TEXT("+%d%% weapon reload speed")},
        {TEXT("vitality"),5,10,TEXT("+%d maximum health")},
        {TEXT("hydration"),5,6,TEXT("%d%% slower hydration drain")},
        {TEXT("momentum"),5,1,TEXT("Up to %d stacks, each +2%% speed and damage")},
        {TEXT("shieldharvest"),3,2,TEXT("+%d shield per hostile kill")},
        {TEXT("hunterscut"),5,5,TEXT("+%d%% scrap from hostile kills")}
    };
    const FSkill* Find(const FString& Id){for(const auto& S:Skills) if(Id==S.Id)return &S;return nullptr;}
}
int32 ETField::Cap(const FString& Id){const auto* S=Find(Id);return S?S->Cap:1;}
int32 ETField::Cost(const FETPerk& P,int Owned){return P.Cost+Owned/2;}
FString ETField::Effect(const FString& Id,int Rank){
    const auto* S=Find(Id);return S?FString(S->Effect).Replace(TEXT("%d"),*FString::FromInt(Rank*S->Step)).Replace(TEXT("%%"),TEXT("%")):FString();
}
int32 UETSave::Rank(const FString& Id) const {
    return FMath::Clamp(FMath::Max(Skills.Contains(Id)?1:0,SkillRanks.FindRef(Id)),0,ETField::Cap(Id));
}
void UETSave::MigrateRanks(){
    for(const FString& Id:Skills){int& R=SkillRanks.FindOrAdd(Id);R=FMath::Clamp(R,1,ETField::Cap(Id));}
}
namespace {
    int Momentum(const UETSave& S){return S.Elapsed>=S.LastHostileKill&&S.Elapsed-S.LastHostileKill<=8.f?FMath::Min(S.Streak,S.Rank(TEXT("momentum"))):0;}
}
float AETCharacter::DamageSkill() const {
    const auto& S=*World->State;return (1.f+.06f*S.Rank(TEXT("marksman")))*(1.f+.02f*Momentum(S));
}
float AETCharacter::MoveSkill() const {
    const auto& S=*World->State;return (1.f+.04f*S.Rank(TEXT("parkour")))*(1.f+.02f*Momentum(S));
}
