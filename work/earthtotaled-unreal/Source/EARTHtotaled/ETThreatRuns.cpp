#include "ETField.h"
#include "ETWake.h"

const TCHAR* ETField::RunName(int Type){
    static const TCHAR* const Names[]={TEXT("No contract"),TEXT("Stray drone patrol"),TEXT("Rogue machine column"),TEXT("Feral hive")};
    return Names[FMath::Clamp(Type,0,3)];
}
void AETWorld::InitFieldSites(){
    FVector At=WakeSpot(TEXT("dispatch"))+FVector(-460,190,0);At.Z=SurfaceHeight(At.X,At.Y)+6;
    Item(TEXT("field/east-dispatch"),TEXT("runboard"),TEXT("East Dispatch / optional Threat Runs"),At);
    At=FVector(-4400,-4400,0);At.Z=SurfaceHeight(At.X,At.Y)+6;
    Item(TEXT("field/allotment"),TEXT("allotment"),TEXT("Shelter 09 / southwest allotment guide"),At);
}
void AETCharacter::ThreatPanel(){
    const auto& R=World->State->Run;
    FString Info=TEXT("Independent bounties. Patrols emerge after 19:00 and return the next night. Both factions post these jobs. Wildlife and herd work are outside the contracts.");
    if(R.Type){
        const auto Delta=R.Rally-GetActorLocation();
        Info+=FString::Printf(TEXT("\n\n%s / %d marked targets left. Range %.0f m, bearing %03d. Payment arrives on clearance."),
            ETField::RunName(R.Type),R.Remaining.Num(),Delta.Size2D()/100.,FMath::RoundToInt(FMath::Fmod(Delta.Rotation().Yaw+360.f,360.f)));
    }else Info+=TEXT("\n\nDrones: 70 XP + 6 scrap. Machines: 120 XP + 12 scrap. Hive: 185 XP + 18 scrap. Accept any run again after clearance.");
    OpenPanel(TEXT("threatruns"),TEXT("DISPATCH / THREAT RUNS"),Info);
}
bool AETCharacter::StartThreatRun(int Type){
    auto& S=*World->State;auto& Run=S.Run;
    if(Run.Type||Type<1||Type>3)return false;
    FRandomStream Random(S.Seed^GetTypeHash(Run.Serial+1));
    FVector Rally{};
    bool Found=false;
    for(int K=0;K<100;++K){
        const auto Offset=FRotator(0,Random.FRandRange(0,360),0).Vector()*Random.FRandRange(11000.f,49000.f);
        Rally=GetActorLocation()+Offset;Rally.Z=World->SurfaceHeight(Rally.X,Rally.Y);
        if(World->InCity(Rally)||World->SafeSettlement(Rally)||World->SettledCity(Rally)||ETWake::Reserved(S.Wake,FVector2D(Rally),1600))continue;
        bool Clear=true;
        for(int I=0;I<4;++I){
            const FVector V=Rally+FVector((I-1)*250,(I%2)*340,0);
            Clear&=!World->SettledCity(V)&&!World->SafeSettlement(V);
        }
        if(Clear){Found=true;break;}
    }
    if(!Found)return false;
    ++Run.Serial;Run.Type=Type;Run.Rally=Rally;
    const int Count=Type==2?2:3;
    for(int I=0;I<Count;++I){
        const FString Id=FString::Printf(TEXT("run/%d/%d"),Run.Serial,I);
        auto& T=S.Threats.Add(Id);T.Kind=Type==1?TEXT("drone"):Type==2?TEXT("robot"):I==0?TEXT("hive"):TEXT("runner");
        T.Position=Rally+FVector((I-1)*250,(I%2)*340,0);
        T.Position.Z=World->SurfaceHeight(T.Position.X,T.Position.Y)+(Type==1?180:12);
        T.Health=Type==1?30:Type==2?120:I==0?480:52;Run.Remaining.Add(Id);
    }
    World->Save();return true;
}
void AETCharacter::RecordHostileKill(AETAgent& Victim,bool Credit){
    auto& S=*World->State;
    if(Credit&&Victim.Kind!=TEXT("deer")&&!Victim.WakeTruce()){
        S.Streak=(S.Elapsed>=S.LastHostileKill&&S.Elapsed-S.LastHostileKill<=8.f)?FMath::Min(S.Streak+1,1000000):1;
        S.LastHostileKill=S.Elapsed;S.Shield=FMath::Min(S.MaxShield(),S.Shield+2*S.Rank(TEXT("shieldharvest")));
        if(S.Streak==3||S.Streak==5||S.Streak==10){
            const int Bonus=S.Streak*4;AddXP(Bonus);S.Scrap+=S.Streak;
            World->Say(FString::Printf(TEXT("%d-kill streak / +%d XP and %d salvage"),S.Streak,Bonus,S.Streak));
        }
    }
    auto& R=S.Run;
    if(!R.Type||!R.Remaining.Remove(Victim.Id))return;
    if(R.Remaining.IsEmpty()){
        const int Type=R.Type,XP=Type==1?70:Type==2?120:185;
        R.Type=0;++R.Completed;S.Scrap+=Type*6;AddXP(XP);
        World->Say(FString::Printf(TEXT("Threat Run clear / +%d XP and %d salvage / Dispatch is open"),XP,Type*6));
    }
    World->Save();
}
