#include "ETGame.h"
#include "ETUnderstory.h"
#include "ETDayCycle.h"
#include "ETGeology.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

namespace {
const FIntPoint ShelterWoods(-1,0);
TArray<AETAgent*> WildDeer(AETRegion* Region) {
    TArray<AETAgent*> Herd;
    if(Region)for(auto* Actor:Region->Residents)if(auto* Deer=Cast<AETAgent>(Actor);IsValid(Deer)&&Deer->Kind=="deer"&&Deer->WakeHerd==INDEX_NONE)Herd.Add(Deer);
    return Herd;
}
}

void ETFlora::DeerTrial(AETWorld& W,AETCharacter& P,TFunctionRef<void(bool,FString)> Check)
{
    if(!P.Panel.IsEmpty()){P.Panel.Empty();UGameplayStatics::SetGamePaused(&W,false);P.SetMouseMode(false);}
    const auto Chapter=W.State->Wake.Chapter;
    const int Restored=W.State->Wake.Restored();
    const int Crossing=W.WakeDeer.FilterByPredicate([](AETAgent* A){return IsValid(A)&&A->WakeHerd==0&&A->HerdMember<3;}).Num();
    Check(Crossing==3,TEXT("All three authored deer remain at the shelter crossing or its restored glade"));
    W.EnsureRegion(ShelterWoods);
    auto* Region=W.Regions.FindRef(ShelterWoods);FVector Meadow;
    if(!Region||!Region->DeerMeadow(0,Meadow)){Check(false,TEXT("The shelter forest has a meadow"));return;}
    const auto First=WildDeer(Region).FilterByPredicate([&](AETAgent* A){return FVector::Dist2D(A->Home,Meadow)<1200;});
    Check(First.Num()>=3&&First.Num()<=4&&!W.InCity(Meadow)&&!Region->OnHighway(FVector2D(Meadow),1000),
        FString::Printf(TEXT("Shelter woods have %d ordinary deer on solid ground at %.0f m / bearing %.0f"),First.Num(),Meadow.Size2D()/100.,ETNav::Bearing(FVector2D(Meadow))));
    if(First.Num()<2)return;
    P.SetActorLocation(Meadow+FVector(0,2300,110));
    bool Present=true,Grounded=true;
    for(int Hour:{12,23}){
        W.State->ClockOffset=ETDay::At(Hour)-W.State->Elapsed;W.UpdateVisibility(P.GetActorLocation());
        for(auto* A:First){
            Present&=!A->IsHidden()&&A->IsActorTickEnabled()&&A->GetActorEnableCollision()&&A->Parts.Num()>0&&A->Parts[0]->GetStaticMesh()!=nullptr;
            const auto At=A->GetActorLocation();
            Grounded&=FMath::Abs(At.Z-W.SurfaceHeight(At.X,At.Y)-8)<2&&W.Geology->SurfaceOpening(At.X,At.Y)>80;
        }
    }
    Check(Present&&Grounded,TEXT("The forest herd has visible deer meshes, collision and surface footing by day and at night"));
    // Exercise the normal flee AI on a clear, short approach to a live woodland deer.
    auto* Lead=First[0];const auto Before=Lead->GetActorLocation();bool Escaped=false;
    for(int Side=0;Side<12&&!Escaped;++Side) {
        const auto Offset=FRotator(0,Side*30,0).Vector()*540;
        auto Near=Before+Offset;Near.Z=W.SurfaceHeight(Near.X,Near.Y)+96;P.SetActorLocation(Near);
        const double Distance=FVector::Dist2D(Near,Lead->GetActorLocation());
        for(int Tick=0;Tick<5;++Tick)Lead->Tick(.04f);
        Escaped=FVector::Dist2D(Near,Lead->GetActorLocation())>Distance+15.;
    }
    Check(Escaped&&Lead->AlertHold==0,TEXT("A woodland deer flees an approaching survivor without becoming an enemy"));
    int Found=0;bool Habitats=true;
    for(const FIntPoint Key:{FIntPoint(-2,0),FIntPoint(-3,0),FIntPoint(-4,1),FIntPoint(2,-2),FIntPoint(11,-6)}) {
        W.EnsureRegion(Key);auto* R=W.Regions.FindRef(Key);const auto Animals=WildDeer(R);
        if(Animals.Num()>=3)++Found;
        for(auto* A:Animals)Habitats&=!W.InCity(A->Home)&&!R->OnHighway(FVector2D(A->Home),500)&&!R->InHighwayGrounds(FVector2D(A->Home));
    }
    Check(Found>=4&&Habitats,FString::Printf(TEXT("Found visible-sized herds in %d of 5 streamed forest regions, including distant wilderness"),Found));
    const FString Wounded=First[0]->Id,Hunted=First[1]->Id;const auto Home=First[0]->Home;
    Lead->TakeDamage(3,FDamageEvent(),nullptr,nullptr);const float Health=Lead->Health;
    W.State->Consumed.Add(Hunted);
    Region->Destroy();W.Regions.Remove(ShelterWoods);W.EnsureRegion(ShelterWoods);
    bool WoundKept=false,HuntKept=true;
    for(auto* A:WildDeer(W.Regions.FindRef(ShelterWoods))){
        if(A->Id==Wounded)WoundKept=A->Home.Equals(Home,.1)&&FMath::IsNearlyEqual(A->Health,Health);
        if(A->Id==Hunted)HuntKept=false;
    }
    Check(WoundKept&&HuntKept,TEXT("Returning to the woods keeps the original deer identities, wounds and prior hunts"));
    Check(Chapter==W.State->Wake.Chapter&&Restored==W.State->Wake.Restored(),TEXT("Forest wildlife leaves the survivor's herd restorations and quest stage intact"));
}

void ETFlora::DeerView(AETWorld& W,AETCharacter& P)
{
    static double Start=0;static bool Captured=false;static FVector Eye,Look;
    const double Now=FPlatformTime::Seconds();
    if(!Start) {
        if(!P.Panel.IsEmpty()){P.Panel.Empty();UGameplayStatics::SetGamePaused(&W,false);P.SetMouseMode(false);}
        P.TestInvincible=true;W.WeatherOverride=ETClimate::Clear;W.State->ClockOffset=ETDay::At(10.5)-W.State->Elapsed;
        W.EnsureRegion(ShelterWoods);auto* R=W.Regions.FindRef(ShelterWoods);FVector Center;
        if(!R||!R->DeerMeadow(0,Center)){FPlatformMisc::RequestExitWithStatus(true,2);return;}
        W.StreamAround(Center,true);const auto Herd=WildDeer(R).FilterByPredicate([&](AETAgent* A){return FVector::Dist2D(A->Home,Center)<1200;});
        if(Herd.Num()<3){FPlatformMisc::RequestExitWithStatus(true,2);return;}
        Look=Center+FVector(0,0,75);int Best=-1;
        for(int Side=0;Side<20;++Side) {
            auto View=Center+FRotator(0,Side*18.,0).Vector()*2450;View.Z=W.SurfaceHeight(View.X,View.Y)+165;int Clear=0;
            FCollisionQueryParams Q;Q.AddIgnoredActor(&P);
            for(auto* A:Herd){FHitResult Hit;if(!W.GetWorld()->LineTraceSingleByChannel(Hit,View,A->Home+FVector(0,0,68),ECC_Visibility,Q)||Hit.GetActor()==A)++Clear;}
            if(Clear>Best){Best=Clear;Eye=View;}
        }
        P.GetCharacterMovement()->DisableMovement();Start=Now;
    }
    P.SetActorLocation(Eye-FVector(0,0,72));P.Controller->SetControlRotation((Look-Eye).Rotation());
    const auto Output=FPaths::ProjectSavedDir()/TEXT("blackpine-deer.png");
    if(!Captured&&Now-Start>3.){FScreenshotRequest::RequestScreenshot(Output,true,false);Captured=true;return;}
    if(Captured&&IFileManager::Get().FileSize(*Output)>2048){FPlatformMisc::RequestExitWithStatus(false,0);return;}
    if(Now-Start>25.)FPlatformMisc::RequestExitWithStatus(true,2);
}
