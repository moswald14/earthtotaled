#include "ETGame.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AETHUD::DrawSkillWeb(AETCharacter* P)
{
    const auto& Perks=P->World->Perks;
    const auto& Save=*P->World->State;
    const float U=FMath::Min(Canvas->SizeX/1120.f,Canvas->SizeY/690.f);
    const float X=(Canvas->SizeX-1060*U)*.5f,Y=(Canvas->SizeY-635*U)*.5f;
    const FLinearColor Ink(.025,.047,.042),Paper(.96,.94,.83),Mint(.62,.88,.72),Amber(.99,.72,.37),Dim(.46,.52,.49);
    auto Box=[&](float L,float T,float W,float H,FLinearColor Color){DrawRect(Color,X+L*U,Y+T*U,W*U,H*U);};
    auto Label=[&](const FString& S,float L,float T,FLinearColor Color,float Size){Text(S,X+L*U,Y+T*U,Color,Size*U);};
    auto Action=[&](FName Key,const FString& S,float L,float T,float W){
        Box(L,T,W,36,Mint); Label(S,L+12,T+8,Ink,1.12f);
        AddHitBox(FVector2D(X+L*U,Y+T*U),FVector2D(W*U,36*U),Key,true);
    };
    auto Location=[&](const FETPerk& N){return FVector2D(X+(19+N.Position.X)*U,Y+(97+N.Position.Y*1.12)*U);};
    auto Connected=[&](const FETPerk& N){return N.Neighbors.ContainsByPredicate([&](const FString& Id){return P->Has(Id);});};
    DrawRect(FLinearColor(.005,.012,.01,.74),0,0,Canvas->SizeX,Canvas->SizeY);
    Box(0,0,1060,635,Ink); Box(0,0,1060,3,Mint); Box(696,100,1,419,Dim.CopyWithNewOpacity(.6f));
    Label(TEXT("Survivor skill web"),26,17,Paper,2.2f);
    Label(FString::Printf(TEXT("Level %d     %d / %d XP"),Save.Level,Save.XP,Save.NextLevelXP()),28,64,Mint,1.12f);
    Box(298,74,350,6,Dim.CopyWithNewOpacity(.45f)); Box(298,74,350*FMath::Clamp(float(Save.XP)/Save.NextLevelXP(),0.f,1.f),6,Amber);
    Label(FString::Printf(TEXT("%d skill point%s available"),Save.Points,Save.Points==1?TEXT(""):TEXT("s")),722,60,Amber,1.25f);
    const FETPerk* Selected=nullptr;
    for(const auto& N:Perks) for(const auto& Id:N.Neighbors) {
        const auto* Other=Perks.FindByPredicate([&](const FETPerk& O){return O.Id==Id;});
        if(!Other) continue;
        const auto A=Location(N),B=Location(*Other);
        const bool Lit=P->Has(N.Id)||P->Has(Id);
        DrawLine(A.X,A.Y,B.X,B.Y,Lit?Mint.CopyWithNewOpacity(.84f):Dim.CopyWithNewOpacity(.45f),U*(Lit?2.3f:1.4f));
    }
    for(const auto& N:Perks) {
        const auto Pos=Location(N); const bool Owned=P->Has(N.Id),Available=Connected(N);
        const FLinearColor Color=Owned?Mint:Available?Amber:Dim;
        const bool Picked=N.Id==P->SelectedPerk;
        if(Picked) Selected=&N;
        auto Diamond=[&](float R,FLinearColor C){
            R*=U;
            DrawLine(Pos.X,Pos.Y-R,Pos.X+R,Pos.Y,C,2*U); DrawLine(Pos.X+R,Pos.Y,Pos.X,Pos.Y+R,C,2*U);
            DrawLine(Pos.X,Pos.Y+R,Pos.X-R,Pos.Y,C,2*U); DrawLine(Pos.X-R,Pos.Y,Pos.X,Pos.Y-R,C,2*U);
        };
        DrawRect(Ink,Pos.X-9*U,Pos.Y-9*U,18*U,18*U); Diamond(9,Color);
        if(Picked) Diamond(14,Paper);
        if(Owned) DrawRect(Color,Pos.X-3*U,Pos.Y-3*U,6*U,6*U);
        float TW=0,TH=0; GetTextSize(N.Name,TW,TH,GEngine->GetSmallFont(),.85f*U);
        DrawRect(Ink,Pos.X-TW*.5-3*U,Pos.Y+12*U,TW+6*U,TH+2*U);
        Text(N.Name,Pos.X-TW*.5,Pos.Y+12*U,Color,.85f*U);
        const double HW=FMath::Max(double(18*U),TW*.5+2*U);
        AddHitBox(Pos-FVector2D(HW,12*U),FVector2D(HW*2,43*U),FName(*(TEXT("perk:")+N.Id)),true);
    }
    if(Selected) {
        const auto& N=*Selected;
        const bool Owned=P->Has(N.Id),Adjacent=Connected(N);
        Label(N.Branch.ToUpper(),722,126,Mint,1.f); Label(N.Name,722,167,Paper,1.55f);
        Wrapped(N.Description,X+722*U,Y+225*U,305*U,U*1.12f);
        if(Owned) Label(TEXT("Learned"),722,368,Mint,1.18f);
        else {
            Label(FString::Printf(TEXT("Cost   %d skill point%s"),N.Cost,N.Cost==1?TEXT(""):TEXT("s")),722,340,Amber,1.15f);
            if(!Adjacent) {
                TArray<FString> Paths;
                for(const auto& Other:Perks) if(N.Neighbors.Contains(Other.Id)) Paths.Add(Other.Name);
                Wrapped(TEXT("Reach this node through ")+FString::Join(Paths,TEXT(" or "))+TEXT("."),X+722*U,Y+385*U,296*U,U);
            } else if(Save.Points<N.Cost) Label(TEXT("Earn more XP to gain a point"),722,388,Dim,.98f);
            else Action(TEXT("learn-perk"),TEXT("Learn perk"),722,385,230);
        }
    }
    Label(TEXT("Select a node to inspect its perk."),26,545,Paper,1.08f);
    Label(TEXT("Earn XP from encounters, quests, scavenging, forestry and mining."),26,581,Dim,.94f);
    Action(TEXT("resume"),TEXT("K   /   Back to survivor"),722,563,300);
}
