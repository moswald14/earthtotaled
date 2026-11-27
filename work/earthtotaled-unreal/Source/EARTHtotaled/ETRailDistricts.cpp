#include "ETRailDistricts.h"
#include "ETGame.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "ProceduralMeshComponent.h"

void AETRegion::AddRailQuarter()
{
    using namespace ETRailDistrict;
    const FVector2D Corner(Coord.X*AETWorld::RegionSize,Coord.Y*AETWorld::RegionSize);
    const int City=ETRing::CityIn(ETNav::District(Corner+FVector2D(AETWorld::RegionSize*.5)));
    if(City==INDEX_NONE) return;
    const auto& Q=Get(City);
    auto Owns=[&](FVector2D P) {
        return FMath::FloorToInt32(P.X/AETWorld::RegionSize)==Coord.X&&FMath::FloorToInt32(P.Y/AETWorld::RegionSize)==Coord.Y;
    };
    auto Ground=[&](FVector2D XY){return FVector(XY,World->Height(XY.X,XY.Y));};
    auto Put=[&](FString Asset,const TCHAR* Material,FVector P,double Yaw=0.,FVector Scale=FVector::OneVector) {
        auto* Actor=World->Static(Asset,Material,P,Scale,FRotator(0,Yaw,0));
        Residents.Add(Actor);
        return Actor;
    };
    auto Letter=[&](AActor* Backing,FString Label,FVector P,double Yaw,int Size) {
        auto* Sign=NewObject<UTextRenderComponent>(Backing);
        Sign->SetupAttachment(Backing->GetRootComponent());Sign->RegisterComponent();
        Sign->SetWorldLocationAndRotation(P,FRotator(0,Yaw,0));
        Sign->SetHorizontalAlignment(EHTA_Center);Sign->SetVerticalAlignment(EVRTA_TextCenter);
        Sign->SetTextRenderColor(Q.Color);Sign->SetWorldSize(Size);
        Sign->SetText(FText::FromString(Label));Sign->SetCullDistance(12000);
    };
    for(int I=0;I<2;++I) {
        const auto XY=Lot(City,I);
        if(!Owns(XY)) continue;
        const auto& V=Q.Places[I];
        // The meshes include a 32 cm foundation slab. Bring its walkable top to the sidewalk.
        const auto Pos=Ground(XY)-FVector(0,0,14);
        const double FrontY=Front(V.Shell);
        const TCHAR* Mesh=V.Shell==EShell::Hall?TEXT("SM_Warehouse"):V.Shell==EShell::House?TEXT("SM_House"):V.Shell==EShell::Flats?TEXT("SM_Tenement"):TEXT("SM_Shop");
        auto* Shell=Put(Mesh,TEXT("Concrete"),Pos,180.);
        if(V.Shell==EShell::House) Put(TEXT("SM_Roof"),TEXT("Rust"),Pos,180.);
        auto* Fascia=Put(TEXT("SM_Block"),TEXT("DarkMetal"),Pos+FVector(0,FrontY+62,295),0,FVector(7.2,.17,.79));
        Letter(Fascia,FString(V.Name).ToUpper(),Pos+FVector(0,FrontY+75,296),90,38);
        const FString Stock=V.Stock;
        // The centre aisle stays wide enough for a walking survivor from the street to the stock.
        const double Side=V.Shell==EShell::Hall?840.:410.;
        if(Stock==TEXT("medical")) {
            for(int N:{-1,1}) Put(TEXT("SM_Bench"),TEXT("Steel"),Pos+FVector(N*Side,-70,20),90);
            Put(TEXT("SM_Shelves"),TEXT("Steel"),Pos+FVector(-Side,-310,22));
        } else if(Stock==TEXT("electronics")) {
            for(int N:{-1,1}) Put(TEXT("SM_Shelves"),TEXT("Steel"),Pos+FVector(N*Side,-180,18),90);
            Put(TEXT("SM_Relay"),TEXT("Steel"),Pos+FVector(Side,185,17),0,FVector(.34));
        } else if(Stock==TEXT("pantry")||Stock==TEXT("relief")) {
            Put(TEXT("SM_Shelves"),TEXT("Wood"),Pos+FVector(Side,-175,18),90);
            Put(TEXT("SM_Barrel"),TEXT("Greenline"),Pos+FVector(-Side,-260,18));
            Put(TEXT("SM_Bench"),TEXT("Wood"),Pos+FVector(-Side,100,16),90);
        } else {
            Put(TEXT("SM_Shelves"),TEXT("Steel"),Pos+FVector(-Side,-190,18),90);
            Put(TEXT("SM_Barrel"),TEXT("Rust"),Pos+FVector(Side,-210,20));
        }
        auto* Light=NewObject<UPointLightComponent>(Shell);
        Light->SetupAttachment(Shell->GetRootComponent());Light->RegisterComponent();
        Light->SetWorldLocation(Pos+FVector(0,-60,280));Light->SetLightColor(FLinearColor(Q.Color));
        Light->SetIntensity(2300);Light->SetAttenuationRadius(730);Light->SetCastShadows(false);Light->SetMaxDrawDistance(9000);
        const FVector Counter(Cache(City,I),Pos.Z+16);
        Put(TEXT("SM_Bench"),TEXT("Wood"),Counter,180,FVector(1.1,1.24,1.35));
        if(auto* Stockpile=World->Item(CacheId(City,I),TEXT("container"),FString(TEXT("Search "))+V.Contents,Counter+FVector(0,0,72),0,V.Stock)) Residents.Add(Stockpile);
    }
    const auto Stop=ETRing::CityStation(City);
    const auto Monument=Stop+FVector2D(City==0?-1650.:1250.,-4370.);
    if(Owns(Monument)) {
        Put(FString(TEXT("SM_Quarter"))+Q.Landmark,TEXT("Steel"),Ground(Monument),City%2?7.:-5.);
    }
    // A street-level sign ties the shops to their boarding point even when the train has left.
    const auto Board=Stop+FVector2D(150,-365);
    if(Owns(Board)) {
        const auto P=Ground(Board);
        auto* Stand=Put(TEXT("SM_Sign"),TEXT("Greenline"),P);
        const int Link=ETRing::Connection(City);
        FString Directions=FString(Q.Yard).ToUpper()+TEXT("\n")+Q.Places[0].Name+TEXT("  /  ")+Q.Places[1].Name;
        if(Link!=INDEX_NONE) Directions+=FString::Printf(TEXT("\n%s  /  68 m %s"),ETRing::ServiceName(Link),City<8?TEXT("east"):TEXT("west"));
        Letter(Stand,Directions,P+FVector(0,-20,330),-90,26);
        Letter(Stand,Directions,P+FVector(0,20,330),90,26);
    }
    if(City==0) return; // The original shelter streets have their own placed landmarks.
    const auto& Style=Street(City);
    const FVector2D Town=FVector2D(ETNav::District(Stop))*ETNav::DistrictSpacing;
    TArray<FTransform> StreetFixtures;
    for(int Row=-3;Row<=3;++Row) for(int Col=-3;Col<=3;++Col) {
        if((3*Row+Col+City)%Style.Spacing!=0||(Col==0&&Row==-2)) continue;
        const FVector2D Yard=Town+FVector2D(Col*6400.,Row*6400.+1630.);
        if(!Owns(Yard)) continue;
        const double Turn=City==8||City==9?25.:((Col%2)?180.:0.);
        StreetFixtures.Emplace(FRotator(0,Turn,0),Ground(Yard),FVector(Style.Scale));
    }
    if(!StreetFixtures.IsEmpty()) {
        auto* H=NewObject<UHierarchicalInstancedStaticMeshComponent>(this);
        H->SetupAttachment(Terrain);
        const FString Asset=FString(TEXT("/Game/EARTHtotaled/Art/SM_"))+Style.Fixture;
        H->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Asset));World->DressMesh(H,Style.Material);
        H->SetCollisionProfileName(TEXT("BlockAll"));H->RegisterComponent();
        H->AddInstances(StreetFixtures,false,true,false);Instances.Add(H);
    }
}
