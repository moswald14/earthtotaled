#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "GameFramework/SaveGame.h"
#include "ETWeatherMath.h"
#include "ETNavigation.h"
#include "ETDayCycle.h"
#include "ETWakeState.h"
#include "ETGame.generated.h"

class UCameraComponent;
class UStaticMeshComponent;
class UHierarchicalInstancedStaticMeshComponent;
class UProceduralMeshComponent;
class UMaterialInterface;
class AETWorld;
class AETRingLine;
class AETWakeVehicle;
class AETCharacter;
class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;
class USpotLightComponent;
class UAudioComponent;
class USoundBase;
class USoundAttenuation;
class UInstancedStaticMeshComponent;
class UMaterialInstanceDynamic;
class USkyAtmosphereComponent;
class UETGeology;
class UPointLightComponent;

struct FETSupply
{
    FString Id, Name, Description;
    bool Usable=false;
};
struct FETRecipe
{
    FString Id, Name, Output;
    int32 Scrap=0, Count=1;
    TMap<FString,int32> Ingredients;
};
struct FETLootEntry { FString Id; int32 Min=1, Max=1; };
struct FETModFind {FString Key,Name,Source,Use; int32 Chance=0,Yield=0;};
struct FETModPack {FString Id,Name; TArray<FETModFind> Finds; bool Enabled=true;};
struct FETVoiceLine { FString Speaker, Text; };

USTRUCT()
struct FETWeapon
{
    GENERATED_BODY()
    FString Name;
    int32 Mag=24, Reserve=180, Pellets=1;
    float Damage=24, Rate=.16f, Reload=1.45f, Spread=.011f, Range=9500;
};

USTRUCT()
struct FETRelic
{
    GENERATED_BODY()
    UPROPERTY() int32 Seed=1;
    UPROPERTY() int32 Loaded=0;
    UPROPERTY() float DamageDealt=0;
};

USTRUCT()
struct FETPerk
{
    GENERATED_BODY()
    FString Id, Name, Description, Branch;
    FVector2D Position=FVector2D::ZeroVector;
    int32 Cost=1;
    TArray<FString> Neighbors;
};

USTRUCT()
struct FETQuest
{
    GENERATED_BODY()
    FString Id, Giver, Title, Description, Item;
    int32 Water=0, Gold=0, Weapon=-1;
};

USTRUCT()
struct FETBuild
{
    GENERATED_BODY()
    UPROPERTY() FVector Anchor=FVector::ZeroVector;
    UPROPERTY() int32 Piece=0;
    UPROPERTY() int32 Facing=0;
    // 0 wood, 1 stone, 2 metal. Legacy frames were assembled from salvage kits.
    UPROPERTY() int32 MaterialTier=0;
    UPROPERTY() bool FromKit=true;
    UPROPERTY() bool Grown=false;
    UPROPERTY() float Integrity=160;
    UPROPERTY() float Work=0;
    UPROPERTY() int32 Iron=0;
    UPROPERTY() int32 GoldOre=0;
};

USTRUCT()
struct FETThreat
{
    GENERATED_BODY()
    UPROPERTY() FString Kind;
    UPROPERTY() FVector Position=FVector::ZeroVector;
    UPROPERTY() float Health=0;
    // Older patrol records did not keep the time until their next attack.
    UPROPERTY() float AttackCooldown=-1;
};

USTRUCT()
struct FETTown {
    GENERATED_BODY()
    UPROPERTY() bool Claimed=false;
    UPROPERTY() int32 Wave=0;
    UPROPERTY() int32 Dead=0;
    UPROPERTY() int32 Treasury=0;
    UPROPERTY() int32 Defenses=0;
    UPROPERTY() float Upkeep=0;
};

UCLASS()
class EARTHTOTALED_API UETSave : public USaveGame
{
    GENERATED_BODY()
public:
    UPROPERTY() int32 Schema=1;
    UPROPERTY() FETWakeSave Wake;
    UPROPERTY() int32 Seed=84371;
    UPROPERTY() FVector Position=FVector(0,-1500,110);
    UPROPERTY() FRotator View=FRotator(0,90,0);
    UPROPERTY() float Health=100;
    UPROPERTY() float Armor=40;
    UPROPERTY() float Shield=60;
    UPROPERTY() float ShieldDelay=0;
    UPROPERTY() bool ModernDay=false;
    UPROPERTY() double ClockOffset=1440;
    UPROPERTY() TMap<FIntPoint,FETTown> Towns;
    UPROPERTY() TMap<FIntPoint,float> TowerRefills;
    float MaxShield() const {return 60.f+15.f*ArmorTier;}
    UPROPERTY() float Hydration=100;
    UPROPERTY() float Elapsed=0;
    UPROPERTY() int32 Water=3;
    // Candybar-wrapper balance. Keep this serialized name so existing saves retain every wrapper.
    UPROPERTY() int32 Gold=0;
    UPROPERTY() int32 Scrap=0;
    UPROPERTY() int32 Meds=3;
    UPROPERTY() int32 Kills=0;
    UPROPERTY() int32 XP=0;
    UPROPERTY() int32 Level=1;
    int32 NextLevelXP() const {return FMath::Clamp(Level,1,1000000)*100;}
    UPROPERTY() int32 Points=2;
    UPROPERTY() int32 Weapon=0;
    UPROPERTY() int32 Cores=0;
    UPROPERTY() bool Welcome=false;
    UPROPERTY() bool Allied=false;
    UPROPERTY() FString Ending;
    UPROPERTY() TArray<FString> Skills={TEXT("survivor")};
    UPROPERTY() TArray<int32> Ammo;
    UPROPERTY() TArray<int32> Reserve;
    UPROPERTY() TArray<int32> Unlocked={0};
    UPROPERTY() TSet<FString> Consumed;
    UPROPERTY() TSet<FString> Items;
    UPROPERTY() TMap<FString,int32> Quests;
    UPROPERTY() TMap<FString,float> Rations;
    UPROPERTY() TMap<FString,float> AgentHealth;
    UPROPERTY() TMap<FString,FVector> DropPositions;
    UPROPERTY() TMap<FString,FVector> Settled;
    UPROPERTY() TMap<FString,int32> DropAmounts;
    UPROPERTY() TMap<FString,FString> DropKinds;
    UPROPERTY() TMap<FString,int32> Inventory;
    UPROPERTY() float StimTime=0;
    UPROPERTY() int32 ViewDistance=500;
    UPROPERTY() int32 RenderScale=100;
    UPROPERTY() bool HasRingDisplay=false;
    UPROPERTY() double RingClock=0;
    UPROPERTY() double EastRingClock=0;
    UPROPERTY() double NorthLinkClock=0;
    UPROPERTY() double SouthLinkClock=0;
    UPROPERTY() int32 RailService=0;
    UPROPERTY() bool OnRing=false;
    UPROPERTY() FVector RingOffset=FVector::ZeroVector;
    UPROPERTY() int32 MusicVolume=50;
    UPROPERTY() int32 WeatherVolume=75;
    // New optional fields keep the original slot and wrapper balance compatible.
    UPROPERTY() TArray<FIntVector> BoreCuts;
    UPROPERTY() TSet<FIntVector> MinedVeins;
    UPROPERTY() TSet<FIntVector> FallenRock;
    UPROPERTY() FVector WinchAnchor=FVector(0,-1500,110);
    UPROPERTY() bool RetroDisplay=true;
    UPROPERTY() TArray<FVector2D> MapPins;
    UPROPERTY() int32 TrackedPin=INDEX_NONE;
    UPROPERTY() bool LightAimAssist=true;
    UPROPERTY() TArray<FIntVector> SquareCuts;
    UPROPERTY() TSet<FIntVector> SealedGalleries;
    UPROPERTY() bool SquareBorer=false;
    UPROPERTY() TArray<float> WeaponDamage;
    UPROPERTY() float JetFuel=100;
    UPROPERTY() int32 BuildStock=12;
    UPROPERTY() TMap<FGuid,FETBuild> Builds;
    UPROPERTY() TMap<FString,FETThreat> Threats;
    UPROPERTY() int32 PatrolNumber=0;
    UPROPERTY() float NextPatrol=8;
    UPROPERTY() TMap<FString,float> FrontLosses;
    UPROPERTY() bool IntroRead=false;
    // Negative is an existing survivor whose original starting geology must remain accessible.
    UPROPERTY() int32 ArrivalStage=-1;
    UPROPERTY() int32 TrenchSides=0;
    UPROPERTY() TArray<FETRelic> Relics;
    UPROPERTY() int32 EquippedRelic=INDEX_NONE;
    UPROPERTY() int32 ArmorTier=0;
    UPROPERTY() float MaxDepth=0;
    UPROPERTY() int32 LandscapeEdition=0;
    UPROPERTY() TSet<FIntPoint> LegacyGround;
    UPROPERTY() bool HasGradedRoads=false;
    UPROPERTY() TSet<FIntPoint> OriginalRoadTerrain;
    UPROPERTY() int32 Waves=0;
    UPROPERTY() int32 RaidSerial=0;
    UPROPERTY() int32 RaidStage=0;
    UPROPERTY() float RaidTime=0;
    UPROPERTY() float RaidWait=100;
    UPROPERTY() FGuid RaidCore;
    float ArmorLimit() const {return 100.f+ArmorTier*35.f;}
};

UCLASS()
class AETConstruction : public AActor
{
    GENERATED_BODY()
public:
    AETConstruction();
    UPROPERTY() UStaticMeshComponent* Mesh;
    FGuid Id;
    FETBuild Design;
    void Configure(AETWorld* World,const FETBuild& Build,bool Preview=false,bool Valid=true);
    static const TCHAR* Name(int32 Piece);
    static FTransform Shape(const FETBuild& Build);
    static FVector Extents(int32 Piece);
    int32 Appearance=-1;
    static constexpr int32 Count=11, Core=4, Turret=5, Miner=6, Nursery=7, Hive=8, Fabricator=9, Spire=10;
    static FIntPoint Cost(int32 Piece);
    static float Durability(int32 Piece,int32 Tier=0);
    static FString Recipe(int32 Piece,int32 Tier=0);
    static const TCHAR* MaterialName(int32 Tier);
    static const TCHAR* Resource(int32 Tier);
    static int32 FrameCost(int32 Tier);
    static bool Joined(const FETBuild& A,const FETBuild& B);
    static bool HasFooting(AETWorld& World,const FETBuild& Build);
    UPROPERTY() UStaticMeshComponent* Mechanism;
};

UCLASS()
class EARTHTOTALED_API AETItem : public AActor
{
    GENERATED_BODY()
public:
    AETItem();
    UPROPERTY() UStaticMeshComponent* Mesh;
    FString Id, Kind, Label, Value;
    int32 Amount=0;
    FVector Destination=FVector::ZeroVector;
    FVector SpillOrigin=FVector::ZeroVector;
    float SpillAge=1;
    void Toss(FVector Start,FVector Finish);
    virtual void Tick(float Delta) override;
};

UCLASS()
class EARTHTOTALED_API AETAgent : public AActor
{
    GENERATED_BODY()
public:
    AETAgent();
    virtual void Tick(float Delta) override;
    virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    void Configure(AETWorld* InWorld, const FString& InId, const FString& InKind);
    UPROPERTY() USceneComponent* Pivot;
    UPROPERTY() TArray<UStaticMeshComponent*> Parts;
    UPROPERTY() AETWorld* World;
    FString Id, Kind;
    float Health=100, AttackTime=0, Phase=0;
    FVector Home=FVector::ZeroVector;
    bool Managed=false, FrontUnit=false;
    int32 TownIndex=INDEX_NONE;
    int32 WakeHerd=INDEX_NONE, HerdMember=0;
    bool TickWakeDeer(float Delta,AETCharacter& Player);
    void AttachBearLegs();
    void PoseBear(float Stride,float Windup);
    bool BearSwipe(AETCharacter& Player,float Delta,bool Clear);
    float BearWindup=0,BearStride=0;
    bool WakeEnemy() const;
    bool WakeTruce() const;
    FIntPoint TownCell=FIntPoint::ZeroValue;
    float RivalScan=0,PlayerAggro=0;
    float FlinchTime=0,ImpactCooldown=0;
    FVector FlinchVelocity=FVector::ZeroVector;
    FVector2D FlinchLean=FVector2D::ZeroVector;
    void ReceiveImpact(float Damage,const FDamageEvent& Event,AActor* Source);
    void TickImpact(float Delta);
    TWeakObjectPtr<AETAgent> Rival;
    AETAgent* FindRival(float Delta);
    FVector AimPoint() const;
    bool Opposes(const AETAgent* Other) const;
    void Snapshot(FETThreat& Record) const;
    bool IsMachine() const;
    bool IsBrood() const;
    bool Awake() const;
    void UpdatePresence(FVector Viewer);
    void Fly(float Delta,AETCharacter* Player,AETAgent* Enemy=nullptr);
};

UCLASS()
class EARTHTOTALED_API AETRegion : public AActor
{
    GENERATED_BODY()
public:
    AETRegion();
    void Generate(AETWorld* InWorld, FIntPoint InCoord);
    void BeginGenerate(AETWorld* InWorld, FIntPoint InCoord);
    bool AdvanceGenerate();
    void AddDetails();
    void AddTownLandmarks();
    void AddRailQuarter();
    bool BearDen(FVector& Site) const;
    void AddBearDen();
    bool BuildTerrainSlice();
    void RefreshSurfaceCollision();
    void BuildRoadSurface();
    bool OnHighway(FVector2D Position,double Margin=0) const;
    TArray<ETNav::FRoad> LocalRoads;
    TArray<ETNav::FSite> HighwaySites;
    void BuildHighwaySites();
    bool InHighwayGrounds(FVector2D P,double Extra=0) const;
    UPROPERTY() UProceduralMeshComponent* RoadMesh=nullptr;
    void TerrainIndices(int32 Grid,TArray<int32>& Indices) const;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    UPROPERTY() UProceduralMeshComponent* Terrain;
    UPROPERTY() TArray<AActor*> Residents;
    UPROPERTY() TArray<UHierarchicalInstancedStaticMeshComponent*> Instances;
    FIntPoint Coord;
    UPROPERTY() AETWorld* World;
    FRandomStream Random;
    int32 Stage=0;
    int32 TerrainGrid=0, WantedTerrainGrid=80, TerrainBuildGrid=0, TerrainRow=0;
    TArray<FVector> TerrainVertices;
    UPROPERTY() TMap<FIntVector,UProceduralMeshComponent*> RockMeshes;
    TMap<FIntPoint,TArray<double>> RockHeights;
};

UCLASS()
class EARTHTOTALED_API AETWorld : public AActor
{
    GENERATED_BODY()
public:
    AETWorld();
    UPROPERTY() AETRingLine* RingTrain=nullptr;
    UPROPERTY() TArray<AETRingLine*> RailLines;
    virtual void BeginPlay() override;
    virtual void Tick(float Delta) override;
    void LoadDesign();
    void EnsureRegion(FIntPoint Coord);
    void BuildStory();
    void RefreshWake();
    void TickWake(AETCharacter& Player,float Delta);
    void FindTrailClue(int Index);
    void PlayTrailClue();
    void SetWakePerks();
    UPROPERTY() TMap<FName,AETItem*> WakeObjects;
    UPROPERTY() TArray<AActor*> WakeScenery;
    UPROPERTY() TArray<AETAgent*> WakeDeer;
    UPROPERTY() AETWakeVehicle* WakeCar=nullptr;
    UPROPERTY() AETWakeVehicle* WakePlane=nullptr;
    int32 WakeLayout=INDEX_NONE;
    float WakePulse=0, WakeFlash=0;
    FVector WakeSpot(FName Node,int Index=0) const;
    void RestoreDrops(AETRegion* Region);
    bool Save();
    AETAgent* AddThreat(const FString& Id,const FString& Kind,FVector Position);
    void SplitRobot(AETAgent* Parent);
    void TickWarFronts(AETCharacter* Player,float Delta);
    AETAgent* FrontFighter(const FString& Id,const TCHAR* Kind,FVector Position);
    UPROPERTY() TMap<FString,AETAgent*> FrontUnits;
    float FrontClock=0;
    int32 BugHits=0,SupertechHits=0;
    bool HighwayTour=false;
    void TickHighwayTour(AETCharacter* Player);
    void RunHighwayChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void RunSurvivorChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void TickPatrols(AETCharacter* Player,float Delta);
    void TickOutposts(AETCharacter* Player,float Delta);
    void StepDevices(float Seconds);
    void StepRaid(AETCharacter* Player,float Seconds);
    bool Muster(FGuid Core);
    void EndRaid(bool Won);
    int32 ThreatTier() const;
    int32 BaseValue(FGuid Core) const;
    FGuid NearestCore(FVector Position,double Radius) const;
    bool RaidMember(const FString& Id) const;
    void HitBuild(AETConstruction* Target,float Amount);
    float OutpostClock=0;
    double Landscape(double X,double Y) const;
    void PrepareLandscape();
    void PrepareHighwayGrades();
    double GradeHighway(double X,double Y,double Natural) const;
    FString SlotName(int32 Number) const;
    void RememberSlot(int32 Number);
    int32 ActiveSlot=1;
    bool SlotQA=false;
    void TickSlotJourney(AETCharacter* Player);
    void RunOutpostChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    bool SafeSettlement(FVector Position) const;
    bool SettledCity(FVector Position) const;
    void MakeTrenchStakes();
    void RunLandfallChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    UPROPERTY() TMap<FString,AETAgent*> LiveThreats;
    float PatrolTick=0;
    bool AtRefuge=false;
    AETConstruction* SpawnBuild(FGuid Id,const FETBuild& Build);
    TMap<FGuid,TWeakObjectPtr<AETConstruction>> LiveBuilds;
    void RemoveBuild(AETConstruction* Build);
    void CheckFoundations(FVector Center,FVector Reach,bool InspectLoose=true);
    void SettleLooseItems(FVector Center,FVector Reach,bool OpeningsOnly=false);
    void RestoreFoundations(AETRegion* Region);
    bool WorldPopulated=false;
    int32 BreakBuild(const FGuid& Id);
    static bool IsTree(FName Mesh);
    static FString TreeId(FVector Root);
    bool FellTree(const FHitResult& Hit);
    bool TreeRoot(const FHitResult& Hit,FVector& Root) const;
    void RunHomesteadChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void RunNavigationChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void CheckAtlasInput(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void RunFrontierChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void TickFrontierJourney(AETCharacter* Player,float Delta);
    void UpdateDaylight();
    void InitAtmosphere();
    void TickAtmosphere(float Delta);
    void UpdateAtmosphereAudio(float Delta,bool Immediate=false);
    void UpdateRain(float Delta,FVector Eye);
    bool IsSheltered(FVector Eye) const;
    float RainFloor(FVector Position) const;
    FString WeatherName() const;
    int32 MusicFor(FVector Position) const;
    void RunSelfTest();
    void TickReclamationPhoto(AETCharacter* Player);
    FString PhotoScene; int PhotoStage=0; double PhotoTime=0;
    void RunReclamationChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void TickTowns(AETCharacter* Player,float Delta);
    int32 TownPopulation(FIntPoint Cell,int32 Wave=0) const;
    int32 TownRemaining(FIntPoint Cell) const;
    void WalkerFell(AETAgent* Walker);
    FVector TowerLocation(FIntPoint Cell) const;
    void TownFortifications(FIntPoint Cell,int32 Tier,bool Repair);
    FString TownStatus(FIntPoint Cell) const;
    TMap<FString,TWeakObjectPtr<AETAgent>> TownUnits;
    mutable TMap<FIntPoint,FVector> TowerCache;
    float TownTick=0;
    TMap<FString,TWeakObjectPtr<AETItem>> LooseActors;
    void SpillLoot(AETAgent* Victim,bool Credited);
    AETItem* LooseItem(const FString& Id,const FString& Kind,int32 Count,FVector Origin,FVector Landing,bool Toss);
    FString DropLabel(const FString& Kind) const;
    void LoadMods();
    static FString ModFolder();
    static bool ParseMod(const FString& Json,FETModPack& Pack,FString& Error);
    bool InstallMod(const FString& Json,FString& Message);
    TArray<FETModPack> Mods;
    TArray<FString> ModErrors;
    const FETModFind* ModFind(const FString& Key) const;
    void ModCache(const FString& SeedId,FString& Summary);
    void RunGeologyChecks(AETCharacter* Player,TFunctionRef<void(bool,const FString&)> Check);
    void CheckMineRestart(AETCharacter* Player);
    void WalkMineCheck(AETCharacter* Player);
    void TickBorerRecoveryCheck(AETCharacter* Player);
    void TickJourneyVerification(float Delta);
    void RunAtmosphereChecks(TFunctionRef<void(bool,const FString&)> Check);
    void LoadExpansion();
    void LoadDialogue();
    bool Speak(const FString& Id,const FString& SpeakerOverride=TEXT(""));
    bool HasSubtitle() const;
    void StreamAround(FVector Position, bool Preload=false);
    double RegionDistance(FIntPoint Coord, FVector Position) const;
    void UpdateVisibility(FVector Position);
    void Sound(const FString& Name, float Volume=.5f, float Pitch=1.f);
    void ImpactSound(FVector Position,bool Metal,bool FirstPerson);
    UPROPERTY() USoundAttenuation* ImpactAttenuation=nullptr;
    void Loot(AETCharacter* Player, const FString& Id, const FString& Table);
    FVector CityCenter(double X, double Y) const;
    TArray<FBox2D> ShelterBuildings;
    bool InCity(FVector P) const;
    FString PlaceName(FVector P) const;
    FString StoryTitle() const;
    FString StoryObjective() const;
    float Hour() const;
    bool IsNight() const;
    bool LastNight=false;
    void UpdateNightWatch(FVector Viewer);
    double Height(double X, double Y) const;
    double SurfaceHeight(double X,double Y) const;
    FVector ResolveStart(FVector SavedPosition) const;
    FVector HouseAnchor(FIntPoint Coord, int32 Index) const;
    AETItem* Item(const FString& Id, const FString& Kind, const FString& Label, FVector Position, int32 Amount=0, const FString& Value=TEXT(""));
    AActor* Static(const FString& Asset, const FString& Material, FVector Position, FVector Scale=FVector::OneVector, FRotator Rotation=FRotator::ZeroRotator);
    UMaterialInterface* Material(const FString& Name) const;
    void DressMesh(UStaticMeshComponent* Component,const FString& BaseMaterial) const;
    uint32 SeedFor(FIntPoint Coord, int32 Salt=0) const;
    UPROPERTY() UETSave* State;
    UPROPERTY() UETGeology* Geology;
    UPROPERTY() TMap<FIntPoint,AETRegion*> Regions;
    UPROPERTY() ADirectionalLight* Sun;
    UPROPERTY() ADirectionalLight* Moon;
    UPROPERTY() ASkyLight* Sky;
    UPROPERTY() AExponentialHeightFog* Fog;
    UPROPERTY() TMap<FString,USoundBase*> Sounds;
    UPROPERTY() TArray<UAudioComponent*> Ambience;
    UPROPERTY() UAudioComponent* DialogueAudio=nullptr;
    UPROPERTY() TArray<UAudioComponent*> Music;
    UPROPERTY() TArray<UAudioComponent*> WeatherAudio;
    UPROPERTY() UInstancedStaticMeshComponent* RainMesh=nullptr;
    UPROPERTY() UStaticMeshComponent* Clouds=nullptr;
    UPROPERTY() UMaterialInstanceDynamic* CloudMaterial=nullptr;
    UPROPERTY() USkyAtmosphereComponent* Atmosphere=nullptr;
    UPROPERTY() ADirectionalLight* LightningLight=nullptr;
    ETClimate::Frame Weather;
    FRandomStream WeatherRandom;
    TArray<FVector> RainPositions;
    TArray<float> RainFloors;
    bool Sheltered=false, AtmosphereAuditWritten=false;
    int32 WeatherOverride=-1;
    float RainTick=0, ShelterTick=0, ThunderTimer=16, ThunderDelay=-1, Lightning=0, AtmosphereAge=0;
    double AudioLastTick=0;
    UPROPERTY() TArray<AActor*> StoryActors;
    TArray<FETWeapon> Weapons;
    TArray<FETSupply> Supplies;
    TArray<FETRecipe> Recipes;
    TMap<FString,TArray<FETLootEntry>> LootTables;
    TMap<FString,FETVoiceLine> VoiceLines;
    FString VoiceId, SubtitleSpeaker, SubtitleText;
    double VoiceStartedAt=0, SubtitleUntil=0;
    TArray<FETPerk> Perks;
    TArray<FETQuest> Quests;
    TArray<FString> Recordings, ChapterNames, Objectives;
    FString Notice;
    float NoticeTime=0, StreamTime=0, SaveTime=0;
    float LightTime=0;
    float VisibilityTime=0;
    bool Review=false, SelfTest=false, TestRan=false;
    // Explicit QA runs use a different slot, including after a process restart.
    FString JourneyEnding, SaveSlot=TEXT("EARTHtotaled-Unreal-01");
    bool JourneyResume=false;
    bool MineResume=false, MineWalk=false;
    bool BorerRecoveryQA=false;
    bool AtRecoveryShelter=false;
    bool FindShelterLanding(FVector& Out) const;
    bool HadSave=false, FrontierQA=false;
    void Say(const FString& Text);
    static constexpr double RegionSize=32000.0;
};

UCLASS()
class AETBoreBurst : public AActor
{
    GENERATED_BODY()
public:
    AETBoreBurst();
    void Ignite(AETWorld* World,FVector Normal,bool Ore,const TCHAR* ContactMaterial=nullptr);
    virtual void Tick(float Delta) override;
private:
    UPROPERTY() UInstancedStaticMeshComponent* Chips;
    UPROPERTY() UPointLightComponent* Flash;
    TArray<FVector> Velocities;
    float Age=0;
    bool Contact=false;
};

UCLASS()
class AETEnemyBolt : public AActor
{
    GENERATED_BODY()
public:
    AETEnemyBolt();
    UPROPERTY() UStaticMeshComponent* Mesh;
    UPROPERTY() AETWorld* Field;
    FVector Velocity=FVector::ZeroVector;
    float Power=0;
    bool FrontBolt=false, BugBolt=false,WakeHostile=false,WakeAllied=false;
    void Launch(AETAgent* Source,FVector Target,bool Acid);
    void Beam(AETWorld* World,FVector To);
    virtual void Tick(float Delta) override;
};

UCLASS()
class EARTHTOTALED_API AETCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AETCharacter();
    virtual void BeginPlay() override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    virtual void Tick(float Delta) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    virtual void PawnClientRestart() override;
    virtual void BaseChange() override;
    virtual void Landed(const FHitResult& Hit) override;
    virtual float TakeDamage(float Damage, const FDamageEvent& Event, AController* Instigator, AActor* Causer) override;
    UPROPERTY() UCameraComponent* Camera;
    UPROPERTY() UStaticMeshComponent* Gun;
    UPROPERTY() UStaticMeshComponent* WakeBody=nullptr;
    UPROPERTY() UStaticMeshComponent* WakeCuff=nullptr;
    void ApplyWakeBody();
    void WakeUse(FName Node);
    void WakeAction(const FString& Id);
    void ReadTrail(int Index);
    int TrailPage=0;
    void WakeJournal();
    void WakeMessage(FString Title,FString Body,const FString& Cue=TEXT(""));
    void WakeClaw();
    void FinishWakeHerd();
    void WakeBurrow();
    void ArchiveJournal();
    AETItem* NearbyWake() const;
    TArray<TPair<FString,FString>> WakeChoices() const;
    void EnterRide(AETWakeVehicle* Vehicle,bool Restore=false);
    void LeaveRide();
    AETWakeVehicle* Ride() const;
    float DriveAxis=0, SteerAxis=0;
    bool LiftHeld=false, DescentHeld=false;
    FName WakePanelNode=NAME_None;
    int32 WakeOffer=0;
    UPROPERTY() USpotLightComponent* Flashlight;
    UPROPERTY() class UPointLightComponent* Muzzle;
    UPROPERTY() AETWorld* World;
    UPROPERTY() AETItem* Focus;
    TWeakObjectPtr<AETConstruction> FocusBuild;
    FGuid DeviceId;
    void OpenDevice(AETConstruction* Device);
    void ServiceDevice(const FString& Action);
    float Hurt(float Amount,const FString& Cause);
    void OpenConsole();
    void CloseConsole();
    FString Command(const FString& Line);
    TSharedPtr<class SWidget> ConsoleWidget;
    TArray<FString> ConsoleLines;
    FString SlotOrigin;
    int32 SelectedSlot=1;
    UPROPERTY() UETSave* PendingImport=nullptr;
    void Slots();
    void SlotAction(const FString& Id);
    FString SlotSummary(int32 Number) const;
    bool ExportProfile(UETSave* Profile,const FString& Path,FString& Message);
    UETSave* ReadProfile(const FString& Path,FString& Message);
    void TravelSlot(int32 Number,bool Start,bool Shelter=false);
    bool StoreSlot(UETSave* Profile,int32 Number);
    FString SlotMessage, PendingSlotAction;
    TArray<FString> SlotLabels;
    bool Firing=false, Aiming=false, Sprinting=false;
    bool Climbing=false;
    FVector ClimbStart, ClimbEnd;
    float ClimbAlpha=0, ShotTime=0, ReloadTime=0, FallSpeed=0;
    float FireGuard=0, CaptureRetry=0;
    float EnemyReacquireTime=0;
    float StepTime=0, HitTime=0, MuzzleTime=0;
    float SuitFlinch=0,SuitImpactCooldown=0,XPNoticeTime=0,LevelNoticeTime=0;
    FVector2D SuitLean=FVector2D::ZeroVector;
    int32 LastXPGain=0;
    FString SelectedPerk;
    bool SweepHeld=false,SweepArmed=false,TestInvincible=false;
    float SweepHold=0,SweepClock=0;
    void CrouchLoot(); void ReleaseLoot(); void TickSweep(float Delta);
    bool TakeLoose(AETItem* Item);
    void TickShield(float Delta);
    void WorkshopAction(const FString& Id); void HiddenTools(); void ReclamationAction(const FString& Id);
    void TownPanel(FIntPoint Cell); void Workshop(); void RefreshWorkshop();
    FIntPoint BoardCell=FIntPoint::ZeroValue;
    FString WorkshopOrigin,WorkshopNote,ProposedMod;
    int32 ModPage=0;
    void ReceiveImpact(float Damage,const FDamageEvent& Event,AActor* Source);
    void TickImpact(float Delta);
    bool InputAudit=false;
    bool Borer=false, LampBeforeBorer=false;
    bool Building=false, JumpHeld=false, Jetting=false, FuelLocked=false, JetUsed=false;
    bool AirJumpSpent=false, ThrustArmed=false;
    float JumpHeldTime=0, JetSoundTime=0, BuildRefresh=0;
    int32 BuildPiece=0, BuildFacing=0, BuildMaterial=0;
    void CycleMaterial();
    void UpgradeFrame();
    bool TreeInReach=false;
    void Controls();
    void ToggleMap();
    void CloseMapInput();
    void PanMap(FVector2D Pixels,const ETNav::FChart& Chart);
    void ZoomMap(double Steps,FVector2D ScreenPoint,const ETNav::FChart& Chart);
    void UseMapPoint(FVector2D Pixel,const ETNav::FChart& Chart,bool Place);
    void MapCommand(FKey Key,FVector2D Viewport);
    FVector2D MapCenter=FVector2D::ZeroVector;
    double MapSpan=60000.;
    FString MapMessage;
    TSharedPtr<class SWidget> MapInput;
    FVector AssistedDirection(FVector Eye,FVector Direction,double Range) const;
    FString GuideOrigin;
    int32 GuidePage=0;
    FString BuildHint;
    TWeakObjectPtr<AETAgent> Sighted;
    UPROPERTY() AETConstruction* Preview=nullptr;
    void JumpPress(); void JumpRelease(); void TickJet(float Delta);
    void ToggleBuild(); void StopBuilding(); void RefreshBuild();
    bool BuildTarget(FETBuild& Candidate,FString& Reason) const;
    void PlaceBuild(); void SalvageBuild(); void TurnTool();
    int32 GunRank(int32 Index) const;
    static float NextGunRank(int32 Rank);
    void Title(); void ContinueSurvivor(); void StartFresh();
    float SurveyTime=0;
    FString SurveyReading;
    void ToggleBorer();
    void FireBorer();
    void Winch();
    void ApplyFieldDisplay();
    FString Panel, PanelTitle, PanelBody;
    FString Contact;
    void Forward(float Axis); void Right(float Axis); void Turn(float Axis); void Look(float Axis);
    void FireStart(); void FireStop(); void AimStart(); void AimStop(); void SprintStart(); void SprintStop();
    void Interact(); void Reload(); void Drink(); void Heal(); void Skills(); void Journal(); void Pause(); void Fullscreen();
    void SelectWeapon(int32 Index);
    void EquipFrame(int32 Index); void EquipRelic(int32 Index);
    FETWeapon CurrentGun() const;
    int32& CurrentMagazine();
    float& CurrentGunDamage();
    int32 CurrentRank() const;
    float CurrentThreshold(int32 Rank) const;
    void RelicRack(); int32 RackIndex=0;
    void StartArrival(); void TickArrival(); void FinishArrival();
    double ArrivalTime=0; float FlightSeconds=0; int32 FlightCue=-1;
    UPROPERTY() class ACameraActor* FlightCamera=nullptr;
    UPROPERTY() UAudioComponent* FlightAudio=nullptr;
    void ReadReleasePage(int Page,bool Automatic); void CheckVersions(bool Automatic); void UpdateAction(const FString& Id); void ShowVersions();
    TArray<FString> ReleaseTags; FString UpdateOrigin, UpdateNote; int32 ReleaseIndex=0;
    bool CheckingVersions=false, ConfirmVersion=false;
    void Weapon1(); void Weapon2(); void Weapon3(); void Weapon4(); void Weapon5(); void Weapon6();
    void Weapon7(); void Weapon8(); void Weapon9(); void Weapon10();
    void NextWeapon(); void PreviousWeapon(); void CycleWeapon(int32 Direction);
    void Inventory(); void UseSupply(const FString& Id); void Craft(const FString& Id);
    void Shoot(); void ClosePanel(); void OpenPanel(const FString& Kind, const FString& Title, const FString& Body);
    void Action(const FString& Id); void ContactPanel(const FString& Name);
    void ShowEnding();
    bool Has(const FString& Skill) const;
    float MaxHealth() const; float LootRate() const;
    void AddXP(int32 Amount);
    void SetMouseMode(bool Menu);
    void Die(const FString& Cause=TEXT("Injury"));
    void ToggleLight();
    void ApplyGraphics();
};

UCLASS()
class EARTHTOTALED_API AETHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
    virtual void NotifyHitBoxClick(FName BoxName) override;
    void Text(const FString& Value, float X, float Y, FLinearColor Color=FLinearColor::White, float Scale=1);
    void Wrapped(const FString& Value, float X, float Y, float Width, float Scale=1);
    void Button(const FString& Id, const FString& Label, float X, float Y, float Width=260);
    void DialogueSubtitle();
    void DrawControls(AETCharacter* Player);
    void DrawReclamation(AETCharacter* Player);
    void DrawLandfall(AETCharacter* Player);
    void DrawSkillWeb(AETCharacter* Player);
    void DrawWake(AETCharacter* Player);
    void DrawWakeTrail(AETCharacter* Player);
    void DrawNavigation(AETCharacter* Player);
    void DrawAtlas(AETCharacter* Player);
    void DrawChart(AETCharacter* Player,const ETNav::FChart& Chart,bool Small);
    ETNav::FContours Contours[2];
};

UCLASS()
class EARTHTOTALED_API AETGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    AETGameMode();
    virtual void InitGame(const FString& MapName, const FString& Options, FString& Error) override;
    virtual AActor* ChoosePlayerStart_Implementation(AController* Player) override;
    UPROPERTY() AETWorld* World;
    UPROPERTY() AActor* Start;
};
