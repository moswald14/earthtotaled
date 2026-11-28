#pragma once
#include "ETGame.h"

namespace ETField {
    int32 Cap(const FString& Id);
    int32 Cost(const FETPerk& Perk,int Owned);
    FString Effect(const FString& Id,int Rank);
    const TCHAR* Crop(int Kind);
    const TCHAR* Track(int Index);
    const TCHAR* RunName(int Type);
    const TCHAR* FarmName(int Piece,int Style);
    FString FarmMesh(int Piece,int Style);
    int Stage(const FETFarm& Farm);
    bool Mineral(FVector2D Position,int Seed);
    // Distance to the nearest abandoned drainage channel and basin.
    void Basin(FVector2D P,int Seed,double& Stream,double& Lake);
}
