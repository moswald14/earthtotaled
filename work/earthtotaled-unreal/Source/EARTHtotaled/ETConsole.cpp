#include "ETGame.h"
#include "ETGeology.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

namespace
{
    class SETConsole : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SETConsole) {} SLATE_ARGUMENT(AETCharacter*,Player) SLATE_END_ARGS()
        void Construct(const FArguments& Args)
        {
            Player=Args._Player;
            ChildSlot.HAlign(HAlign_Fill).VAlign(VAlign_Top).Padding(28)
            [SNew(SBorder).Padding(16).BorderBackgroundColor(FLinearColor(.025,.047,.048,1))
                [SNew(SVerticalBox)
                 +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Text(FText::FromString(TEXT("EARTHtotaled  /  field console       F1 or Esc to close")))]
                 +SVerticalBox::Slot().AutoHeight().Padding(0,8)[SNew(STextBlock).AutoWrapText(true).Text_Lambda([this](){
                     FString Lines;
                     if(auto* P=Player.Get()) for(int I=FMath::Max(0,P->ConsoleLines.Num()-6);I<P->ConsoleLines.Num();++I) Lines+=P->ConsoleLines[I]+TEXT("\n");
                     return FText::FromString(Lines);
                 })]
                 +SVerticalBox::Slot().AutoHeight()[SAssignNew(Input,SEditableTextBox).ClearKeyboardFocusOnCommit(false)
                    .HintText(FText::FromString(TEXT("Type help, then press Enter"))).OnTextCommitted(this,&SETConsole::Submit)]
                ]];
        }
        TSharedRef<SWidget> Entry() const {return Input.ToSharedRef();}
        void FocusInput() {FSlateApplication::Get().SetAllUserFocus(Input,EFocusCause::SetDirectly);}
        void FocusWhenMounted()
        {
            RegisterActiveTimer(0,FWidgetActiveTimerDelegate::CreateSP(this,&SETConsole::AcquireFocus));
        }
        EActiveTimerReturnType AcquireFocus(double,float)
        {
            if(!Player.IsValid()||Player->Panel!=TEXT("console")) return EActiveTimerReturnType::Stop;
            FocusInput();
            return Input->HasKeyboardFocus()||Input->HasFocusedDescendants()?EActiveTimerReturnType::Stop:EActiveTimerReturnType::Continue;
        }
        FReply OnPreviewKeyDown(const FGeometry& Geometry,const FKeyEvent& Key) override
        {
            const auto K=Key.GetKey();
            if(K==EKeys::Escape||K==EKeys::F1) {if(auto* P=Player.Get()) P->CloseConsole(); return FReply::Handled();}
            if((K==EKeys::Up||K==EKeys::Down)&&History.Num()) {
                Cursor=FMath::Clamp(Cursor+(K==EKeys::Up?-1:1),0,History.Num());
                Input->SetText(FText::FromString(History.IsValidIndex(Cursor)?History[Cursor]:TEXT(""))); return FReply::Handled();
            }
            return SCompoundWidget::OnPreviewKeyDown(Geometry,Key);
        }
    private:
        void Submit(const FText& Text,ETextCommit::Type How)
        {
            if(How!=ETextCommit::OnEnter) return;
            const FString Line=Text.ToString().Left(180).TrimStartAndEnd();
            if(auto* P=Player.Get();P&&!Line.IsEmpty()) {
                History.Add(Line); if(History.Num()>32) History.RemoveAt(0); Cursor=History.Num();
                const FString Reply=P->Command(Line);
                P->ConsoleLines.Add(TEXT("> ")+Line); P->ConsoleLines.Add(Reply);
                if(P->ConsoleLines.Num()>64) P->ConsoleLines.RemoveAt(0,2);
                const FString Record=Line+TEXT(" -> ")+Reply+TEXT("\n");
                FFileHelper::SaveStringToFile(Record,*(FPaths::ProjectSavedDir()/TEXT("field-console.log")),FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM,&IFileManager::Get(),FILEWRITE_Append);
                Input->SetText(FText::GetEmpty()); FocusInput();
            }
        }
        TWeakObjectPtr<AETCharacter> Player; TSharedPtr<SEditableTextBox> Input;
        TArray<FString> History; int32 Cursor=0;
    };
}
void AETCharacter::OpenConsole()
{
    if(Panel==TEXT("console")) {CloseConsole(); return;}
    if(!World||!Panel.IsEmpty()||!GEngine||!GEngine->GameViewport) return;
    OpenPanel(TEXT("console"),TEXT("FIELD CONSOLE"),TEXT(""));
    auto Widget=SNew(SETConsole).Player(this);
    ConsoleWidget=Widget; GEngine->GameViewport->AddViewportWidgetContent(Widget,550);
    if(auto* PC=Cast<APlayerController>(Controller)) {
        FInputModeUIOnly Mode; Mode.SetWidgetToFocus(Widget->Entry()); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode);
    }
    Widget->FocusInput(); Widget->FocusWhenMounted();
}
void AETCharacter::CloseConsole()
{
    if(ConsoleWidget.IsValid()&&GEngine&&GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(ConsoleWidget.ToSharedRef());
    ConsoleWidget.Reset(); ClosePanel();
}
FString AETCharacter::Command(const FString& Line)
{
    TArray<FString> Args; Line.ToLower().ParseIntoArrayWS(Args);
    if(!Args.Num()) return TEXT("Type help.");
    const FString& Op=Args[0]; auto* S=World->State;
    if(Op==TEXT("help")) return TEXT("status | pos | base | war | heal | give wood/stone/iron/gold/water 1..500 | time day/night | spawn bug/supertech/spitter/bulwark/skimmer | raid start/end | weather auto/clear/rain/storm | save");
    if(Op==TEXT("time")&&Args.Num()==2&&(Args[1]==TEXT("day")||Args[1]==TEXT("night"))) {
        const float Target=Args[1]==TEXT("day")?6.f:19.f;
        S->Elapsed+=FMath::Fmod(Target-World->Hour()+24.f,24.f)*120.f;
        World->UpdateDaylight(); World->UpdateNightWatch(GetActorLocation());
        return World->IsNight()?TEXT("Nightfall at 19:00."):TEXT("Daybreak at 06:00.");
    }
    if(Op==TEXT("status")) return FString::Printf(TEXT("Health %.0f  armor %.0f/%.0f (tier %d)  hydration %.1f%%  bottles %d  iron %d  gold ore %d  slot %02d"),S->Health,S->Armor,S->ArmorLimit(),S->ArmorTier,S->Hydration,S->Water,S->Inventory.FindRef(TEXT("ironore")),S->Inventory.FindRef(TEXT("goldore")),World->ActiveSlot);
    const FVector P=GetActorLocation();
    if(Op==TEXT("war")) return FString::Printf(TEXT("%d highway fighters in range / Bug hits %d / Supertech hits %d"),World->FrontUnits.Num(),World->BugHits,World->SupertechHits);
    if(Op==TEXT("pos")) return FString::Printf(TEXT("Seed %d  X %.0f m  Y %.0f m  depth %.1f m  deepest %.1f m  %d active regions"),S->Seed,P.X/100,P.Y/100,FMath::Max(0.,(World->SurfaceHeight(P.X,P.Y)-P.Z)/100),S->MaxDepth,World->Regions.Num());
    const FGuid Core=World->NearestCore(P,10500);
    if(Op==TEXT("base")) return FString::Printf(TEXT("Local value %d  raid stage %d  raids held %d  enemy tier %d  next muster %.0f s"),World->BaseValue(Core),S->RaidStage,S->Waves,World->ThreatTier(),S->RaidWait);
    if(Op==TEXT("heal")) {S->Health=MaxHealth(); S->Armor=S->ArmorLimit(); S->Hydration=100; return TEXT("Restored health, suit and hydration.");}
    if(Op==TEXT("save")) return World->Save()?TEXT("Survivor saved."):TEXT("Save unavailable in this session.");
    if(Op==TEXT("give")&&Args.Num()==3) {
        int32 N=0;
        if(!LexTryParseString(N,*Args[2])||N<1||N>500) return TEXT("Choose an amount from 1 to 500.");
        if(Args[1]==TEXT("water")) S->Water+=N;
        else if(Args[1]==TEXT("iron")||Args[1]==TEXT("gold")) S->Inventory.FindOrAdd(Args[1]+TEXT("ore"))+=N;
        else if(Args[1]==TEXT("wood")||Args[1]==TEXT("stone")) S->Inventory.FindOrAdd(Args[1])+=N;
        else return TEXT("Use wood, stone, iron, gold or water.");
        return TEXT("Added to survivor pack.");
    }
    if(Op==TEXT("raid")&&Args.Num()==2) {
        if(Args[1]==TEXT("start")&&!World->IsNight()) return TEXT("Raids start at night. Use time night to advance.");
        if(Args[1]==TEXT("start")) return World->Muster(Core)?TEXT("Raid mustering."):TEXT("Need a nearby relay, open approach, and no active raid.");
        if(Args[1]==TEXT("end")&&S->RaidStage) {World->EndRaid(false); return TEXT("Raid stood down.");}
    }
    if(Op==TEXT("weather")&&Args.Num()==2) {
        const TArray<FString> Names={TEXT("auto"),TEXT("clear"),TEXT("rain"),TEXT("storm")};
        const int32 I=Names.IndexOfByKey(Args[1]);
        if(I==INDEX_NONE) return TEXT("Use auto, clear, rain or storm.");
        World->WeatherOverride=I==0?-1:I==1?ETClimate::Clear:I==2?ETClimate::Rain:ETClimate::Storm; return TEXT("Weather override updated.");
    }
    if(Op==TEXT("spawn")&&Args.Num()==2) {
        if(!World->IsNight()) return TEXT("Hostiles emerge at night. Use time night to advance.");
        const TSet<FString> Kinds={TEXT("runner"),TEXT("spitter"),TEXT("bulwark"),TEXT("skimmer"),TEXT("robot")};
        const FString Kind=Args[1]==TEXT("supertech")?TEXT("robot"):Args[1]==TEXT("bug")?TEXT("runner"):Args[1];
        if(!Kinds.Contains(Kind)) return TEXT("Unknown hostile type.");
        FVector Site=P+GetActorForwardVector()*900; Site.Z=World->SurfaceHeight(Site.X,Site.Y)+(Args[1]==TEXT("skimmer")?310:8);
        return World->AddThreat(TEXT("patrol-admin-")+FGuid::NewGuid().ToString(),Kind,Site)?TEXT("Spawned nearby."):TEXT("Spawn failed.");
    }
    return TEXT("Unknown command or arguments. Type help.");
}

