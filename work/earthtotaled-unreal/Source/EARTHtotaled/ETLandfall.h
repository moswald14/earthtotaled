#pragma once
#include "CoreMinimal.h"
struct FETWeapon;
class AETWorld; class AETCharacter;

namespace ETLandfall {
    void DriveTest(AETWorld& World,AETCharacter& Player);
    inline constexpr int Pistol=6;
    inline constexpr const TCHAR* Version=TEXT("v1.16.0");
    inline FVector2D Corner(int Side) {
        static const FVector2D Corners[]={{-1080,-2280},{1080,-2280},{1080,1320},{-1080,1320}};
        return Corners[Side%4];
    }
    inline const TCHAR* Bank(int Side) {
        static const TCHAR* Names[]={TEXT("south"),TEXT("east"),TEXT("north"),TEXT("west")};
        return Names[Side%4];
    }
    inline FVector2D Stake(int Side) {return (Corner(Side)+Corner(Side+1))*.5;}
    inline int NextBank(int Mask) {for(int I=0;I<4;++I) if(!(Mask&(1<<I))) return I;return INDEX_NONE;}
    inline int RollDrop(FRandomStream& Random,bool Hostile) {
        if(!Hostile||Random.RandRange(1,400)!=1) return 0;
        return Random.RandRange(1,MAX_int32-1);
    }
    inline int RelicSeed(const FString& Kind) {
        if(!Kind.StartsWith(TEXT("relic."))) return 0;
        const FString Digits=Kind.Mid(6);
        int32 N=0;
        return LexTryParseString(N,*Digits)&&N>0&&FString::FromInt(N)==Digits?N:0;
    }
    struct FRoll {
        int Frame, Path, First, Second, Pace;
        float Power, Cycle, Accuracy;
        FString Name;
        explicit FRoll(int Seed);
        float Next(int Rank) const;
        int Rank(float Damage) const;
        FETWeapon Build(const FETWeapon& FrameGun,int Rank) const;
        FString Details(int Rank) const;
    };
}
