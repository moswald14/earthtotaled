#pragma once

#include "CoreMinimal.h"
#include "UObject/Object.h"
#include "ETGeology.generated.h"

class AETRegion;
class AETWorld;
class AStaticMeshActor;
class UProceduralMeshComponent;

// All subsurface positions use world X/Y and Z relative to the original ground.
namespace ETRock
{
    constexpr int32 Cells = 16;
    constexpr int32 Step = 100;
    constexpr int32 Span = Cells*Step;
    constexpr double ZOffset = 50;
    constexpr double PulseRadius = 285;
    constexpr double ToolRange = 2750;
    FIntVector Chunk(FVector P);
    FIntPoint Region(FIntVector ChunkKey);

    struct FCavity
    {
        FVector A, B;
        double Radius;
        double Distance(FVector P) const;
        FBox Bounds() const;
    };
    struct FVein
    {
        FIntVector Id;
        FCavity Shape;
        bool Gold;
        int32 Yield;
    };
    struct FChunkField
    {
        TArray<FCavity> Voids;
        TArray<FVector> BoxCenters;
        TSet<FIntVector> Fallen;
        double AirDistance(FVector P,double SurfaceZ) const;
    };
}

UCLASS()
class UETGeology : public UObject
{
    GENERATED_BODY()
public:
    void Initialize(AETWorld* InWorld);
    bool PrepareRegionSlice(AETRegion& Region);
    void FinishRegion(AETRegion& Region);
    void RegisterGroundProp(AStaticMeshActor* Prop);
    void ForgetGroundProp(AStaticMeshActor* Prop);
    bool CanBore(const FHitResult& Hit) const;
    bool CutTrench(int32 Side,FString& Reward);
    bool BugCanStep(FVector From,FVector To) const;
    void IndexTrench(int32 Side,TSet<FIntVector>& Dirty);
    // All depths share the same surface ownership. A nonempty reason denies the whole edit.
    const TCHAR* DigRestriction(const FBox2D& Footprint) const;
    bool Blast(const FHitResult& Hit,FString& Reward,bool Square=false,bool Recover=true);
    bool ReplacesSurface(FIntPoint Patch) const;
    double SurfaceOpening(double X,double Y) const;
    bool IsOpen(FVector WorldPosition) const;
    bool ExtractNear(FVector Anchor,int32& Iron,int32& Gold);
    static int32 Grade(double Depth) {return 1+FMath::Clamp(FMath::FloorToInt(Depth/1600.),0,7);}
    FString Survey(FVector Eye,FVector Forward) const;
    static constexpr double CutQuantum=25;
private:
    UPROPERTY() AETWorld* World;
    TMap<FIntVector,ETRock::FChunkField> Fields;
    TMap<FIntPoint,TSet<FIntVector>> RegionChunks;
    TMap<FIntPoint,int32> LowestCut;
    TSet<FIntVector> IndexedCuts, IndexedBoxes, ActiveGalleries;
    TMap<FIntPoint,TArray<TWeakObjectPtr<AStaticMeshActor>>> GroundProps;
    TMap<TWeakObjectPtr<AStaticMeshActor>,TWeakObjectPtr<UProceduralMeshComponent>> Pavement;

    void AddCavity(const ETRock::FCavity& Shape,TSet<FIntVector>& Dirty);
    ETRock::FChunkField& TouchChunk(FIntVector Key,TSet<FIntVector>& Dirty);
    void IndexFallen(FIntVector Node,TSet<FIntVector>& Dirty);
    FBox BreakDetachedRock(FVector Center,FVector Reach,TSet<FIntVector>& Dirty);
    void IndexCut(FIntVector Cut,TSet<FIntVector>& Dirty,bool Square=false,bool Restoring=false);
    TArray<ETRock::FCavity> Gallery(FIntVector Cell) const;
    TArray<ETRock::FVein> Veins(const FBox& Bounds) const;
    bool RemainingOre(const ETRock::FVein& Vein,FVector From,FVector& Target) const;
    void Rebuild(AETRegion& Region,FIntVector Key);
    void UpdateProp(AStaticMeshActor& Prop);
    void ClearScatter(AETRegion& Region,FIntPoint Patch);
};
