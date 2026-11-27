#pragma once

#include "CoreMinimal.h"
#include "ETNavigation.h"

namespace ETRing
{
    constexpr int Stops=8, Spans=1024;
    constexpr double Radius=357770.87639996636, Cruise=11000., Dwell=12., Ramp=3.;
    const FVector2D Center=FVector2D(2.,1.)*ETNav::DistrictSpacing+FVector2D(-1200.,-10800.);
    const FIntPoint Offsets[Stops]={{-2,-1},{-1,-2},{1,-2},{2,-1},{2,1},{1,2},{-1,2},{-2,1}};
    inline const TCHAR* Name(int Stop) {
        const TCHAR* Names[]={TEXT("Shelter 09"),TEXT("Cinder Quay"),TEXT("East Orchard"),TEXT("Kiln Exchange"),
            TEXT("Signal Heights"),TEXT("Amber Works"),TEXT("Blackpine Gate"),TEXT("West Viaduct")};
        return Names[Stop];
    }
    inline FVector2D Radial(double A) {return FVector2D(FMath::Cos(A),FMath::Sin(A));}
    inline FVector2D Position(double A) {return Center+Radial(A)*Radius;}
    inline FVector2D Station(int I) {return Center+FVector2D(Offsets[I])*ETNav::DistrictSpacing;}
    struct FPhase {int Stop; double Angle,Speed,Seconds; bool Docked;};
    struct FTimetable {
        double Angles[Stops+1],Arrival[Stops+1]={};
        FTimetable() {
            for(int I=0;I<Stops;++I) Angles[I]=FMath::Atan2(double(Offsets[I].Y),double(Offsets[I].X));
            Angles[Stops]=Angles[0]+UE_TWO_PI;
            for(int I=0;I<Stops;++I) Arrival[I+1]=Arrival[I]+Dwell+Ramp+(Angles[I+1]-Angles[I])*Radius/Cruise;
        }
        double Wrap(double T) const {return FMath::Fmod(FMath::Max(0.,T),Arrival[Stops]);}
        FPhase At(double Clock) const {
            const double T=Wrap(Clock);
            int I=0; while(I+1<Stops&&T>=Arrival[I+1]) ++I;
            const double Travel=T-Arrival[I]-Dwell;
            if(Travel<=0) return {I,Angles[I],0.,-Travel,true};
            const double Left=Arrival[I+1]-T;
            double Dist,Speed;
            if(Travel<Ramp) {Speed=Cruise*Travel/Ramp; Dist=Speed*Travel*.5;}
            else if(Left<Ramp) {Speed=Cruise*Left/Ramp; Dist=(Angles[I+1]-Angles[I])*Radius-Speed*Left*.5;}
            else {Speed=Cruise; Dist=Cruise*(Travel-.5*Ramp);}
            return {(I+1)%Stops,Angles[I]+Dist/Radius,Speed,Left,false};
        }
    };
    inline const FTimetable& Schedule() {static const FTimetable T;return T;}
}
