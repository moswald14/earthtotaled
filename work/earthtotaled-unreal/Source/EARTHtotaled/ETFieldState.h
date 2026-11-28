#pragma once
#include "CoreMinimal.h"
#include "ETFieldState.generated.h"

// Segment endpoints are the designed waterline in world space, ordered upstream to downstream.
USTRUCT()
struct FETFurrow {
    GENERATED_BODY()
    UPROPERTY() FVector Inlet=FVector::ZeroVector;
    UPROPERTY() FVector Outlet=FVector::ZeroVector;
};

USTRUCT()
struct FETFarm {
    GENERATED_BODY()
    UPROPERTY() int32 Style=0;
    UPROPERTY() int32 Crop=0;
    UPROPERTY() float Growth=0;
    UPROPERTY() float Moisture=35;
    UPROPERTY() int32 Fertility=0;
    UPROPERTY() int32 Stock=0;
    UPROPERTY() int32 Orders=0;
    UPROPERTY() float Time=0;
};

USTRUCT()
struct FETThreatRun {
    GENERATED_BODY()
    UPROPERTY() int32 Serial=0;
    UPROPERTY() int32 Type=0;
    UPROPERTY() FVector Rally=FVector::ZeroVector;
    UPROPERTY() TSet<FString> Remaining;
    UPROPERTY() int32 Completed=0;
};
