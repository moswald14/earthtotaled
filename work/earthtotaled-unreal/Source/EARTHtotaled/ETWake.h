#pragma once
#include "ETWakeState.h"

class AETWorld;
class AETCharacter;
class AETItem;
namespace ETWake
{
    struct FSite {const TCHAR* Name; FVector2D Center;};
    const FSite& Herd(int Index);
    FVector2D Glade(int Index);
    FVector2D Field(int Index,const FETWakeSave& Save);
    void SurveyFields(AETWorld& World);
    FVector2D Location(const FETWakeSave& Save,FName Node,int Index=0);
    double StripZ(const AETWorld& World,int Index);
    int StripAt(const FETWakeSave& Save,FVector2D XY);
    bool Paved(const FETWakeSave& Save,FVector2D XY);
    bool Reserved(const FETWakeSave& Save,FVector2D XY,double Extra=0.);
    bool Refuge(const FETWakeSave& S,FVector2D XY);
    int NextHerd(const FETWakeSave& S);
    const TCHAR* Frequency(int HerdIndex);
    FString Objective(const FETWakeSave& S);
    FName Target(const FETWakeSave& S);
    FString Destination(const FETWakeSave& S);
    enum EClue { CaseFold, Hooves, FernFold, DashFold, Carrier, PawSign, LiftFold, ClueCount };
    struct FClue {const TCHAR* Title; const TCHAR* Cue;};
    const FClue& Clue(int Index);
    FVector2D Waypoint(const AETWorld& World);
    bool BugPack(const AETCharacter& P);
    bool CanBuild(const FETWakeSave& S,int Piece);
    void Trial(AETWorld& World,AETCharacter& P);
}
