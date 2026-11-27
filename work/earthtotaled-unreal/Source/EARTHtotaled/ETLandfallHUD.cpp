#include "ETGame.h"
#include "ETLandfall.h"
#include "Engine/Canvas.h"

void AETHUD::DrawLandfall(AETCharacter* P)
{
    const float W=Canvas->SizeX,H=Canvas->SizeY,U=FMath::Min(W/900.f,H/600.f);
    const FLinearColor Paper(.93f,.91f,.8f),Mint(.43f,.9f,.69f),Amber(1,.56f,.18f),Ink(.009f,.017f,.023f),Steel(.038f,.054f,.064f);
    if(P->Panel==TEXT("arrival")) {
        const float T=P->FlightSeconds;
        if(T<7) {
            DrawRect(FLinearColor(.003,.007,.019),0,0,W,H);
            FRandomStream Stars(1986);
            for(int I=0;I<165;++I) {
                const float Angle=Stars.FRand()*2*PI,Speed=Stars.FRandRange(.08f,.19f),Seed=Stars.FRand();
                const float Radius=FMath::Fmod(Seed+T*Speed+(T>4?(T-4)*(T-4)*.24f:0),1.f);
                const FVector2D Axis(FMath::Cos(Angle)*W*.86f,FMath::Sin(Angle)*H*.81f),Middle(W*.53f,H*.43f);
                const auto Head=Middle+Axis*Radius,Tail=Middle+Axis*FMath::Max(0.f,Radius-(T>4?.11f:.002f));
                DrawLine(Tail.X,Tail.Y,Head.X,Head.Y,FLinearColor(.36f+.5f*Radius,.58f+.3f*Radius,.88f),FMath::Max(1.f,U*Radius*2));
            }
            if(T<4.3f) for(int I=0;I<128;++I) {
                const float X=W*I/128.f,F=(X/W-.4f),Y=H*(.67f+F*F*.68f-T*.018f);
                DrawRect(FLinearColor(.045,.14,.21),X,Y,W/128.f+1,H-Y);
                DrawRect(FLinearColor(.32,.56,.65),X,Y,W/128.f+1,2*U);
            }
        }
        // Windscreen bracing and amber flight instruments stay fixed to the pilot's seat.
        const FLinearColor Edge(.19f,.23f,.23f),Glass(.59f,.79f,.73f,.58f);
        for(int Side:{-1,1}) {
            const float Outer=Side<0?-W*.035f:W*1.035f,Inner=Side<0?W*.203f:W*.797f;
            DrawLine(Outer,H*.015f,Inner,H*.811f,Edge,17*U);
            DrawLine(Outer+Side*2*U,H*.015f,Inner+Side*2*U,H*.811f,Steel,13*U);
            DrawLine(Outer,H*.014f,Inner,H*.81f,Ink,3*U);
            for(int I=1;I<6;++I) {const float F=I/6.f;
                DrawRect(Edge,FMath::Lerp(Outer,Inner,F)-2*U,FMath::Lerp(H*.015f,H*.811f,F),4*U,3*U);}
        }
        if(T>=13) {
            const FVector2D Strike(W*.77f,H*.27f);
            FRandomStream Fracture(9);
            for(int I=0;I<9;++I) {
                FVector2D Last=Strike;float A=I*(2*PI/9.f)+Fracture.FRandRange(-.18f,.18f);
                for(int J=0;J<3;++J) {
                    const FVector2D Next=Last+FVector2D(FMath::Cos(A),FMath::Sin(A))*Fracture.FRandRange(12.f,48.f)*U;
                    DrawLine(Last.X,Last.Y,Next.X,Next.Y,Glass,1.f*U);
                    if(J==1) DrawLine(Next.X,Next.Y,Next.X+16*U*FMath::Cos(A+.6f),Next.Y+16*U*FMath::Sin(A+.6f),Glass,1.f*U);
                    Last=Next;A+=Fracture.FRandRange(-.26f,.26f);
                }
            }
        }
        DrawRect(Ink,0,H*.8f,W,H*.2f);DrawRect(Edge,0,H*.795f,W,3*U);
        DrawRect(FLinearColor(.011f,.035f,.031f,.85f),W*.041f,H*.043f,W*.315f,H*.125f);
        const float DialX=W*.70f,DialY=H*.895f,Radius=H*.055f;
        for(int I=0;I<=20;++I) {
            const float A=PI*(.78f+1.44f*I/20.f);
            const float Inner=Radius*(I%5?.88f:.72f);
            DrawLine(DialX+Inner*FMath::Cos(A),DialY+Inner*FMath::Sin(A),DialX+Radius*FMath::Cos(A),DialY+Radius*FMath::Sin(A),Paper,U);
        }
        const float Alt=T<7?1.f:FMath::Square(FMath::Clamp((13.f-T)/6.f,0.f,1.f));
        const float Needle=PI*(.78f+1.44f*Alt);
        DrawLine(DialX,DialY,DialX+Radius*.78f*FMath::Cos(Needle),DialY+Radius*.78f*FMath::Sin(Needle),Amber,2*U);
        Text(TEXT("ALT"),DialX-10*U,DialY+Radius*.3f,Mint,.66f*U);
        for(int I=0;I<3;++I) DrawRect((T>=13||I==int(T*2)%3)?Amber:Steel,W*.055f+I*18*U,H*.934f,11*U,3*U);
        Text(TEXT("fraudCORP    /    FLIGHT RECORDER"),W*.055f,H*.06f,Mint,1.1f*U);
        const TCHAR* Code=T<4?TEXT("1986     ORBITAL DELIVERY  /  FC-09"):T<7?TEXT("CHRONOMETER DISAGREEMENT"):TEXT("2253     EARTH  /  SHELTER 09");
        Text(Code,W*.055f,H*.115f,Amber,1.12f*U);
        const TCHAR* Caption=T<4?TEXT("1986. You haul cargo between planets for fraudCORP."):T<7?TEXT("The stars fold. Your clock runs through 267 years in three seconds."):T<13?TEXT("2253. The landing beacon at SHELTER 09 is still transmitting."):T<15?TEXT("IMPACT   /   HULL BREACH"):TEXT("I am going to require a gun");
        Wrapped(Caption,W*.055f,H*.85f,W*.64f,1.35f*U);
        if(T>12.98f&&T<13.38f) DrawRect(FLinearColor(1,.86f,.63f,FMath::Clamp(1-(T-13.f)/.38f,0.f,1.f)),0,0,W,H);
        Button(TEXT("skip"),TEXT("Skip opening"),W*.77f,H*.86f,W*.18f);return;
    }
    const float X=W*.12f,Y=H*.13f,Width=W*.76f;
    DrawRect(Ink,X,Y,Width,H*.73f);DrawRect(Mint,X,Y,Width,2*U);
    Text(P->PanelTitle,X+25*U,Y+21*U,Paper,1.7f*U);
    Wrapped(P->PanelBody,X+25*U,Y+81*U,Width-50*U,1.12f*U);
    const float Row=Y+H*.53f,Col=(Width-78*U)/3;
    if(P->Panel==TEXT("updates")) {
        if(P->ConfirmVersion) Button(TEXT("install"),TEXT("Install selected version"),X+25*U,Row,Col*1.6f);
        else {
            if(P->ReleaseIndex>0) Button(TEXT("prev"),TEXT("Newer version"),X+25*U,Row,Col);
            if(P->ReleaseIndex+1<P->ReleaseTags.Num()) Button(TEXT("next"),TEXT("Older version"),X+Col+39*U,Row,Col);
            if(P->ReleaseTags.IsValidIndex(P->ReleaseIndex)) Button(TEXT("choose"),TEXT("Choose this version"),X+2*Col+52*U,Row,Col);
            else if(!P->CheckingVersions) Button(TEXT("retry"),TEXT("Check again"),X+25*U,Row,Col);
        }
        Button(TEXT("later"),TEXT("Return without updating"),X+25*U,Row+46*U,Col*1.6f);
    } else {
        if(P->RackIndex>0) Button(TEXT("prev"),TEXT("Previous gun"),X+25*U,Row,Col);
        if(P->RackIndex+1<P->World->State->Relics.Num()) Button(TEXT("next"),TEXT("Next gun"),X+Col+39*U,Row,Col);
        if(P->World->State->Relics.IsValidIndex(P->RackIndex)) Button(TEXT("equip"),TEXT("Equip this gun"),X+2*Col+52*U,Row,Col);
        Button(TEXT("back"),TEXT("Back to pack"),X+25*U,Row+46*U,Col);
    }
}
