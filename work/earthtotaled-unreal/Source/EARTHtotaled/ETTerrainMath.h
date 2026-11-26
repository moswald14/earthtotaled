#pragma once
#include <cmath>
#include <cstdint>

// Shared by the runtime terrain and the standalone seam/determinism check.
namespace ETMath
{
inline uint32_t Mix(uint32_t V) {V^=V>>16; V*=0x7feb352d; V^=V>>15; V*=0x846ca68b; return V^(V>>16);}
inline double Lerp(double A,double B,double T) {return A+(B-A)*T;}
inline double Noise(double X,double Y,uint32_t Seed)
{
    const int32_t IX=static_cast<int32_t>(std::floor(X)),IY=static_cast<int32_t>(std::floor(Y));
    auto Sample=[Seed](int32_t A,int32_t B){return double(Mix(uint32_t(A)*73856093u^uint32_t(B)*19349663u^Seed))/4294967295.0*2-1;};
    double U=X-IX,V=Y-IY; U=U*U*U*(U*(U*6-15)+10); V=V*V*V*(V*(V*6-15)+10);
    return Lerp(Lerp(Sample(IX,IY),Sample(IX+1,IY),U),Lerp(Sample(IX,IY+1),Sample(IX+1,IY+1),U),V);
}
inline double Relief(double X,double Y,uint32_t Seed)
{
    const double Bend=Noise(X/78000.,Y/78000.,Seed+177)*19500.;
    const double Basin=Noise((X+Bend)/47000.,(Y-Bend*.48)/47000.,Seed+283);
    const double Ridge=1.-std::abs(Noise((X-Bend)/28700.,(Y+Bend)/39400.,Seed+521));
    const double Bank=std::fmin(1.,std::abs(Basin)*4.);
    const double Shelf=Bank*Bank*(3.-2.*Bank);
    return Shelf*(1450.+3250.*Ridge*Ridge)-1650.+Noise(X/9000.,Y/9000.,Seed+771)*190.*Bank;
}
inline double Height(double X,double Y,uint32_t Seed)
{
    return Noise(X/61000,Y/61000,Seed)*2400+Noise(X/17000,Y/17000,Seed+17)*630+Noise(X/5100,Y/5100,Seed+43)*105;
}
}
