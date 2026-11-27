#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ETRing.h"
#include "ETRingLine.generated.h"

class UBoxComponent;
class UHierarchicalInstancedStaticMeshComponent;
class AETWorld;
class AETCharacter;

UCLASS()
class AETRingLine : public AActor
{
    GENERATED_BODY()
public:
    AETRingLine();
    void Initialize(AETWorld* World);
    virtual void Tick(float Delta) override;
    FVector RoutePoint(double Angle) const;
    FTransform Platform(int Stop) const;
    bool Riding(const AETCharacter& P) const;
    FString Prompt(const AETCharacter& P) const;
    bool UseStation(AETCharacter& P);
    void PositionCar(double Clock,float Delta=0);
    void MatchViewDistance(int Metres);
    void Capture(AETCharacter& P);
    FVector SavedPosition() const;
    void Restore(AETCharacter& P);
    void TestRide(AETCharacter& P);
    ETRing::FPhase Phase{};
    UPROPERTY() UBoxComponent* Deck;
private:
    int BoardingArea(const AETCharacter& P,bool& Street) const;
    UPROPERTY() AETWorld* Zone;
    UPROPERTY() UStaticMeshComponent* Sled;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Guide;
    UPROPERTY() UHierarchicalInstancedStaticMeshComponent* Piers;
    TArray<double> Heights;
    int LastDistance=0;
};
