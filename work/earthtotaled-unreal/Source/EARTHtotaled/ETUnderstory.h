#pragma once
#include "CoreMinimal.h"
class AETWorld;
class AETCharacter;
namespace ETFlora {
    void Start(AETWorld* World);
    void Trial(AETWorld& World,AETCharacter& Player,TFunctionRef<void(bool,FString)> Check);
}
