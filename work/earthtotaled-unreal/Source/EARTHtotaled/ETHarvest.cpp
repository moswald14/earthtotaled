#include "ETGame.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Kismet/GameplayStatics.h"

bool AETWorld::IsTree(FName Name)
{
    return Name==TEXT("SM_Trunk")||Name==TEXT("SM_Tree")||Name==TEXT("SM_Pine")||
        Name==TEXT("SM_PineB")||Name==TEXT("SM_Birch")||Name==TEXT("SM_DeadTree");
}
FString AETWorld::TreeId(FVector Root)
{
    // The crown and its simple trunk collider have the same world-space root.
    return FString::Printf(TEXT("tree/%lld/%lld"),FMath::RoundToInt64(Root.X),FMath::RoundToInt64(Root.Y));
}
bool AETWorld::TreeRoot(const FHitResult& Hit,FVector& Root) const
{
    const auto* Mesh=Cast<UStaticMeshComponent>(Hit.GetComponent());
    if(!Mesh||!Mesh->GetStaticMesh()||!IsTree(Mesh->GetStaticMesh()->GetFName())) return false;
    if(const auto* Group=Cast<UInstancedStaticMeshComponent>(Mesh)) {
        FTransform Tree;
        if(!Group->GetInstanceTransform(Hit.Item,Tree,true)) return false;
        Root=Tree.GetLocation();
    } else Root=Mesh->GetComponentLocation();
    return true;
}
bool AETWorld::FellTree(const FHitResult& Hit)
{
    FVector Root;
    if(Hit.Distance>400||!TreeRoot(Hit,Root)) return false;
    const FString Id=TreeId(Root);
    if(State->Consumed.Contains(Id)) return false;
    if(auto* Region=Cast<AETRegion>(Hit.GetActor())) {
        for(auto* Group:Region->Instances) {
            if(!Group->GetStaticMesh()||!IsTree(Group->GetStaticMesh()->GetFName())) continue;
            for(int32 I=Group->GetInstanceCount()-1;I>=0;--I) {
                FTransform T;
                if(Group->GetInstanceTransform(I,T,true)&&T.GetLocation().Equals(Root,.75)) Group->RemoveInstance(I);
            }
        }
    } else if(auto* Tree=Cast<AStaticMeshActor>(Hit.GetActor())) {
        Tree->SetActorEnableCollision(false); Tree->GetStaticMeshComponent()->SetVisibility(false);
    } else return false;
    State->Consumed.Add(Id); State->Inventory.FindOrAdd(TEXT("wood"))+=8;
    if(auto* Survivor=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0))) Survivor->AddXP(6);
    Sound(TEXT("FieldLatch"),.64f,.62f); Say(TEXT("Tree felled   +8 wood   /   B to build")); Save();
    return true;
}
