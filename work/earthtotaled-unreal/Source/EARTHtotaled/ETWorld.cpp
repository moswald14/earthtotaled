#include "ETGame.h"
#include "ETLandfall.h"
#include "ETRingLine.h"
#include "ETRailDistricts.h"
#include "ETSaveDisk.h"
#include "ETGeology.h"
#include "ETTerrainMath.h"
#include "ETUnderstory.h"
#include "ETSpawnSafety.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "Engine/GameViewportClient.h"
#include "ProceduralMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Engine/PostProcessVolume.h"
#include "GameFramework/PlayerStart.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Components/PointLightComponent.h"
#include "Components/TextRenderComponent.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "HAL/PlatformTime.h"

namespace
{
    FString Str(const TSharedPtr<FJsonObject>& O, const TCHAR* K) { FString S; O->TryGetStringField(K,S); return S; }
    double Num(const TSharedPtr<FJsonObject>& O, const TCHAR* K, double D=0) { double N; return O->TryGetNumberField(K,N)?N:D; }
    uint32 Mix(uint32 V) {return ETMath::Mix(V);}
    double RawHeight(double X,double Y,uint32 Seed) {return ETMath::Height(X,Y,Seed);}
}

AETItem::AETItem()
{
    PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bStartWithTickEnabled=false;
    Mesh=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh")); SetRootComponent(Mesh);
    Mesh->SetCollisionProfileName(TEXT("BlockAll"));
}

AETWorld::AETWorld() { PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true; }
void AETWorld::LoadDesign()
{
    if(State) return;
    SlotQA=FParse::Param(FCommandLine::Get(),TEXT("ETSlotQA"));
    GConfig->GetInt(SlotQA?TEXT("OutpostQA"):TEXT("Survivors"),TEXT("ActiveSlot"),ActiveSlot,GGameUserSettingsIni);
    ActiveSlot=FMath::Clamp(FCString::Atoi(GetWorld()->URL.GetOption(TEXT("ETSlot="),*FString::FromInt(ActiveSlot))),1,10);
    SaveSlot=SlotName(ActiveSlot);
    SelfTest=FParse::Param(FCommandLine::Get(),TEXT("ETSelfTest"));
    FParse::Value(FCommandLine::Get(),TEXT("ETJourney="),JourneyEnding);
    if(JourneyEnding!=TEXT("garden")&&JourneyEnding!=TEXT("signal")) JourneyEnding.Empty();
    JourneyResume=!JourneyEnding.IsEmpty()&&FParse::Param(FCommandLine::Get(),TEXT("ETJourneyResume"));
    if(!JourneyEnding.IsEmpty()) SaveSlot=TEXT("EARTHtotaled-QA-")+JourneyEnding;
    FString ReviewScene; FParse::Value(FCommandLine::Get(),TEXT("ETReview="),ReviewScene); Review=SelfTest||!ReviewScene.IsEmpty();
    HighwayTour=(ReviewScene==TEXT("bug")||ReviewScene==TEXT("supertech"))&&FParse::Param(FCommandLine::Get(),TEXT("ETHighwayTour"));
    MineResume=FParse::Param(FCommandLine::Get(),TEXT("ETReadMineQA"));
    MineWalk=ReviewScene==TEXT("mine")&&FParse::Param(FCommandLine::Get(),TEXT("ETWalkMine"));
    if(MineResume) SaveSlot=TEXT("EARTHtotaled-QA-Faultline");
    FrontierQA=FParse::Param(FCommandLine::Get(),TEXT("ETFieldJourney"));
    if(FrontierQA) SaveSlot=TEXT("EARTHtotaled-QA-Fieldworks");
    BorerRecoveryQA=FParse::Param(FCommandLine::Get(),TEXT("ETBorerRecoveryQA"));
    if(BorerRecoveryQA) SaveSlot=TEXT("EARTHtotaled-QA-BorerRecovery");
    Review|=!JourneyEnding.IsEmpty()||MineResume;
    if(Review&&JourneyEnding.IsEmpty()&&!MineResume&&!FrontierQA) SaveSlot=TEXT("EARTHtotaled-QA-Preview");
    if((!Review||JourneyResume||MineResume)&&UGameplayStatics::DoesSaveGameExist(SaveSlot,0)) State=Cast<UETSave>(UGameplayStatics::LoadGameFromSlot(SaveSlot,0));
    HadSave=State&&State->Schema==1;
    if(!HadSave) {State=Cast<UETSave>(UGameplayStatics::CreateSaveGameObject(UETSave::StaticClass())); State->BuildStock=0; if(!Review&&!FrontierQA&&!BorerRecoveryQA) State->ArrivalStage=0;}
    if(!State->HasRingDisplay) {
        if(State->RenderScale==100) State->RenderScale=125;
        State->HasRingDisplay=true;
    }
    if(Review&&FParse::Param(FCommandLine::Get(),TEXT("ETPerfTest"))) FParse::Value(FCommandLine::Get(),TEXT("ETRenderScale="),State->RenderScale);
    if(!State->ModernDay) {
        if(HadSave) {State->ClockOffset=ETDay::FromOld(State->Elapsed); State->Shield=State->Health<25?0.f:60.f; State->ShieldDelay=10;}
        else {State->ClockOffset=ETDay::At(9);State->RetroDisplay=false;}
        State->ModernDay=true;
    }
    if(Review&&JourneyEnding.IsEmpty()&&!MineResume) {
        State->ClockOffset=ETDay::At(16.5);
        State->Elapsed=1;
        if(ReviewScene==TEXT("forest")) {State->Position=FVector(44000,4000,2000); State->View=FRotator(0,150,0);}
        if(ReviewScene==TEXT("city")) {State->Position=FVector(157800,-8050,2000); State->View=FRotator(0,68,0);}
        if(ReviewScene==TEXT("night")) State->Elapsed=1080;
        if(ReviewScene==TEXT("bug")||ReviewScene==TEXT("supertech")) {
            const auto Site=ETNav::Site(ReviewScene==TEXT("bug")?FIntPoint(-1,0):FIntPoint::ZeroValue,0);
            State->Position=FVector(Site.At(0,-6700),0);
            State->View=FRotator(Site.Bug?12:23,FVector(Site.Center-FVector2D(State->Position),0).Rotation().Yaw,0);
            State->Elapsed=2350; State->RetroDisplay=false;
        }
        if(ReviewScene==TEXT("bench")) {State->Position=FVector(300,-40,110); State->View=FRotator(-10,90,0);}
        if(ReviewScene==TEXT("mine")) {State->Position=FVector(410,-2750,110); State->View=FRotator(-17,0,0);}
    }
    if(Review&&FParse::Param(FCommandLine::Get(),TEXT("ETPhoto"))) {PhotoScene=ReviewScene;State->IntroRead=true;State->RetroDisplay=false;State->ClockOffset=ETDay::At(11);State->Elapsed=0;}
    for(auto& Pair:State->Towns) if(Pair.Value.Wave>0) Pair.Value.Claimed=true;
    PrepareLandscape();
    PrepareHighwayGrades();
    Geology=NewObject<UETGeology>(this);
    Geology->Initialize(this);
    FString Source;
    TSharedPtr<FJsonObject> Root;
    if(!FFileHelper::LoadFileToString(Source,*(FPaths::ProjectContentDir()/TEXT("Data/design.json"))) || !FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Source),Root) || !Root)
    {
        UE_LOG(LogTemp,Error,TEXT("EARTHtotaled design.json is missing. Run Tools/export-legacy.mjs before building."));
        Weapons.Add(FETWeapon()); State->Ammo={24}; State->Reserve={180}; State->WeaponDamage.SetNumZeroed(1); State->Weapon=0; return;
    }
    for(const auto& V:Root->GetArrayField(TEXT("weapons")))
    {
        auto O=V->AsObject(); FETWeapon W; W.Name=Str(O,TEXT("name")); W.Mag=Num(O,TEXT("mag")); W.Reserve=Num(O,TEXT("reserve"));
        W.Damage=Num(O,TEXT("damage")); W.Rate=Num(O,TEXT("rate")); W.Reload=Num(O,TEXT("reload")); W.Spread=Num(O,TEXT("spread")); W.Pellets=Num(O,TEXT("pellets")); W.Range=Num(O,TEXT("range"))*100;
        Weapons.Add(W);
    }
    for(const auto& V:Root->GetArrayField(TEXT("skills")))
    {
        auto O=V->AsObject(); FETPerk P; P.Id=Str(O,TEXT("id")); P.Name=Str(O,TEXT("name")); P.Description=Str(O,TEXT("description")); P.Branch=Str(O,TEXT("branch")); P.Cost=Num(O,TEXT("cost"));
        P.Position=FVector2D(Num(O,TEXT("x")),Num(O,TEXT("y")));
        for(const auto& R:O->GetArrayField(TEXT("requires"))) P.Neighbors.Add(R->AsString()); Perks.Add(P);
    }
    for(const auto& V:Root->GetArrayField(TEXT("quests")))
    {
        auto O=V->AsObject(); FETQuest Q; Q.Id=Str(O,TEXT("id")); Q.Giver=Str(O,TEXT("giver")); Q.Title=Str(O,TEXT("title")); Q.Description=Str(O,TEXT("description")); Q.Item=Str(O,TEXT("item"));
        Q.Water=Num(O,TEXT("water")); Q.Gold=Num(O,TEXT("gold")); Q.Weapon=Num(O,TEXT("weapon"),-1); Quests.Add(Q);
    }
    for(const auto& V:Root->GetArrayField(TEXT("story"))) { auto O=V->AsObject(); Recordings.Add(Str(O,TEXT("title"))+TEXT("\n")+Str(O,TEXT("speaker"))+TEXT("\n\n")+Str(O,TEXT("text"))); }
    for(const auto& V:Root->GetArrayField(TEXT("chapters"))) { auto O=V->AsObject(); ChapterNames.Add(Str(O,TEXT("title"))); Objectives.Add(Str(O,TEXT("objective"))); }
    LoadExpansion();
    LoadMods();
    for(int32 I=State->Ammo.Num();I<Weapons.Num();++I) State->Ammo.Add(Weapons[I].Mag);
    State->WeaponDamage.SetNumZeroed(Weapons.Num());
    for(int32 I=0;I<Weapons.Num();++I) State->Ammo[I]=FMath::Clamp(State->Ammo[I],0,Weapons[I].Mag);
    if(State->Elapsed>=1) State->IntroRead=true;
    for(int32 I=State->Reserve.Num();I<Weapons.Num();++I) State->Reserve.Add(Weapons[I].Reserve);
    State->Weapon=FMath::Clamp(State->Weapon,0,Weapons.Num()-1); State->Cores=FMath::Clamp(State->Cores,0,3);
}
void AETWorld::BeginPlay()
{
    Super::BeginPlay(); LoadDesign(); BuildStory();
    for(TActorIterator<AActor> It(GetWorld());It;++It) if(It->ActorHasTag(TEXT("ET_BakedStart"))) {
        StoryActors.Add(*It);
        if(auto* Prop=Cast<AStaticMeshActor>(*It)) {
            Geology->RegisterGroundProp(Prop);
            auto* Component=Prop->GetStaticMeshComponent();
            if(const auto* Mesh=Component->GetStaticMesh().Get()) {
                const FName Name=Mesh->GetFName();
                if(Name==TEXT("SM_House")||Name==TEXT("SM_Tenement")) Component->EmptyOverrideMaterials();
                if(Name==TEXT("SM_House")||Name==TEXT("SM_Warehouse")||Name==TEXT("SM_Tenement")||Name==TEXT("SM_Canopy")||Name==TEXT("SM_Shop")) {
                    const FBox Bounds=Component->Bounds.GetBox();
                    ShelterBuildings.Emplace(FVector2D(Bounds.Min),FVector2D(Bounds.Max));
                }
            }
        }
    }
    WorldPopulated=true;
    for(const auto& It:Regions) RestoreFoundations(It.Value);
    LastNight=IsNight(); UpdateVisibility(State->Position);
}
uint32 AETWorld::SeedFor(FIntPoint C,int32 Salt) const { return Mix(uint32(C.X)*73856093u^uint32(C.Y)*19349663u^uint32(State->Seed)^uint32(Salt)*83492791u); }
FVector AETWorld::HouseAnchor(FIntPoint C,int32 Index) const
{
    FRandomStream R(SeedFor(C,Index+100));
    return FVector(C.X*RegionSize + (Index==0?8500:23500)+R.FRandRange(-2200,2200), C.Y*RegionSize+16000+R.FRandRange(-6500,6500),0);
}
FVector AETWorld::CityCenter(double X,double Y) const
{
    // An unbounded lattice of districts, separated by roughly a kilometre of forest.
    const double Spacing=ETNav::DistrictSpacing;
    const FIntPoint Cell=ETNav::District(FVector2D(X,Y));
    return FVector(Cell.X*Spacing,Cell.Y*Spacing,Cell==FIntPoint::ZeroValue?0:RawHeight(Cell.X*Spacing,Cell.Y*Spacing,uint32(State->Seed)));
}
bool AETWorld::InCity(FVector P) const {return FVector::Dist2D(P,CityCenter(P.X,P.Y))<27000;}
FString AETWorld::PlaceName(FVector P) const
{
    auto C=CityCenter(P.X,P.Y); double D=FVector::Dist2D(P,C);
    if(D<27000) {
        const auto Cell=ETNav::District(FVector2D(C));
        const int RailCity=ETRing::CityIn(Cell);
        return RailCity>0?FString(ETRing::CityName(RailCity)).ToUpper()+TEXT("  /  ")+TownStatus(Cell):TownStatus(Cell);
    }
    if(const auto* R=Regions.FindRef({FMath::FloorToInt32(P.X/RegionSize),FMath::FloorToInt32(P.Y/RegionSize)}))
        for(const auto& S:R->HighwaySites) if(S.InGrounds(FVector2D(P),1700.))
            return FString(S.Bug?TEXT("BUG / "):TEXT("SUPERTECH / "))+FString(S.Name()).ToUpper();
    if(D<38000) return TEXT("THE OUTSKIRTS / ABANDONED HOMESTEADS");
    return TEXT("BLACKPINE FOREST / UNCHARTED WILDERNESS");
}
float AETWorld::Hour() const {return ETDay::Hour(State->ClockOffset+State->Elapsed);}
bool AETWorld::IsNight() const {const float H=Hour(); return H>=19||H<6;}
void AETWorld::UpdateNightWatch(FVector Viewer)
{
    const bool Night=IsNight();
    if(LastNight==Night) return;
    LastNight=Night;
    for(TActorIterator<AETAgent> It(GetWorld());It;++It) {
        It->UpdatePresence(Viewer);
        if(Night) It->AttackTime=FMath::Max(It->AttackTime,2.f);
    }
    if(Night) {State->NextPatrol=FMath::Min(State->NextPatrol,8.f); PatrolTick=0;}
    else for(TActorIterator<AETEnemyBolt> It(GetWorld());It;++It) if(It->Power>0&&!It->FrontBolt) It->Destroy();
    Say(Night?TEXT("19:00   Nightfall. Hostiles are emerging."):TEXT("06:00   Daybreak. Roaming patrols withdraw. Fighting continues at the highway exits."));
}
FString AETWorld::StoryTitle() const
{
    if(State->ArrivalStage==1) return TEXT("FIRST WATCH / SHELTER 09");
    if(State->ArrivalStage==2&&State->Cores==0) return TEXT("THE SECOND TRANSMISSION");
    if(State->Ending==TEXT("garden")) return TEXT("THE FIRST RAIN");
    if(State->Ending==TEXT("signal")) return TEXT("A LIGHT IN THE DARK");
    return ChapterNames.IsValidIndex(State->Cores)?ChapterNames[State->Cores]:TEXT("KEEP GOING");
}
FString AETWorld::StoryObjective() const
{
    if(State->ArrivalStage==1) {const int Side=ETLandfall::NextBank(State->TrenchSides); if(Side>=0) return FString::Printf(TEXT("G: borer  /  fire at the %s trench stake"),ETLandfall::Bank(Side));}
    if(State->ArrivalStage==2&&State->Cores==0) return TEXT("Follow Eli\'s radio call to the transit core");
    if(!State->Ending.IsEmpty()) return TEXT("Keep exploring / help settlements / stay alive");
    return Objectives.IsValidIndex(State->Cores)?Objectives[State->Cores]:TEXT("Explore the wilderness");
}
void AETWorld::UpdateDaylight()
{
    if(!Sun||!Moon||!Sky) return;
    const float Angle=(Hour()-6.f)/24.f*2*PI, Elevation=FMath::Sin(Angle);
    const float Day=FMath::SmoothStep(-.08f,.22f,Elevation);
    Sun->SetActorRotation(FRotator(-Elevation*75,Hour()*15-120,0));
    auto SL=Cast<UDirectionalLightComponent>(Sun->GetLightComponent());
    SL->SetIntensity(7.f*Day*(1-Weather.Cloud*.83f)); SL->SetLightColor(FMath::Lerp(FLinearColor(1,.53f,.30f),FLinearColor(1,.97f,.92f),FMath::Clamp(Elevation*2,0.f,1.f)));
    Moon->SetActorRotation(FRotator(Elevation*65,Hour()*15+60,0)); Moon->GetLightComponent()->SetIntensity(.23f*(1-Day));
    Sky->GetLightComponent()->SetIntensity((.32f+Day*.85f)*(1-Weather.Cloud*.28f));
    if(Fog) {
        Fog->GetComponent()->SetFogDensity(.009f+(1-Day)*.016f+Weather.Haze*.065f);
        const FLinearColor Fair=FMath::Lerp(FLinearColor(.09f,.13f,.21f),FLinearColor(.30f,.39f,.50f),Day);
        const FLinearColor Wet=FMath::Lerp(FLinearColor(.07f,.10f,.16f),FLinearColor(.24f,.29f,.33f),Day);
        Fog->GetComponent()->SetFogInscatteringColor(FMath::Lerp(Fair,Wet,Weather.Cloud));
    }
    if(Atmosphere) {
        // Unreal's scattering coefficients are measured per kilometre.
        Atmosphere->SetMieScatteringScale(.003996f + Weather.Haze*.026f + Weather.Cloud*.01f);
        Atmosphere->SetSkyLuminanceFactor(FMath::Lerp(FLinearColor::White,FLinearColor(.64f,.71f,.80f),Weather.Cloud));
    }
    if(CloudMaterial) {
        CloudMaterial->SetScalarParameterValue(TEXT("Coverage"),Weather.Cloud);
        CloudMaterial->SetVectorParameterValue(TEXT("CloudTint"),FMath::Lerp(FLinearColor(.012f,.018f,.03f),FLinearColor(.32f,.36f,.39f),Day)*(1-Weather.Storminess*.42f));
    }
}
double AETWorld::Height(double X,double Y) const
{
    double H=Landscape(X,Y);
    const double Distance=FMath::Sqrt(X*X+Y*Y);
    double T=FMath::Clamp((Distance-16000)/11000.0,0.0,1.0); T=T*T*(3-2*T);
    H*=T;
    const FVector City=CityCenter(X,Y); const double CD=FVector::Dist2D(FVector(X,Y,0),City);
    if(City.Size2D()>1) {double Blend=FMath::Clamp((CD-28000)/8000.0,0.0,1.0); Blend=Blend*Blend*(3-2*Blend); H=FMath::Lerp(City.Z,H,Blend);}
    FIntPoint C(FMath::FloorToInt(X/RegionSize),FMath::FloorToInt(Y/RegionSize));
    for(int32 DY=-1;DY<=1;++DY) for(int32 DX=-1;DX<=1;++DX) for(int32 I=0;I<2;++I)
    {
        auto P=HouseAnchor(C+FIntPoint(DX,DY),I);
        if(InCity(P)) continue;
        const double D=FMath::Max(FMath::Abs(X-P.X)/1100.0,FMath::Abs(Y-P.Y)/1400.0);
        double Blend=FMath::Clamp((D-1)/1.2,0.0,1.0); Blend=Blend*Blend*(3-2*Blend);
        if(Blend<1) H=FMath::Lerp(Landscape(P.X,P.Y),H,Blend);
    }
    return GradeHighway(X,Y,H);
}
UMaterialInterface* AETWorld::Material(const FString& Name) const
{
    return LoadObject<UMaterialInterface>(nullptr,*(TEXT("/Game/EARTHtotaled/Materials/M_")+Name+TEXT(".M_")+Name));
}
void AETWorld::DressMesh(UStaticMeshComponent* Component,const FString& BaseMaterial) const
{
    if(!Component||!Component->GetStaticMesh()) return;
    const auto& Slots=Component->GetStaticMesh()->GetStaticMaterials();
    for(int32 I=0;I<Slots.Num();++I) {
        FString Slot=Slots[I].MaterialSlotName.ToString(); FString Name=BaseMaterial;
        for(const TCHAR* Known:{TEXT("Steel"),TEXT("DarkMetal"),TEXT("Rubber"),TEXT("Wood"),TEXT("Bark"),TEXT("Glass"),TEXT("Greenline"),TEXT("Skin"),TEXT("Signal"),TEXT("Gold"),TEXT("Leaf"),TEXT("Pine"),TEXT("Moss"),TEXT("BirchBark"),TEXT("Brick"),TEXT("Plaster"),TEXT("Concrete"),TEXT("Rust"),TEXT("Canvas"),TEXT("CircuitLight"),TEXT("ResinLight"),TEXT("AlloyHull"),TEXT("Carapace"),TEXT("Pallor"),TEXT("RotCloth"),TEXT("GreenBlade"),TEXT("DryBlade"),TEXT("Hide")})
            if(Slot==Known||Slot.EndsWith(FString(TEXT("_"))+Known)) {Name=Known; break;}
        Component->SetMaterial(I,Material(Name));
    }
}
AActor* AETWorld::Static(const FString& Asset,const FString& Mat,FVector P,FVector Scale,FRotator Rotation)
{
    auto A=GetWorld()->SpawnActor<AStaticMeshActor>(P,Rotation);
    auto C=A->GetStaticMeshComponent(); C->SetMobility(EComponentMobility::Movable);
    FString Path=Asset.StartsWith(TEXT("/"))?Asset:TEXT("/Game/EARTHtotaled/Art/")+Asset+TEXT(".")+Asset;
    C->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Path)); DressMesh(C,Mat);
    C->SetCollisionProfileName(TEXT("BlockAll")); C->SetWorldScale3D(Scale); C->SetCastShadow(Asset!=TEXT("SM_Block")); C->SetCullDistance(State->ViewDistance*100.f);
    Geology->RegisterGroundProp(A);
    return A;
}
AETItem* AETWorld::Item(const FString& Id,const FString& Kind,const FString& Label,FVector P,int32 Amount,const FString& Value)
{
    if(State->Consumed.Contains(Id)) return nullptr;
    if(const FVector* Moved=State->Settled.Find(Id)) P=*Moved;
    auto A=GetWorld()->SpawnActor<AETItem>(P,FRotator::ZeroRotator); A->Id=Id; A->Kind=Kind; A->Label=Label; A->Amount=Amount; A->Value=Value;
    FString Asset=(Kind==TEXT("human")||Kind==TEXT("friend"))?TEXT("SM_Contact"):Kind==TEXT("core")?TEXT("SM_Core"):Kind==TEXT("trap")?TEXT("SM_Trap"):TEXT("SM_Cache");
    if(Kind==TEXT("container")||Kind==TEXT("loot")) {
        FString Type=Kind==TEXT("loot")?(Amount==0?TEXT("pantry"):Amount==1?TEXT("medical"):Amount==2?TEXT("armory"):TEXT("tools")):Value;
        Asset=Type==TEXT("medical")?TEXT("SM_MedCase"):Type==TEXT("armory")?TEXT("SM_WeaponCase"):Type==TEXT("tools")?TEXT("SM_Toolbox"):Type==TEXT("electronics")?TEXT("SM_ElectronicsCase"):TEXT("SM_PantryCase");
    }
    if(Kind==TEXT("weapon")) Asset=TEXT("SM_WeaponCase");
    if(const int Seed=ETLandfall::RelicSeed(Kind)) {Asset=FString::Printf(TEXT("SM_Weapon%d"),ETLandfall::FRoll(Seed).Frame);A->SetActorScale3D(FVector(1.4));}
    if(Kind==TEXT("tower")) Asset=TEXT("SM_HillCistern");
    if(Kind==TEXT("stake")) Asset=TEXT("SM_Sign");
    if(Kind==TEXT("survey")) Asset=TEXT("SM_Sign");
    // "gold" is the legacy saved drop kind; new and previously dropped currency are wrappers.
    if(Kind==TEXT("gold")) Asset=TEXT("SM_Wrappers");
    else if(Kind==TEXT("townboard")) Asset=TEXT("SM_Sign");
    else if(Kind==TEXT("medkit")) {Asset=TEXT("SM_MedCase");A->SetActorScale3D(FVector(.38));}
    else if(Kind==TEXT("gunparts")||Kind==TEXT("scrap")||Kind.StartsWith(TEXT("mod."))) {Asset=TEXT("SM_ElectronicsCase");A->SetActorScale3D(FVector(.30));}
    else if(Kind==TEXT("ironore")||Kind==TEXT("goldore")||Kind==TEXT("chitin")) {Asset=TEXT("SM_Rock");A->SetActorScale3D(FVector(.25));}
    else if(Kind==TEXT("cloth")||Kind==TEXT("harvest")) {Asset=TEXT("SM_PantryCase");A->SetActorScale3D(FVector(.24));}
    A->Mesh->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Asset+TEXT(".")+Asset)));
    DressMesh(A->Mesh,Kind==TEXT("friend")?TEXT("Greenline"):Kind==TEXT("core")?TEXT("Signal"):Kind==TEXT("human")?TEXT("Canvas"):TEXT("Steel"));
    if(Kind==TEXT("goldore")||Kind==TEXT("ironore")||Kind==TEXT("chitin"))
        A->Mesh->SetMaterial(0,Material(Kind==TEXT("goldore")?TEXT("FaultGold"):Kind==TEXT("ironore")?TEXT("FaultIron"):TEXT("Carapace")));
    A->Mesh->SetCullDistance(16000);
    if(Kind==TEXT("container")&&Value==TEXT("medical")) A->Mesh->SetMaterial(0,Material(TEXT("Canvas")));
    return A;
}
void AETWorld::EnsureRegion(FIntPoint C)
{
    if(auto* Existing=Regions.FindRef(C)) {
        if(Existing->Stage<6) {while(!Existing->AdvanceGenerate()) {} RestoreDrops(Existing);}
        return;
    }
    auto R=GetWorld()->SpawnActor<AETRegion>(); Regions.Add(C,R); R->Generate(this,C); RestoreDrops(R);
}
void AETWorld::RestoreDrops(AETRegion* Region)
{
    for(const auto& Pair:State->Builds) {
        const FVector& At=Pair.Value.Anchor;
        if(FMath::FloorToInt(At.X/RegionSize)==Region->Coord.X&&FMath::FloorToInt(At.Y/RegionSize)==Region->Coord.Y) SpawnBuild(Pair.Key,Pair.Value);
    }
    for(auto It:State->DropPositions)
    {
        if(State->Consumed.Contains(It.Key)) continue;
        FIntPoint C(FMath::FloorToInt(It.Value.X/RegionSize),FMath::FloorToInt(It.Value.Y/RegionSize)); if(C!=Region->Coord) continue;
        FString Kind=State->DropKinds.FindRef(It.Key);
        // Adopt an already-live drop if its tile was not ready when the encounter
        // died. Repeated restoration must never create a second reward for one ID.
        AETItem* Drop=LooseActors.FindRef(It.Key).Get();
        if(!Drop) Drop=Item(It.Key,Kind,DropLabel(Kind),It.Value,State->DropAmounts.FindRef(It.Key));
        if(Drop) {
            LooseActors.Add(It.Key,Drop); Drop->Mesh->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
            if(auto Previous=Cast<AETRegion>(Drop->GetOwner())) if(Previous!=Region) Previous->Residents.Remove(Drop);
            Drop->SetOwner(Region); Region->Residents.AddUnique(Drop);
        }
    }
    if(WorldPopulated) RestoreFoundations(Region);
}
void AETWorld::Tick(float Delta)
{
    Super::Tick(Delta);
    auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0)); if(!P||P->Panel==TEXT("travel")) return;
    static const bool ArrivalQA=FParse::Param(FCommandLine::Get(),TEXT("ETLandfallQA"))&&FCString::Strifind(FCommandLine::Get(),TEXT("-UserDir="));
    if(ArrivalQA) ETLandfall::DriveTest(*this,*P);
    static const bool RingQA=FParse::Param(FCommandLine::Get(),TEXT("ETRingRide"))&&FCString::Strifind(FCommandLine::Get(),TEXT("-UserDir="));
    if(RingQA&&RingTrain) RingTrain->TestRide(*P);
    static const bool TransitQA=FParse::Param(FCommandLine::Get(),TEXT("ETInterchange"))&&FCString::Strifind(FCommandLine::Get(),TEXT("-UserDir="));
    if(TransitQA&&RingTrain) RingTrain->TestInterchange(*P);
    if(P->Panel==TEXT("arrival")) {P->TickArrival();return;}
    if(SlotQA&&!SelfTest) TickSlotJourney(P);
    if(FrontierQA) TickFrontierJourney(P,Delta);
    if(BorerRecoveryQA) TickBorerRecoveryCheck(P);
    if(SelfTest&&!TestRan) {TestRan=true; RunSelfTest(); return;}
    if(MineResume) {CheckMineRestart(P); return;}
    if(MineWalk) {WalkMineCheck(P); return;}
    if(!JourneyEnding.IsEmpty()) TickJourneyVerification(Delta);
    if(HighwayTour) TickHighwayTour(P);
    if(!PhotoScene.IsEmpty()) TickReclamationPhoto(P);
    TickAtmosphere(Delta);
    // Only the score/mix updates while menus pause survival, weather and streaming.
    if(UGameplayStatics::IsGamePaused(this)) return;
    if(AtRecoveryShelter) {
        AtRecoveryShelter=false;
        if(Save()) Say(TEXT("Shelter 09 / wounds treated / equipment and excavations recovered"));
    }
    NoticeTime=FMath::Max(0.f,NoticeTime-Delta);
    // Opt-in, disposable rendered timing sample; never runs in a normal saved game.
    static const bool PerfEnabled=FParse::Param(FCommandLine::Get(),TEXT("ETPerfTest"));
    static double PerfStart=0,PerfLast=0; static bool PerfFinished=false; static TArray<double> FrameTimes;
    if(Review&&PerfEnabled&&!PerfFinished) {
        const double Now=FPlatformTime::Seconds(); if(!PerfStart) {PerfStart=Now; PerfLast=Now;}
        if(Now-PerfStart>5) FrameTimes.Add((Now-PerfLast)*1000); PerfLast=Now;
        if(Now-PerfStart>30&&FrameTimes.Num()>30) {
            PerfFinished=true; FrameTimes.Sort(); double Total=0; for(double T:FrameTimes) Total+=T;
            const double Average=Total/FrameTimes.Num(),P95=FrameTimes[FMath::FloorToInt((FrameTimes.Num()-1)*.95)];
            const auto* Client=GetWorld()->GetGameViewport();
            const FIntPoint Size=Client&&Client->Viewport?Client->Viewport->GetSizeXY():FIntPoint::ZeroValue;
            const FString Report=FString::Printf(TEXT("{\"samples\":%d,\"average_ms\":%.3f,\"p95_ms\":%.3f,\"average_fps\":%.2f,\"render_scale\":%d,\"output_width\":%d,\"output_height\":%d,\"loaded_regions\":%d,\"scope\":\"25 second stationary view after 5 second warmup\"}"),FrameTimes.Num(),Average,P95,1000/Average,State->RenderScale,Size.X,Size.Y,Regions.Num());
            FFileHelper::SaveStringToFile(Report,*(FPaths::ProjectSavedDir()/TEXT("graphics-performance.json")));
            if(FParse::Param(FCommandLine::Get(),TEXT("ETPerfExit"))&&FCString::Strifind(FCommandLine::Get(),TEXT("-UserDir="))) FPlatformMisc::RequestExit(false);
            P->OpenPanel(TEXT("pause"),TEXT("GRAPHICS CHECK COMPLETE"),TEXT("Temporary test session / your saved survivor is untouched."));
        }
    }
    LightTime-=Delta; if(LightTime<=0) {LightTime=.25f; UpdateDaylight();}
    auto Pos=P->GetActorLocation(); UpdateNightWatch(Pos); StreamAround(Pos);
    TickTowns(P,Delta); TickPatrols(P,Delta); TickWarFronts(P,Delta); TickOutposts(P,Delta);
    VisibilityTime-=Delta; if(VisibilityTime<=0) {VisibilityTime=.5f; UpdateVisibility(Pos);}
    if(Ambience.Num()==3) {
        const float CityMix=InCity(Pos)?1.f:0.f, Night=IsNight()?1.f:0.f;
        const float Targets[]={.65f*(1-CityMix)*(1-Night),.7f*(1-CityMix)*Night,.7f*CityMix};
        for(int32 I=0;I<3;++I) Ambience[I]->SetVolumeMultiplier(FMath::FInterpTo(Ambience[I]->VolumeMultiplier,Targets[I],Delta,.5f));
    }
    SaveTime+=Delta; if(SaveTime>20) {Save(); SaveTime=0;}

}
bool AETWorld::Save()
{
    auto P=Cast<AETCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
    if(!State||!P||P->Panel==TEXT("travel")||State->Health<=0||(Review&&JourneyEnding.IsEmpty())) return false;
    State->Position=P->Climbing?P->ClimbStart:P->GetActorLocation(); State->View=P->GetControlRotation();
    if(auto* Line=AETRingLine::PassengerLine(*P)) Line->Capture(*P);
    else State->OnRing=false;
    for(auto It:LiveThreats) if(IsValid(It.Value)) if(auto* R=State->Threats.Find(It.Key)) It.Value->Snapshot(*R);
    if(!ETSaveDisk::Write(State,SaveSlot)) {Say(TEXT("Save failed. Check available storage.")); return false;}
    HadSave=true; return true;
}
void AETWorld::Say(const FString& Text) {Notice=Text; NoticeTime=7;}

void AETWorld::BuildStory()
{
    Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,18000),FRotator(-21,-42,0));
    auto Light=Cast<UDirectionalLightComponent>(Sun->GetLightComponent()); Light->SetMobility(EComponentMobility::Movable);
    Light->SetIntensity(5.5f); Light->SetLightColor(FLinearColor(1,.79f,.55f)); Light->SetAtmosphereSunLight(true); Light->SetLightSourceAngle(1.1f);
    Moon=GetWorld()->SpawnActor<ADirectionalLight>();
    auto Moonlight=Cast<UDirectionalLightComponent>(Moon->GetLightComponent());
    Moonlight->SetMobility(EComponentMobility::Movable); Moonlight->SetAtmosphereSunLightIndex(1); Moonlight->SetLightColor(FLinearColor(.43f,.58f,1.f));
    auto AtmosphereActor=GetWorld()->SpawnActor<ASkyAtmosphere>(); Atmosphere=AtmosphereActor->GetComponent(); Atmosphere->SetMieScatteringScale(.003996f); Atmosphere->SetRayleighScatteringScale(.0331f);
    Sky=GetWorld()->SpawnActor<ASkyLight>(); Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true); Sky->GetLightComponent()->SetIntensity(1.1f);
    Fog=GetWorld()->SpawnActor<AExponentialHeightFog>(); auto FC=Fog->GetComponent();
    FC->SetFogDensity(.012f); FC->SetFogHeightFalloff(.22f); FC->SetVolumetricFog(true);
    auto PP=GetWorld()->SpawnActor<APostProcessVolume>(); PP->bUnbound=true;
    PP->Settings.bOverride_AmbientOcclusionIntensity=true;PP->Settings.AmbientOcclusionIntensity=.68f;
    PP->Settings.bOverride_AmbientOcclusionRadius=true;PP->Settings.AmbientOcclusionRadius=62.f;
    PP->Settings.bOverride_AutoExposureMinBrightness=true; PP->Settings.AutoExposureMinBrightness=.2f;
    PP->Settings.bOverride_AutoExposureMaxBrightness=true; PP->Settings.AutoExposureMaxBrightness=2.f;
    PP->Settings.bOverride_BloomIntensity=true; PP->Settings.BloomIntensity=.25f;
    auto House=[this](FString Id,FVector P,FString Person)
    {
        auto Lamp=NewObject<UPointLightComponent>(this); Lamp->SetIntensity(1600); Lamp->SetAttenuationRadius(1000); Lamp->SetLightColor(FLinearColor(1,.66f,.31f)); Lamp->SetCastShadows(false); Lamp->RegisterComponent(); Lamp->SetWorldLocation(P+FVector(0,0,300));
        Item(Id,TEXT("human"),Person,P+FVector(-220,200,20),0,Person);
        Item(Id+TEXT("-bench"),TEXT("bench"),TEXT("Work bench"),P+FVector(300,300,80));
    };
    House(TEXT("bunker"),FVector(0,0,0),TEXT("Nora"));
    for(const FVector& Root:{FVector(-1320,-1950,0),FVector(-1590,-2390,0),FVector(1420,-2250,0),FVector(1790,-1940,0)})
        StoryActors.Add(Static(TEXT("SM_Birch"),TEXT("BirchBark"),Root,FVector(.75),FRotator(0,Root.X*.1,0)));
    House(TEXT("riverside"),FVector(-12000,-4000,0),TEXT("Ada"));
    House(TEXT("cistern"),FVector(11500,8500,0),TEXT("Malik"));
    House(TEXT("transit-aid"),FVector(-9000,8500,0),TEXT("Ivo"));
    for(int32 Row=-1;Row<=1;++Row) for(int32 Col=-1;Col<=1;++Col) {
        FVector P(Col*7000+1000,Row*7000-1800,0); if(P.Size2D()<3500) continue;
        Item(FString::Printf(TEXT("city-%d,%d"),Col,Row),TEXT("loot"),TEXT("Search household supplies"),P+FVector(-300,200,70),FMath::Abs(Col+Row)%3);
    }
    Item(TEXT("salt"),TEXT("friend"),TEXT("SALT-7 / Greenline"),FVector(1100,500,0),0,TEXT("SALT-7"));
    if(State->ArrivalStage>=0) MakeTrenchStakes();
    if(State->ArrivalStage<0) if(auto* Marker=Item(TEXT("faultline-survey"),TEXT("survey"),TEXT("Read the mine survey"),FVector(750,-3110,0))) {
        auto* Sign=NewObject<UTextRenderComponent>(Marker); Sign->SetupAttachment(Marker->Mesh);
        Sign->SetText(FText::FromString(TEXT("SHELTER MINE\nQUARRY 067")));
        Sign->SetHorizontalAlignment(EHTA_Center); Sign->SetVerticalAlignment(EVRTA_TextCenter);
        Sign->SetWorldSize(38); Sign->SetTextRenderColor(FColor(255,224,150));
        Sign->SetRelativeLocation(FVector(0,16,330)); Sign->SetRelativeRotation(FRotator(0,90,0)); Sign->RegisterComponent();
    }
    // Battery work lamps mark the descent and pick out the two exposed seams.
    if(State->ArrivalStage<0) for(const FVector Point:{FVector(790,-2940,170),FVector(2500,-2910,-500),FVector(3500,-2880,-680)}) {
        auto* Lamp=NewObject<UPointLightComponent>(this); Lamp->SetLightColor(FLinearColor(1,.61f,.27f));
        Lamp->SetIntensity(2800); Lamp->SetAttenuationRadius(840); Lamp->SetCastShadows(false); Lamp->RegisterComponent(); Lamp->SetWorldLocation(Point);
    }
    const FVector Sites[]={FVector(-11500,11500,0),FVector(12500,-9000,0),FVector(-12500,-11500,0)};
    const TCHAR* Labels[]={TEXT("Transit core"),TEXT("Botanical core"),TEXT("Observatory core")};
    for(int32 I=0;I<3;++I)
    {
        Item(FString::Printf(TEXT("core-%d"),I),TEXT("core"),Labels[I],Sites[I]+FVector(0,100,100),I);
        FString GuardId=FString::Printf(TEXT("guard-%d"),I);
        FVector Border=Sites[I].GetSafeNormal2D()*29100.; Border.Z=SurfaceHeight(Border.X,Border.Y)+10;
        if(!State->Consumed.Contains(GuardId)&&!SettledCity(Border)) if(auto* A=GetWorld()->SpawnActor<AETAgent>(Border,FRotator::ZeroRotator)) A->Configure(this,GuardId,TEXT("robot"));
    }
    Item(TEXT("filter"),TEXT("quest"),TEXT("Ceramic water filter"),Sites[0]+FVector(220,200,40),0,TEXT("filter"));
    Item(TEXT("pump"),TEXT("quest"),TEXT("Pump fuse"),Sites[1]+FVector(220,200,40),0,TEXT("pump"));
    Item(TEXT("medicine"),TEXT("quest"),TEXT("Sealed medicine"),FVector(-12000,-4200,40),0,TEXT("medicine"));
    auto Route=Item(TEXT("relay-ladder"),TEXT("climb"),TEXT("Climb service ladder / Climbing Grip"),FVector(10900,8500,70));
    if(Route) Route->Destination=FVector(11100,8500,540);
    Item(TEXT("relay"),TEXT("quest"),TEXT("Rooftop relay module"),FVector(11500,8700,440),0,TEXT("relay"));
    Item(TEXT("uplink"),TEXT("uplink"),TEXT("HELIOS / final command"),FVector(0,-15000,80));
    for(int32 I=0;I<6;++I)
        Item(FString::Printf(TEXT("relief-%d"),I),TEXT("water"),TEXT("Sealed relief water"),FVector(2200+I*1900,2000-I*600,40),2);
    Item(TEXT("starter-drifter"),TEXT("weapon"),State->ArrivalStage<0?TEXT("Drifter .44 / ranger's sidearm"):TEXT("RUST-9 / ranger's carbine"),FVector(-2800,-600,60),State->ArrivalStage<0?6:0);
    Item(TEXT("starter-medical"),TEXT("container"),TEXT("Search first-aid case"),FVector(2800,-500,60),0,TEXT("medical"));
    Item(TEXT("starter-pantry"),TEXT("container"),TEXT("Search shelter provisions"),FVector(-1600,-100,40),0,TEXT("relief"));
    Item(TEXT("starter-tools"),TEXT("container"),TEXT("Search repair depot"),FVector(-5000,1300,40),0,TEXT("tools"));
    Item(TEXT("starter-electronics"),TEXT("container"),TEXT("Search radio parts"),FVector(-6000,1400,40),0,TEXT("electronics"));
    Item(TEXT("starter-fuel"),TEXT("container"),TEXT("Search service station"),FVector(5000,500,50),0,TEXT("tools"));
    for(int32 I=0;I<4;++I) {
        const FVector Buildings[]={FVector(-7200,-6500,0),FVector(8200,4000,0),FVector(-2500,6500,0),FVector(6500,11500,0)};
        Item(FString::Printf(TEXT("starter-rooftop-%d"),I),TEXT("container"),TEXT("Search upstairs stash"),Buildings[I]+FVector(-330,260,830),0,TEXT("armory"));
    }
    for(const TCHAR* Name:{TEXT("ForestDay"),TEXT("ForestNight"),TEXT("CityWind")}) {
        auto A=NewObject<UAudioComponent>(this); A->bAutoActivate=false; A->bAllowSpatialization=false; A->SetSound(Sounds.FindRef(Name)); A->RegisterComponent(); A->SetVolumeMultiplier(0); A->Play(); Ambience.Add(A);
    }
    InitAtmosphere(); ETFlora::Start(this);
    // Interactive story actors remain persistent, but rendering and AI obey player distance.
    for(TActorIterator<AETItem> It(GetWorld());It;++It) if(!It->GetOwner()) StoryActors.Add(*It);
    for(TActorIterator<AETAgent> It(GetWorld());It;++It) if(!It->GetOwner()) StoryActors.Add(*It);
    UpdateDaylight();
}

AETRegion::AETRegion()
{
    Terrain=CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Terrain")); SetRootComponent(Terrain);
    Terrain->bUseAsyncCooking=false; Terrain->bUseComplexAsSimpleCollision=true;
    Terrain->SetCollisionProfileName(TEXT("BlockAll"));
}
void AETRegion::Generate(AETWorld* W,FIntPoint C)
{
    BeginGenerate(W,C); while(!AdvanceGenerate()) {}
}
void AETRegion::BeginGenerate(AETWorld* W,FIntPoint C)
{
    World=W; Coord=C; Stage=0; Random.Initialize(W->SeedFor(C));
    WantedTerrainGrid=W->RegionDistance(C,W->State->Position)<16000?160:80;
    SetActorLocation(FVector(C.X*AETWorld::RegionSize,C.Y*AETWorld::RegionSize,0));
    const FVector2D Origin(GetActorLocation());
    const auto Area=FBox2D(Origin,Origin+FVector2D(AETWorld::RegionSize)).ExpandBy(1300);
    ETNav::Roads(Area,[this](const ETNav::FRoad& Road){LocalRoads.Add(Road);});
    ETNav::Sites(Area,[this](const ETNav::FSite& S){HighwaySites.Add(S);});
}
bool AETRegion::AdvanceGenerate()
{
    if(Stage>=6) return true;
    auto W=World; const auto C=Coord; const FVector Origin=GetActorLocation(); auto& R=Random;
    if(Stage==0) {
    if(!BuildTerrainSlice()) return false;
    } else if(Stage==1) {
    auto Instance=[&](FString Asset,FString Mat)
    {
        auto H=NewObject<UHierarchicalInstancedStaticMeshComponent>(this); H->SetupAttachment(Terrain);
        H->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*(TEXT("/Game/EARTHtotaled/Art/")+Asset+TEXT(".")+Asset)));
        W->DressMesh(H,Mat); H->SetCollisionEnabled(ECollisionEnabled::NoCollision); H->SetCullDistances(20000,W->State->ViewDistance*100); H->SetCastShadow(Asset!=TEXT("SM_Grass")); H->RegisterComponent(); Instances.Add(H); return H;
    };
    auto Rocks=Instance(TEXT("SM_Rock"),TEXT("Concrete")); auto Grass=Instance(TEXT("SM_BladePatch"),TEXT("GreenBlade")); auto Weeds=Instance(TEXT("SM_RoadWeed"),TEXT("GreenBlade"));
    for(auto* Cover:{Grass,Weeds}) {Cover->ComponentTags.Add(TEXT("ET_DetailNear"));Cover->SetCastShadow(false);Cover->SetCullDistances(8500,12500);} auto Trees=Instance(TEXT("SM_Tree"),TEXT("Bark")); auto Pines=Instance(TEXT("SM_Pine"),TEXT("Pine"));
    auto PinesB=Instance(TEXT("SM_PineB"),TEXT("Pine")); auto Birches=Instance(TEXT("SM_Birch"),TEXT("BirchBark"));
    auto Trunks=Instance(TEXT("SM_Trunk"),TEXT("Bark")); Trunks->SetCollisionProfileName(TEXT("BlockAll")); Trunks->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Trunks->SetCastShadow(false);
    for(int32 I=0;I<1100;++I)
    {
        FVector P(R.FRandRange(0,32000),R.FRandRange(0,32000),0); const FVector WP=Origin+P;
        if(W->InCity(WP)) continue;
        bool Pad=false; for(int32 J=0;J<2;++J) if(FVector::Dist2D(WP,W->HouseAnchor(C,J))<2400) Pad=true; if(Pad) continue;
        P.Z=W->Height(WP.X,WP.Y)-3;
        float S=R.FRandRange(.75,1.55); auto H=I<300?(I%9==0?Trees:I%5==0?Birches:I%2==0?PinesB:Pines):I<420?Rocks:Grass;
        FTransform T(FRotator(0,R.FRandRange(0,360),0),P,FVector(S,S,S));
        if(OnHighway(FVector2D(WP),I<420?155.:25.)||InHighwayGrounds(FVector2D(WP))) continue;
        if(I<300&&W->State->Consumed.Contains(W->TreeId(WP))) continue;
        H->AddInstance(T);
        if(I<300) Trunks->AddInstance(T);
    }
    FRandomStream Meadow(W->SeedFor(C,8467));
    for(int I=0;I<44;++I) {
        const FVector2D Root(Meadow.FRandRange(1400,30600),Meadow.FRandRange(1400,30600));
        const FVector Mid=Origin+FVector(Root,0);
        if(W->InCity(Mid)||InHighwayGrounds(FVector2D(Mid))||FVector::Dist2D(Mid,W->HouseAnchor(C,0))<2250||FVector::Dist2D(Mid,W->HouseAnchor(C,1))<2250) continue;
        for(int J=0;J<34;++J) {
            FVector P(Root+FVector2D(Meadow.FRandRange(-1120,1120),Meadow.FRandRange(-1120,1120)),0);const auto WP=Origin+P;
            if(OnHighway(FVector2D(WP),15)||W->Geology->SurfaceOpening(WP.X,WP.Y)<25) continue;
            P.Z=W->SurfaceHeight(WP.X,WP.Y)-2;const float S=Meadow.FRandRange(.75,1.60);
            (J%8==0?Weeds:Grass)->AddInstance(FTransform(FRotator(0,Meadow.FRandRange(0,360),0),P,FVector(S)));
        }
    }
    } else if(Stage==2) {
    for(int32 I=0;I<2;++I)
    {
        FVector P=W->HouseAnchor(C,I); if(W->InCity(P)||InHighwayGrounds(FVector2D(P),700)||OnHighway(FVector2D(P),750)) continue; P.Z=W->Height(P.X,P.Y);
        const FString Prefix=FString::Printf(TEXT("r%d,%d-house%d"),C.X,C.Y,I);
        Residents.Add(W->Static(TEXT("SM_House"),TEXT("Concrete"),P)); Residents.Add(W->Static(TEXT("SM_Roof"),TEXT("Steel"),P));
        Residents.Add(W->Static(TEXT("SM_Interior"),TEXT("Wood"),P));
        for(int32 J=0;J<3;++J)
        {
            auto Loot=W->Item(Prefix+FString::Printf(TEXT("-loot%d"),J),TEXT("loot"),TEXT("Search abandoned supplies"),P+FVector(-220+J*220,240,65),int32(W->SeedFor(C,I*3+J)%4));
            if(Loot) Residents.Add(Loot);
        }
        auto Ladder=W->Item(Prefix+TEXT("-ladder"),TEXT("climb"),TEXT("Climb roof / Climbing Grip"),P+FVector(-610,0,60));
        if(Ladder) {Ladder->Destination=P+FVector(-380,0,540); if(I==1) {Ladder->Value=TEXT("mantle"); Ladder->Label=TEXT("Advanced roof route / Over the Edge");} Residents.Add(Ladder);}
    }
    } else if(Stage==3) {
    // City streets and buildings use world-space IDs: unloading never rerolls their loot.
    FVector City=W->CityCenter(Origin.X+16000,Origin.Y+16000);
    if(City.Size2D()>1 && FVector::Dist2D(Origin+FVector(16000,16000,0),City)<52000) {
        auto Belongs=[&](FVector P) {return FMath::FloorToInt(P.X/AETWorld::RegionSize)==C.X && FMath::FloorToInt(P.Y/AETWorld::RegionSize)==C.Y;};
        for(int32 Y=-3;Y<=3;++Y) for(int32 X=-3;X<=3;++X) {
            FVector P=City+FVector(X*6400,Y*6400,0); if(!Belongs(P)) continue;
            const FString Prefix=FString::Printf(TEXT("city%d,%d-block%d,%d"),int32(City.X/160000),int32(City.Y/160000),X,Y);
            const int RailCity=ETRing::CityIn(ETNav::District(FVector2D(City)));
            const bool Landmark=(X==-1&&Y==0)||(X==1&&Y==0)||(X==0&&Y==-1)||(X==0&&Y==1)||(X==2&&Y==2)||(RailCity!=INDEX_NONE&&X==0&&Y==-2);
            for(int32 Side=-1;Side<=1&&!Landmark;Side+=2) {
                FVector B=P+FVector(Side*1600,0,0);
                bool Tall=(X+Y+Side)%3==0;
                if(RailCity==4||RailCity==9||RailCity==11||RailCity==14) Tall|=(X%2==0&&Y!=0);
                Residents.Add(W->Static(Tall?TEXT("SM_Tenement"):TEXT("SM_House"),TEXT("Concrete"),B));
                Residents.Add(W->Static(TEXT("SM_Interior"),TEXT("Wood"),B));
                if(!Tall) Residents.Add(W->Static(TEXT("SM_Roof"),TEXT("Rust"),B));
                for(int32 J=0;J<3;++J) if(auto Loot=W->Item(Prefix+FString::Printf(TEXT("-%d-%d"),Side,J),TEXT("loot"),TEXT("Search city apartment"),B+FVector(-300+J*220,200,70),J)) Residents.Add(Loot);
                if(Tall) {
                    if(auto Loot=W->Item(Prefix+FString::Printf(TEXT("-%d-upper"),Side),TEXT("loot"),TEXT("Upstairs emergency stores"),B+FVector(-330,260,450),0)) Residents.Add(Loot);
                    if(auto Loot=W->Item(Prefix+FString::Printf(TEXT("-%d-top"),Side),TEXT("loot"),TEXT("Top floor armory"),B+FVector(-330,260,830),2)) Residents.Add(Loot);
                }
            }
            Residents.Add(W->Static(TEXT("SM_Block"),TEXT("Asphalt"),P+FVector(0,-1700,7),FVector(64,8,.08)));
            Residents.Add(W->Static(TEXT("SM_Block"),TEXT("Asphalt"),P+FVector(3200,1500,7),FVector(8,64,.08)));
            const uint32 BlockSeed=W->SeedFor({X+int32(City.X/6400),Y+int32(City.Y/6400)},73);
            if(BlockSeed%4==0) Residents.Add(W->Static(TEXT("SM_Wreck"),TEXT("Rust"),P+FVector(700+BlockSeed%420,-1340,0),FVector::OneVector,FRotator(0,int(BlockSeed%17)-8,0)));
            for(int32 J=0;J<4;++J) Residents.Add(W->Static(TEXT("SM_Rock"),TEXT("Concrete"),P+FVector(-700+J*500,-900,0),FVector(.6f+J*.2f)));
            // Outlying human refuges let a long expedition resupply without returning to origin.
            if(X==0&&Y==0) {
                if(auto Human=W->Item(Prefix+TEXT("-refuge"),TEXT("human"),TEXT("Forest refuge keeper"),P+FVector(-1800,0,20),0,TEXT("Refuge Keeper"))) Residents.Add(Human);
                if(auto Bench=W->Item(Prefix+TEXT("-bench"),TEXT("bench"),TEXT("Refuge workbench"),P+FVector(-1300,250,75))) Residents.Add(Bench);
            }
        }
    }
    } else if(Stage==4) {
    for(int32 I=0;I<5;++I)
    {
        FVector P=Origin+FVector(R.FRandRange(3000,29000),R.FRandRange(3000,29000),0); if(P.Size2D()<20000) continue; P.Z=W->Height(P.X,P.Y)+30;
        FString Id=FString::Printf(TEXT("r%d,%d-agent%d"),C.X,C.Y,I); if(W->State->Consumed.Contains(Id)||(I<3&&W->SettledCity(P))) continue;
        auto A=GetWorld()->SpawnActor<AETAgent>(P,FRotator::ZeroRotator); A->SetOwner(this); A->Configure(W,Id,I==0?TEXT("robot"):I==1?TEXT("wolf"):I==2?TEXT("boar"):TEXT("deer")); Residents.Add(A);
    }
    for(int32 I=0;I<3;++I)
    {
        FVector P=Origin+FVector(R.FRandRange(1500,30000),R.FRandRange(1500,30000),0); if(P.Size2D()<20000) continue; P.Z=W->Height(P.X,P.Y)+8;
        auto Trap=W->Item(FString::Printf(TEXT("r%d,%d-trap%d"),C.X,C.Y,I),TEXT("trap"),TEXT("Disarm rusted trap"),P); if(Trap) Residents.Add(Trap);
    }    } else if(Stage==5) {AddDetails(); BuildHighwaySites(); AddTownLandmarks(); AddRailQuarter();}
    ++Stage;
    if(Stage>=6) {
        for(auto A:Residents) if(IsValid(A)) A->SetOwner(this);
        W->Geology->FinishRegion(*this);
    }
    return Stage>=6;
}

void AETRegion::EndPlay(const EEndPlayReason::Type Reason)
{
    for(auto A:Residents) if(IsValid(A)) {
        if(IsValid(World)) {
            if(auto* Frame=Cast<AETConstruction>(A)) World->LiveBuilds.Remove(Frame->Id);
            if(auto* Drop=Cast<AETItem>(A);Drop&&World->LooseActors.FindRef(Drop->Id).Get()==Drop) World->LooseActors.Remove(Drop->Id);
        }
        if(IsValid(World)&&World->Geology) if(auto* Prop=Cast<AStaticMeshActor>(A)) World->Geology->ForgetGroundProp(Prop);
        A->Destroy();
    }
    Super::EndPlay(Reason);
}

FVector AETWorld::ResolveStart(FVector P) const
{
    const double Surface=SurfaceHeight(P.X,P.Y);
    const bool Underground=P.Z<Surface&&Geology->IsOpen(P)&&Geology->IsOpen(P+FVector(0,0,65));
    if(!Underground) P.Z=FMath::Max(P.Z,Surface+110);
    return P;
}

bool AETWorld::FindShelterLanding(FVector& Out) const
{
    // Search the loaded shelter, including the player's changes to the ground.
    const FVector Shelter=GetDefault<UETSave>()->Position;
    FCollisionQueryParams Q(SCENE_QUERY_STAT(ETShelterLanding),false);
    for(int32 Ring=0;Ring<=6;++Ring) for(int32 Side=0;Side<(Ring?12:1);++Side) {
        const double Angle=PI*Side/6.;
        FVector Spot=Shelter+FVector(FMath::Cos(Angle)*Ring*250,FMath::Sin(Angle)*Ring*250,0);
        if(Spot.SizeSquared2D()>FMath::Square(2250.)) continue;
        Spot.Z=SurfaceHeight(Spot.X,Spot.Y);
        FHitResult Floor;
        if(!GetWorld()->LineTraceSingleByChannel(Floor,Spot+FVector(0,0,520),Spot-FVector(0,0,65),ECC_Pawn,Q)||
           Floor.ImpactNormal.Z<.72||Cast<AETItem>(Floor.GetActor())||Cast<AETAgent>(Floor.GetActor())) continue;
        const FVector Landing=Floor.ImpactPoint+FVector(0,0,100);
        if(GetWorld()->OverlapBlockingTestByChannel(Landing,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(34,92),Q)) continue;
        Out=Landing; return true;
    }
    return false;
}
bool ETSpawn::FindClear(AETWorld& World,FVector Checkpoint,FVector& Result)
{
    const auto* Capsule=GetDefault<AETCharacter>()->GetCapsuleComponent();
    const float Half=Capsule->GetUnscaledCapsuleHalfHeight();
    const auto Shape=FCollisionShape::MakeCapsule(Capsule->GetUnscaledCapsuleRadius()+1.f,Half+1.f);
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ETCheckpointClearance),false);
    auto Try=[&](const FVector& At) {
        if(At.Z<World.SurfaceHeight(At.X,At.Y)&&!World.Geology->IsOpen(At)) return false;
        if(World.GetWorld()->OverlapBlockingTestByChannel(At,FQuat::Identity,ECC_Pawn,Shape,Query)) return false;
        Result=At;
        return true;
    };
    if(Try(Checkpoint)) return true;
    // A crouched survivor, a sloped roof or a reloaded prop may occupy the standing capsule.
    for(double Radius:{12.,28.,55.,95.,145.,220.,340.,500.,750.,1100.}) {
        if(Try(Checkpoint+FVector(0,0,Radius))) return true;
        for(int Side=0;Side<8;++Side) {
            const double Angle=Side*PI/4.;
            const FVector Out(FMath::Cos(Angle),FMath::Sin(Angle),0);
            if(Try(Checkpoint+Out*Radius)||Try(Checkpoint+(Out+FVector::UpVector).GetSafeNormal()*Radius)) return true;
        }
        if(Try(Checkpoint-FVector(0,0,Radius))) return true;
    }
    // Find the top of an unusually tall obstruction at the same horizontal position.
    FHitResult Top;
    const auto Above=Checkpoint+FVector(0,0,30000);
    if(World.GetWorld()->LineTraceSingleByChannel(Top,Above,Checkpoint,ECC_Pawn,Query))
        return Try(Top.ImpactPoint+FVector(0,0,Half+8.));
    return false;
}
AETGameMode::AETGameMode() {DefaultPawnClass=AETCharacter::StaticClass(); HUDClass=AETHUD::StaticClass();}
void AETGameMode::InitGame(const FString& MapName,const FString& Options,FString& Error)
{
    Super::InitGame(MapName,Options,Error); World=GetWorld()->SpawnActor<AETWorld>(); World->LoadDesign();
    for(int I=0;I<ETRing::Services;++I) {
        auto* Line=GetWorld()->SpawnActor<AETRingLine>();
        Line->Initialize(World,I);
        World->RailLines.Add(Line);
    }
    World->RingTrain=World->RailLines[ETRing::West];
    FVector P=World->State->Position;
    if(World->Review&&!World->JourneyResume&&!World->MineResume) P.Z=World->Height(P.X,P.Y)+110;
    else P=World->State->OnRing&&World->RailLines.IsValidIndex(World->State->RailService)?World->RailLines[World->State->RailService]->SavedPosition():World->ResolveStart(P);
    if(World->HadSave&&UGameplayStatics::HasOption(Options,TEXT("ETRecoverShelter"))) {
        World->StreamAround(GetDefault<UETSave>()->Position,true);
        if(World->FindShelterLanding(P)) {
            World->State->Position=P; World->State->WinchAnchor=P; World->State->View=GetDefault<UETSave>()->View;
            World->State->Health=FMath::Max(50.f,World->State->Health);
            World->State->Hydration=FMath::Max(50.f,World->State->Hydration);
            World->AtRecoveryShelter=true; World->State->OnRing=false;
        } else World->Say(TEXT("Shelter landing obstructed / resuming at your last save"));
    }
    World->StreamAround(P,true);
    FVector Clear;
    if(ETSpawn::FindClear(*World,P,Clear)) P=Clear;
    else {
        World->StreamAround(GetDefault<UETSave>()->Position,true);
        if(World->FindShelterLanding(Clear)) P=Clear;
    }
    FActorSpawnParameters Marker;
    Marker.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    Start=GetWorld()->SpawnActor<APlayerStart>(P,FRotator(0,World->State->View.Yaw,0),Marker);
}
AActor* AETGameMode::ChoosePlayerStart_Implementation(AController* Player) {return Start?Start:Super::ChoosePlayerStart_Implementation(Player);}
