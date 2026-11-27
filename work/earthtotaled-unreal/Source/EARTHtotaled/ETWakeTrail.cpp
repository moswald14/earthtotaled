#include "ETGame.h"
#include "ETWake.h"

using namespace ETWake;

const FClue& ETWake::Clue(int Index)
{
    static const FClue Log[]={
        {TEXT("The square under the red case"),TEXT("waketrailcase")},
        {TEXT("Hooves on untouched leaves"),TEXT("waketrailhooves")},
        {TEXT("Oil on the folded foil"),TEXT("waketrailfern")},
        {TEXT("Inside the roadster"),TEXT("waketraildash")},
        {TEXT("The Hollow Field carrier"),TEXT("waketrailrelay")},
        {TEXT("Five toes at Hollow Field"),TEXT("waketrailbear")},
        {TEXT("The custody lift grille"),TEXT("waketraillift")}};
    static_assert(UE_ARRAY_COUNT(Log)==ClueCount);
    return Log[Index];
}

FString ETWake::Destination(const FETWakeSave& S)
{
    if(S.TaskSide)return FString(Herd(S.TaskHerd).Name)+(S.ReadSource?TEXT(" glade"):TEXT(" relay"));
    const FName Key=Target(S);
    static const TMap<FName,FString> Labels={{"thrum",TEXT("Thrum / Shelter 09")},{"band",TEXT("Red medical case")},
        {"herd",TEXT("Deer crossing")},{"glade",TEXT("Quiet glade")},{"garage",TEXT("Roadside motor pool")},
        {"car",TEXT("Roadster")},{"roadgate",TEXT("Amber road gate")},{"dispatch",TEXT("East Dispatch")},
        {"runway",TEXT("Hollow Field runway")},{"hangar",TEXT("Hollow Field hangar")},{"plane",TEXT("Survey plane")},
        {"land",TEXT("Crown Field")},{"vault",TEXT("MegaWAKE vault")},{"gate",TEXT("Freight gate")},
        {"burrow",TEXT("Root seam")},{"cell",TEXT("Kit's cell")},{"evac",TEXT("Evacuation lift")},
        {"strike",TEXT("Sunseed console")},{"home",TEXT("Kit / Shelter 09")}};
    return Labels.FindRef(Key);
}

void AETWorld::FindTrailClue(int Index)
{
    State->Wake.TrailFound|=1<<Index;
}

void AETWorld::PlayTrailClue()
{
    auto& S=State->Wake;
    const int Pending=S.TrailFound&~S.TrailHeard;
    if(!Pending)return;
    // Keep old field notes in the journal if the survivor has already left that part of the search.
    const int LastChapter[]={FirstHerd,Garage,Driving,Dispatch,Surveys,Flight,Cell};
    for(int I=0;I<ClueCount;++I)if(Pending&(1<<I)){
        S.TrailHeard|=1<<I;
        if(S.Chapter<=LastChapter[I]&&Speak(Clue(I).Cue))break;
    }
    Save();
}

void AETCharacter::ReadTrail(int Index)
{
    auto& S=World->State->Wake;if(Index<0||Index>=ClueCount||!(S.TrailFound&(1<<Index)))return;
    const auto& Entry=Clue(Index);const auto* Line=World->VoiceLines.Find(Entry.Cue);if(!Line)return;
    TrailPage=Index;WakePanelNode="trail";S.TrailHeard|=1<<Index;
    const FString Transcript=Line->Speaker+TEXT("\n")+Line->Text+TEXT("\n\nCurrent lead / ")+Objective(S);
    WakeMessage(Entry.Title,Transcript,Entry.Cue);
}
