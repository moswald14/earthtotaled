#pragma once
#include "ETNavigation.h"

namespace ETWar
{
    // Reinforcements move on the world clock, including when their tiles are unloaded.
    inline bool BugAdvance(const ETNav::FSite& Site,float Elapsed)
    {
        const double Interval=75.+Site.Variant%29;
        const double Offset=(uint32(Site.Cell.X)*17u+uint32(Site.Cell.Y)*31u+uint32(Site.Axis)*19u)%71;
        return FMath::Fmod(FMath::FloorToDouble((Elapsed+Offset)/Interval),2.)==0.;
    }
}
