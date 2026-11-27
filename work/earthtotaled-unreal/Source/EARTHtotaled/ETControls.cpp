#include "ETGame.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"

void AETCharacter::Controls()
{
    if(Panel==TEXT("controls")) {ClosePanel(); return;}
    if(!Panel.IsEmpty()&&Panel!=TEXT("title")&&Panel!=TEXT("pause")) return;
    GuideOrigin=Panel; GuidePage=Building?1:Borer?2:0;
    OpenPanel(TEXT("controls"),TEXT("FIELD GUIDE"),TEXT(""));
}
void AETHUD::DrawControls(AETCharacter* P)
{
    struct Entry {const TCHAR* Key; const TCHAR* Action;};
    static const Entry Pages[][9]={
        {{TEXT("W A S D   /   Mouse"),TEXT("Move and look around. The compass and minimap show your nearest city.")},
         {TEXT("Shift   /   Space"),TEXT("Shift sprints. Tap Space twice for two free jumps. Hold the second press to fly. Land to refuel.")},
         {TEXT("Left / right mouse"),TEXT("Fire or aim. Light aim assist helps near misses while aiming. Toggle it in Pause.")},
         {TEXT("R   /   1-0 or wheel"),TEXT("Reload your magazine or switch between guns. Reserve ammo is unlimited.")},
         {TEXT("E   /   Ctrl"),TEXT("Talk, loot, fell trees or climb marked routes with Climbing Grip. Hold Ctrl near loose loot to sweep.")},
         {TEXT("E / Space at stations"),TEXT("Follow the teal Ringline on M. E takes the lift or waits for the train. Jump on or off the open deck.")},
         {TEXT("H   /   Q   /   F"),TEXT("Drink, use a medkit, or toggle your lamp. Shield recharges after 10 seconds out of combat.")},
         {TEXT("Tab   /   K   /   J   /   M"),TEXT("Open your pack, skill web, journal, or field atlas. Right-click the map to drop a pin.")},
         {TEXT("Esc   /   F1   /   F11"),TEXT("Pause and manage saves, open the field console, or toggle fullscreen.")}},
        {{TEXT("E at a tree"),TEXT("Fell it for 8 wood. The birches beside Shelter 09 are a good place to start.")},
         {TEXT("B   /   Mouse wheel"),TEXT("Enter build mode and choose a wall, floor, ramp, roof, relay, turret, or miner.")},
         {TEXT("C   /   R   /   T"),TEXT("C selects material. R rotates. T upgrades the frame under your crosshair.")},
         {TEXT("Left mouse"),TEXT("Place a green preview within 15 m. Aim at firm ground or a frame edge.")},
         {TEXT("Right mouse   /   B"),TEXT("Salvage the piece under your crosshair, or put the builder away.")},
         {TEXT("Wood   /   160 HP"),TEXT("Each piece costs 4 wood. One building kit also makes a wooden piece.")},
         {TEXT("Stone   /   320 HP"),TEXT("Each piece costs 4 stone. Two successful borer cuts supply one piece.")},
         {TEXT("Metal   /   600 HP"),TEXT("Each piece costs 2 iron ore. Look for rust-red seams underground.")},
         {TEXT("Foundations"),TEXT("Frames and rock can span a gap. Removing the last support collapses the span.")}},
        {{TEXT("06:00 to 19:00"),TEXT("52 real minutes of daylight. Clear zombies in infested towns to reopen the refuge keeper.")},
         {TEXT("19:00 to 06:00"),TEXT("Roaming creatures and patrols emerge at night. Dawn pauses raids. Highway soldiers focus on their rivals in daylight.")},
         {TEXT("G   /   Left mouse"),TEXT("Mine open wilderness for stone and XP. City ground, streets and structure footprints are protected at every depth.")},
         {TEXT("Right mouse   /   R"),TEXT("Hold to survey for ore. Press R to switch between round and square cuts.")},
         {TEXT("V with borer out"),TEXT("Winch to the surface from underground. Your jetpack also works in a shaft.")},
         {TEXT("Relay   /   Turret"),TEXT("A relay costs 2 iron + 1 gold ore. A turret costs 3 + 1 and needs a relay within 75 m.")},
         {TEXT("Turtle miner   /   E"),TEXT("Build for 4 iron + 1 gold ore. Use E to collect the hopper or service a machine.")},
         {TEXT("E at a relay"),TEXT("Repair, upgrade your armor, or trace a nest after defending a valuable base.")},
         {TEXT("Esc   /   Survivor saves"),TEXT("Copy, import or export survivors here. Town boards fund watch shifts and mark water towers.")}}
    };
    const float SW=Canvas->SizeX,SH=Canvas->SizeY;
    const float U=FMath::Clamp(FMath::Min(SW/1366.f,SH/850.f),.60f,2.2f);
    const float Width=FMath::Min(SW*.94f,1060*U),Height=FMath::Min(SH*.94f,655*U);
    const float X=(SW-Width)*.5f,Top=(SH-Height)*.5f,Bottom=Top+Height;
    const float Left=X+26*U,Inside=Width-52*U,KeyWidth=Inside*.255f;
    const FLinearColor Ink(.012f,.025f,.019f,.99f),Mint(.63f,.82f,.65f),Paper(.98f,.94f,.82f),Line(.17f,.24f,.19f);
    DrawRect(FLinearColor(.007,.009,.007,.42),0,0,SW,SH);
    DrawRect(Ink,X,Top,Width,Height); DrawRect(Mint,X,Top,4*U,Height);
    Text(TEXT("FIELD GUIDE"),Left,Top+12*U,Paper,2.05f*U);
    const TCHAR* Ids[]={TEXT("guide-field"),TEXT("guide-build"),TEXT("guide-survive")};
    const TCHAR* Titles[]={TEXT("Field controls"),TEXT("Harvest & build"),TEXT("Mining & survival")};
    const float Tabs=Top+62*U,Tab=Inside/3;
    auto TabButton=[&](const TCHAR* Id,const TCHAR* Label,float BX,float BY,float BW,bool Active) {
        DrawRect(Active?Mint:Line,BX,BY,BW,35*U);
        Text(Label,BX+12*U,BY+7*U,Active?Ink:Paper,1.22f*U);
        AddHitBox(FVector2D(BX,BY),FVector2D(BW,35*U),FName(Id),true);
    };
    for(int I=0;I<3;++I) {
        TabButton(Ids[I],Titles[I],Left+I*Tab,Tabs,Tab-4*U,I==P->GuidePage);
    }
    const float Begin=Tabs+49*U,End=Bottom-58*U,Row=(End-Begin)/9;
    const auto& Page=Pages[FMath::Clamp(P->GuidePage,0,2)];
    for(int I=0;I<9;++I) {
        const float Y=Begin+I*Row;
        DrawLine(Left,Y+Row-1,Left+Inside,Y+Row-1,Line,1);
        const float Scale=1.23f*U,Available=Inside-KeyWidth-12*U;
        float Tw,Th; GetTextSize(Page[I].Action,Tw,Th,GEngine->GetSmallFont(),Scale);
        const float YText=Y+FMath::Max(1.f,(Row-Th-(Tw>Available?21*Scale:0))*.5f);
        Text(Page[I].Key,Left+6*U,Y+(Row-Th)*.5f,Mint,Scale);
        Wrapped(Page[I].Action,Left+KeyWidth,YText,Available,Scale);
    }
    TabButton(TEXT("guide-back"),TEXT("Back   /   Esc or F2"),Left,Bottom-47*U,205*U,false);
}
