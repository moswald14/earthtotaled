#include "ETGame.h"
#include "ETGeology.h"
#include "ETWake.h"
#include "Engine/World.h"

bool AETRegion::DeerMeadow(int Group,FVector& Center) const
{
    FRandomStream Sites(World->SeedFor(Coord,4673+Group));
    const FVector Origin=GetActorLocation();
    const bool ShelterWoods=Coord==FIntPoint(-1,0)&&Group==0;
    for(int Attempt=0;Attempt<24;++Attempt) {
        // Two separate ranges per region put the herds within walking distance along a forest run.
        const double X=Sites.FRandRange(1400,14600)+Group*16000;
        const double Y=Sites.FRandRange(1400,30600);
        FVector Site=Origin+FVector(X,Y,0);
        if(ShelterWoods&&Attempt<12) {
            Site=Attempt==0?FVector(-29400,6200,0):FVector(-29600+Sites.FRandRange(-1250,1250),6900+Sites.FRandRange(-1450,1450),0);
        }
        const FVector2D XY(Site);
        if(FVector::DistSquared2D(Site,World->CityCenter(Site.X,Site.Y))<FMath::Square(28200.)||
           OnHighway(XY,1500)||InHighwayGrounds(XY,1700)||ETWake::Reserved(World->State->Wake,XY,1550))continue;
        if(FVector::DistSquared2D(Site,World->HouseAnchor(Coord,0))<FMath::Square(2400.)||
           FVector::DistSquared2D(Site,World->HouseAnchor(Coord,1))<FMath::Square(2400.))continue;
        bool Built=false;
        for(const auto& It:World->State->Builds)if(FVector::DistSquared2D(It.Value.Anchor,Site)<FMath::Square(1400.)){Built=true;break;}
        if(Built||World->Geology->SurfaceOpening(Site.X,Site.Y)<900)continue;
        const double Ground=World->SurfaceHeight(Site.X,Site.Y);
        if(FMath::Abs(World->SurfaceHeight(Site.X+350,Site.Y)-Ground)>160||
           FMath::Abs(World->SurfaceHeight(Site.X,Site.Y+350)-Ground)>160)continue;
        Center=FVector(XY,Ground);return true;
    }
    return false;
}

void AETRegion::AddDeerHerd(int Group)
{
    FVector Center;if(!DeerMeadow(Group,Center))return;
    FRandomStream Layout(World->SeedFor(Coord,8231+Group));
    const float Heading=Layout.FRandRange(0,360);
    const int Count=Layout.RandRange(3,4);
    const FString Lead=FString::Printf(TEXT("r%d,%d-agent%d"),Coord.X,Coord.Y,3+Group);
    for(int Member=0;Member<Count;++Member) {
        // Previous survivors may already have hunted, or wounded, this region's original deer.
        const FString Id=Member?Lead+FString::Printf(TEXT("/herd%d"),Member):Lead;
        if(World->State->Consumed.Contains(Id))continue;
        for(int Try=0;Try<6;++Try) {
            const auto Offset=FRotator(0,Heading+Member*360./Count+Try*8.,0).Vector()*(330.+(Member%2)*155.+Try*67.);
            auto Site=Center+Offset;Site.Z=World->SurfaceHeight(Site.X,Site.Y)+8;
            if(FMath::Abs(Site.Z-Center.Z)>270||World->Geology->SurfaceOpening(Site.X,Site.Y)<90)continue;
            // Trunks, boulders and built walls must leave space for the actual animal.
            if(GetWorld()->OverlapBlockingTestByChannel(Site+FVector(0,0,75),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeSphere(58)))continue;
            if(auto* Deer=GetWorld()->SpawnActor<AETAgent>(Site,FRotator(0,Heading+Member*21.,0))) {
                Deer->SetOwner(this);Deer->Configure(World,Id,TEXT("deer"));
                Deer->Phase=Member*2.7f+FMath::Fmod(Heading*.1f,9.f);
                Residents.Add(Deer);
            }
            break;
        }
    }
}
