#include "ETRingLine.h"
#include "ETRailDistricts.h"
#include "ETGame.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"

// An isolated packaged survivor drives the ordinary movement, interaction and save entry points.
void AETRingLine::TestInterchange(AETCharacter& P)
{
    using namespace ETRailDistrict;
    static const bool Doors=FParse::Param(FCommandLine::Get(),TEXT("ETDoors"));
    static const bool Links=FParse::Param(FCommandLine::Get(),TEXT("ETShuttles"));
    static const bool Resume=FParse::Param(FCommandLine::Get(),TEXT("ETTransitReturn"));
    static int Stage=0,Leg=0;
    static int Place=[] {int Start=0;FParse::Value(FCommandLine::Get(),TEXT("ETStartVenue="),Start);return FMath::Clamp(Start,0,31);}();
    static double Since=0,Peak=0;
    static bool Left=false,Checkpoint=false;
    static FString Log;
    // Loading or suspending the window can advance the wall clock without moving a character.
    const double Now=GetWorld()->GetTimeSeconds();
    const FString Folder=FPaths::ProjectSavedDir();
    auto Write=[&](FString S){Log+=S+TEXT("\n");FFileHelper::SaveStringToFile(Log,*(Folder/TEXT("transit.txt")));};
    auto End=[&](bool OK,FString S){Write(FString(OK?TEXT("PASS "):TEXT("FAIL "))+S);FPlatformMisc::RequestExitWithStatus(true,OK?0:1);};
    auto Next=[&](int N){Stage=N;Since=Now;};
    if(P.Panel==TEXT("title")) {P.Action(Resume?TEXT("continue"):TEXT("new"));return;}
    if(P.Panel==TEXT("arrival")) P.FinishArrival();
    if(!P.Panel.IsEmpty()) {P.ClosePanel();return;}
    if(!Since) Since=Now;
    if(Now-Since>(Stage==23?85.:37.)) {
        const auto At=P.GetActorLocation();
        const FCollisionQueryParams Query(SCENE_QUERY_STAT(ETTransitBlocked),false,&P);
        FHitResult Sight;
        const auto Eye=P.Camera->GetComponentLocation();
        if(GetWorld()->LineTraceSingleByChannel(Sight,Eye,Eye+P.Camera->GetForwardVector()*420,ECC_Visibility,Query)) {
            const auto* Component=Cast<UStaticMeshComponent>(Sight.GetComponent());
            Write(FString::Printf(TEXT("Sight hit %s at %s from %s"),Component?*GetNameSafe(Component->GetStaticMesh()):*GetNameSafe(Sight.GetComponent()),*Sight.ImpactPoint.ToString(),*Eye.ToString()));
        }
        if(Doors) for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Id==CacheId(Place/2,Place%2)) {
            Write(FString::Printf(TEXT("Stock centre %s, camera rotation %s, wanted %s"),*It->Mesh->Bounds.Origin.ToString(),*P.Camera->GetComponentRotation().ToString(),*(It->Mesh->Bounds.Origin-Eye).Rotation().ToString()));break;
        }
        for(const FVector Dir:{FVector(0,-1,0),FVector(0,1,0),FVector(1,0,0),FVector(-1,0,0)}) {
            FHitResult Hit;
            if(GetWorld()->SweepSingleByChannel(Hit,At,At+Dir*360,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(33,86),Query)) {
                const auto* Component=Cast<UStaticMeshComponent>(Hit.GetComponent());
                Write(FString::Printf(TEXT("Blocking %s %s at %s, actor %s"),*Dir.ToString(),Component?*GetNameSafe(Component->GetStaticMesh()):*GetNameSafe(Hit.GetComponent()),*Hit.ImpactPoint.ToString(),Hit.GetActor()?*Hit.GetActor()->GetTransform().ToString():TEXT("-")));
            }
        }
        End(false,FString::Printf(TEXT("stage %d, venue %d, leg %d, pos %s, base %s, focus %s"),Stage,Place,Leg,*P.GetActorLocation().ToString(),*GetNameSafe(P.GetMovementBaseObject()),P.Focus?*P.Focus->Id:TEXT("-")));return;
    }
    P.TestInvincible=true;
    Zone->WeatherOverride=ETClimate::Clear;
    auto* Move=P.GetCharacterMovement(); const auto Here=P.GetActorLocation();
    auto SetUp=[&](FVector Location){
        Zone->StreamAround(Location,true);P.SetBase(static_cast<UPrimitiveComponent*>(nullptr));Move->StopMovementImmediately();
        Move->SetMovementMode(MOVE_Falling);P.SetActorLocation(Location,false,nullptr,ETeleportType::TeleportPhysics);
    };
    auto Walk=[&](FVector2D Target,double Near=36.) {
        const auto D=Target-FVector2D(P.GetActorLocation());
        if(D.Size()<=Near){Move->StopMovementImmediately();return true;}
        P.AddMovementInput(FVector(D.GetSafeNormal(),0),FMath::Clamp(D.Size()/145.,.22,1.));return false;
    };
    auto Look=[&](FVector At){P.GetController()->SetControlRotation((At-P.Camera->GetComponentLocation()).Rotation());};
    auto InsideStock=[&](int City,int Venue) {
        const auto Target=Cache(City,Venue);const auto Id=CacheId(City,Venue);
        static TWeakObjectPtr<AETItem> Stock;
        if(!Stock.IsValid()||Stock->Id!=Id) {
            Stock.Reset();
            for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Id==Id){Stock=*It;break;}
        }
        if(!Stock.IsValid()) return false;
        Look(Stock->Mesh->Bounds.Origin);
        Walk(Target+FVector2D(0,170),30);
        if(Here.Y>Lot(City,Venue).Y+Front(Get(City).Places[Venue].Shell)-95||!P.Focus||P.Focus->Id!=Id) return false;
        const int Before=Zone->State->Scrap;P.Interact();
        return Zone->State->Consumed.Contains(Id)&&Zone->State->Scrap>Before;
    };
    if(Resume) {
        if(Doors) {
            for(int I=0;I<32;++I) if(!Zone->State->Consumed.Contains(CacheId(I/2,I%2))) {End(false,TEXT("searched shop missing from checkpoint"));return;}
            for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Id.StartsWith(TEXT("rail-quarter/"))) {End(false,TEXT("searched stock returned on Continue"));return;}
            End(true,TEXT("Continue retained all 32 searched buildings without respawning their stock"));return;
        }
        static FVector Start=Here;
        if(Now-Since>3) {
            const auto* Train=PassengerLine(P);
            End(Train&&Train->Service==ETRing::SouthLink&&Train->TravelDirection().X<0&&FVector::Dist2D(Start,Here)>7000,
                TEXT("moving westbound South Shuttle save resumed on its original service and deck"));
        }
        return;
    }
    if(Doors) {
        const int City=Place/2,Venue=Place%2;
        auto* LocalRing=Zone->RailLines[City/ETRing::Stops];
        const int Stop=City%ETRing::Stops;
        const auto PlatformView=LocalRing->Platform(Stop);
        const auto Side=PlatformView.GetRotation().GetAxisY();
        const auto L=PlatformView.InverseTransformPosition(Here);
        if(Stage==0&&Venue==0) {
            Zone->State->Towns.FindOrAdd(ETNav::District(ETRing::CityStation(City))).Claimed=true;
            SetUp(PlatformView.TransformPosition(FVector(0,608,0))+FVector(0,0,97));
            LocalRing->Clock()=ETRing::DockTime(LocalRing->Service,(Stop+1)%ETRing::Stops);
            LocalRing->PositionCar(LocalRing->Clock());
            Next(30);return;
        }
        if(Stage==30) {
            const auto StreetView=ETRing::CityStation(City)+FVector2D(100.,-3300.);
            Look(FVector(StreetView,Zone->Height(StreetView.X,StreetView.Y)+800));
            if(Now-Since>1.3&&Move->IsMovingOnGround()) {
                P.Interact();
                if(!LocalRing->Phase.Docked||LocalRing->Phase.Stop!=Stop){End(false,TEXT("station did not call its own ring"));return;}
                Next(32);
            }
            return;
        }
        if(Stage==32) {P.AddMovementInput(-Side);if(L.Y<486){P.JumpPress();Next(33);}return;}
        if(Stage==33) {
            if(Now-Since>.22) P.JumpRelease();
            if(L.Y>55) P.AddMovementInput(-Side);
            else if(LocalRing->Riding(P)){Move->StopMovementImmediately();Next(34);}return;
        }
        if(Stage==34) {P.AddMovementInput(Side);if(L.Y>215&&LocalRing->Riding(P)){P.JumpPress();Next(35);}return;}
        if(Stage==35) {
            if(Now-Since>.22) P.JumpRelease();
            if(L.Y<760) P.AddMovementInput(Side);
            else if(Move->IsMovingOnGround()&&!LocalRing->Riding(P)){Move->StopMovementImmediately();P.Interact();Next(36);}return;
        }
        if(Stage==36) {
            if(L.Z>-3150){End(false,TEXT("station street exit did not reach the ground"));return;}
            Write(FString::Printf(TEXT("Boarded, jumped off and reached the street at %s"),ETRing::CityName(City)));
            Next((City==1||City==3||City==11||City==15)?37:38);return;
        }
        if(Stage==37) {
            const auto Landmark=ETRing::CityStation(City)+FVector2D(1250,-4370);
            Look(FVector(Landmark,Zone->Height(Landmark.X,Landmark.Y)+1150));
            if(Now-Since>.9){FScreenshotRequest::RequestScreenshot(Folder/FString::Printf(TEXT("station-%02d.png"),City),true,false);Next(38);}
            return;
        }
        if(Stage==0||(Stage==38&&Now-Since>.2)) {
            const auto XY=Lot(City,Venue)+FVector2D(0,Front(Get(City).Places[Venue].Shell)+235);
            SetUp(FVector(XY,Zone->Height(XY.X,XY.Y)+110));Next(1);return;
        }
        if(Stage==1&&InsideStock(City,Venue)) {
            Write(FString::Printf(TEXT("Walked in and looted %s / %s"),ETRing::CityName(City),Get(City).Places[Venue].Name));
            if(++Place==32){End(Zone->Save(),TEXT("walked through all 32 doors and collected their stocked inventory"));return;}
            Next(0);
        }
        return;
    }
    if(Links) {
        const int LineId=Leg<2?ETRing::NorthLink:ETRing::SouthLink;
        auto* Line=Zone->RailLines[LineId];const int From=Leg%2;
        const auto Home=Line->Platform(From);
        const auto D=Home.GetRotation().GetAxisY();
        const auto Local=Home.InverseTransformPosition(Here);
        if(Stage==0) {
            const auto RingDock=Zone->RailLines[ETRing::West]->Platform(ETRing::CityAt(LineId,0));
            SetUp(RingDock.TransformPosition(FVector(0,766,0))+FVector(0,0,96));
            Line->Clock()=ETRing::DockTime(LineId,1-From);Line->PositionCar(Line->Clock());
            P.Interact();Next(16);return;
        }
        if(Stage>=16&&Stage<=18) {
            const auto Foot=FVector2D(Zone->RailLines[ETRing::West]->StreetLift(ETRing::CityAt(LineId,0)));
            const auto Lift=FVector2D(Line->StreetLift(From));
            const double StreetY=Lift.Y+115;
            if(Stage==16&&Walk({Foot.X,StreetY})) Next(17);
            else if(Stage==17&&Walk({Lift.X,StreetY})) Next(18);
            else if(Stage==18&&Walk(Lift)) {
                P.Interact();P.Interact();
                Write(FString::Printf(TEXT("Walked the street transfer from %s to %s"),ETRing::CityName(ETRing::CityAt(LineId,0)),ETRing::ServiceName(LineId)));
                Next(20);
            }
            return;
        }
        if(Stage==20) {
            if(!Line->Phase.Docked||Line->Phase.Stop!=From){End(false,TEXT("called wrong shuttle"));return;}
            if(!Move->IsMovingOnGround()) return;
            P.AddMovementInput(-D);
            if(Local.Y<490){P.JumpPress();Next(21);}return;
        }
        if(Stage==21) {
            if(Now-Since>.23) P.JumpRelease();
            if(Local.Y>45) P.AddMovementInput(-D);
            else if(Line->Riding(P)){Move->StopMovementImmediately();Peak=0;Left=false;Next(23);}return;
        }
        if(Stage==23) {
            if(!Line->Riding(P)){
                const auto Offset=Line->Deck->GetComponentTransform().InverseTransformPosition(Here);
                End(false,FString::Printf(TEXT("lost %s on leg %d at %.2f s / %.1f km/h / deck offset %s / floor %s / velocity %s"),
                    ETRing::ServiceName(LineId),Leg,Line->Clock(),Line->Phase.Speed*.036,*Offset.ToString(),*GetNameSafe(P.GetMovementBaseObject()),*P.GetVelocity().ToString()));return;
            }
            if(!Line->Phase.Docked) {
                if(Line->TravelDirection().X!=(From?-1.:1.)){End(false,TEXT("shuttle travelled in wrong direction"));return;}
                Left=true;Peak=FMath::Max(Peak,Line->Phase.Speed);
                if(Leg==3&&Peak>=ETRing::Cruise*.99&&!Checkpoint) {
                    const FString Slot=Folder/TEXT("SaveGames")/(Zone->SaveSlot+TEXT(".sav"));
                    if(!Zone->Save()||IFileManager::Get().Copy(*(Folder/TEXT("link-checkpoint.sav")),*Slot)!=COPY_OK){End(false,TEXT("could not keep the mid-shuttle checkpoint"));return;}
                    Checkpoint=true;
                }
            } else if(Left&&Line->Phase.Stop==1-From) {
                if(Peak<ETRing::Cruise*.99){End(false,TEXT("shuttle did not reach full speed"));return;}
                Write(FString::Printf(TEXT("%s reached %s at %.0f km/h"),ETRing::ServiceName(LineId),ETRing::CityName(ETRing::CityAt(LineId,1-From)),Peak*.036));Next(24);
            }return;
        }
        const auto Arrival=Line->Platform(1-From);
        const auto L=Arrival.InverseTransformPosition(Here);
        if(Stage==24) {P.AddMovementInput(D);if(L.Y>215&&Line->Riding(P)){P.JumpPress();Next(25);}return;}
        if(Stage==25) {
            if(Now-Since>.22) P.JumpRelease();
            if(L.Y<600) P.AddMovementInput(D);
            else if(!Line->Riding(P)&&Move->IsMovingOnGround()) {
                Move->StopMovementImmediately();
                if(++Leg==4){End(true,TEXT("jumped off both shuttles after riding each in both directions"));return;}
                if(Leg%2){P.Interact();Next(20);}else Next(0);
            }
        }
        return;
    }
    // A continuous excursion at Windfall. Only the initial incoming passenger is placed by the fixture.
    constexpr int City=8,Station=0;
    auto* Ring=Zone->RailLines[ETRing::East];const auto Home=Ring->Platform(Station);
    const auto Side=Home.GetRotation().GetAxisY(),L=Home.InverseTransformPosition(Here);
    const auto Street=FVector2D(Ring->StreetLift(Station));
    const auto LotXY=Lot(City,0);
    const auto Lane=FVector2D(LotXY.X,Street.Y+80);
    if(Stage==0) {
        Ring->Clock()=ETRing::Period(Ring->Service)-1.2;Ring->PositionCar(Ring->Clock());
        SetUp(Ring->Deck->GetComponentLocation()+FVector(0,0,124));Move->SetMovementMode(MOVE_Walking);P.SetBase(Ring->Deck);
        Next(1);return;
    }
    if(Stage==1) {if(Ring->Phase.Docked&&Ring->Phase.Stop==Station){P.AddMovementInput(Side);if(L.Y>215){P.JumpPress();Next(2);}}return;}
    if(Stage==2) {
        if(Now-Since>.22) P.JumpRelease();
        if(L.Y<765) P.AddMovementInput(Side);
        else if(Move->IsMovingOnGround()&&!Ring->Riding(P)){Move->StopMovementImmediately();P.Interact();Next(3);}return;
    }
    if(Stage==3) {
        if(L.Z>-3200){End(false,TEXT("street lift did not descend"));return;}
        if(Walk({Street.X,Lane.Y})){Write(TEXT("Jumped off the East Ring and took the lift down at Windfall Reach"));Next(4);}return;
    }
    if(Stage==4) {if(Walk(Lane)) Next(5);return;}
    if(Stage==5) {
        Look(FVector(LotXY,Zone->Height(LotXY.X,LotXY.Y)+400));
        if(Walk(LotXY+FVector2D(0,1030))) {FScreenshotRequest::RequestScreenshot(Folder/TEXT("windfall.png"),true,false);Next(6);}return;
    }
    if(Stage==6) {if(InsideStock(City,0)){Write(TEXT("Walked from the platform into Blade Repair Hall and collected the turbine service kit"));Next(7);}return;}
    if(Stage==7) {if(Walk(Lane)) Next(8);return;}
    if(Stage==8) {if(Walk({Street.X,Lane.Y})) Next(9);return;}
    if(Stage==9) {if(Walk(Street)) {P.Interact();Next(10);}return;}
    if(Stage==10) {
        if(L.Z<0){End(false,TEXT("return lift did not reach the platform"));return;}
        if(Ring->Phase.Docked&&Ring->Phase.Stop==Station){End(false,TEXT("first departure never left during scavenging"));return;}
        P.Interact();if(!Ring->Phase.Docked||Ring->Phase.Stop!=Station){End(false,TEXT("wait failed to bring the next ring train"));return;}
        Next(11);return;
    }
    if(Stage==11) {if(!Move->IsMovingOnGround())return;P.AddMovementInput(-Side);if(L.Y<480){P.JumpPress();Next(12);}return;}
    if(Stage==12) {
        if(Now-Since>.22) P.JumpRelease();
        if(L.Y>40)P.AddMovementInput(-Side);
        else if(Ring->Riding(P)){Move->StopMovementImmediately();Next(13);}return;
    }
    if(Stage==13&&Ring->Riding(P)&&Ring->Phase.Speed>ETRing::Cruise*.85)
        End(true,TEXT("returned from the shop, waited for a later train, jumped aboard and departed Windfall"));
}
