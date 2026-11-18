#pragma once
#include <cmath>
#include <cstdint>

// Weather follows saved survival time, so reopening a save cannot reroll a storm.
namespace ETClimate
{
    enum Type { Clear, Overcast, Mist, Rain, Storm };
    struct Frame { int Kind=Clear; float Cloud=0, Rainfall=0, Haze=0, Storminess=0, Wind=0; };
    inline Frame Profile(int Kind)
    {
        switch(Kind) {
        case Overcast: return {Kind,.72f,0,.16f,0,.45f};
        case Mist: return {Kind,.48f,0,1,0,.12f};
        case Rain: return {Kind,.88f,.65f,.32f,0,.65f};
        case Storm: return {Kind,.98f,1,.55f,1,1};
        default: return {Clear,.19f,0,0,0,.16f};
        }
    }
    inline int Front(std::int64_t Period, std::uint32_t Seed)
    {
        if(Period<=0) return Clear;
        const int Order[]={Clear,Overcast,Rain,Overcast,Clear,Mist,Overcast,Storm,Rain,Clear};
        const auto Cycle=Period/10;
        const int Offset=Cycle==0?0:int((Seed+std::uint32_t(Cycle)*7u)%10u);
        return Order[(Period+Offset)%10];
    }
    inline Frame Sample(double Seconds,std::uint32_t Seed)
    {
        if(!std::isfinite(Seconds)||Seconds<0) Seconds=0;
        const auto Period=std::int64_t(std::floor(Seconds/300.0));
        const auto A=Profile(Front(Period-1,Seed)), B=Profile(Front(Period,Seed));
        float T=float((Seconds-Period*300.0)/45.0); if(T>1) T=1;
        T=T*T*(3-2*T);
        return {B.Kind,A.Cloud+(B.Cloud-A.Cloud)*T,A.Rainfall+(B.Rainfall-A.Rainfall)*T,
            A.Haze+(B.Haze-A.Haze)*T,A.Storminess+(B.Storminess-A.Storminess)*T,A.Wind+(B.Wind-A.Wind)*T};
    }
}
