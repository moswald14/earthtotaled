#include "ETGame.h"
#include "ETTerrainMath.h"
#include "ETGeology.h"

namespace
{
    FIntPoint Tile(FVector P) {return FIntPoint(FMath::FloorToInt(P.X/AETWorld::RegionSize),FMath::FloorToInt(P.Y/AETWorld::RegionSize));}
}
namespace {
    constexpr double RoadCell=6400.;
    constexpr double RoadVerge=7200.;
    FIntPoint RoadTile(double X,double Y) {return {FMath::FloorToInt32(X/RoadCell),FMath::FloorToInt32(Y/RoadCell)};}
    struct FRoadCut {
        FIntPoint From,To;
        double Along,Across;
        int Axis;
    };
    bool LocateHighway(double X,double Y,FRoadCut& Cut) {
        constexpr double Span=ETNav::DistrictSpacing;
        for(int Axis=0;Axis<2;++Axis) {
            const double Run=Axis?Y:X,Side=Axis?X:Y;
            const int Link=FMath::FloorToInt32(Run/Span),Row=FMath::FloorToInt32(Side/Span+.5);
            const double Along=Run-Link*Span;
            if(Along<25200.||Along>Span-25200.) continue;
            const FIntPoint From=Axis?FIntPoint(Row,Link):FIntPoint(Link,Row);
            const FIntPoint To=From+(Axis?FIntPoint(0,1):FIntPoint(1,0));
            const double Street=Axis?3200.:-1700.;
            const double Start=From==FIntPoint::ZeroValue?2600.:Street,End=To==FIntPoint::ZeroValue?2600.:Street;
            const double Center=Row*Span+FMath::Lerp(Start,End,FMath::Clamp((Along-51000.)/58000.,0.,1.));
            const double Across=FMath::Abs(Side-Center);
            if(Across<RoadVerge) {Cut={From,To,Along,Across,Axis};return true;}
        }
        return false;
    }
}
void AETWorld::PrepareHighwayGrades()
{
    if(State->HasGradedRoads) return;
    if(HadSave) {
        auto Preserve=[&](FVector At) {
            FRoadCut Cut;
            if(LocateHighway(At.X,At.Y,Cut)) State->OriginalRoadTerrain.Add(RoadTile(At.X,At.Y));
        };
        Preserve(State->Position);Preserve(State->WinchAnchor);
        for(const auto& Build:State->Builds) Preserve(Build.Value.Anchor);
        for(const auto& Bore:State->BoreCuts) Preserve(FVector(Bore)*UETGeology::CutQuantum);
        for(const auto& Cut:State->SquareCuts) Preserve(FVector(Cut)*ETRock::Step);
        for(const auto& V:State->MinedVeins) if(V.Z<0) Preserve(FVector((V.X+.5)*1150,(V.Y+.5)*1150,0));
    }
    State->HasGradedRoads=true;
}
double AETWorld::GradeHighway(double X,double Y,double Natural) const
{
    FRoadCut Cut;
    if(!LocateHighway(X,Y,Cut)) return Natural;
    double Weight=(1.-FMath::SmoothStep(1400.,RoadVerge,Cut.Across))*
        FMath::SmoothStep(25200.,30500.,FMath::Min(Cut.Along,ETNav::DistrictSpacing-Cut.Along));
    if(!State->OriginalRoadTerrain.IsEmpty()) {
        const auto Near=RoadTile(X,Y);
        for(int J=-1;J<=1;++J) for(int I=-1;I<=1;++I) {
            const auto K=Near+FIntPoint(I,J);if(!State->OriginalRoadTerrain.Contains(K)) continue;
            const double DX=FMath::Max(0.,FMath::Abs(X-(K.X+.5)*RoadCell)-RoadCell*.5);
            const double DY=FMath::Max(0.,FMath::Abs(Y-(K.Y+.5)*RoadCell)-RoadCell*.5);
            Weight=FMath::Min(Weight,FMath::SmoothStep(900.,6000.,FMath::Sqrt(DX*DX+DY*DY)));
        }
        if(Weight==0.) return Natural;
    }
    auto StreetHeight=[&](FIntPoint C) {
        return C==FIntPoint::ZeroValue?0.:ETMath::Height(C.X*ETNav::DistrictSpacing,C.Y*ETNav::DistrictSpacing,State->Seed);
    };
    const double T=(Cut.Along-25200.)/(ETNav::DistrictSpacing-50400.);
    const double S=FMath::Sin(PI*T);
    const double Phase=(SeedFor(Cut.From,801+Cut.Axis)%4096)*(2.*PI/4096.);
    const double Crown=S*S*(780.*FMath::Sin(2.*PI*T+Phase)+230.*FMath::Sin(4.*PI*T-Phase));
    const double Bed=FMath::Lerp(StreetHeight(Cut.From),StreetHeight(Cut.To),T*T*(3.-2.*T))+Crown;
    return FMath::Lerp(Natural,Bed,Weight);
}
void AETWorld::PrepareLandscape()
{
    if(State->LandscapeEdition>0) return;
    if(HadSave) {
        State->LegacyGround.Add(Tile(State->Position)); State->LegacyGround.Add(Tile(State->WinchAnchor));
        for(const auto& B:State->Builds) State->LegacyGround.Add(Tile(B.Value.Anchor));
        for(const auto& C:State->BoreCuts) State->LegacyGround.Add(Tile(FVector(C)*UETGeology::CutQuantum));
        for(const auto& C:State->SquareCuts) State->LegacyGround.Add(Tile(FVector(C)*ETRock::Step));
        for(const auto& V:State->MinedVeins) if(V.Z<0) State->LegacyGround.Add(Tile(FVector((V.X+.5)*1150,(V.Y+.5)*1150,0)));
    }
    State->LandscapeEdition=1;
}
double AETWorld::Landscape(double X,double Y) const
{
    const uint32 Seed=State->Seed;
    const double Old=ETMath::Height(X,Y,Seed);
    // Around existing excavations the original surface defines the underground coordinate frame.
    const double RadiusSquared=X*X+Y*Y;
    if(RadiusSquared<=27000.*27000.) return Old;
    const double CityEdge=RadiusSquared>=39000.*39000.?1.:FMath::SmoothStep(27000.,39000.,FMath::Sqrt(RadiusSquared));
    double Blend=1;
    if(State->LegacyGround.Num()) {
        const FIntPoint C=Tile(FVector(X,Y,0));
        if(State->LegacyGround.Contains(C)) return Old;
        for(int32 J=-1;J<=1;++J) for(int32 I=-1;I<=1;++I) {
            const FIntPoint K=C+FIntPoint(I,J);
            if(!State->LegacyGround.Contains(K)) continue;
            const double DX=FMath::Max(FMath::Abs(X-(K.X+.5)*RegionSize)-RegionSize*.5,0.);
            const double DY=FMath::Max(FMath::Abs(Y-(K.Y+.5)*RegionSize)-RegionSize*.5,0.);
            Blend=FMath::Min(Blend,FMath::SmoothStep(600.,5800.,FMath::Sqrt(DX*DX+DY*DY)));
        }
    }
    return Old+CityEdge*Blend*(ETMath::Relief(X,Y,Seed)-DryDepth(X,Y));
}

