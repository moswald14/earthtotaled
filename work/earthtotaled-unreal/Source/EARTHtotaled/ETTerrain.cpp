#include "ETGame.h"
#include "ProceduralMeshComponent.h"

bool AETRegion::BuildTerrainSlice()
{
    const int32 Grid=WantedTerrainGrid;
    if(TerrainGrid==Grid&&TerrainBuildGrid==0) return true;
    if(TerrainBuildGrid!=Grid) {TerrainBuildGrid=Grid; TerrainRow=0; TerrainVertices.Reset((Grid+1)*(Grid+1));}
    const double Step=AETWorld::RegionSize/Grid; const FVector Origin=GetActorLocation();
    const int32 LastRow=FMath::Min(Grid,TerrainRow+7);
    for(;TerrainRow<=LastRow;++TerrainRow) for(int32 X=0;X<=Grid;++X) {
        const int32 Y=TerrainRow; const double WX=Origin.X+X*Step,WY=Origin.Y+Y*Step;
        double Z=World->Height(WX,WY);
        // Both detail levels share the same boundary curve, so changing detail cannot open cracks.
        if(X==0||X==Grid) {
            const double Low=FMath::FloorToDouble(WY/800)*800;
            Z=FMath::Lerp(World->Height(WX,Low),World->Height(WX,Low+800),(WY-Low)/800);
        } else if(Y==0||Y==Grid) {
            const double Low=FMath::FloorToDouble(WX/800)*800;
            Z=FMath::Lerp(World->Height(Low,WY),World->Height(Low+800,WY),(WX-Low)/800);
        }
        TerrainVertices.Add(FVector(X*Step,Y*Step,Z));
    }
    if(TerrainRow<=Grid) return false;
    TArray<FVector> Normals; TArray<int32> Indices; TArray<FVector2D> UV; TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
    Normals.Reserve(TerrainVertices.Num()); UV.Reserve(TerrainVertices.Num()); Tangents.Reserve(TerrainVertices.Num()); Indices.Reserve(Grid*Grid*6);
    for(int32 Y=0;Y<=Grid;++Y) for(int32 X=0;X<=Grid;++X) {
        const double WX=Origin.X+X*Step,WY=Origin.Y+Y*Step; FVector N;
        if(X==0||X==Grid||Y==0||Y==Grid) N=FVector(World->Height(WX-80,WY)-World->Height(WX+80,WY),World->Height(WX,WY-80)-World->Height(WX,WY+80),160);
        else N=FVector(TerrainVertices[Y*(Grid+1)+X-1].Z-TerrainVertices[Y*(Grid+1)+X+1].Z,TerrainVertices[(Y-1)*(Grid+1)+X].Z-TerrainVertices[(Y+1)*(Grid+1)+X].Z,2*Step);
        N.Normalize(); Normals.Add(N); UV.Add(FVector2D(WX/400,WY/400));
        Tangents.Add(FProcMeshTangent(FVector(1,0,-N.X/FMath::Max(.01,N.Z)).GetSafeNormal(),false));
        if(X<Grid&&Y<Grid) {int32 A=Y*(Grid+1)+X,B=A+1,D=A+Grid+1,E=D+1; Indices.Append({A,D,B,B,D,E});}
    }
    // The old surface remains usable until the replacement's vertices and collision are ready.
    Terrain->CreateMeshSection_LinearColor(0,TerrainVertices,Indices,Normals,UV,Colors,Tangents,true);
    Terrain->SetMaterial(0,World->Material(TEXT("Ground"))); TerrainGrid=Grid; TerrainBuildGrid=0; TerrainVertices.Reset(); return true;
}
