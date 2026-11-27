#include "ETWake.h"
#include "ETGame.h"
#include "ETWakeVehicle.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "HAL/PlatformTime.h"

using namespace ETWake;
namespace {
    const FLinearColor Bark(.025,.047,.040,.975),Leaf(.57,.89,.66),Cream(.96,.94,.83),Signal(.99,.69,.37),Mute(.64,.72,.65);
}
void AETHUD::DrawWake(AETCharacter* P) {
    const auto& S=P->World->State->Wake;const float U=FMath::Min(Canvas->SizeX/1080.f,Canvas->SizeY/670.f);
    const float W=980*U,H=588*U,X=(Canvas->SizeX-W)*.5f,Y=(Canvas->SizeY-H)*.5f;
    const bool Shaking=S.Chapter==Vault&&P->WakePanelNode=="vault";
    const double Now=FPlatformTime::Seconds();const float Tremor=Shaking?U*1.5f*FMath::Sin(Now*24.f):0;
    DrawRect(FLinearColor(0,.01,.008,.56),0,0,Canvas->SizeX,Canvas->SizeY);DrawRect(Bark,X+Tremor,Y,W,H);
    DrawRect(Shaking?Signal:Leaf,X+Tremor,Y,W,3*U);
    Text(TEXT("SHELTER 09   /   MEGAWAKE FIELD CHANNEL"),X+32*U,Y+23*U,Leaf,.93f*U);
    Text(P->PanelTitle,X+31*U,Y+55*U,Cream,1.8f*U);
    const int Winner=S.Winner();
    FString Scores=FString::Printf(TEXT("BUG JOBS   %d       SUPERTECH JOBS   %d       HERDS   %d/9"),S.Count(Bug),S.Count(Frame),S.Restored());
    if(Winner)Scores+=Winner==Bug?TEXT("       SUPERTECH PATROLS HOSTILE"):TEXT("       BUG PATROLS HOSTILE");
    else if(S.Count(Bug)==3||S.Count(Frame)==3)Scores+=TEXT("       FOURTH JOB CHANGES ALLEGIANCE");
    else if(S.Chapter>=Surveys)Scores+=TEXT("       HOSTILITY AT 4 JOBS");
    Text(Scores,X+32*U,Y+108*U,Winner||S.WarningSeen?Signal:Mute,1.02f*U);
    DrawRect(Leaf.CopyWithNewOpacity(.3f),X+32*U,Y+143*U,W-64*U,U);
    const auto Options=P->WakeChoices();const float Col=440*U;
    const float BodySpace=(H/U-52-(FMath::DivideAndRoundUp(Options.Num(),2)-1)*46-175)*U;
    auto Layout=[&](float Font,bool Paint){
        float Top=0;TArray<FString> Paragraphs;P->PanelBody.ParseIntoArray(Paragraphs,TEXT("\n"),false);
        for(const FString& Para:Paragraphs){
            if(Para.IsEmpty()){Top+=10*Font;continue;}
            TArray<FString> Words;Para.ParseIntoArrayWS(Words);FString Line;
            auto Flush=[&](){if(Paint)Text(Line,X+32*U,Y+164*U+Top,Cream,Font);Top+=23*Font;Line.Empty();};
            for(const FString& Word:Words){
                auto Candidate=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;float TW,TH;
                GetTextSize(Candidate,TW,TH,GEngine->GetSmallFont(),Font);
                if(TW>W-70*U&&!Line.IsEmpty()){Flush();Line=Word;}else Line=Candidate;}
            if(!Line.IsEmpty())Flush();
        }
        return Top;
    };
    float BodyFont=1.24f*U;while(Layout(BodyFont,false)>BodySpace&&BodyFont>U)BodyFont-=.06f*U;Layout(BodyFont,true);
    for(int I=0;I<Options.Num();++I) {
        const float L=X+(32+(I%2)*464)*U,T=Y+(H/U-52-(FMath::DivideAndRoundUp(Options.Num(),2)-1-I/2)*46)*U;
        DrawRect(I==Options.Num()-1?FLinearColor(.14,.21,.17):FLinearColor(.24,.33,.24),L,T,Col,38*U);
        DrawRect(I==Options.Num()-1?Mute:Leaf,L,T,3*U,38*U);
        Text(Options[I].Value,L+13*U,T+7*U,Cream,1.1f*U);
        AddHitBox({L,T},{Col,38*U},FName(*Options[I].Key),true);
    }
}
void AETHUD::DrawWakeTrail(AETCharacter* P) {
    auto& W=*P->World;const auto& S=W.State->Wake;const float U=FMath::Clamp(FMath::Min(Canvas->SizeX/1300.f,Canvas->SizeY/800.f),.7f,1.4f);
    if(W.WakeFlash>0)DrawRect(FLinearColor(.85,1,.75,FMath::Min(.56f,W.WakeFlash*.17f)),0,0,Canvas->SizeX,Canvas->SizeY);
    if(S.Chapter==Missing&&W.State->ArrivalStage==1){Text(TEXT("J   Kit's field journal"),32,136,Mute,.92f);return;}
    auto XY=Waypoint(W);auto Delta=XY-FVector2D(P->GetActorLocation());double Range=Delta.Size();
    const auto Angle=FMath::FindDeltaAngleDegrees(double(P->GetControlRotation().Yaw),FMath::RadiansToDegrees(FMath::Atan2(Delta.Y,Delta.X)));
    FString Guide=FString::Printf(TEXT("%s   %.0f m   %s     /     J  journal   M  atlas"),*Destination(S),Range/100,FMath::Abs(Angle)<10?TEXT("ahead"):Angle>0?TEXT("turn right"):TEXT("turn left"));
    Text(Guide,32,133,Signal,.91f);
    if(S.Chapter==Vault&&Range<1500){
        const double T=W.State->Elapsed;DrawRect(Signal.CopyWithNewOpacity(.10f+.08f*FMath::Sin(T*9)),0,0,Canvas->SizeX,4*U);}
    if(Range>300||Target(S)=="vault"){
        FVector Pos(XY,W.SurfaceHeight(XY.X,XY.Y)+195);const auto HUD=Project(Pos);
        if(FMath::Abs(Angle)<72&&HUD.Z>0&&HUD.X>28&&HUD.X<Canvas->SizeX-70&&HUD.Y>150&&HUD.Y<Canvas->SizeY-150){
            float X=HUD.X,Y=HUD.Y,R=7*U;DrawLine(X,Y-R,X+R,Y,Signal,2*U);DrawLine(X+R,Y,X,Y+R,Signal,2*U);
            DrawLine(X,Y+R,X-R,Y,Signal,2*U);DrawLine(X-R,Y,X,Y-R,Signal,2*U);}
    }
    FString Help;
    if(auto* V=P->Ride()) {
        if(V->Type==Car)Help=TEXT("R engine   W accelerate   A/D steer   S brake   Space handbrake   E exit");
        else if(S.Chapter==Flight&&S.Aircraft.Flying&&Range<30000)
            Help=TEXT("Crown approach   Align with the strip   S to slow early   Ctrl to descend   Land below 68 km/h");
        else Help=TEXT("R engine   W throttle   A/D turn   Space climb   Ctrl descend   S brake   E exit on ground");
    }else if(!IsValid(P->Focus)){
        for(auto* Parked:{W.WakeCar,W.WakePlane})if(IsValid(Parked)&&Parked->Boardable(*P)){Help=Parked->Type==Car?TEXT("E   Enter roadster"):TEXT("E   Board survey plane");break;}
        if(Help.IsEmpty())if(auto* N=P->NearbyWake())Help=TEXT("E   ")+N->Label;
    }
    if(!Help.IsEmpty()){
        float TW,TH;GetTextSize(Help,TW,TH,GEngine->GetSmallFont(),U);const float X=(Canvas->SizeX-TW)*.5f,Y=Canvas->SizeY*.73f;
        DrawRect(Bark.CopyWithNewOpacity(.88f),X-12*U,Y-4*U,TW+24*U,TH+9*U);Text(Help,X,Y,Cream,U);
    }
}
