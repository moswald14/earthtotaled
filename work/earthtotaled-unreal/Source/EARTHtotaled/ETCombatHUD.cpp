#include "ETCombat.h"
#include "ETGame.h"
#include "Engine/Canvas.h"
#include "CanvasItem.h"
#include "Engine/Texture2D.h"
#include "TextureResource.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"

namespace {
    const FLinearColor Back(.012f,.024f,.024f,.90f), Track(.14f,.19f,.18f), Ink(.96f,.95f,.85f);
    const FLinearColor Health(.99f,.39f,.26f), Shield(.32f,.85f,.98f), Gold(.98f,.74f,.43f);

    void Arc(AETHUD& Hud,FVector2D Center,float Radius,float From,float To,float Fraction,FLinearColor Color,float Width)
    {
        constexpr int Segments=26;
        const float End=FMath::Lerp(From,To,FMath::Clamp(Fraction,0.f,1.f));
        const auto Point=[&](float Degrees) {
            const float A=FMath::DegreesToRadians(Degrees);
            return Center+FVector2D(FMath::Cos(A),FMath::Sin(A))*Radius;
        };
        auto Prev=Point(From);
        for(int I=1;I<=Segments&&Fraction>0;++I) {
            auto Next=Point(FMath::Lerp(From,End,float(I)/Segments));
            Hud.DrawLine(Prev.X,Prev.Y,Next.X,Next.Y,Color,Width);
            Prev=Next;
        }
    }

    void Tag(AETHUD& Hud,const FString& Value,float X,float Y,float Scale,FLinearColor Color,float Align=.5f)
    {
        float W,H; Hud.GetTextSize(Value,W,H,GEngine->GetSmallFont(),Scale);
        X-=W*Align;
        Hud.DrawRect(Back,X-5*Scale,Y-2*Scale,W+10*Scale,H+4*Scale);
        Hud.Text(Value,X,Y,Color,Scale);
    }

    void Edges(AETHUD& Hud,UCanvas& Canvas,float W,float H,FVector2D Source,float Strength,FLinearColor Color)
    {
        if(Strength<=0||!Canvas.DefaultTexture) return;
        const bool Unknown=Source.IsNearlyZero();
        TArray<FCanvasUVTri> Mesh;
        for(int Edge=0;Edge<4;++Edge) {
            const bool Horizontal=Edge%2==0;
            const float Side=Edge==0?Source.X:Edge==1?Source.Y:Edge==2?-Source.X:-Source.Y;
            const float Weight=Unknown?.42f:FMath::Clamp((Side-.15f)/.85f,0.f,1.f);
            if(Weight<=0) continue;
            const float Along=Horizontal?W:H;
            const float Offset=Horizontal?Source.Y:-Source.X;
            const float Center=.5f+Offset*.22f;
            const float Depth=FMath::Clamp(H*.085f,30.f,100.f);
            const auto Point=[&](float T,float Inset)->FVector2D {
                if(Horizontal)return {Along*T,Edge==0?Inset:H-Inset};
                return {Edge==3?Inset:W-Inset,Along*T};
            };
            const auto Outer=[&](float T){
                const float Taper=Unknown?.82f:1.f-FMath::SmoothStep(.10f,.49f,FMath::Abs(T-Center));
                return Color.CopyWithNewOpacity(Strength*Weight*Taper);
            };
            const auto Clear=Color.CopyWithNewOpacity(0);
            // Shared vertices interpolate the alpha across the border without seams.
            for(int I=0;I<12;++I) {
                const float A=I/12.f,B=(I+1)/12.f;
                FCanvasUVTri T{};
                T.V0_Pos=Point(A,0); T.V1_Pos=Point(B,0); T.V2_Pos=Point(B,Depth);
                T.V0_Color=Outer(A); T.V1_Color=Outer(B); T.V2_Color=Clear; Mesh.Add(T);
                T.V1_Pos=T.V2_Pos; T.V1_Color=Clear; T.V2_Pos=Point(A,Depth); Mesh.Add(T);
            }
        }
        if(!Mesh.IsEmpty()){
            FCanvasTriangleItem Wash(Mesh,Canvas.DefaultTexture->GetResource());
            Wash.BlendMode=SE_BLEND_Translucent; Canvas.DrawItem(Wash);
        }
        if(Unknown)return;
        // An outlined notch remains readable against pale sky or dark trees.
        const bool Horizontal=FMath::Abs(Source.X)>=FMath::Abs(Source.Y);
        const float U=FMath::Clamp(H/800.f,.7f,1.8f);
        const FVector2D Normal=Horizontal?FVector2D(0,Source.X>0?1:-1):FVector2D(Source.Y>0?-1:1,0);
        const FVector2D Along(-Normal.Y,Normal.X);
        const FVector2D Origin=Horizontal?FVector2D((.5f+Source.Y*.22f)*W,Source.X>0?5*U:H-5*U):
            FVector2D(Source.Y>0?W-5*U:5*U,(.5f-Source.X*.22f)*H);
        const auto Tip=Origin+Normal*11*U;
        for(int Sign:{-1,1}){
            const auto End=Origin+Along*(Sign*19.f*U);
            Hud.DrawLine(End.X,End.Y,Tip.X,Tip.Y,Back.CopyWithNewOpacity(Strength),6*U);
            Hud.DrawLine(End.X,End.Y,Tip.X,Tip.Y,Color.CopyWithNewOpacity(Strength),3*U);
        }
    }
}

void AETHUD::DrawCombat(AETCharacter& P)
{
    const auto& S=*P.World->State;
    const float W=Canvas->SizeX,H=Canvas->SizeY;
    const float U=FMath::Clamp(FMath::Min(W/1440.f,H/850.f),.74f,1.55f);
    const bool Driving=P.Ride()!=nullptr;
    const float Radius=(Driving?36.f:P.Aiming?42.f:54.f)*U;
    const FVector2D Center(W*.5f,H*.5f);
    const float Intensity=FMath::Clamp(S.HitFlashIntensity,0,100)*.01f;
    if(P.DamageFlash>0)
        Edges(*this,*Canvas,W,H,ETCombat::Bearing(P.HitBearing,P.GetControlRotation().Yaw),FMath::Sqrt(P.DamageFlash/.32f)*Intensity,P.HealthWasHit?Health:Shield);

    const auto Meter=[&](float Start,float End,float Fraction,FLinearColor Color){
        Arc(*this,Center,Radius,Start,End,1,Back,7*U);
        Arc(*this,Center,Radius,Start,End,1,Track,4*U);
        Arc(*this,Center,Radius,Start,End,Fraction,Color,4*U);
    };
    Meter(143,217,S.Health/P.MaxHealth(),Health);
    Meter(37,-37,S.Shield/S.MaxShield(),Shield);
    const bool Low=S.Health>0&&S.Health<=P.MaxHealth()*.25f;
    const float TextScale=U*(Driving?1.f:1.18f);
    Tag(*this,FString::Printf(TEXT("%s %d"),Low?TEXT("LOW HP"):TEXT("HP"),FMath::CeilToInt(S.Health)),Center.X-Radius-10*U,Center.Y-7*U,TextScale,Low?Gold:Health,1);
    Tag(*this,FString::Printf(TEXT("SHIELD %d"),FMath::CeilToInt(S.Shield)),Center.X+Radius+10*U,Center.Y-7*U,TextScale,Shield,0);
    if(P.ShieldBreakTime>0) Tag(*this,TEXT("SHIELD BROKEN"),Center.X,Center.Y-55*U,.94f*U,Shield);
    if(P.AlertCaptionTime>0&&S.AlertCaptions) {
        const auto Direction=ETCombat::Direction(ETCombat::Bearing(P.CaptionOrigin-P.GetActorLocation(),P.GetControlRotation().Yaw));
        Tag(*this,P.AlertCaption+TEXT("  /  ")+Direction,Center.X,H*.625f,U,Ink);
    }
    if(auto* Enemy=P.CombatTarget()) {
        const bool TopRow=W>=1040&&W<1440;
        const float Width=TopRow?FMath::Clamp(W*(Driving?.33f:.38f),330.f,540.f):FMath::Min(W*.44f,(Driving?415.f:530.f)*U);
        const float X=TopRow?FMath::Min((W-Width)*.5f,W-435.f-Width):(W-Width)*.5f;
        const float Y=W>=1440?22.f+69*U:TopRow?23.f:145.f+69*U;
        const float BarH=Driving?7*U:11*U;
        DrawRect(Back,X,Y,Width,34*U+BarH);
        const auto* Voice=ETCombat::Voice(Enemy->Kind);
        Text(Voice->Name,X+10*U,Y+3*U,Ink,1.27f*U);
        const auto Values=FString::Printf(TEXT("%d / %d"),FMath::CeilToInt(Enemy->Health),FMath::CeilToInt(Enemy->FullHealth));
        float TW,TH;GetTextSize(Values,TW,TH,GEngine->GetSmallFont(),.88f*U);
        Text(Values,X+Width-TW-10*U,Y+6*U,Gold,.88f*U);
        const float Fill=FMath::Clamp(Enemy->Health/FMath::Max(1.f,Enemy->FullHealth),0.f,1.f);
        DrawRect(Track,X+10*U,Y+25*U,Width-20*U,BarH);
        DrawRect(Gold,X+10*U,Y+25*U,(Width-20*U)*Fill,BarH);
    }
}
