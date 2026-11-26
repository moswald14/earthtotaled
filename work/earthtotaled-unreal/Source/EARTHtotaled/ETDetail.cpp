#include "ETGame.h"
#include "Engine/World.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "ProceduralMeshComponent.h"

void AETRegion::AddDetails()
{
    auto W=World; const auto C=Coord;
    auto Put=[&](const TCHAR* Mesh,const TCHAR* Material,FVector P,FVector Scale=FVector::OneVector,FRotator Rotation=FRotator::ZeroRotator) {
        auto A=W->Static(Mesh,Material,P,Scale,Rotation); A->SetOwner(this); Residents.Add(A); return A;
    };
    auto Cache=[&](FString Id,FString Table,FVector P,FString Label) {
        if(auto A=W->Item(Id,TEXT("container"),Label,P,0,Table)) {A->SetOwner(this); Residents.Add(A);}
    };
    auto Sign=[&](FString Label,FVector P,bool Existing=false) {
        AStaticMeshActor* Stand=nullptr;
        if(Existing) for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It) {
            if(!It->GetActorLocation().Equals(P,1)) continue;
            const auto* Mesh=It->GetStaticMeshComponent()->GetStaticMesh().Get();
            if(Mesh&&Mesh->GetFName()==TEXT("SM_Sign")) {Stand=*It;break;}
        }
        if(!Stand) Stand=Cast<AStaticMeshActor>(Put(TEXT("SM_Sign"),TEXT("Greenline"),P));
        auto A=GetWorld()->SpawnActor<AActor>(); A->SetOwner(this);
        auto T=NewObject<UTextRenderComponent>(A); A->SetRootComponent(T); T->RegisterComponent();
        T->SetText(FText::FromString(Label)); T->SetHorizontalAlignment(EHTA_Center); T->SetVerticalAlignment(EVRTA_TextCenter);
        T->SetWorldSize(46); T->SetTextRenderColor(FColor(230,240,210));
        T->SetWorldLocation(P+FVector(0,-15,330)); T->SetWorldRotation(FRotator(0,-90,0)); T->SetCullDistance(12000);
        if(Stand) {A->AttachToActor(Stand,FAttachmentTransformRules::KeepWorldTransform); T->SetVisibility(Stand->GetStaticMeshComponent()->IsVisible());}
        Residents.Add(A);
    };
    auto Belongs=[&](FVector P) {return FMath::FloorToInt(P.X/AETWorld::RegionSize)==C.X&&FMath::FloorToInt(P.Y/AETWorld::RegionSize)==C.Y;};
    const FVector Origin(C.X*AETWorld::RegionSize,C.Y*AETWorld::RegionSize,0);
    // Decorative clusters share components and never add invisible collision to travel routes.
    // Separate randomness leaves existing caches, animals and story placements unchanged.
    FRandomStream Decor(W->SeedFor(C,1300));
    UHierarchicalInstancedStaticMeshComponent* TreeTrunks=nullptr;
    for(auto* H:Instances) if(H->GetStaticMesh()&&H->GetStaticMesh()->GetFName()==TEXT("SM_Trunk")) {TreeTrunks=H; break;}
    TMap<FString,UHierarchicalInstancedStaticMeshComponent*> DetailMeshes;
    auto Detail=[&](const TCHAR* Asset,const TCHAR* Material,FVector P,FVector Scale,float Yaw,const TCHAR* Range) {
        if(OnHighway(FVector2D(P),120)||InHighwayGrounds(FVector2D(P))) return;
        UHierarchicalInstancedStaticMeshComponent* H=DetailMeshes.FindRef(Asset);
        if(!H) {
            H=NewObject<UHierarchicalInstancedStaticMeshComponent>(this); H->SetupAttachment(Terrain);
            const FString Name=Asset;
            H->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Name+TEXT(".")+Name)));
            W->DressMesh(H,Material); H->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            H->ComponentTags.Add(FName(Range));
            const float Limit=FString(Range)==TEXT("ET_DetailNear")?14000.f:FString(Range)==TEXT("ET_DetailMid")?24000.f:35000.f;
            const float End=FMath::Min(Limit,W->State->ViewDistance*100.f); H->SetCullDistances(End*.75f,End);
            H->SetCastShadow(Name!=TEXT("SM_Fern")&&Name!=TEXT("SM_Shrub"));
            H->RegisterComponent(); Instances.Add(H); DetailMeshes.Add(Name,H);
        }
        const FTransform Transform(FRotator(0,Yaw,0),P-Origin,Scale);
        if(W->IsTree(FName(Asset))) {
            if(W->State->Consumed.Contains(W->TreeId(P))) return;
            if(TreeTrunks) TreeTrunks->AddInstance(Transform);
        }
        H->AddInstance(Transform);
    };
    auto ForestClear=[&](FVector P) {
        if(!Belongs(P)||W->InCity(P)) return false;
        // Include neighboring homesteads: a cluster crossing a tile edge must not fill a door or campsite.
        for(int32 DY=-1;DY<=1;++DY) for(int32 DX=-1;DX<=1;++DX) for(int32 I=0;I<2;++I)
            if(FVector::DistSquared2D(P,W->HouseAnchor(C+FIntPoint(DX,DY),I))<FMath::Square(2850.f)) return false;
        return true;
    };
    // Patches, gaps and decaying timber give the forest a floor rather than a uniform tree grid.
    for(int32 Patch=0;Patch<36;++Patch) {
        FVector Center=Origin+FVector(Decor.FRandRange(1100,30900),Decor.FRandRange(1100,30900),0);
        if(!ForestClear(Center)) continue;
        for(int32 I=0;I<16;++I) {
            const float A=Decor.FRandRange(0,2*PI), Radius=FMath::Sqrt(Decor.FRand())*850;
            FVector P=Center+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0); if(!ForestClear(P)) continue;
            P.Z=W->Height(P.X,P.Y)-2;
            const bool Shrub=I>=12; const float S=Decor.FRandRange(Shrub?.38f:.5f,Shrub?.82f:1.12f);
            Detail(Shrub?TEXT("SM_Shrub"):TEXT("SM_Fern"),TEXT("Grass"),P,FVector(S,S,S*Decor.FRandRange(.8f,1.2f)),Decor.FRandRange(0,360),TEXT("ET_DetailNear"));
        }
        if(Patch%3==0) {
            Center.Z=W->Height(Center.X,Center.Y)-6;
            const bool Log=Patch%2==0;
            const float S=Decor.FRandRange(.7f,1.15f);
            Detail(Log?TEXT("SM_FallenLog"):TEXT("SM_Stump"),TEXT("Bark"),Center,FVector(S),Decor.FRandRange(0,360),TEXT("ET_DetailMid"));
        }
        if(Patch%2==0) {
            FVector P=Center+FVector(Decor.FRandRange(-400,400),Decor.FRandRange(-400,400),0);
            if(ForestClear(P)) {P.Z=W->Height(P.X,P.Y)-4; const float S=Decor.FRandRange(.7f,1.1f); Detail(TEXT("SM_Birch"),TEXT("Bark"),P,FVector(S,S,Decor.FRandRange(.8f,1.15f)),Decor.FRandRange(0,360),TEXT("ET_DetailFar"));}
        }
    }
    if(C==FIntPoint::ZeroValue) {
        Sign(TEXT("SHELTER 09"),FVector(-700,-700,0),true);
        Sign(TEXT("RANGER SUPPLY"),FVector(-2800,-1500,0),true);
        Sign(TEXT("FIRST AID"),FVector(2800,-1500,0),true);
    }
    const FVector City=W->CityCenter(Origin.X+16000,Origin.Y+16000);
    if(City.Size2D()>1&&FVector::Dist2D(Origin+FVector(16000,16000,0),City)<52000) {
        for(int32 Y=-3;Y<=3;++Y) for(int32 X=-3;X<=3;++X) {
            const FVector P=City+FVector(X*6400,Y*6400,0); if(!Belongs(P)) continue;
            const FString Id=FString::Printf(TEXT("v11-city%d,%d-%d,%d"),int32(City.X/160000),int32(City.Y/160000),X,Y);
            Put(TEXT("SM_Lamp"),TEXT("Steel"),P+FVector(2600,-1000,0));
            Put(TEXT("SM_Barrel"),TEXT("Rust"),P+FVector(2300,900,0));
            // Rear courts and the edges of sidewalks reclaim the city, leaving entrances and fire escapes open.
            for(int32 I=0;I<6;++I) {
                FVector D=P+FVector(Decor.FRandRange(-2400,2400),Decor.FRandRange(1600,2450),0);
                D.Z=W->Height(D.X,D.Y)-3;
                const float S=Decor.FRandRange(.48f,.92f);
                Detail(TEXT("SM_Rubble"),TEXT("Concrete"),D,FVector(S,S,Decor.FRandRange(.5f,.9f)),Decor.FRandRange(0,360),TEXT("ET_DetailMid"));
            }
            for(int32 I=0;I<14;++I) {
                const float Side=I%2?1.f:-1.f;
                FVector D=P+FVector(Side*Decor.FRandRange(2450,2600),Decor.FRandRange(-650,2400),0);
                D.Z=W->Height(D.X,D.Y)-2;
                const bool Shrub=I%3==0; const float S=Decor.FRandRange(.35f,.7f);
                Detail(Shrub?TEXT("SM_Shrub"):TEXT("SM_Fern"),TEXT("Grass"),D,FVector(S),Decor.FRandRange(0,360),TEXT("ET_DetailNear"));
            }
            if((X+Y*3)%3==0&&!(X==0&&(Y==0||Y==1))) {
                FVector D=P+FVector(-1300,2050,0); D.Z=W->Height(D.X,D.Y)-5;
                Detail(TEXT("SM_WallRuin"),TEXT("Concrete"),D,FVector(.95f,.8f,Decor.FRandRange(.6f,1.1f)),Decor.FRandRange(-22,22),TEXT("ET_DetailFar"));
                Detail(TEXT("SM_Rebar"),TEXT("Rust"),D+FVector(150,10,90),FVector(.8f),Decor.FRandRange(0,360),TEXT("ET_DetailMid"));
                Detail(TEXT("SM_Rubble"),TEXT("Concrete"),D+FVector(100,-170,0),FVector(1.15f),Decor.FRandRange(0,360),TEXT("ET_DetailMid"));
            }
            if((X+Y)%4==0) {
                FVector D=P+FVector(1600,2400,0); D.Z=W->Height(D.X,D.Y)-3;
                Detail(TEXT("SM_Birch"),TEXT("Bark"),D,FVector(.65f,.65f,.8f),Decor.FRandRange(0,360),TEXT("ET_DetailFar"));
            }
            if(X==-1&&Y==0) {
                Put(TEXT("SM_Warehouse"),TEXT("Concrete"),P);
                for(int32 I=0;I<3;++I) {Put(TEXT("SM_Shelves"),TEXT("Steel"),P+FVector(-900+I*900,550,16)); Cache(Id+FString::FromInt(I),I==1?TEXT("electronics"):TEXT("tools"),P+FVector(-900+I*900,200,45),TEXT("Search depot stock"));}
                Sign(TEXT("SALVAGE DEPOT"),P+FVector(-1600,-700,0));
                if(auto A=W->Item(Id+TEXT("-gun"),TEXT("weapon"),TEXT("Open depot weapon case"),P+FVector(1000,500,55),7+W->SeedFor(C,91)%3)) Residents.Add(A);
                if(auto A=W->Item(Id+TEXT("-bench"),TEXT("bench"),TEXT("Depot workbench"),P+FVector(-1000,550,80))) Residents.Add(A);
            } else if(X==0&&Y==-1) {
                Put(TEXT("SM_Shop"),TEXT("Concrete"),P+FVector(-1500,0,0)); Put(TEXT("SM_Shop"),TEXT("Concrete"),P+FVector(1500,0,0));
                Sign(TEXT("MEDICAL AID"),P+FVector(-1500,-750,0)); Sign(TEXT("PROVISIONS"),P+FVector(1500,-750,0));
                Cache(Id+TEXT("-med"),TEXT("medical"),P+FVector(-1800,200,50),TEXT("Search clinic supplies"));
                Cache(Id+TEXT("-pantry"),TEXT("pantry"),P+FVector(1200,200,50),TEXT("Search grocery stock"));
                Put(TEXT("SM_Shelves"),TEXT("Wood"),P+FVector(1900,300,16));
            } else if(X==1&&Y==0) {
                Put(TEXT("SM_Canopy"),TEXT("Steel"),P+FVector(-1000,0,0)); Put(TEXT("SM_Shop"),TEXT("Concrete"),P+FVector(1500,0,0));
                for(int32 I=-1;I<=1;I+=2) Put(TEXT("SM_Pump"),TEXT("Rust"),P+FVector(-1000+I*450,0,0));
                Sign(TEXT("LAST STOP / FUEL"),P+FVector(1100,-750,0));
                Cache(Id+TEXT("-fuel"),TEXT("tools"),P+FVector(1200,250,45),TEXT("Search service tools"));
                Cache(Id+TEXT("-counter"),TEXT("pantry"),P+FVector(1700,250,45),TEXT("Search station counter"));
            } else if(X==0&&Y==1) {
                for(int32 I=-1;I<=1;I+=2) {
                    Put(TEXT("SM_CampShelter"),TEXT("Canvas"),P+FVector(I*900,100,0)); Put(TEXT("SM_Bench"),TEXT("Wood"),P+FVector(I*900,-300,0));
                    Cache(Id+FString::FromInt(I),TEXT("relief"),P+FVector(I*900,150,45),TEXT("Search relief pallet"));
                }
                Put(TEXT("SM_WaterTank"),TEXT("Greenline"),P+FVector(0,500,0)); Sign(TEXT("GREENLINE REFUGE"),P+FVector(0,-750,0));
                if(auto A=W->Item(Id+TEXT("-keeper"),TEXT("human"),TEXT("Human relief keeper"),P+FVector(-500,0,20),0,TEXT("Refuge Keeper"))) Residents.Add(A);
                if(auto A=W->Item(Id+TEXT("-friend"),TEXT("friend"),TEXT("Greenline field trader"),P+FVector(500,0,20),0,TEXT("SALT-7"))) Residents.Add(A);
            } else if(X==2&&Y==2) {
                Put(TEXT("SM_Tenement"),TEXT("Concrete"),P); Put(TEXT("SM_Interior"),TEXT("Wood"),P);
                Sign(TEXT("CIVIL DEFENSE"),P+FVector(-1200,-750,0));
                Cache(Id+TEXT("-police"),TEXT("armory"),P+FVector(-300,200,60),TEXT("Search defense armory"));
                if(auto A=W->Item(Id+TEXT("-rifle"),TEXT("weapon"),TEXT("Recover rooftop rifle"),P+FVector(-330,260,830),9)) Residents.Add(A);
            }
        }
    }
    // A reproducible campsite belongs to each wilderness house. Never place it in roads.
    for(int32 I=0;I<2;++I) {
        FVector Home=W->HouseAnchor(C,I); if(W->InCity(Home)||InHighwayGrounds(FVector2D(Home),700)||OnHighway(FVector2D(Home),750)) continue;
        FVector P=Home+FVector(1600,0,0); P.Z=W->Height(P.X,P.Y);
        const FString Id=FString::Printf(TEXT("v11-camp%d,%d-%d"),C.X,C.Y,I);
        Put(TEXT("SM_CampShelter"),TEXT("Canvas"),P); Put(TEXT("SM_Barrel"),TEXT("Rust"),P+FVector(400,120,0));
        Cache(Id,TEXT("pantry"),P+FVector(0,100,25),TEXT("Search woodland camp"));
        Cache(Id+TEXT("-parts"),TEXT("tools"),Home+FVector(350,-230,65),TEXT("Search homestead toolbox"));
        if(W->SeedFor(C,I+800)%5==0) {
            if(auto A=W->Item(Id+TEXT("-gun"),TEXT("weapon"),TEXT("Open ranger weapon case"),P+FVector(-120,30,40),6+W->SeedFor(C,I+801)%4)) Residents.Add(A);
        }
    }
}
