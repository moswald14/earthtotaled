#include "ETGame.h"
#include "ETTerrainMath.h"
#include "ETGeology.h"

namespace
{
    FIntPoint Tile(FVector P) {return FIntPoint(FMath::FloorToInt(P.X/AETWorld::RegionSize),FMath::FloorToInt(P.Y/AETWorld::RegionSize));}
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
    return Old+CityEdge*Blend*ETMath::Relief(X,Y,Seed);
}

