#pragma once

#include "CoreMinimal.h"

namespace ETNav
{
    constexpr double DistrictSpacing=160000.;
    constexpr double MinSpan=11000., MaxSpan=420000.;
    constexpr int32 MaxPins=32;
    struct FRoad
    {
        FVector2D A,B;
        const TCHAR* Name;
        double Width;
        bool Highway;
        bool Feeder=false;
        bool Horizontal() const {return FMath::Abs(B.X-A.X)>FMath::Abs(B.Y-A.Y);}
        double Distance(FVector2D P) const;
    };
    FIntPoint District(FVector2D Position);
    FVector2D Refuge(FIntPoint District);
    const TCHAR* RefugeName(FIntPoint District);
    struct FSite
    {
        FIntPoint Cell;
        int Axis=0;
        bool Bug=false;
        uint32 Variant=0;
        FVector2D Exit,Along,Inward,Center;
        const TCHAR* Name() const;
        FString Id() const;
        FVector2D At(double Across,double In) const {return Center+Along*Across+Inward*In;}
        bool InGrounds(FVector2D P,double Extra=0) const;
    };
    FSite Site(FIntPoint Cell,int Axis);
    void Sites(const FBox2D& Area,TFunctionRef<void(const FSite&)> Visit);
    void Roads(const FBox2D& Area,TFunctionRef<void(const FRoad&)> Visit);
    double Bearing(FVector2D Delta);
    FString Distance(double Centimeters);

    struct FChart
    {
        FVector2D Position,Size,Center;
        double Span;
        double Scale() const {return Size.Y/Span;}
        FVector2D Plot(FVector2D World) const {return Position+Size*.5+(World-Center)*Scale();}
        FVector2D WorldAt(FVector2D Pixel) const {return Center+(Pixel-Position-Size*.5)/Scale();}
        bool Inside(FVector2D Pixel) const;
        FBox2D Bounds() const {return FBox2D(WorldAt(Position),WorldAt(Position+Size));}
    };
    FChart Atlas(FVector2D Viewport,FVector2D Center,double Span);
    FBox2D AtlasButton(const FChart& Chart,int Index);
    bool Clip(const FBox2D& Box,FVector2D& A,FVector2D& B);
    struct FContours
    {
        FBox2D Area{ForceInit};
        double Step=0;
        int32 Seed=0;
        TArray<TPair<FVector2D,FVector2D>> Lines;
    };
}
