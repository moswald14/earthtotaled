#include "ETWakeVehicle.h"
#include "ETWake.h"
#include "ETGame.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"

using namespace ETWake;

AETWakeVehicle::AETWakeVehicle() {
    Hull=CreateDefaultSubobject<UBoxComponent>(TEXT("Chassis"));SetRootComponent(Hull);
    Hull->SetCollisionProfileName(TEXT("BlockAllDynamic"));Hull->SetMobility(EComponentMobility::Movable);
}
UStaticMeshComponent* AETWakeVehicle::Part(const TCHAR* Shape,const TCHAR* Mat,FVector Pos,FVector Size,FRotator Rot) {
    const FString Name=Shape;auto* M=NewObject<UStaticMeshComponent>(this);
    M->SetupAttachment(Hull);M->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Engine/BasicShapes/")+Name+TEXT(".")+Name)));
    M->SetCollisionEnabled(ECollisionEnabled::NoCollision);M->SetMaterial(0,Field->Material(Mat));
    M->SetRelativeLocation(Pos);M->SetRelativeRotation(Rot);M->SetRelativeScale3D(Size/100.);M->RegisterComponent();return M;
}
FETVehicleSave& AETWakeVehicle::State() const {return Type==Car?Field->State->Wake.Roadster:Field->State->Wake.Aircraft;}
FVector AETWakeVehicle::SeatPosition() const {return GetActorTransform().TransformPosition(Type==Car?FVector(-30,-24,38):FVector(18,0,51));}
bool AETWakeVehicle::Boardable(const AETCharacter& P) const {return Field&&(Type==Car?Field->State->Wake.HasCar():Field->State->Wake.HasPlane())&&FVector::DistSquared(P.GetActorLocation(),GetActorLocation())<FMath::Square(Type==Car?390.:585.);}
void AETWakeVehicle::Prepare(AETWorld& W,int T) {
    Field=&W;Type=T;
    if(T==Car) {
        Hull->SetBoxExtent({180,78,34});
        Part(TEXT("Cube"),TEXT("DarkMetal"),{0,0,-8},{338,144,26});
        Part(TEXT("Sphere"),TEXT("Rust"),{88,0,12},{194,148,49});
        Part(TEXT("Cube"),TEXT("Rust"),{-122,0,10},{82,143,41});
        for(int Side:{-1,1}) {
            Part(TEXT("Cube"),TEXT("AlloyHull"),{-14,Side*66.,12},{137,12,45});
            Part(TEXT("Cube"),TEXT("Steel"),{21,Side*64.,48},{8,7,63},FRotator(-15,0,0));
            Part(TEXT("Sphere"),TEXT("Signal"),{167,Side*48.,20},{12,28,17});
            for(int X:{-107,108}) {
                Part(TEXT("Cylinder"),TEXT("Rubber"),{double(X),Side*75.,-6},{60,60,21},FRotator(0,0,90));
                Part(TEXT("Cylinder"),TEXT("Steel"),{double(X),Side*86.,-6},{32,32,4},FRotator(0,0,90));
            }
            Part(TEXT("Cube"),TEXT("Canvas"),{-60,Side*32.,24},{17,42,51},FRotator(-12,0,0));
        }
        Part(TEXT("Cube"),TEXT("Steel"),{13,0,78},{8,132,7});
        Part(TEXT("Cube"),TEXT("DarkMetal"),{32,0,32},{23,115,8});
        auto* Fold=NewObject<UStaticMeshComponent>(this);Fold->SetupAttachment(Hull);
        Fold->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/EARTHtotaled/Art/SM_KitFold.SM_KitFold")));
        Fold->SetRelativeLocation({31,16,36});Fold->SetRelativeRotation(FRotator(0,-21,0));Fold->SetCollisionEnabled(ECollisionEnabled::NoCollision);Fold->RegisterComponent();
        Part(TEXT("Cube"),TEXT("Steel"),{171,0,-6},{12,154,12});
    } else {
        Hull->SetBoxExtent({187,84,31});
        Part(TEXT("Sphere"),TEXT("AlloyHull"),{0,0,6},{382,93,92});
        Part(TEXT("Sphere"),TEXT("DarkMetal"),{30,0,53},{90,77,34});
        Part(TEXT("Sphere"),TEXT("AlloyHull"),{-160,0,36},{173,46,49});
        for(int Side:{-1,1}) {
            Part(TEXT("Sphere"),TEXT("AlloyHull"),{0,Side*200.,44},{110,436,18},FRotator(0,-Side*5.,0));
            Part(TEXT("Cube"),TEXT("Signal"),{-12,Side*378.,46},{68,18,7});
            Part(TEXT("Sphere"),TEXT("AlloyHull"),{-195,Side*52.,40},{62,123,10});
            Part(TEXT("Cube"),TEXT("Steel"),{58,Side*45.,-24},{8,9,77},FRotator(0,0,Side*24.));
            Part(TEXT("Cylinder"),TEXT("Rubber"),{63,Side*65.,-54},{45,45,16},FRotator(0,0,90));
        }
        Part(TEXT("Sphere"),TEXT("Signal"),{-200,0,73},{85,12,87},FRotator(-18,0,0));
        Part(TEXT("Sphere"),TEXT("Rust"),{185,0,12},{46,63,63});
        Propeller=Part(TEXT("Cube"),TEXT("DarkMetal"),{209,0,12},{6,11,160},FRotator(0,0,30));
    }
    if(State().Position.IsNearlyZero()) ParkAt(0);
    SetActorLocationAndRotation(State().Position,FRotator(0,State().Heading,0));
}
void AETWakeVehicle::ParkAt(int I) {
    auto& V=State();V=FETVehicleSave();FVector2D P=Type==Car?FVector2D(5750,2600):ETWake::Field(I,Field->State->Wake)+FVector2D(-3450,0);
    V.Position=FVector(P,Type==Car?Field->SurfaceHeight(P.X,P.Y)+49:StripZ(*Field,I)+79);
    SetActorLocationAndRotation(V.Position,FRotator::ZeroRotator);
}
void AETWakeVehicle::StartEngine() {
    auto& V=State();
    if(V.Flying||FMath::Abs(V.Speed)>80){Field->Say(TEXT("Brake to a stop before switching off the engine"));return;}
    V.Started=!V.Started;Field->Sound(TEXT("Switch"),.6f,V.Started?.82f:.6f);
    Field->Say(V.Started?TEXT("Engine running / W accelerates"):TEXT("Engine off"));
}
void AETWakeVehicle::Drive(AETCharacter& P,float Seconds) {
    auto& V=State();auto& S=Field->State->Wake;const bool Air=Type==Plane;
    // Small swept steps preserve obstacle checks and runway contact at low frame rates.
    const float Dt=FMath::Min(Seconds,.25f);const int Steps=FMath::Max(1,FMath::CeilToInt(Dt/.025f));
    const float H=Dt/Steps;const float Before=V.Heading;
    for(int I=0;I<Steps;++I) {
        const float Input=V.Started?P.DriveAxis:0;
        if(Input>0) V.Speed=FMath::Min(Air?6200.f:2500.f,V.Speed+Input*(Air?900.f:780.f)*H);
        else if(Input<0) V.Speed=FMath::Max(!Air&&P.DescentHeld?-570.f:0.f,V.Speed+Input*(Air?1500.f:2400.f)*H);
        else if(!V.Flying)V.Speed=FMath::FInterpConstantTo(V.Speed,0.f,H,Air?75.f:190.f);
        if(!Air&&P.LiftHeld) V.Speed=FMath::Max(0.f,V.Speed-3100*H);
        const float Steering=FMath::Clamp(V.Speed/340.f,-1.f,1.f);
        V.Heading=FMath::UnwindDegrees(V.Heading+P.SteerAxis*(Air?(V.Flying?42.f:31.f):67.f)*Steering*H);
        FVector Next=V.Position+FRotator(0,V.Heading,0).Vector()*V.Speed*H;
        const int Strip=StripAt(S,FVector2D(Next));
        const double Floor=(Air&&Strip>=0?StripZ(*Field,Strip):Field->SurfaceHeight(Next.X,Next.Y))+(Air?79:49);
        if(Air&&V.Started&&P.LiftHeld&&V.Speed>=1300&&!V.Flying){V.Flying=true;S.TookOff=true;Field->Say(TEXT("Above the canopy / look east for the pulse towers at Crown Field"));}
        if(V.Flying) {
            const float Lift=P.LiftHeld?1120.f:P.DescentHeld?-900.f:V.Speed<900?-450.f:0.f;
            Next.Z=FMath::Clamp(V.Position.Z+Lift*H,Floor,Floor+18000);
            if(Next.Z<=Floor+1&&Lift<0&&V.Speed<1900) V.Flying=false;
        } else Next.Z=Floor;
        if(!Air||!V.Flying) {
            FHitResult Hit;FCollisionQueryParams Q(SCENE_QUERY_STAT(WakeChassis),false,this);Q.AddIgnoredActor(&P);
            const auto Nose=FRotator(0,V.Heading,0).Vector()*(Air?150.:125.)*FMath::Sign(V.Speed);
            const FVector A=V.Position+Nose+FVector(0,0,10),B=Next+Nose+FVector(0,0,10);
            if(FMath::Abs(V.Speed)>1&&GetWorld()->SweepSingleByChannel(Hit,A,B,FQuat::Identity,ECC_Visibility,FCollisionShape::MakeSphere(18),Q)&&Hit.ImpactNormal.Z<.6&&!Cast<AETAgent>(Hit.GetActor())) {
                V.Speed=0;Next=V.Position;
            }
        }
        V.Position=Next;
        if(Type==Car&&S.Chapter==Driving&&!S.DriveGate&&FVector2D::Distance(FVector2D(Next),Location(S,"roadgate"))<1100) {
            S.DriveGate=true;Field->Say(TEXT("Clean pass. Brake beside the dispatch cabin and press E to get out."));Field->Save();
        }
    }
    SetActorLocationAndRotation(V.Position,FRotator(V.Flying?(P.LiftHeld?7:P.DescentHeld?-7:0):0,V.Heading,V.Flying?P.SteerAxis*14:0));
    if(Propeller&&V.Started)Propeller->AddLocalRotation(FRotator(0,0,Dt*1420));
    P.SetActorLocation(SeatPosition(),false);auto R=P.GetControlRotation();R.Yaw+=FMath::FindDeltaAngleDegrees(Before,V.Heading);if(P.Controller)P.Controller->SetControlRotation(R);
    AudioTime-=Dt;if(V.Started&&AudioTime<=0){Field->Sound(Air?TEXT("JetThrust"):TEXT("FieldLatch"),Air?.12f:.095f,.48f+V.Speed/11000);AudioTime=Air?.66f:.46f;}
}
AETWakeVehicle* AETCharacter::Ride() const {
    if(!World||!World->State)return nullptr;const int S=World->State->Wake.Seat;return S==Car?World->WakeCar:S==Plane?World->WakePlane:nullptr;
}
void AETCharacter::EnterRide(AETWakeVehicle* V,bool Restore) {
    if(!IsValid(V)||(!Restore&&(!V->Boardable(*this)||!Panel.IsEmpty())))return;
    auto& S=World->State->Wake;if((V->Type==Car&&!S.HasCar())||(V->Type==Plane&&!S.HasPlane()))return;
    StopBuilding();Firing=Aiming=Climbing=false;ReloadTime=0;JumpRelease();ReleaseLoot();DriveAxis=SteerAxis=0;DescentHeld=LiftHeld=false;
    GetCharacterMovement()->StopMovementImmediately();GetCharacterMovement()->DisableMovement();GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    S.Seat=V->Type;SetActorLocation(V->SeatPosition());if(Controller&&!Restore)Controller->SetControlRotation(FRotator(0,V->State().Heading,0));
    ApplyWakeBody();if(!Restore){
        if(V->Type==Car&&S.Chapter==Driving)World->FindTrailClue(DashFold);
        World->Sound(TEXT("FieldLatch"));World->Save();}
}
void AETCharacter::LeaveRide() {
    auto* V=Ride();if(!V)return;auto& State=V->State();
    if(State.Flying||FMath::Abs(State.Speed)>125){World->Say(TEXT("Land and brake to walking speed before exiting"));return;}
    FVector Exit;bool Found=false;
    for(auto Offset:{FVector(0,-1,0),FVector(0,1,0),FVector(-1,0,0),FVector(1,0,0)}) {
        FVector At=V->GetActorLocation()+V->GetActorRotation().RotateVector(Offset)*(V->Type==Plane?485:215);
        double Z=World->SurfaceHeight(At.X,At.Y);int I=StripAt(World->State->Wake,FVector2D(At));if(I>=0)Z=FMath::Max(Z,StripZ(*World,I));
        At.Z=Z+100;FCollisionQueryParams Q(SCENE_QUERY_STAT(WakeExit),false,this);Q.AddIgnoredActor(V);
        if(!GetWorld()->OverlapBlockingTestByChannel(At,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(32,85),Q)){Exit=At;Found=true;break;}
    }
    if(!Found){World->Say(TEXT("Pull into an open spot to exit"));return;}
    auto& S=World->State->Wake;S.Seat=Foot;State.Started=false;State.Speed=0;GetCapsuleComponent()->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    SetActorLocation(Exit);GetCharacterMovement()->SetMovementMode(MOVE_Walking);FallSpeed=0;ApplyWakeBody();
    if(V->Type==Car&&S.Chapter==Driving&&S.DriveGate&&FVector2D::Distance(FVector2D(Exit),Location(S,"dispatch"))<1900)S.Chapter=Dispatch;
    if(V->Type==Plane&&StripAt(S,FVector2D(State.Position))==1){S.Airfields|=2;if(S.Chapter==Flight&&S.TookOff)S.Chapter=Vault;}
    World->RefreshWake();World->Say(Objective(S));World->Save();
}
