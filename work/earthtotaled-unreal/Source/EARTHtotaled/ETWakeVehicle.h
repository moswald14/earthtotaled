#pragma once
#include "GameFramework/Actor.h"
#include "ETWakeState.h"
#include "ETWakeVehicle.generated.h"

class AETWorld;
class AETCharacter;
class UBoxComponent;
class UStaticMeshComponent;

UCLASS()
class EARTHTOTALED_API AETWakeVehicle : public AActor
{
    GENERATED_BODY()
public:
    AETWakeVehicle();
    UPROPERTY() UBoxComponent* Hull;
    UPROPERTY() UStaticMeshComponent* Propeller=nullptr;
    UPROPERTY() AETWorld* Field=nullptr;
    int Type=ETWake::Car;
    float AudioTime=0;
    void Prepare(AETWorld& W,int VehicleType);
    void Drive(AETCharacter& P,float Seconds);
    void ParkAt(int FieldIndex);
    void StartEngine();
    FETVehicleSave& State() const;
    FVector SeatPosition() const;
    bool Boardable(const AETCharacter& P) const;
private:
    UStaticMeshComponent* Part(const TCHAR* Shape,const TCHAR* Material,FVector Position,FVector Size,FRotator Rotation=FRotator::ZeroRotator);
};
