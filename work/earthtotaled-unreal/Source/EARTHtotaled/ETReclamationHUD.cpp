#include "ETGame.h"
#include "Engine/Canvas.h"

void AETHUD::DrawReclamation(AETCharacter* P)
{
    const float SW=Canvas->SizeX,SH=Canvas->SizeY,U=FMath::Min(SW/960.f,SH/650.f);
    const float W=FMath::Min(SW*.92f,940.f*U),X=(SW-W)*.5f,Y=(SH-600*U)*.5f;
    const FLinearColor Pale(.89,.91,.82),Accent(.57,.83,.66),Dim(.59,.66,.59),Background(.017,.032,.031,.98);
    DrawRect(Background,X,Y,W,600*U);DrawRect(Accent,X,Y,W,2*U);
    Text(P->PanelTitle,X+24*U,Y+17*U,Pale,1.56f*U);
    const bool Mods=P->Panel==TEXT("mods");
    const bool Preview=Mods&&!P->ProposedMod.IsEmpty();
    const FString Body=Preview?P->WorkshopNote:P->PanelBody;
    Wrapped(Body,X+24*U,Y+64*U,W-48*U,.92f*U);
    if(Mods&&!Preview&&!P->WorkshopNote.IsEmpty()) Wrapped(P->WorkshopNote,X+24*U,Y+339*U,W-48*U,.84f*U);
    TArray<TPair<FString,FString>> Actions;
    auto Add=[&](const TCHAR* Id,const FString& Label){Actions.Emplace(Id,Label);};
    if(Mods) {
        if(Preview) {Add(TEXT("install"),TEXT("Install this preview"));Add(TEXT("discard"),TEXT("Discard preview"));}
        else {
            Add(TEXT("chatgpt"),TEXT("Connect my ChatGPT"));Add(TEXT("browser-ai"),TEXT("Open ChatGPT in browser"));Add(TEXT("preview"),TEXT("Preview clipboard"));
            Add(TEXT("starter"),TEXT("Preview starter pack"));Add(TEXT("kit"),TEXT("Open author kit"));Add(TEXT("guide"),TEXT("Modding & GitHub guide"));
            Add(TEXT("folder"),TEXT("Open Mods folder"));Add(TEXT("refresh"),TEXT("Reload mods"));
            if(P->World->Mods.IsValidIndex(P->ModPage)) {
                const auto& M=P->World->Mods[P->ModPage];Add(TEXT("toggle"),M.Enabled?TEXT("Disable pack"):TEXT("Enable pack"));
                if(P->World->Mods.Num()>1) {Add(TEXT("previous"),TEXT("Previous pack"));Add(TEXT("next"),TEXT("Next pack"));}
                if(P->WorkshopOrigin==TEXT("pause")) Add(TEXT("redeem"),TEXT("Redeem collected finds"));
            }
        }
    } else if(P->Panel==TEXT("devtools")) {
        Add(TEXT("cheat-shield"),P->TestInvincible?TEXT("Disable invincibility"):TEXT("Enable invincibility"));
        Add(TEXT("cheat-supply"),TEXT("Grant test supplies"));Add(TEXT("cheat-town"),TEXT("Travel to infestation"));
        Add(TEXT("cheat-tower"),TEXT("Travel to cistern"));Add(TEXT("cheat-clear"),TEXT("Clear local zombies"));Add(TEXT("cheat-loot"),TEXT("Scatter test loot"));
        Add(TEXT("cheat-day"),TEXT("Advance to dawn"));Add(TEXT("cheat-night"),TEXT("Advance to night"));
    } else {
        const auto* T=P->World->State->Towns.Find(P->BoardCell);
        Add(TEXT("mark-tower"),TEXT("Track hill cistern"));Add(TEXT("mark-town"),TEXT("Track nearest infestation"));
        if(T&&T->Claimed) {
            Add(TEXT("deposit"),TEXT("Fund watch / 20 wrappers"));
            if(T->Defenses==0) Add(TEXT("walls"),TEXT("Build walls / 48 wrappers"));
            if(T->Defenses==1) Add(TEXT("sentries"),TEXT("Build sentries / 90 wrappers"));
            if(T->Defenses) Add(TEXT("repair-town"),TEXT("Repair defenses / 12 wrappers"));
        }
    }
    Add(TEXT("back"),TEXT("Back   /   Esc"));
    const int Rows=(Actions.Num()+2)/3;const float Left=X+24*U,Gap=8*U,CW=(W-48*U-2*Gap)/3;
    const float Top=Y+(582-Rows*38)*U;
    for(int I=0;I<Actions.Num();++I) {
        const float BX=Left+I%3*(CW+Gap),BY=Top+I/3*38*U;
        DrawRect(FLinearColor(.115,.183,.151),BX,BY,CW,32*U);Text(Actions[I].Value,BX+8*U,BY+7*U,Pale,.90f*U);
        AddHitBox(FVector2D(BX,BY),FVector2D(CW,32*U),FName(*Actions[I].Key),true);
    }
}
