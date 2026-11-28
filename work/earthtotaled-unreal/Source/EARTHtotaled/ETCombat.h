#pragma once
#include "CoreMinimal.h"

class AETAgent;
class AETCharacter;
class AETWorld;

namespace ETCombat {
    struct FVoice { const TCHAR* Kind; const TCHAR* Name; const TCHAR* Cue; const TCHAR* Caption; };
    inline constexpr FVoice Voices[] = {
        {TEXT("robot"), TEXT("Supertech Warden"), TEXT("AlertWarden"), TEXT("Warden locks on")},
        {TEXT("drone"), TEXT("Shard Drone"), TEXT("AlertShard"), TEXT("Shard drone chirps")},
        {TEXT("runner"), TEXT("Bug Runner"), TEXT("AlertRunner"), TEXT("Runner clicks")},
        {TEXT("spitter"), TEXT("Bug Spitter"), TEXT("AlertSpitter"), TEXT("Spitter hisses")},
        {TEXT("bulwark"), TEXT("Bug Bulwark"), TEXT("AlertBulwark"), TEXT("Bulwark bellows")},
        {TEXT("skimmer"), TEXT("Bug Skimmer"), TEXT("AlertSkimmer"), TEXT("Skimmer wings buzz")},
        {TEXT("hive"), TEXT("Bug Hive"), TEXT("AlertHive"), TEXT("Hive answers in chorus")},
        {TEXT("walker"), TEXT("Hollow Walker"), TEXT("AlertWalker"), TEXT("Hollow walker moans")},
        {TEXT("wolf"), TEXT("Wolf"), TEXT("AlertWolf"), TEXT("Wolf growls")},
        {TEXT("boar"), TEXT("Wild Boar"), TEXT("AlertBoar"), TEXT("Boar snorts")},
        {TEXT("bear"), TEXT("Woodland Bear"), TEXT("AlertBear"), TEXT("Bear roars")}
    };
    const FVoice* Voice(const FString& Kind);
    bool HostileInDaylight(const AETAgent& Agent);
    FVector2D Bearing(FVector TowardSource, float ViewYaw);
    const TCHAR* Direction(FVector2D Bearing);
    void Trial(AETWorld& World, AETCharacter& Player, bool Visual);
    void EncounterTrial(AETWorld& World,AETCharacter& Player);
    void HighwayContactTrial(AETWorld& World,AETCharacter& Player,const FString& Route);
}
