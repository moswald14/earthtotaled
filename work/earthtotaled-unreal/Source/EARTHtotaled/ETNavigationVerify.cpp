#include "ETGame.h"
#include "ETRing.h"
#include "ETRailDistricts.h"
#include "ETGeology.h"
#include "Camera/CameraComponent.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "ProceduralMeshComponent.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"

namespace
{
    bool AsphaltAt(const AETRegion& R,FVector2D WorldPoint)
    {
        const auto* Mesh=R.RoadMesh?R.RoadMesh->GetProcMeshSection(1):nullptr;
        if(!Mesh) return false;
        const auto At=WorldPoint-FVector2D(R.GetActorLocation());
        for(int I=0;I+2<Mesh->ProcIndexBuffer.Num();I+=3) {
            FVector2D V[3]; for(int J=0;J<3;++J) V[J]=FVector2D(Mesh->ProcVertexBuffer[Mesh->ProcIndexBuffer[I+J]].Position);
            const double Area=FVector2D::CrossProduct(V[1]-V[0],V[2]-V[0]);
            if(FMath::Abs(Area)<1.e-5) continue;
            const double U=FVector2D::CrossProduct(At-V[0],V[2]-V[0])/Area,Vv=FVector2D::CrossProduct(V[1]-V[0],At-V[0])/Area;
            if(U>=-.00001&&Vv>=-.00001&&U+Vv<=1.00001) return true;
        }
        return false;
    }
}
void AETWorld::RunNavigationChecks(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    using namespace ETNav;
    Check(District({-80001,500})==FIntPoint(-1,0)&&District({-79999,500})==FIntPoint::ZeroValue&&
        District({80001,-82000})==FIntPoint(1,-1),TEXT("Nearest city changes at the actual district boundary in all quadrants"));
    Check(Bearing({0,-1})==0&&Bearing({1,0})==90&&Bearing({0,1})==180&&Bearing({-1,0})==270,TEXT("North-up chart and clockwise compass agree at all four cardinal headings"));
    const FVector2D View(1600,900),Center(-164000,85000); auto Chart=Atlas(View,Center,60000);
    const FVector2D Point=Chart.Position+Chart.Size*FVector2D(.24,.69);
    const auto Anchor=Chart.WorldAt(Point);
    P->MapSpan=Chart.Span; P->MapCenter=Center; P->ZoomMap(2,Point,Chart);
    auto Zoomed=Atlas(View,P->MapCenter,P->MapSpan);
    Check(Zoomed.Span<Chart.Span&&Zoomed.WorldAt(Point).Equals(Anchor,.01)&&Zoomed.Plot(Anchor).Equals(Point,.001),TEXT("Wheel zoom holds the same world location under the cursor"));
    P->PanMap({95,-37},Zoomed); auto Panned=Atlas(View,P->MapCenter,P->MapSpan);
    Check(Panned.Plot(Anchor).Equals(Point+FVector2D(95,-37),.001),TEXT("Dragging moves mapped features by the exact screen-space offset"));
    P->ZoomMap(-100,Point,Panned); auto Wide=Atlas(View,P->MapCenter,P->MapSpan); P->ZoomMap(100,Point,Wide);
    Check(Wide.Span==MaxSpan&&P->MapSpan>=MinSpan,TEXT("Atlas zoom stays inside its playable chart scales"));
    State->MapPins.Reset(); State->TrackedPin=INDEX_NONE;
    P->UseMapPoint(Point,Chart,true); P->UseMapPoint(Point+FVector2D(90,30),Chart,true); P->UseMapPoint(Point,Chart,false);
    Check(State->MapPins.Num()==2&&State->TrackedPin==0&&State->MapPins[0].Equals(Anchor,.01),TEXT("Independent world-space pins can be placed and reselected without duplicates"));
    FString Msg; const FString File=FPaths::ProjectSavedDir()/TEXT("atlas-roundtrip.etsave"); State->LightAimAssist=false;
    auto* Restored=P->ExportProfile(State,File,Msg)?P->ReadProfile(File,Msg):nullptr;
    Check(Restored&&Restored->MapPins==State->MapPins&&Restored->TrackedPin==0&&!Restored->LightAimAssist,TEXT("Portable survivor restores personal pins, active marker and aim preference"));
    const auto Far=Refuge({-5,4}); auto Elsewhere=Atlas(View,Far,60000);
    const int LoadedCount=Regions.Num(); P->UseMapPoint(Elsewhere.Plot(Far),Elsewhere,false);
    Check(State->MapPins.Num()==3&&State->MapPins.Last().Equals(Far,.01)&&Regions.Num()==LoadedCount,TEXT("Clicking a distant city creates a refuge waypoint without loading remote terrain"));
    State->LightAimAssist=true;
    const FVector2D RoadAt(47035,2610); EnsureRegion({1,0}); auto* R=Regions.FindChecked({1,0});
    Check(R->OnHighway(RoadAt)&&AsphaltAt(*R,RoadAt)&&!AsphaltAt(*R,RoadAt+FVector2D(0,1160)),TEXT("Blackpine Highway is visible on the actual forest terrain at the charted coordinates"));
    bool Clear=true; for(auto* H:R->Instances) if(H->GetStaticMesh()&&H->GetStaticMesh()->GetFName()==TEXT("SM_Trunk"))
        for(int I=0;I<H->GetInstanceCount();++I) {FTransform T; H->GetInstanceTransform(I,T,true); if(R->OnHighway(FVector2D(T.GetLocation()),120)) Clear=false;}
    Check(Clear,TEXT("Generated forest trunks leave the road clear for travel"));
    EnsureRegion({0,0}); auto* West=Regions.FindChecked({0,0});
    Check(AsphaltAt(*West,{31998,2602})&&AsphaltAt(*R,{32002,2602})&&West->RoadMesh&&West->RoadMesh->GetProcMeshSection(2),TEXT("New pavement meets across streaming seams and city crossings have painted geometry"));
    const FVector Ground(RoadAt,SurfaceHeight(RoadAt.X,RoadAt.Y));
    FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(ETNavigationTest),false,P); FString Reward;
    const bool Cut=GetWorld()->LineTraceSingleByChannel(H,Ground+FVector(0,0,200),Ground-FVector(0,0,450),ECC_Visibility,Q)&&Geology->Blast(H,Reward,true);
    Check(!Cut&&Reward.Contains(TEXT("roadbed"))&&!Geology->IsOpen(Ground-FVector(0,0,50))&&AsphaltAt(*R,RoadAt),TEXT("A square bore in the forest highway cannot open its roadbed"));
    R->Destroy(); Regions.Remove({1,0}); EnsureRegion({1,0});
    Check(AsphaltAt(*Regions.FindChecked({1,0}),RoadAt),TEXT("Reloading a protected highway tile preserves its asphalt"));

    P->StopBuilding(); P->Borer=false; P->Panel.Empty(); P->Aiming=true; P->Climbing=false;
    const FVector Origin(23000,-6000,14500); P->SetActorLocation(Origin-FVector(0,0,72));
    P->Command(TEXT("time night"));
    auto* Target=GetWorld()->SpawnActor<AETAgent>(Origin+FVector(4200,0,-80),FRotator::ZeroRotator);
    Target->Configure(this,TEXT("atlas-aim-robot"),TEXT("robot"));
    Target->Health=500;
    const auto AimTo=Target->Parts[0]->Bounds.Origin;
    const auto To=(AimTo-Origin).GetSafeNormal();
    auto Hits=[&](FVector Direction) {
        FHitResult Impact; return GetWorld()->LineTraceSingleByChannel(Impact,Origin,Origin+Direction*5500,ECC_Visibility,Q)&&Impact.GetActor()==Target;
    };
    FVector Near=To.RotateAngleAxis(1.1,FVector::UpVector); bool Helped=false;
    for(double Side:{-1.,1.}) for(double Angle=.15;Angle<1.65&&!Helped;Angle+=.025) {
        const FVector Try=To.RotateAngleAxis(Angle*Side,FVector::UpVector);
        if(!Hits(Try)&&Hits(P->AssistedDirection(Origin,Try,5500))) {Near=Try; Helped=true;}
    }
    const auto Assisted=P->AssistedDirection(Origin,Near,5500);
    const double Change=FMath::RadiansToDegrees(FMath::Acos(FMath::Clamp(FVector::DotProduct(Near,Assisted),-1.,1.)));
    Check(Helped&&Change>0&&Change<=.701,FString::Printf(TEXT("ADS can correct a near miss onto an exposed hostile by at most seven tenths of a degree / change %.3f"),Change));
    const auto WideMiss=To.RotateAngleAxis(4,FVector::UpVector);
    Check(P->AssistedDirection(Origin,To,5500).Equals(To)&&P->AssistedDirection(Origin,WideMiss,5500).Equals(WideMiss)&&P->AssistedDirection(Origin,Near,500).Equals(Near),TEXT("Direct hits, wide misses and out-of-range targets keep their original shot"));
    auto* Cover=Static(TEXT("SM_Block"),TEXT("Concrete"),(Origin+AimTo)*.5,FVector(.25,5,5));
    Check(P->AssistedDirection(Origin,Near,5500).Equals(Near),TEXT("A solid wall blocks aim assistance")); Cover->Destroy();
    P->Aiming=false; const bool Hip=P->AssistedDirection(Origin,Near,5500).Equals(Near);
    P->Aiming=true; P->Borer=true; const bool Survey=P->AssistedDirection(Origin,Near,5500).Equals(Near);
    P->Borer=false; State->LightAimAssist=false; const bool Off=P->AssistedDirection(Origin,Near,5500).Equals(Near);
    Check(Hip&&Survey&&Off,TEXT("Assist stays off for hip fire, the survey borer and the disabled preference")); State->LightAimAssist=true;
    const auto Weapon=State->Weapon; auto& Spec=Weapons[Weapon]; const float Spread=Spec.Spread; Spec.Spread=0;
    auto* PC=P->GetController(); PC->SetControlRotation(Near.Rotation()); FMinimalViewInfo V; P->Camera->GetCameraView(0,V);
    const float OldHP=Target->Health; State->Ammo[Weapon]=Spec.Mag; P->Shoot(); Spec.Spread=Spread;
    Check(Helped&&Target->Health<OldHP&&State->Ammo[Weapon]==Spec.Mag-1,TEXT("A real aimed shot follows the small correction and spends one round"));
    const FString Old=Target->Kind; Target->Kind=TEXT("deer");
    Check(P->AssistedDirection(Origin,Near,5500).Equals(Near),TEXT("Aim assistance never seeks nonhostile wildlife")); Target->Kind=Old;
    P->Command(TEXT("time day")); Check(P->AssistedDirection(Origin,Near,5500).Equals(Near),TEXT("Dormant daytime encounters do not attract aim"));
    Target->Destroy(); P->Aiming=false;
}

void AETWorld::CheckAtlasInput(AETCharacter* P,TFunctionRef<void(bool,const FString&)> Check)
{
    auto& Slate=FSlateApplication::Get();
    const auto Window=GetWorld()->GetGameViewport()->GetWindow();
    if(!Window.IsValid()) {Check(false,TEXT("Atlas accepts pointer events in the viewport"));return;}
    FVector2D Size; GetWorld()->GetGameViewport()->GetViewportSize(Size);
    const auto G=P->MapInput->GetCachedGeometry();
    const auto Chart=[&](){return ETNav::Atlas(Size,P->MapCenter,P->MapSpan);};
    const auto Screen=[&](FVector2D Pixel){return FVector2D(G.LocalToAbsolute(Pixel*FVector2D(G.GetLocalSize())/Size));};
    const TSet<FKey> Left={EKeys::LeftMouseButton};
    const FModifierKeysState Mod;
    auto Click=[&](FKey Button,FVector2D Pixel){
        const auto At=Screen(Pixel); const auto Mouse=Button==EKeys::RightMouseButton?EMouseButtons::Right:EMouseButtons::Left;
        Slate.OnMouseDown(Window->GetNativeWindow(),Mouse,At); Slate.OnMouseUp(Mouse,At);
    };
    const auto Initial=Chart(); const auto Cursor=Initial.Position+Initial.Size*FVector2D(.67,.36); const auto Anchor=Initial.WorldAt(Cursor);
    Slate.OnMouseWheel(1,Screen(Cursor));
    Check(P->MapSpan<Initial.Span&&Chart().WorldAt(Cursor).Equals(Anchor,1.),TEXT("Wheel input zooms the actual map around the mouse pointer"));
    const FVector2D Shift(83,47),Start=Cursor+FVector2D(42,-10); const auto Before=P->MapCenter; const double Zoom=Chart().Scale();
    Slate.OnMouseDown(Window->GetNativeWindow(),EMouseButtons::Left,Screen(Start));
    Slate.ProcessMouseMoveEvent(FPointerEvent(Slate.GetUserIndexForMouse(),FSlateApplication::CursorPointerIndex,Screen(Start+Shift),Screen(Start),Left,EKeys::Invalid,0,Mod));
    Slate.OnMouseUp(EMouseButtons::Left,Screen(Start+Shift));
    Check(P->MapCenter.Equals(Before-Shift/Zoom,1.)&&State->MapPins.IsEmpty(),TEXT("A left-button drag pans the map without placing a pin"));
    const auto Mark=Chart().WorldAt(Cursor); Click(EKeys::RightMouseButton,Cursor);
    Click(EKeys::RightMouseButton,Cursor+FVector2D(105,60));
    Check(State->MapPins.Num()==2&&State->TrackedPin==1&&State->MapPins[0].Equals(Mark,1.),TEXT("Two right-clicks create distinct map waypoints"));
    auto Key=[&](FKey K){FKeyEvent E(K,Mod,0u,false,0,0);Slate.ProcessKeyDownEvent(E);Slate.ProcessKeyUpEvent(E);};
    Key(EKeys::Delete); Key(EKeys::C);
    const auto Here=FVector2D(P->GetActorLocation()),Refuge=ETNav::Refuge(ETNav::District(Here));
    Check(State->TrackedPin==INDEX_NONE&&State->MapPins.Num()==1&&Chart().Inside(Chart().Plot(Here))&&Chart().Inside(Chart().Plot(Refuge)),TEXT("Delete removes the selected pin and C frames the nearest city with the survivor"));
    Key(EKeys::Home); Key(EKeys::Subtract); Key(EKeys::Subtract);
    Check(P->MapCenter.Equals(Here,.01),TEXT("Home recenters the large chart on the player"));
    if(State->MapPins.Num()) Click(EKeys::LeftMouseButton,Chart().Plot(State->MapPins[0]));
    Check(State->TrackedPin==0&&State->MapPins.Num()==1,TEXT("Clicking a saved marker restores its compass bearing"));
    Key(EKeys::L);
    const int PriorPins=State->MapPins.Num();
    const auto Venue=ETRailDistrict::Lot(0,1);
    Click(EKeys::LeftMouseButton,Chart().Plot(Venue));
    Check(State->MapPins.Num()==PriorPins+1&&State->MapPins.IsValidIndex(State->TrackedPin)&&State->MapPins[State->TrackedPin].Equals(Venue,1.),TEXT("L frames the local station and clicking a shop sets its waypoint"));
    Key(EKeys::Delete);Key(EKeys::R);
    bool AllVisible=P->MapCenter.Equals(ETRing::Center+ETRing::EastOffset()*.5,1.);
    for(int I=0;I<ETRing::Stops*2;++I) AllVisible&=Chart().Inside(Chart().Plot(ETRing::CityStation(I)));
    Check(AllVisible,TEXT("R frames both loops and sixteen station districts from the actual atlas"));
    Click(EKeys::LeftMouseButton,Chart().Plot(ETRing::Station(4)));
    Check(State->MapPins.IsValidIndex(State->TrackedPin)&&State->MapPins[State->TrackedPin].Equals(ETRing::Station(4),1.),TEXT("Clicking a rail stop tracks its boarding platform"));
}
