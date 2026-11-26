#include "ETGame.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"

FVector AETCharacter::AssistedDirection(FVector Eye,FVector Direction,double Range) const
{
    if(!World||!Aiming||Borer||Building||!Panel.IsEmpty()||!World->State->LightAimAssist) return Direction;
    const double Limit=FMath::Min(Range,6500.);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETAimAssist),false,this);
    FHitResult CenterHit;
    double Unblocked=Limit;
    if(GetWorld()->LineTraceSingleByChannel(CenterHit,Eye,Eye+Direction*Limit,ECC_Visibility,Query)) {
        if(Cast<AETAgent>(CenterHit.GetActor())||Cast<AETItem>(CenterHit.GetActor())) return Direction;
        Unblocked=CenterHit.Distance;
    }
    double BestDot=FMath::Cos(FMath::DegreesToRadians(1.65));
    FVector Best=Direction;
    for(TActorIterator<AETAgent> It(GetWorld());It;++It) {
        const auto* A=*It;
        if(A->Health<=0||A->Kind==TEXT("deer")||!A->Awake()||A->IsHidden()||!A->GetActorEnableCollision()||A->Parts.IsEmpty()) continue;
        const FVector Offset=A->Parts[0]->Bounds.Origin-Eye;
        const double Length=Offset.Size();
        if(Length<5||Length>Unblocked) continue;
        const FVector Toward=Offset/Length;
        const double Dot=FVector::DotProduct(Direction,Toward);
        if(Dot<=BestDot) continue;
        FHitResult Visible;
        if(!GetWorld()->LineTraceSingleByChannel(Visible,Eye,Eye+Toward*(Length+50),ECC_Visibility,Query)||Visible.GetActor()!=A) continue;
        BestDot=Dot; Best=Toward;
    }
    const double Angle=FMath::Acos(FMath::Clamp(FVector::DotProduct(Direction,Best),-1.,1.));
    if(Angle<UE_SMALL_NUMBER) return Direction;
    const double Fraction=FMath::Min(.55,FMath::DegreesToRadians(.7)/Angle);
    return FMath::Lerp(Direction,Best,Fraction).GetSafeNormal();
}
