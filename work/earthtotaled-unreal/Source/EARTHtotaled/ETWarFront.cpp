#include "ETGame.h"
#include "ETWarClock.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "EngineUtils.h"

FVector AETAgent::AimPoint() const
{
    return Parts.Num()?Parts[0]->Bounds.Origin:GetActorLocation()+FVector(0,0,55);
}

bool AETAgent::Opposes(const AETAgent* Other) const
{
    return IsValid(Other)&&Other!=this&&Other->Health>0&&Other->Awake()&&Other->GetActorEnableCollision()&&
        ((IsMachine()&&Other->IsBrood())||(IsBrood()&&Other->IsMachine()));
}

AETAgent* AETAgent::FindRival(float Delta)
{
    if(!IsMachine()&&!IsBrood()) return nullptr;
    RivalScan-=Delta;
    if(RivalScan>0&&(!Rival.IsValid()||Opposes(Rival.Get()))) return Rival.Get();
    RivalScan=.55f+float(GetTypeHash(Id)%23)*.01f;
    Rival.Reset();
    const auto Eye=AimPoint();
    double Nearest=FMath::Square(5300.);
    double HearingRange=FMath::Square(5400.);
    AETAgent* Heard=nullptr;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETFrontSight),false,this);
    for(TActorIterator<AETAgent> It(GetWorld());It;++It) {
        auto* Candidate=*It;
        if(!Opposes(Candidate)) continue;
        const auto Target=Candidate->AimPoint(); const double D=FVector::DistSquared(Eye,Target);
        if(D>=Nearest) continue;
        FHitResult Hit;
        if(GetWorld()->LineTraceSingleByChannel(Hit,Eye,Target,ECC_Visibility,Query)&&Hit.GetActor()!=Candidate) {
            if(FrontUnit&&Candidate->FrontUnit&&D<HearingRange) {Heard=Candidate; HearingRange=D;}
            continue;
        }
        Rival=Candidate; Nearest=D;
    }
    // Columns hear nearby rivals over a blind crest and advance until their weapons have line of sight.
    if(!Rival.IsValid()) Rival=Heard;
    return Rival.Get();
}

AETAgent* AETWorld::FrontFighter(const FString& Id,const TCHAR* Kind,FVector Position)
{
    if(auto* A=FrontUnits.FindRef(Id);IsValid(A)) return A;
    if(State->FrontLosses.FindRef(Id)>State->Elapsed) return nullptr;
    auto* A=GetWorld()->SpawnActor<AETAgent>(Position,FRotator::ZeroRotator);
    if(!A) return nullptr;
    A->FrontUnit=true; A->Configure(this,Id,Kind);
    if(!IsValid(A)) return nullptr;
    A->AttackTime=.9f+float(GetTypeHash(Id)%13)*.075f;
    FrontUnits.Add(Id,A); return A;
}

void AETWorld::TickWarFronts(AETCharacter* P,float Delta)
{
    FrontClock-=Delta; if(FrontClock>0) return; FrontClock=1.15f;
    const FVector Here=P->GetActorLocation(); const FVector2D XY(Here);
    for(auto It=State->FrontLosses.CreateIterator();It;++It)
        if(It.Value()<=State->Elapsed||It.Value()>State->Elapsed+70) It.RemoveCurrent();
    for(auto It=FrontUnits.CreateIterator();It;++It) {
        auto* A=It.Value();
        if(IsValid(A)&&FVector::DistSquared2D(Here,A->Home)<FMath::Square(24500.)) {A->UpdatePresence(Here);continue;}
        if(IsValid(A)) A->Destroy();
        It.RemoveCurrent();
    }
    if(Here.Z<SurfaceHeight(Here.X,Here.Y)-1400) return;
    ETNav::Sites(FBox2D(XY-FVector2D(17000),XY+FVector2D(17000)),[&](const ETNav::FSite& S) {
        // A relief column changes its line of approach with the campaign clock.
        // Only nearby companies become actors; distant highway streaming stays bounded.
        const int Relief=int(FMath::Fmod(FMath::FloorToDouble(State->Elapsed/65.),4.));
        const bool BugPush=ETWar::BugAdvance(S,State->Elapsed);
        const double Shift=(Relief-1.5)*240.+(BugPush!=S.Bug?1450.:-350.);
        auto Spawn=[&](int Slot,bool Bug,FVector2D Spot,bool Guard) {
            if(FVector2D::DistSquared(XY,Spot)>FMath::Square(15900.)) return;
            auto* Tile=Regions.FindRef({FMath::FloorToInt32(Spot.X/RegionSize),FMath::FloorToInt32(Spot.Y/RegionSize)});
            if(!Tile||Tile->Stage<6) return;
            const FString Key=S.Id()+FString::Printf(TEXT("/%d"),Slot);
            if(IsValid(FrontUnits.FindRef(Key))||State->FrontLosses.FindRef(Key)>State->Elapsed) return;
            const TCHAR* Kind=Bug?(Guard?TEXT("bulwark"):Slot%3==0?TEXT("spitter"):TEXT("runner")):TEXT("robot");
            if(Bug&&!Guard&&ThreatTier()>=2&&Slot%3==2) Kind=TEXT("skimmer");
            // Never muster on an excavated opening, on a roof, or inside a survivor's building.
            for(int Try=0;Try<4;++Try) {
                const auto At=Spot+S.Inward*(Try*190.);
                const auto Ground=FVector(At,SurfaceHeight(At.X,At.Y));
                FHitResult Floor; FCollisionQueryParams Q(SCENE_QUERY_STAT(ETFrontMuster),false);
                if(!GetWorld()->LineTraceSingleByChannel(Floor,Ground+FVector(0,0,300),Ground-FVector(0,0,155),ECC_Visibility,Q)||
                    Floor.ImpactNormal.Z<.61||Cast<AETAgent>(Floor.GetActor())||Cast<AETConstruction>(Floor.GetActor())||Cast<AETItem>(Floor.GetActor())) continue;
                const auto Raised=Floor.ImpactPoint+FVector(0,0,FCString::Strcmp(Kind,TEXT("skimmer"))==0?255:9);
                if(GetWorld()->OverlapBlockingTestByChannel(Raised+FVector(0,0,75),FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(32,60),Q)) continue;
                FrontFighter(Key,Kind,Raised); return;
            }
        };
        for(int I=0;I<2;++I) {
            const auto Lane=S.Exit+S.Inward*(3250+I*670+Shift);
            Spawn(I,S.Bug,Lane+S.Along*(650+I*230),false);
            Spawn(3+I,!S.Bug,Lane-S.Along*(725-I*140),false);
            Spawn(6+I,S.Bug,S.At(I?2270:-2160,-2950),true);
        }
        Spawn(BugPush?8:9,BugPush,S.Exit+S.Inward*(3350.+Shift)+S.Along*(BugPush==S.Bug?1150.:-1080.),false);
    });
}
