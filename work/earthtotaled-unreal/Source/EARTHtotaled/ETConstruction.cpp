#include "ETGame.h"
#include "ETGeology.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
#include "Materials/MaterialInterface.h"

namespace
{
    constexpr double Bay=400, Storey=280;
    const TCHAR* const Names[]={TEXT("Wall"),TEXT("Floor"),TEXT("Ramp"),TEXT("Roof"),TEXT("Relay"),TEXT("Auto turret"),TEXT("Turtle miner")};
    FVector Facing(int32 Quarter) {return FRotator(0,Quarter*90,0).Vector();}
    FVector SideOf(FVector Offset)
    {
        if(FMath::Abs(Offset.X)>FMath::Abs(Offset.Y)) return FVector(FMath::Sign(Offset.X),0,0);
        return FVector(0,FMath::Sign(Offset.Y),0);
    }
}

AETConstruction::AETConstruction()
{
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("SalvageFrame"));
    Mechanism=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MachineHead")); Mechanism->SetupAttachment(Mesh);
    Mechanism->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    SetRootComponent(Mesh); Mesh->SetMobility(EComponentMobility::Movable);
}
const TCHAR* AETConstruction::Name(int32 Piece) {return Names[FMath::Clamp(Piece,0,Count-1)];}
FVector AETConstruction::Extents(int32 Piece)
{
    switch(Piece) {
        case 0:return FVector(13,200,140);
        case 1:return FVector(200,200,10);
        case 2:return FVector(FMath::Sqrt(Bay*Bay+Storey*Storey)*.5,200,10);
        case Core:return FVector(66,66,83);
        case Turret:return FVector(60,60,50);
        case Miner:return FVector(70,47,34);
        default:return FVector(200,200,70);
    }
}
FTransform AETConstruction::Shape(const FETBuild& B)
{
    FRotator Rotation(0,B.Facing*90,0);
    FVector Center=B.Anchor;
    if(B.Piece==0) Center+=Facing(B.Facing)*Bay*.5+FVector(0,0,Storey*.5);
    else if(B.Piece==1) Center.Z+=6;
    else if(B.Piece==2) {Rotation.Pitch=FMath::RadiansToDegrees(FMath::Atan2(Storey,Bay)); Center.Z+=Storey*.5;}
    else Center.Z+=Extents(B.Piece).Z;
    return FTransform(Rotation,Center);
}
void AETConstruction::Configure(AETWorld* W,const FETBuild& B,bool Ghost,bool Valid)
{
    Design=B; SetActorTransform(Shape(B));
    const int32 Style=(B.Piece*3+B.MaterialTier)*3+(Ghost?(Valid?1:2):0);
    if(Style==Appearance) return;
    Appearance=Style;
    const FString Asset=B.Piece==Core?TEXT("SM_Relay"):B.Piece==Turret?TEXT("SM_SentryStand"):B.Piece==Miner?TEXT("SM_Turtle"):FString(TEXT("SM_Field"))+Name(B.Piece);
    const FString Moving=B.Piece==Turret?TEXT("SM_SentryHead"):B.Piece==Miner?TEXT("SM_TurtleBit"):TEXT("");
    Mechanism->SetStaticMesh(Moving.IsEmpty()?nullptr:LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Moving+TEXT(".")+Moving)));
    Mechanism->SetRelativeLocation(B.Piece==Miner?FVector(52,0,-7):FVector(0,0,23));
    Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Asset+TEXT(".")+Asset)));
    Mesh->SetCollisionProfileName(Ghost?TEXT("NoCollision"):TEXT("BlockAll"));
    Mesh->SetCastShadow(!Ghost);
    if(Ghost) {
        auto Mat=W->Material(Valid?TEXT("BuildReady"):TEXT("BuildBlocked"));
        for(int32 I=0;I<Mesh->GetNumMaterials();++I) Mesh->SetMaterial(I,Mat);
        for(int32 I=0;I<Mechanism->GetNumMaterials();++I) Mechanism->SetMaterial(I,Mat);
    } else if(B.Piece<Core) {
        auto* Skin=W->Material(B.MaterialTier==2?TEXT("Steel"):B.MaterialTier==1?TEXT("Brick"):TEXT("Wood"));
        for(int32 I=0;I<Mesh->GetNumMaterials();++I) Mesh->SetMaterial(I,Skin);
    } else {W->DressMesh(Mesh,TEXT("Steel")); W->DressMesh(Mechanism,TEXT("Steel"));}
}
AETConstruction* AETWorld::SpawnBuild(FGuid Id,const FETBuild& B)
{
    if(auto* Existing=LiveBuilds.FindRef(Id).Get()) return Existing;
    const FIntPoint Cell(FMath::FloorToInt(B.Anchor.X/RegionSize),FMath::FloorToInt(B.Anchor.Y/RegionSize));
    auto* Region=Regions.FindRef(Cell);
    if(!IsValid(Region)) return nullptr;
    auto* A=GetWorld()->SpawnActor<AETConstruction>();
    if(!A) return nullptr;
    A->Id=Id; A->Configure(this,B); A->SetOwner(Region); Region->Residents.Add(A); LiveBuilds.Add(Id,A);
    return A;
}
void AETWorld::RemoveBuild(AETConstruction* A)
{
    if(!IsValid(A)) return;
    const FGuid Id=A->Id; const FVector At=A->Design.Anchor;
    if(BreakBuild(Id)) CheckFoundations(At,FVector(460,460,340));
}
void AETCharacter::CycleMaterial()
{
    if(!Building||!Panel.IsEmpty()||BuildPiece>=AETConstruction::Core) return;
    BuildMaterial=(BuildMaterial+1)%3; RefreshBuild();
}
void AETCharacter::UpgradeFrame()
{
    if(!Building||!Panel.IsEmpty()) return;
    const FVector Eye=Camera->GetComponentLocation();
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETUpgradeFrame),false,this);
    FHitResult H;
    GetWorld()->LineTraceSingleByChannel(H,Eye,Eye+Camera->GetForwardVector()*1500,ECC_Visibility,Q);
    auto* A=Cast<AETConstruction>(H.GetActor());
    auto* B=A?World->State->Builds.Find(A->Id):nullptr;
    if(!B||B->Piece>=AETConstruction::Core) {World->Say(TEXT("Aim at one of your frames to upgrade it")); return;}
    if(B->MaterialTier==2) {World->Say(TEXT("This frame is already metal")); return;}
    const int Tier=B->MaterialTier+1,Cost=A->FrameCost(Tier);
    auto& Stock=World->State->Inventory;
    if(Stock.FindRef(A->Resource(Tier))<Cost) {World->Say(TEXT("Upgrade needs ")+A->Recipe(B->Piece,Tier)); return;}
    const float Fraction=B->Integrity/A->Durability(B->Piece,B->MaterialTier);
    Stock.FindOrAdd(A->Resource(Tier))-=Cost;
    B->MaterialTier=Tier; B->FromKit=false; B->Integrity=Fraction*A->Durability(B->Piece,Tier);
    A->Configure(World,*B); World->Sound(TEXT("FieldLatch")); World->Say(FString(TEXT("Upgraded to "))+A->MaterialName(Tier)); World->Save(); RefreshBuild();
}
void AETCharacter::StopBuilding()
{
    if(!Building) return;
    Building=false; if(IsValid(Preview)) Preview->Destroy(); Preview=nullptr;
    Gun->SetVisibility(true); Firing=Aiming=false;
}
void AETCharacter::ToggleBuild()
{
    if(!World||!Panel.IsEmpty()||Climbing) return;
    if(Building) {StopBuilding(); return;}
    if(Borer) SelectWeapon(World->State->Weapon);
    Firing=Aiming=false; ReloadTime=0; Building=true; Gun->SetVisibility(false);
    BuildFacing=(FMath::RoundToInt(GetControlRotation().Yaw/90)+4)%4;
    Preview=GetWorld()->SpawnActor<AETConstruction>(); RefreshBuild();
}
bool AETCharacter::BuildTarget(FETBuild& B,FString& Reason) const
{
    const FVector Eye=Camera->GetComponentLocation(), Ray=Camera->GetForwardVector();
    FHitResult H;
    FCollisionQueryParams Trace(SCENE_QUERY_STAT(ETBuildSite),false,this);
    if(Preview) Trace.AddIgnoredActor(Preview);
    if(!GetWorld()->LineTraceSingleByChannel(H,Eye,Eye+Ray*1500,ECC_Visibility,Trace)) {Reason=TEXT("Aim at ground or a frame within 15 m"); return false;}
    B.Piece=BuildPiece; B.Facing=BuildFacing; B.MaterialTier=BuildPiece<AETConstruction::Core?BuildMaterial:0;
    B.Integrity=AETConstruction::Durability(B.Piece,B.MaterialTier);
    B.Anchor=H.ImpactPoint;
    const auto* Support=Cast<AETConstruction>(H.GetActor());
    if(Support) {
        const auto& Base=Support->Design;
        B.Anchor=Base.Anchor;
        const FVector Offset=H.ImpactPoint-Base.Anchor;
        if(Base.Piece>=AETConstruction::Core) {Reason=TEXT("Aim at clear ground or a floor beside the machine"); return false;}
        if(BuildPiece>=AETConstruction::Core&&Base.Piece==1) B.Anchor.Z+=17;
        if(Base.Piece==0) {
            if(BuildPiece==0&&Offset.Z>Storey*.62) B.Anchor.Z+=Storey;
            else if(BuildPiece!=0&&Offset.Z>Storey*.42) B.Anchor.Z+=Storey;
            else if(BuildPiece==0 && Base.Facing==BuildFacing) B.Anchor+=SideOf(Offset-Facing(Base.Facing)*Bay*.5)*Bay;
        } else if(Base.Piece==2&&Offset.Z>Storey*.60) {
            B.Anchor+=Facing(Base.Facing)*Bay; B.Anchor.Z+=Storey;
        } else if(Base.Piece==3) {
            B.Anchor+=SideOf(Offset)*Bay;
        } else if(BuildPiece==1||BuildPiece==Base.Piece) {
            B.Anchor+=SideOf(Offset)*Bay;
        }
    } else {
        B.Anchor.X=FMath::GridSnap(B.Anchor.X,Bay); B.Anchor.Y=FMath::GridSnap(B.Anchor.Y,Bay);
        if(H.ImpactNormal.Z<.55||Cast<AETItem>(H.GetActor())||Cast<AETAgent>(H.GetActor())) {
            Reason=TEXT("Start on firm ground or connect to a frame"); return false;
        }
        FVector Probe=B.Anchor;
        FHitResult Foot;
        if(!GetWorld()->LineTraceSingleByChannel(Foot,Probe+FVector(0,0,90),Probe-FVector(0,0,125),ECC_Visibility,Trace)||
           Foot.ImpactNormal.Z<.55||Cast<AETAgent>(Foot.GetActor())||Cast<AETItem>(Foot.GetActor())) {Reason=TEXT("Find a level footing"); return false;}
        B.Anchor.Z=Foot.ImpactPoint.Z+4;
        // Level the lower bearing surface across the slope. A centre-only sample can
        // leave the uphill edge buried and reject an otherwise clear deck.
        {
            FVector FootCenter=Probe,Half=AETConstruction::Extents(B.Piece);
            const FRotator Heading(0,B.Facing*90,0);
            if(B.Piece==0) FootCenter+=Facing(B.Facing)*Bay*.5;
            if(B.Piece==2) {FootCenter-=Facing(B.Facing)*Bay*.5; Half.X=13;}
            for(int32 Y:{-1,1}) for(int32 X:{-1,1}) {
                const FVector Edge=FootCenter+Heading.RotateVector(FVector(X*Half.X*.96,Y*Half.Y*.96,0));
                FHitResult Bearing;
                if(GetWorld()->LineTraceSingleByChannel(Bearing,Edge+FVector(0,0,115),Edge-FVector(0,0,125),ECC_Visibility,Trace)&&
                   Bearing.GetActor()==Foot.GetActor()&&Bearing.ImpactNormal.Z>.55) B.Anchor.Z=FMath::Max(B.Anchor.Z,Bearing.ImpactPoint.Z+4);
            }
        }
    }
    if(Support&&!AETConstruction::Joined(B,Support->Design)) {Reason=TEXT("Align a full edge with the supporting frame"); return false;}
    if(!Support&&!AETConstruction::HasFooting(*World,B)) {Reason=TEXT("The frame needs a solid footing"); return false;}
    const FTransform T=AETConstruction::Shape(B);
    const FVector Half=AETConstruction::Extents(B.Piece);
    // The frame must clear the survivor, even though the placement ray ignores their capsule.
    FCollisionQueryParams Clearance(SCENE_QUERY_STAT(ETBuildSpace),false);
    if(Preview) Clearance.AddIgnoredActor(Preview);
    if(Support) Clearance.AddIgnoredActor(Support);
    FVector Inner=(Half-FVector(11)).ComponentMax(FVector(2));
    if(B.Piece==2) Inner.X-=32;
    if(GetWorld()->OverlapBlockingTestByChannel(T.GetLocation(),T.GetRotation(),ECC_Visibility,FCollisionShape::MakeBox(Inner),Clearance)) {
        Reason=TEXT("Space occupied");
        if(World->SelfTest) {
            TArray<FOverlapResult> Hits;
            GetWorld()->OverlapMultiByChannel(Hits,T.GetLocation(),T.GetRotation(),ECC_Visibility,FCollisionShape::MakeBox(Inner),Clearance);
            for(const auto& Hit:Hits) if(Hit.bBlockingHit) {Reason+=TEXT(" / ")+GetNameSafe(Hit.GetComponent()); break;}
        }
        return false;
    }
    const FVector Local=T.InverseTransformPosition(GetActorLocation());
    const FVector PawnPad=FVector(34,34,88);
    if(FBox(-Half-PawnPad,Half+PawnPad).IsInside(Local)) {Reason=TEXT("Step clear of the frame"); return false;}
    for(auto It:World->LiveBuilds) if(auto* Placed=It.Value.Get()) {
        if(Placed->Design.Piece==B.Piece && Placed->GetActorLocation().Equals(T.GetLocation(),28) &&
           (B.Piece!=2||Placed->Design.Facing==B.Facing)) {Reason=TEXT("A frame already occupies this socket"); return false;}
    }
    if(B.Piece>=AETConstruction::Core) {
        const auto Cost=AETConstruction::Cost(B.Piece);
        if(B.Piece==AETConstruction::Core && B.Anchor.Z<World->SurfaceHeight(B.Anchor.X,B.Anchor.Y)-300) {Reason=TEXT("The relay antenna needs a surface site"); return false;}
        if(B.Piece==AETConstruction::Turret&&!World->NearestCore(B.Anchor,7500).IsValid()) {Reason=TEXT("Build a relay within 75 m to power the turret"); return false;}
        if(World->State->Inventory.FindRef(TEXT("ironore"))<Cost.X||World->State->Inventory.FindRef(TEXT("goldore"))<Cost.Y) {Reason=TEXT("Mine ore with G to afford this machine"); return false;}
    } else {
        B.FromKit=false;
        if(World->State->Inventory.FindRef(AETConstruction::Resource(BuildMaterial))<AETConstruction::FrameCost(BuildMaterial)) {
            if(BuildMaterial==0&&World->State->BuildStock>0) B.FromKit=true;
            else {
                Reason=BuildMaterial==0?TEXT("Fell a tree with E for wood"):BuildMaterial==1?TEXT("Blast rock with G for stone"):TEXT("Mine an iron seam with G");
                return false;
            }
        }
    }
    Reason=TEXT("LMB place"); return true;
}
void AETCharacter::RefreshBuild()
{
    if(!Building||!IsValid(Preview)) return;
    FETBuild B; B.Piece=BuildPiece; B.Facing=BuildFacing;
    const bool Valid=BuildTarget(B,BuildHint);
    Preview->SetActorHiddenInGame(BuildHint.StartsWith(TEXT("Aim at")));
    Preview->Configure(World,B,true,Valid);
}
void AETCharacter::PlaceBuild()
{
    if(!Building||!Panel.IsEmpty()) return;
    FETBuild B; FString Why;
    if(!BuildTarget(B,Why)) {World->Say(Why); return;}
    const FGuid Id=FGuid::NewGuid();
    if(!World->SpawnBuild(Id,B)) {World->Say(TEXT("Ground is still streaming")); return;}
    auto* S=World->State; S->Builds.Add(Id,B);
    if(B.Piece>=AETConstruction::Core) {
        const auto Cost=AETConstruction::Cost(B.Piece);
        S->Inventory.FindOrAdd(TEXT("ironore"))-=Cost.X; S->Inventory.FindOrAdd(TEXT("goldore"))-=Cost.Y;
    } else if(B.FromKit) --S->BuildStock;
    else S->Inventory.FindOrAdd(AETConstruction::Resource(B.MaterialTier))-=AETConstruction::FrameCost(B.MaterialTier);
    World->Sound(TEXT("FieldLatch")); World->Save(); RefreshBuild();
}
void AETCharacter::SalvageBuild()
{
    const FVector Eye=Camera->GetComponentLocation();
    FHitResult H; FCollisionQueryParams Q(SCENE_QUERY_STAT(ETSalvageFrame),false,this);
    if(!GetWorld()->LineTraceSingleByChannel(H,Eye,Eye+Camera->GetForwardVector()*1500,ECC_Visibility,Q)) return;
    auto* A=Cast<AETConstruction>(H.GetActor());
    if(!A||!World->State->Builds.Contains(A->Id)) return;
    const auto B=World->State->Builds.FindRef(A->Id);
    if(B.Piece>=AETConstruction::Core) {
        const auto Cost=AETConstruction::Cost(B.Piece);
        World->State->Inventory.FindOrAdd(TEXT("ironore"))+=B.Iron+Cost.X/2;
        World->State->Inventory.FindOrAdd(TEXT("goldore"))+=B.GoldOre+Cost.Y/2;
    } else if(B.FromKit) ++World->State->BuildStock;
    else World->State->Inventory.FindOrAdd(AETConstruction::Resource(B.MaterialTier))+=AETConstruction::FrameCost(B.MaterialTier);
    World->RemoveBuild(A);
    World->Sound(TEXT("FieldLatch"),.45f,.7f); World->Save(); RefreshBuild();
}
