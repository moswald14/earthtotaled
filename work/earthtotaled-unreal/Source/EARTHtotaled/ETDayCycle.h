#pragma once
#include "CoreMinimal.h"

namespace ETDay {
    constexpr double Period=4440.;
    inline double At(double Hour) {
        if(Hour<6.) return Hour*120.;
        if(Hour<19.) return 720.+(Hour-6.)*240.;
        return 3840.+(Hour-19.)*120.;
    }
    inline double Phase(double Seconds) {return FMath::Fmod(FMath::Fmod(Seconds,Period)+Period,Period);}
    inline double Hour(double Seconds) {
        const double P=Phase(Seconds);
        return P<720.?P/120.:P<3840.?6.+(P-720.)/240.:19.+(P-3840.)/120.;
    }
    inline double FromOld(double Elapsed) {
        const double H=16.5+Elapsed/120.;
        return FMath::FloorToDouble(H/24.)*Period+At(FMath::Fmod(H,24.))-Elapsed;
    }
}
