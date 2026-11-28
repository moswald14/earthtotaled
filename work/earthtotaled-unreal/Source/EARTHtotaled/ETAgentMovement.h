#pragma once
#include "CoreMinimal.h"
class AETAgent;
class AActor;
class AETWorld;

namespace ETAgentMovement {
    bool FindGroundSpawn(AETWorld& World,FVector& Position);
    void Face(AETAgent& Agent,FVector Direction,float Delta);
    void Walk(AETAgent& Agent,FVector Direction,float Speed,float Delta,AActor* Target);
    void FlyTo(AETAgent& Agent,FVector Goal,float Speed,float Delta);
    void Patrol(AETAgent& Agent,float Delta);
}
