#pragma once
#include "CoreMinimal.h"
class AETWorld;
class AETCharacter;
namespace ETFlora {
    void Start(AETWorld* World);
    void DeerTrial(AETWorld& World,AETCharacter& Player,TFunctionRef<void(bool,FString)> Check);
    void DeerView(AETWorld& World,AETCharacter& Player);
    void Trial(AETWorld& World,AETCharacter& Player,TFunctionRef<void(bool,FString)> Check);
}
