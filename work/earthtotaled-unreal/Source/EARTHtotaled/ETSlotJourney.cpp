#include "ETGame.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"

void AETWorld::TickSlotJourney(AETCharacter* P)
{
    static int32 Step=0,Checks=0,Failures=0;
    static bool Done=false;
    static double ConsoleOpened=0;
    static FString Report;
    static const bool Cold=FParse::Param(FCommandLine::Get(),TEXT("ETSlotCold"));
    static const double Start=FPlatformTime::Seconds();
    if(Done) return;
    const FString File=FPaths::ProjectSavedDir()/(Cold?TEXT("outposts-reopen.txt"):TEXT("outposts-play.txt"));
    auto Check=[&](bool OK,const FString& Name){++Checks; if(!OK) ++Failures; Report+=(OK?TEXT("PASS "):TEXT("FAIL "))+Name+TEXT("\n"); FFileHelper::SaveStringToFile(Report,*File);};
    auto Finish=[&](){Done=true; FFileHelper::SaveStringToFile(FString::Printf(TEXT("ET_OUTPOST %d checks / %d failures\n"),Checks,Failures)+Report,*File); FPlatformMisc::RequestExitWithStatus(true,Failures?1:0);};
    if(FPlatformTime::Seconds()-Start>200) {Check(false,FString::Printf(TEXT("Journey timed out at %d"),Step)); Finish(); return;}
    auto* PC=Cast<APlayerController>(P->Controller); if(!PC) return;
    if(Step==0) {
        if(P->Panel!=TEXT("title")) return;
        if(Cold) {
            Check(HadSave&&ActiveSlot==10&&State->Gold==913&&State->Inventory.FindRef(TEXT("ironore"))==17,TEXT("Fresh process remembers selected imported survivor"));
            P->Action(TEXT("continue")); Step=5; return;
        }
        Check(!HadSave&&ActiveSlot==1,TEXT("Fresh isolated profile starts at slot one"));
        P->Action(TEXT("slots")); Check(P->SlotLabels.Num()==10&&P->Panel==TEXT("slots"),TEXT("Title opens ten labelled slots"));
        P->Action(TEXT("pick-3")); P->Action(TEXT("slot-new")); Step=1; return;
    }
    if(Step==1) {
        if(P->Panel!=TEXT("story")||ActiveSlot!=3) return;
        P->Action(TEXT("resume")); State->Gold=913; State->Water=7;
        Check(State->Seed!=84371,TEXT("New slot generates an independent world seed"));
        P->OpenConsole(); Check(P->Panel==TEXT("console")&&P->ConsoleWidget.IsValid()&&PC->bShowMouseCursor,TEXT("Fullscreen console opens a focused text field"));
        ConsoleOpened=FPlatformTime::Seconds(); Step=2; return;
    }
    if(Step==2) {
        if(FPlatformTime::Seconds()-ConsoleOpened<.7) return;
        auto& Slate=FSlateApplication::Get(); const FModifierKeysState Mod;
        const FString Focus=Slate.GetKeyboardFocusedWidget()?Slate.GetKeyboardFocusedWidget()->GetTypeAsString():TEXT("none");
        for(TCHAR C:FString(TEXT("give iron 17"))) Slate.ProcessKeyCharEvent(FCharacterEvent(C,Mod,0u,false));
        auto Press=[&](FKey K){FKeyEvent Key(K,Mod,0u,false,0,0); Slate.ProcessKeyDownEvent(Key); Slate.ProcessKeyUpEvent(Key);};
        Press(EKeys::Enter);
        Check(State->Inventory.FindRef(TEXT("ironore"))==17,FString::Printf(TEXT("Typing a command and pressing Enter changes the live inventory / focus %s, iron %d"),*Focus,State->Inventory.FindRef(TEXT("ironore"))));
        Press(EKeys::Escape);
        Check(P->Panel.IsEmpty()&&!PC->bShowMouseCursor&&!PC->IsLookInputIgnored()&&GetWorld()->GetGameViewport()->GetMouseCaptureMode()==EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown,TEXT("Escape dismisses console and recaptures mouse without a held button"));
        if(!P->Panel.IsEmpty()) P->CloseConsole();
        P->Pause(); P->Action(TEXT("slots")); P->Action(TEXT("pick-9")); P->Action(TEXT("slot-copy"));
        Check(UGameplayStatics::DoesSaveGameExist(SlotName(9),0)&&ActiveSlot==3,TEXT("Pause can save a copy to another slot while keeping current survivor"));
        Step=3; return;
    }
    if(Step==3) {
        P->Action(TEXT("slot-export"));
        Check(P->SlotMessage.StartsWith(TEXT("Exported to")),TEXT("Windows Save As dialog exports selected survivor / ")+P->Panel+TEXT(" / ")+P->SlotMessage);
        P->Action(TEXT("pick-10")); P->Action(TEXT("slot-import"));
        Check(UGameplayStatics::DoesSaveGameExist(SlotName(10),0),TEXT("Windows Open dialog imports file into an empty slot / ")+P->Panel+TEXT(" / ")+P->SlotMessage);
        if(Failures) {Finish(); return;}
        P->Action(TEXT("slot-play")); Step=4; return;
    }
    if(Step==4) {
        if(P->Panel==TEXT("travel")||ActiveSlot!=10) return;
        Check(State->Gold==913&&State->Water==7&&State->Inventory.FindRef(TEXT("ironore"))==17,TEXT("Playing the imported slot restores its exact supplies"));
        P->Pause(); P->Action(TEXT("slots")); P->Action(TEXT("pick-3")); P->Action(TEXT("slot-new"));
        Check(!P->PendingSlotAction.IsEmpty(),TEXT("An occupied slot requires explicit replacement"));
        P->Action(TEXT("slot-cancel")); P->Action(TEXT("slot-back"));
        const auto* Original=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SlotName(3),0));
        Check(Original&&Original->Gold==913&&P->Panel==TEXT("pause"),TEXT("Cancelling keeps the old survivor and returns to the pause menu"));
        Finish(); return;
    }
    if(Step==5&&P->Panel.IsEmpty()) {
        Check(!PC->bShowMouseCursor&&ActiveSlot==10&&State->Water==7,TEXT("Cold-loaded survivor resumes fullscreen gameplay with mouse capture")); Finish();
    }
}

