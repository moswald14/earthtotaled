#include "ETField.h"
#include "ETGeology.h"
#include "ProceduralMeshComponent.h"

double AETWorld::SurfaceHeight(double X,double Y) const
{
    // Both heightfield LODs and volumetric patches share these exact edge curves.
    auto AlongY=[&](double Step) {const double A=FMath::FloorToDouble(Y/Step)*Step; return FMath::Lerp(Height(X,A),Height(X,A+Step),(Y-A)/Step);};
    auto AlongX=[&](double Step) {const double A=FMath::FloorToDouble(X/Step)*Step; return FMath::Lerp(Height(A,Y),Height(A+Step,Y),(X-A)/Step);};
    if(FMath::IsNearlyZero(FMath::Fmod(X,RegionSize))) return AlongY(800);
    if(FMath::IsNearlyZero(FMath::Fmod(Y,RegionSize))) return AlongX(800);
    if(FMath::IsNearlyZero(FMath::Fmod(X,double(ETRock::Span)))) return AlongY(400);
    if(FMath::IsNearlyZero(FMath::Fmod(Y,double(ETRock::Span)))) return AlongX(400);
    return Height(X,Y);
}
void AETRegion::TerrainIndices(int32 Grid,TArray<int32>& Out) const
{
    const int32 Patches=int32(AETWorld::RegionSize)/ETRock::Span, Width=Grid/Patches;
    Out.Reset(Grid*Grid*6);
    for(int32 PY=0;PY<Patches;++PY) for(int32 PX=0;PX<Patches;++PX) {
        if(World->Geology->ReplacesSurface({Coord.X*Patches+PX,Coord.Y*Patches+PY})) continue;
        for(int32 Y=PY*Width;Y<(PY+1)*Width;++Y) for(int32 X=PX*Width;X<(PX+1)*Width;++X) {
            const int32 A=Y*(Grid+1)+X,B=A+1,C=A+Grid+1,E=C+1;
            Out.Append({A,C,B,B,C,E});
        }
    }
}
void AETRegion::RefreshSurfaceCollision()
{
    const auto* Current=Terrain->GetProcMeshSection(0);
    if(!Current||!TerrainGrid) return;
    FProcMeshSection Replacement=*Current;
    TArray<int32> Indices;
    TerrainIndices(TerrainGrid,Indices);
    Replacement.ProcIndexBuffer.Reset(Indices.Num());
    for(int32 I:Indices) Replacement.ProcIndexBuffer.Add(uint32(I));
    Terrain->SetProcMeshSection(0,Replacement);
}

bool AETRegion::BuildTerrainSlice()
{
    const int32 Grid=WantedTerrainGrid;
    if(TerrainGrid==Grid&&TerrainBuildGrid==0) return true;
    if(TerrainBuildGrid!=Grid) {TerrainBuildGrid=Grid; TerrainRow=0; TerrainVertices.Reset((Grid+1)*(Grid+1));}
    const double Step=AETWorld::RegionSize/Grid; const FVector Origin=GetActorLocation();
    const int32 LastRow=FMath::Min(Grid,TerrainRow+7);
    for(;TerrainRow<=LastRow;++TerrainRow) for(int32 X=0;X<=Grid;++X) {
        const int32 Y=TerrainRow; const double WX=Origin.X+X*Step,WY=Origin.Y+Y*Step;
        const double Z=World->SurfaceHeight(WX,WY);
        TerrainVertices.Add(FVector(X*Step,Y*Step,Z));
    }
    if(TerrainRow<=Grid) return false;
    // Rehydrate at most one saved rock mesh in each streaming slice.
    if(!TerrainGrid&&!World->Geology->PrepareRegionSlice(*this)) return false;
    TArray<FVector> Normals; TArray<int32> Indices; TArray<FVector2D> UV; TArray<FLinearColor> Colors; TArray<FProcMeshTangent> Tangents;
    Normals.Reserve(TerrainVertices.Num()); UV.Reserve(TerrainVertices.Num()); Tangents.Reserve(TerrainVertices.Num()); Indices.Reserve(Grid*Grid*6);
    for(int32 Y=0;Y<=Grid;++Y) for(int32 X=0;X<=Grid;++X) {
        const double WX=Origin.X+X*Step,WY=Origin.Y+Y*Step; FVector N;
        if(X==0||X==Grid||Y==0||Y==Grid) N=FVector(World->Height(WX-80,WY)-World->Height(WX+80,WY),World->Height(WX,WY-80)-World->Height(WX,WY+80),160);
        else N=FVector(TerrainVertices[Y*(Grid+1)+X-1].Z-TerrainVertices[Y*(Grid+1)+X+1].Z,TerrainVertices[(Y-1)*(Grid+1)+X].Z-TerrainVertices[(Y+1)*(Grid+1)+X].Z,2*Step);
        const double Depth=World->DryDepth(WX,WY); double Stream,Lake; ETField::Basin({WX,WY},World->State->Seed,Stream,Lake);
        const float Clay=FMath::SmoothStep(16.,130.,Depth);
        const float Salt=Clay*FMath::Max(0.,1.-FMath::Abs(Lake-.86)/.14);
        Colors.Add(FLinearColor(Clay,Salt,0,1));
        N.Normalize(); Normals.Add(N); UV.Add(FVector2D(WX/400,WY/400));
        Tangents.Add(FProcMeshTangent(FVector(1,0,-N.X/FMath::Max(.01,N.Z)).GetSafeNormal(),false));
    }
    TerrainIndices(Grid,Indices);
    // The old surface remains usable until the replacement's vertices and collision are ready.
    Terrain->CreateMeshSection_LinearColor(0,TerrainVertices,Indices,Normals,UV,Colors,Tangents,true);
    Terrain->SetMaterial(0,World->Material(TEXT("DryGround"))); TerrainGrid=Grid; TerrainBuildGrid=0; TerrainVertices.Reset();
    BuildRoadSurface(); return true;
}
