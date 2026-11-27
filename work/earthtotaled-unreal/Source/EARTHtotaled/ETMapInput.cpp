#include "ETGame.h"
#include "ETRing.h"
#include "Engine/GameViewportClient.h"
#include "Engine/World.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SNullWidget.h"

namespace
{
    class SETMapInput final : public SCompoundWidget
    {
    public:
        SLATE_BEGIN_ARGS(SETMapInput){} SLATE_ARGUMENT(AETCharacter*,Player) SLATE_END_ARGS()
        void Construct(const FArguments& Args)
        {
            Player=Args._Player; SetVisibility(EVisibility::Visible); ChildSlot[SNullWidget::NullWidget];
            RegisterActiveTimer(0,FWidgetActiveTimerDelegate::CreateLambda([this](double,float) {
                if(Player.IsValid()&&Player->Panel==TEXT("map"))
                    FSlateApplication::Get().SetKeyboardFocus(AsShared(),EFocusCause::SetDirectly);
                return EActiveTimerReturnType::Stop;
            }));
        }
        virtual bool SupportsKeyboardFocus() const override {return true;}
        virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E) override {
            if(!Player.IsValid()) return FReply::Unhandled();
            if(!E.IsRepeat()||(E.GetKey()==EKeys::Up||E.GetKey()==EKeys::Down||E.GetKey()==EKeys::Left||E.GetKey()==EKeys::Right))
                Player->MapCommand(E.GetKey(),Viewport());
            return FReply::Handled();
        }
        virtual FReply OnMouseButtonDown(const FGeometry& G,const FPointerEvent& E) override {
            if(!Player.IsValid()) return FReply::Unhandled();
            const auto Size=Viewport(),Pixel=Point(G,E,Size);
            const auto Chart=ETNav::Atlas(Size,Player->MapCenter,Player->MapSpan);
            if(E.GetEffectingButton()==EKeys::RightMouseButton&&Chart.Inside(Pixel)) Player->UseMapPoint(Pixel,Chart,true);
            if(E.GetEffectingButton()==EKeys::LeftMouseButton) {
                if(Chart.Inside(Pixel)) {
                    Down=Previous=Pixel; Dragging=true; Moved=false;
                    return FReply::Handled().CaptureMouse(AsShared());
                }
                const FKey Keys[]={EKeys::Subtract,EKeys::Add,EKeys::Home,EKeys::C,EKeys::Delete,EKeys::M};
                for(int I=0;I<6;++I) if(ETNav::AtlasButton(Chart,I).IsInsideOrOn(Pixel)) {Player->MapCommand(Keys[I],Size); break;}
            }
            return FReply::Handled();
        }
        virtual FReply OnMouseMove(const FGeometry& G,const FPointerEvent& E) override {
            if(Dragging&&Player.IsValid()) {
                const auto Size=Viewport(),Pixel=Point(G,E,Size);
                if(Moved||FVector2D::Distance(Down,Pixel)>3.) {
                    Moved=true; Player->PanMap(Pixel-Previous,ETNav::Atlas(Size,Player->MapCenter,Player->MapSpan)); Previous=Pixel;
                }
            }
            return FReply::Handled();
        }
        virtual FReply OnMouseButtonUp(const FGeometry& G,const FPointerEvent& E) override {
            if(E.GetEffectingButton()==EKeys::LeftMouseButton&&Dragging&&Player.IsValid()) {
                const auto Size=Viewport(),Pixel=Point(G,E,Size);
                const auto Chart=ETNav::Atlas(Size,Player->MapCenter,Player->MapSpan);
                if(!Moved&&Chart.Inside(Pixel)) Player->UseMapPoint(Pixel,Chart,false);
                Dragging=false; return FReply::Handled().ReleaseMouseCapture();
            }
            return FReply::Handled();
        }
        virtual void OnMouseCaptureLost(const FCaptureLostEvent& Event) override {Dragging=false; SCompoundWidget::OnMouseCaptureLost(Event);}
        virtual FReply OnMouseWheel(const FGeometry& G,const FPointerEvent& E) override {
            if(Player.IsValid()) {
                const auto Size=Viewport(),Pixel=Point(G,E,Size);
                const auto Chart=ETNav::Atlas(Size,Player->MapCenter,Player->MapSpan);
                if(Chart.Inside(Pixel)) Player->ZoomMap(E.GetWheelDelta(),Pixel,Chart);
            }
            return FReply::Handled();
        }
    private:
        TWeakObjectPtr<AETCharacter> Player;
        FVector2D Down=FVector2D::ZeroVector,Previous=FVector2D::ZeroVector;
        bool Dragging=false,Moved=false;
        FVector2D Viewport() const {
            FVector2D Size(1280,720);
            if(auto* P=Player.Get()) if(auto* V=P->GetWorld()->GetGameViewport()) V->GetViewportSize(Size);
            return Size;
        }
        static FVector2D Point(const FGeometry& G,const FPointerEvent& E,FVector2D Size) {
            const FVector2D Local=G.AbsoluteToLocal(E.GetScreenSpacePosition()),Bounds=G.GetLocalSize();
            return {Local.X*Size.X/FMath::Max(1.,Bounds.X),Local.Y*Size.Y/FMath::Max(1.,Bounds.Y)};
        }
    };
    constexpr double ChartLimit=1.99e9;
    FVector2D InWorld(FVector2D P) {return {FMath::Clamp(P.X,-ChartLimit,ChartLimit),FMath::Clamp(P.Y,-ChartLimit,ChartLimit)};}
}
void AETCharacter::ToggleMap()
{
    if(Panel==TEXT("map")) {ClosePanel(); return;}
    auto* View=GetWorld()->GetGameViewport();
    if(!Panel.IsEmpty()||!World||!View) return;
    MapCenter=FVector2D(GetActorLocation()); MapMessage.Empty();
    OpenPanel(TEXT("map"),TEXT("FIELD ATLAS"),TEXT(""));
    auto Widget=SNew(SETMapInput).Player(this);
    MapInput=Widget; View->AddViewportWidgetContent(Widget,510);
    if(auto* PC=Cast<APlayerController>(Controller)) {
        FInputModeUIOnly Input; Input.SetWidgetToFocus(Widget); Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Input);
    }
}
void AETCharacter::CloseMapInput()
{
    if(MapInput.IsValid()) if(auto* View=GetWorld()->GetGameViewport()) View->RemoveViewportWidgetContent(MapInput.ToSharedRef());
    MapInput.Reset();
}
void AETCharacter::EndPlay(const EEndPlayReason::Type Reason)
{
    if(MapInput.IsValid()) CloseMapInput();
    Super::EndPlay(Reason);
}
void AETCharacter::PanMap(FVector2D Pixels,const ETNav::FChart& Chart)
{
    MapCenter=InWorld(MapCenter-Pixels/Chart.Scale());
}
void AETCharacter::ZoomMap(double Steps,FVector2D Pixel,const ETNav::FChart& Chart)
{
    const FVector2D Fixed=Chart.WorldAt(Pixel);
    MapSpan=FMath::Clamp(Chart.Span*FMath::Pow(.76,FMath::Clamp(Steps,-20.,20.)),ETNav::MinSpan,ETNav::MaxSpan);
    MapCenter=InWorld(Fixed-(Fixed-Chart.Center)*(MapSpan/Chart.Span));
}
void AETCharacter::UseMapPoint(FVector2D Pixel,const ETNav::FChart& Chart,bool Place)
{
    if(!Chart.Inside(Pixel)) return;
    auto* S=World->State; int Found=INDEX_NONE;
    double Best=FMath::Clamp(Chart.Size.Y*.025,9.,19.);
    for(int I=0;I<S->MapPins.Num();++I) {
        const double D=FVector2D::Distance(Chart.Plot(S->MapPins[I]),Pixel);
        if(D<Best){Found=I;Best=D;}
    }
    const auto WorldPoint=Chart.WorldAt(Pixel); const auto SelectedCity=ETNav::District(WorldPoint);
    const auto Refuge=ETNav::Refuge(SelectedCity);
    const double RefugeDistance=FVector2D::Distance(Pixel,Chart.Plot(Refuge));
    bool AtRefuge=RefugeDistance<Best;
    TOptional<FVector2D> Landmark;
    double Nearest=FMath::Min(RefugeDistance,Best);
    auto Consider=[&](FVector2D At) {
        const double D=FVector2D::Distance(Pixel,Chart.Plot(At));
        if(D<Nearest) {Nearest=D;Landmark=At;AtRefuge=false;}
    };
    const FVector2D Reach(Best/Chart.Scale());
    ETNav::Sites(FBox2D(WorldPoint-Reach,WorldPoint+Reach),[&](const ETNav::FSite& S){
        Consider(S.Center);});
    for(int I=0;I<ETRing::Stops;++I) Consider(ETRing::Station(I));
    if(Found!=INDEX_NONE) S->TrackedPin=Found;
    else if(AtRefuge&&SelectedCity==ETNav::District(FVector2D(GetActorLocation()))) S->TrackedPin=INDEX_NONE;
    else if(!Place&&!AtRefuge&&!Landmark.IsSet()) return;
    else if(S->MapPins.Num()<ETNav::MaxPins) S->TrackedPin=S->MapPins.Add(InWorld(AtRefuge?Refuge:Landmark.Get(WorldPoint)));
    else {MapMessage=TEXT("Pin list is full. Select a pin and press Delete to make room."); return;}
    MapMessage.Empty(); World->Save();
}
void AETCharacter::MapCommand(FKey Key,FVector2D Viewport)
{
    if(Panel!=TEXT("map")) return;
    const auto Chart=ETNav::Atlas(Viewport,MapCenter,MapSpan);
    const FVector2D Here(GetActorLocation()); auto* S=World->State;
    if(Key==EKeys::M||Key==EKeys::Escape) {ClosePanel(); return;}
    if(Key==EKeys::Home) MapCenter=Here;
    else if(Key==EKeys::R) {
        MapCenter=ETRing::Center;
        MapSpan=FMath::Min(ETNav::MaxSpan,2.22*ETRing::Radius*FMath::Max(1.,Chart.Size.Y/Chart.Size.X));
    }
    else if(Key==EKeys::C) {
        const auto City=ETNav::Refuge(ETNav::District(Here)),Gap=(Here-City).GetAbs();
        MapCenter=(City+Here)*.5; MapSpan=FMath::Clamp(FMath::Max3(24000.,Gap.Y*1.45,Gap.X*1.45*Chart.Size.Y/Chart.Size.X),ETNav::MinSpan,ETNav::MaxSpan);
        S->TrackedPin=INDEX_NONE; World->Save();
    } else if(Key==EKeys::Delete&&S->MapPins.IsValidIndex(S->TrackedPin)) {
        S->MapPins.RemoveAt(S->TrackedPin); S->TrackedPin=INDEX_NONE; World->Save();
    } else if(Key==EKeys::Add||Key==EKeys::Equals||Key==EKeys::Subtract||Key==EKeys::Hyphen) {
        ZoomMap(Key==EKeys::Add||Key==EKeys::Equals?1:-1,Chart.Position+Chart.Size*.5,Chart);
    } else if(Key==EKeys::Left||Key==EKeys::Right||Key==EKeys::Up||Key==EKeys::Down) {
        const FVector2D Step(Key==EKeys::Left?-1.:Key==EKeys::Right?1.:0.,Key==EKeys::Up?-1.:Key==EKeys::Down?1.:0.);
        MapCenter=InWorld(MapCenter+Step*MapSpan*.12);
    } else if(Key==EKeys::F11) Fullscreen();
    MapMessage.Empty();
}
