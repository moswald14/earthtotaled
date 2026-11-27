#pragma once
#include "CoreMinimal.h"
#include "ETWakeState.generated.h"

namespace ETWake
{
    enum Chapter { Missing, Triage, FirstHerd, Garage, Driving, Dispatch, Surveys, Runway, Hangar, Flight, Vault, Passage, Cell, Escort, Strike, Home };
    enum Form { Human, Bug, Frame };
    enum Ride { Foot, Car, Plane };
    constexpr int Herds=8;
}

USTRUCT()
struct FETVehicleSave
{
    GENERATED_BODY()
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() float Heading=0;
    UPROPERTY() float Speed=0;
    UPROPERTY() bool Started=false;
    UPROPERTY() bool Flying=false;
};

USTRUCT()
struct FETWakeSave
{
    GENERATED_BODY()
    UPROPERTY() int32 Chapter=ETWake::Missing;
    UPROPERTY() int32 Body=ETWake::Human;
    UPROPERTY() int32 Silenced=0;
    UPROPERTY() int32 Retuned=0;
    UPROPERTY() int32 TaskSide=0;
    UPROPERTY() int32 TaskHerd=0;
    UPROPERTY() bool ReadSource=false;
    UPROPERTY() bool SharedLead=false;
    UPROPERTY() int32 WarningSeen=0;
    UPROPERTY() bool DriveGate=false;
    UPROPERTY() bool TookOff=false;
    UPROPERTY() int32 Airfields=0;
    UPROPERTY() FIntPoint FieldSurvey=FIntPoint(-1,-1);
    UPROPERTY() int32 Seat=ETWake::Foot;
    UPROPERTY() FETVehicleSave Roadster;
    UPROPERTY() FETVehicleSave Aircraft;
    UPROPERTY() FVector EscortPosition=FVector::ZeroVector;
    UPROPERTY() int32 VaultLine=0;
    UPROPERTY() int32 EndingHerds=0;
    int Count(int Side) const {return FMath::CountBits(uint32(Side==ETWake::Bug?Silenced:Retuned));}
    int Winner() const {return Count(ETWake::Bug)>=4?ETWake::Bug:Count(ETWake::Frame)>=4?ETWake::Frame:ETWake::Human;}
    int Restored() const {return int(Chapter>=ETWake::Garage)+Count(ETWake::Bug)+Count(ETWake::Frame);}
    bool HasCar() const {return Chapter>=ETWake::Driving;}
    bool HasPlane() const {return Chapter>=ETWake::Flight;}
    bool Rescued() const {return Chapter>=ETWake::Strike;}
    bool Valid() const;
};
