#include "ETGeology.h"
#include "ETGame.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "ProceduralMeshComponent.h"

using namespace ETRock;

namespace
{
    template<typename Func> void CoveredPatches(const FBox& Box,Func Visit)
    {
        const auto A=Chunk(Box.Min), B=Chunk(Box.Max);
        for(int32 Y=A.Y;Y<=B.Y;++Y) for(int32 X=A.X;X<=B.X;++X) Visit(FIntPoint(X,Y));
    }
}
void UETGeology::RegisterGroundProp(AStaticMeshActor* A)
{
    auto* C=A->GetStaticMeshComponent();
    if(!C||!C->GetStaticMesh()) return;
    const FName Mesh=C->GetStaticMesh()->GetFName();
    if(AETWorld::IsTree(Mesh)) {
        if(World->State->Consumed.Contains(World->TreeId(A->GetActorLocation()))) {
            C->SetVisibility(false); A->SetActorEnableCollision(false); return;
        }
        if(C->GetCollisionEnabled()==ECollisionEnabled::NoCollision) {
            C->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
            C->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
        }
    }
    const bool Slab=Mesh==TEXT("SM_Block")&&C->Bounds.BoxExtent.Z<=18;
    const bool Scatter=Mesh==TEXT("SM_Tree")||Mesh==TEXT("SM_Pine")||Mesh==TEXT("SM_PineB")||
        Mesh==TEXT("SM_Birch")||Mesh==TEXT("SM_DeadTree")||Mesh==TEXT("SM_Rock")||Mesh==TEXT("SM_Shrub")||
        Mesh==TEXT("SM_Fern")||Mesh==TEXT("SM_Stump")||Mesh==TEXT("SM_FallenLog")||Mesh==TEXT("SM_Rubble");
    C->ComponentTags.AddUnique(Slab?TEXT("ET_Paving"):TEXT("ET_GroundProp"));
    if(!Slab&&!Scatter) C->ComponentTags.AddUnique(TEXT("ET_Foundation"));
    if(Mesh==TEXT("SM_Rock")) C->ComponentTags.AddUnique(TEXT("ET_LooseRock"));
    bool Edited=false;
    CoveredPatches(C->Bounds.GetBox(),[&](FIntPoint Key){GroundProps.FindOrAdd(Key).AddUnique(A); Edited|=ReplacesSurface(Key);});
    if(Edited) UpdateProp(*A);
}
void UETGeology::ForgetGroundProp(AStaticMeshActor* A)
{
    auto* C=A->GetStaticMeshComponent();
    if(!C||(!C->ComponentHasTag(TEXT("ET_Paving"))&&!C->ComponentHasTag(TEXT("ET_GroundProp")))) return;
    Pavement.Remove(A);
    CoveredPatches(C->Bounds.GetBox(),[&](FIntPoint Key){
        if(auto* List=GroundProps.Find(Key)) {List->Remove(A); if(List->IsEmpty()) GroundProps.Remove(Key);}
    });
}
void UETGeology::ClearScatter(AETRegion& R,FIntPoint Patch)
{
    const double X=Patch.X*double(Span),Y=Patch.Y*double(Span);
    const FBox Area(FVector(X,Y,-100000),FVector(X+Span,Y+Span,100000));
    for(auto* Group:R.Instances) if(IsValid(Group)) {
        TArray<int32> Removed;
        for(int32 I:Group->GetInstancesOverlappingBox(Area,true)) {
            FTransform Transform;
            if(!Group->GetInstanceTransform(I,Transform,true)) continue;
            const auto P=Transform.GetLocation();
            if(P.X>=X&&P.X<=X+Span&&P.Y>=Y&&P.Y<=Y+Span&&SurfaceOpening(P.X,P.Y)<-12) Removed.Add(I);
        }
        if(!Removed.IsEmpty()) Group->RemoveInstances(Removed);
    }
}
void UETGeology::UpdateProp(AStaticMeshActor& Actor)
{
    auto* Source=Actor.GetStaticMeshComponent();
    if(!Source->ComponentHasTag(TEXT("ET_Paving"))) {
        const FVector P=Actor.GetActorLocation();
        if(Source->ComponentHasTag(TEXT("ET_Foundation"))) {
            const FBox Bounds=Source->Bounds.GetBox();
            FVector Center=Bounds.GetCenter(),Half=Bounds.GetExtent()*.88;
            const FName Name=Source->GetStaticMesh()->GetFName();
            if(Name==TEXT("SM_Sign")) {
                for(int Post:{-180,180}) {
                    const FVector Foot=Source->GetComponentTransform().TransformPosition(FVector(Post,0,0));
                    if(SurfaceOpening(Foot.X,Foot.Y)>=-12) return;
                }
            }
            if(Name==TEXT("SM_Lamp")) {Center=P; Half=FVector(7,7,0);}
            // These separate meshes make up a single house, including upper-storey furniture.
            if(Name==TEXT("SM_House")||Name==TEXT("SM_Roof")||Name==TEXT("SM_Interior")||Name==TEXT("SM_Tenement")) {
                Center=P; Half=FVector(490,465,0);
            }
            if(Name!=TEXT("SM_Sign")) for(int Y=-1;Y<=1;++Y) for(int X=-1;X<=1;++X)
                if(SurfaceOpening(Center.X+X*Half.X,Center.Y+Y*Half.Y)>=-12) return;
        } else if(SurfaceOpening(P.X,P.Y)>=-12) return;
        Source->SetVisibility(false,true); Source->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        return;
    }
    const FBox Box=Source->Bounds.GetBox();
    const FVector Size=Box.GetSize(), Base(Box.Min.X,Box.Min.Y,Box.Max.Z);
    const int32 NX=FMath::Max(1,FMath::CeilToInt(Size.X/85)), NY=FMath::Max(1,FMath::CeilToInt(Size.Y/85));
    TArray<double> D; D.Reserve((NX+1)*(NY+1));
    bool Cut=false;
    auto Point=[&](int32 X,int32 Y){return FVector(X*Size.X/NX,Y*Size.Y/NY,0);};
    for(int32 Y=0;Y<=NY;++Y) for(int32 X=0;X<=NX;++X) {
        const FVector P=Base+Point(X,Y);
        const double Value=SurfaceOpening(P.X,P.Y)-18;
        D.Add(Value); Cut|=Value<0;
    }
    if(!Cut) return;
    TArray<FVector> V,Normals; TArray<FVector2D> UV; TArray<int32> Indices;
    auto Triangle=[&](FVector A,FVector B,FVector C,double DA,double DB,double DC) {
        const FVector P[]={A,B,C}; const double Dist[]={DA,DB,DC};
        const int32 Start=V.Num();
        auto Vertex=[&](FVector Q){V.Add(Q); Normals.Add(FVector::UpVector); UV.Add(FVector2D(Q.X,Q.Y)/400.);};
        for(int32 I=0;I<3;++I) {
            const int32 J=(I+1)%3;
            if(Dist[I]>=0) Vertex(P[I]);
            if((Dist[I]>=0)!=(Dist[J]>=0)) Vertex(FMath::Lerp(P[I],P[J],Dist[I]/(Dist[I]-Dist[J])));
        }
        for(int32 I=Start+2;I<V.Num();++I) Indices.Append({Start,I-1,I});
    };
    for(int32 Y=0;Y<NY;++Y) for(int32 X=0;X<NX;++X) {
        const int32 A=Y*(NX+1)+X,B=A+1,C=A+NX+1,E=C+1;
        Triangle(Point(X,Y),Point(X,Y+1),Point(X+1,Y),D[A],D[C],D[B]);
        Triangle(Point(X+1,Y),Point(X,Y+1),Point(X+1,Y+1),D[B],D[C],D[E]);
    }
    auto* Mesh=Pavement.FindRef(&Actor).Get();
    if(!Mesh) {
        Mesh=NewObject<UProceduralMeshComponent>(&Actor);
        Mesh->SetupAttachment(Source); Mesh->SetAbsolute(true,true,true);
        Mesh->SetWorldLocation(Base); Mesh->SetCastShadow(false); Mesh->SetCullDistance(Source->LDMaxDrawDistance);
        Mesh->bUseAsyncCooking=false; Mesh->bUseComplexAsSimpleCollision=true; Mesh->SetCollisionProfileName(TEXT("BlockAll"));
        Mesh->ComponentTags.Add(TEXT("ET_Paving")); Mesh->RegisterComponent(); Pavement.Add(&Actor,Mesh);
    }
    if(!Indices.IsEmpty()) {
        Mesh->CreateMeshSection_LinearColor(0,V,Indices,Normals,UV,{}, {},true);
        Mesh->SetMaterial(0,Source->GetMaterial(0));
    } else Mesh->ClearAllMeshSections();
    Source->SetVisibility(false,false); Source->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
