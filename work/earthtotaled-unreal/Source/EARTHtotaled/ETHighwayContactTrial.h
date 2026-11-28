#pragma once

class AActor;
class AETAgent;
class AETEnemyBolt;

namespace ETHighwayContactTrial {
    void Shot(AETEnemyBolt& Bolt,const AETAgent& Shooter);
    void Hit(AActor* Causer,float AppliedDamage);
}
