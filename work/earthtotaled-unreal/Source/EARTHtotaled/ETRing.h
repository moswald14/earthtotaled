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
    enum EService { West, East, NorthLink, SouthLink, Services };
    inline bool Shuttle(int Line) {return Line==NorthLink||Line==SouthLink;}
    inline const TCHAR* ServiceName(int Line) {
        const TCHAR* Names[]={TEXT("West Ring"),TEXT("East Ring"),TEXT("North Shuttle"),TEXT("South Shuttle")};
        return Names[Line];
    }
    inline FVector2D EastOffset() {return {ETNav::DistrictSpacing*6.,0};}
    inline FVector2D CityStation(int City) {return Station(City%Stops)+(City>=Stops?EastOffset():FVector2D::ZeroVector);}
    inline const TCHAR* CityName(int City) {
        if(City<Stops) return Name(City);
        const TCHAR* Names[]={TEXT("Windfall Reach"),TEXT("Echo Basin"),TEXT("Copper Hollow"),TEXT("Lastbell"),
            TEXT("Reedwater"),TEXT("Highwood"),TEXT("Meridian Dome"),TEXT("Saltmarket")};
        return Names[City-Stops];
    }
    inline int CityAt(int Line,int Stop) {
        if(Line==NorthLink) return Stop==0?3:8;
        if(Line==SouthLink) return Stop==0?4:15;
        return Line*Stops+Stop;
    }
    inline int Connection(int City) {
        if(City==3||City==8) return NorthLink;
        if(City==4||City==15) return SouthLink;
        return INDEX_NONE;
    }
    inline int CityIn(FIntPoint Cell) {
        for(int I=0;I<2*Stops;++I)
            if(Cell==FIntPoint(I<Stops?2:8,1)+Offsets[I%Stops]) return I;
        return INDEX_NONE;
    }
    inline FVector2D Dock(int Line,int Stop) {
        return CityStation(CityAt(Line,Stop))+(Shuttle(Line)?FVector2D(Stop==0?6800.:-6800.,0.):FVector2D::ZeroVector);
    }
    inline FVector2D LiftPoint(int Line,int Stop) {return Dock(Line,Stop)+FVector2D(1200,1940);}
    constexpr int ShuttleSpans=140;
    inline int SpanCount(int Line) {return Shuttle(Line)?ShuttleSpans:Spans;}
    inline int StopCount(int Line) {return Shuttle(Line)?2:Stops;}
    inline double StepSize(int Line) {return Shuttle(Line)?1./ShuttleSpans:UE_TWO_PI/Spans;}
    inline FVector2D Track(int Line,double Param) {
        if(Shuttle(Line)) return FMath::Lerp(Dock(Line,0),Dock(Line,1),FMath::Clamp(Param,0.,1.));
        return Position(Param)+(Line==East?EastOffset():FVector2D::ZeroVector);
    }
    struct FPhase {int Stop; double Angle,Speed,Seconds; bool Docked; int Direction=1;};
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
    inline double ShuttleLength() {return Dock(NorthLink,1).X-Dock(NorthLink,0).X;}
    inline double ShuttleLeg() {return Dwell+Ramp+ShuttleLength()/Cruise;}
    inline double Period(int Line) {return Shuttle(Line)?2.*ShuttleLeg():Schedule().Arrival[Stops];}
    inline double DockTime(int Line,int Stop) {return Shuttle(Line)?Stop*ShuttleLeg():Schedule().Arrival[Stop];}
    inline double DockParam(int Line,int Stop) {return Shuttle(Line)?double(Stop):Schedule().Angles[Stop];}
    inline FPhase ServiceAt(int Line,double Clock) {
        if(!Shuttle(Line)) return Schedule().At(Clock);
        const double Leg=ShuttleLeg();
        const double Time=FMath::Fmod(FMath::Max(0.,Clock),2.*Leg);
        const int From=Time>=Leg?1:0,Dir=From?-1:1;
        const double T=Time-From*Leg-Dwell,Left=Leg-Dwell-T;
        if(T<=0) return {From,double(From),0,-T,true,Dir};
        double Speed=Cruise,Dist=Cruise*(T-Ramp*.5);
        if(T<Ramp) {Speed=Cruise*T/Ramp;Dist=Speed*T*.5;}
        else if(Left<Ramp) {Speed=Cruise*Left/Ramp;Dist=ShuttleLength()-Speed*Left*.5;}
        return {1-From,From+Dir*Dist/ShuttleLength(),Speed,Left,false,Dir};
    }
}
