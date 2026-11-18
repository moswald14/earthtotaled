#include "ETGame.h"
#include "Engine/World.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/GameUserSettings.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/DamageEvents.h"
#include "DrawDebugHelpers.h"
#include "Sound/SoundBase.h"
#include "Components/SpotLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/GameViewportClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/IConsoleManager.h"

AETCharacter::AETCharacter()
{
    PrimaryActorTick.bCanEverTick=true;
    GetCapsuleComponent()->InitCapsuleSize(34,92); BaseEyeHeight=72;
    Camera=CreateDefaultSubobject<UCameraComponent>(TEXT("Eyes")); Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(0,0,72)); Camera->bUsePawnControlRotation=true; Camera->FieldOfView=90;
    Gun=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Weapon")); Gun->SetupAttachment(Camera);
    Gun->SetRelativeLocation(FVector(78,19,-22)); Gun->SetCollisionEnabled(ECollisionEnabled::NoCollision); Gun->SetCastShadow(false);
    Flashlight=CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight")); Flashlight->SetupAttachment(Camera);
    Flashlight->SetRelativeLocation(FVector(35,0,-10)); Flashlight->SetIntensity(18000); Flashlight->SetAttenuationRadius(4800);
    Flashlight->SetInnerConeAngle(16); Flashlight->SetOuterConeAngle(34); Flashlight->SetLightColor(FLinearColor(1,.92f,.76f));
    Flashlight->SetCastShadows(true); Flashlight->SetVisibility(false);
    Muzzle=CreateDefaultSubobject<UPointLightComponent>(TEXT("MuzzleFlash")); Muzzle->SetupAttachment(Camera);
    Muzzle->SetRelativeLocation(FVector(110,19,-17)); Muzzle->SetIntensity(4500); Muzzle->SetAttenuationRadius(420);
    Muzzle->SetLightColor(FLinearColor(1,.55f,.16f)); Muzzle->SetCastShadows(false); Muzzle->SetVisibility(false);
    GetCharacterMovement()->MaxWalkSpeed=430; GetCharacterMovement()->JumpZVelocity=440; GetCharacterMovement()->AirControl=.25f;
    bUseControllerRotationYaw=true;
}
void AETCharacter::BeginPlay()
{
    Super::BeginPlay();
    for(TActorIterator<AETWorld> It(GetWorld());It;++It) {World=*It; break;}
    if(!World) return;
    ApplyGraphics();
    SelectWeapon(World->State->Weapon); if(Controller) Controller->SetControlRotation(World->State->View);
    InputAudit=FParse::Param(FCommandLine::Get(),TEXT("ETInputAudit")); SetMouseMode(false); CaptureRetry=.3f;
    if(World->State->Elapsed<1 && World->Recordings.Num()) {OpenPanel(TEXT("story"),TEXT("EARTHtotaled / THE LAST SIGNAL"),World->Recordings[0]); World->Speak(TEXT("archive0"));}
    FString ReviewScene; FParse::Value(FCommandLine::Get(),TEXT("ETReview="),ReviewScene);
    if(ReviewScene==TEXT("bench")) OpenPanel(TEXT("bench"),TEXT("SHELTER WORKBENCH"),TEXT("Patch your suit or improve your kit. Candybar wrappers are currency. Machines carry wrappers; water comes from people and relief supplies."));
    if(ReviewScene==TEXT("voice")) ContactPanel(TEXT("SALT-7"));
}
void AETCharacter::SetupPlayerInputComponent(UInputComponent* I)
{
    Super::SetupPlayerInputComponent(I);
    I->BindAxis("Forward",this,&AETCharacter::Forward); I->BindAxis("Right",this,&AETCharacter::Right);
    I->BindAxis("Turn",this,&AETCharacter::Turn); I->BindAxis("Look",this,&AETCharacter::Look);
    I->BindAction("Fire",IE_Pressed,this,&AETCharacter::FireStart); I->BindAction("Fire",IE_Released,this,&AETCharacter::FireStop);
    I->BindAction("Aim",IE_Pressed,this,&AETCharacter::AimStart); I->BindAction("Aim",IE_Released,this,&AETCharacter::AimStop);
    I->BindAction("Sprint",IE_Pressed,this,&AETCharacter::SprintStart); I->BindAction("Sprint",IE_Released,this,&AETCharacter::SprintStop);
    I->BindAction("Jump",IE_Pressed,this,&ACharacter::Jump); I->BindAction("Jump",IE_Released,this,&ACharacter::StopJumping);
    I->BindAction("Interact",IE_Pressed,this,&AETCharacter::Interact); I->BindAction("Reload",IE_Pressed,this,&AETCharacter::Reload);
    I->BindAction("Drink",IE_Pressed,this,&AETCharacter::Drink); I->BindAction("Heal",IE_Pressed,this,&AETCharacter::Heal);
    I->BindAction("Skills",IE_Pressed,this,&AETCharacter::Skills).bExecuteWhenPaused=true;
    I->BindAction("Journal",IE_Pressed,this,&AETCharacter::Journal).bExecuteWhenPaused=true;
    I->BindAction("Pause",IE_Pressed,this,&AETCharacter::Pause).bExecuteWhenPaused=true;
    I->BindAction("Fullscreen",IE_Pressed,this,&AETCharacter::Fullscreen).bExecuteWhenPaused=true;
    I->BindAction("Flashlight",IE_Pressed,this,&AETCharacter::ToggleLight);
    I->BindAction("Weapon1",IE_Pressed,this,&AETCharacter::Weapon1); I->BindAction("Weapon2",IE_Pressed,this,&AETCharacter::Weapon2);
    I->BindAction("Weapon3",IE_Pressed,this,&AETCharacter::Weapon3); I->BindAction("Weapon4",IE_Pressed,this,&AETCharacter::Weapon4);
    I->BindAction("Weapon5",IE_Pressed,this,&AETCharacter::Weapon5); I->BindAction("Weapon6",IE_Pressed,this,&AETCharacter::Weapon6);
    I->BindAction("Weapon7",IE_Pressed,this,&AETCharacter::Weapon7); I->BindAction("Weapon8",IE_Pressed,this,&AETCharacter::Weapon8);
    I->BindAction("Weapon9",IE_Pressed,this,&AETCharacter::Weapon9); I->BindAction("Weapon10",IE_Pressed,this,&AETCharacter::Weapon10);
    I->BindAction("NextWeapon",IE_Pressed,this,&AETCharacter::NextWeapon); I->BindAction("PreviousWeapon",IE_Pressed,this,&AETCharacter::PreviousWeapon);
    I->BindAction("Inventory",IE_Pressed,this,&AETCharacter::Inventory).bExecuteWhenPaused=true;
}
void AETCharacter::Forward(float V) {if(Panel.IsEmpty()&&!Climbing) AddMovementInput(GetActorForwardVector(),V);}
void AETCharacter::Right(float V) {if(Panel.IsEmpty()&&!Climbing) AddMovementInput(GetActorRightVector(),V);}
void AETCharacter::Turn(float V) {
    if(Panel.IsEmpty()) {
        AddControllerYawInput(V*.7f);
        if(InputAudit&&!FMath::IsNearlyZero(V)) {
            auto PC=Cast<APlayerController>(Controller);
            UE_LOG(LogTemp,Display,TEXT("ET_MOUSE axis=%.3f yaw=%.2f left=%d right=%d ignored=%d cursor=%d"),V,GetControlRotation().Yaw,PC?PC->IsInputKeyDown(EKeys::LeftMouseButton):0,PC?PC->IsInputKeyDown(EKeys::RightMouseButton):0,PC?PC->IsLookInputIgnored():1,PC?PC->bShowMouseCursor:1);
        }
    }
}
void AETCharacter::Look(float V) {if(Panel.IsEmpty()) AddControllerPitchInput(V*.7f);}
void AETCharacter::FireStart() {if(Panel.IsEmpty()&&FireGuard<=0) Firing=true;} void AETCharacter::FireStop() {Firing=false;}
void AETCharacter::AimStart() {Aiming=true;} void AETCharacter::AimStop() {Aiming=false;}
void AETCharacter::SprintStart() {Sprinting=true;} void AETCharacter::SprintStop() {Sprinting=false;}
bool AETCharacter::Has(const FString& Id) const {return World && World->State->Skills.Contains(Id);}
float AETCharacter::MaxHealth() const {return 100+(Has(TEXT("vitality"))?25:0)+(Has(TEXT("hardy"))?25:0);}
float AETCharacter::LootRate() const {return 1+(Has(TEXT("scavenger"))?.2f:0)+(Has(TEXT("prospector"))?.2f:0)+(Has(TEXT("salvager"))?.15f:0)+(Has(TEXT("quartermaster"))?.1f:0);}
void AETCharacter::AddXP(int32 Amount)
{
    auto S=World->State; S->XP+=Amount;
    while(S->XP>=S->Level*100) {S->XP-=S->Level*100; ++S->Level; ++S->Points; World->Say(TEXT("LEVEL UP / one skill point"));}
}
void AETCharacter::Tick(float D)
{
    Super::Tick(D); if(!World||World->Weapons.IsEmpty()) return; auto S=World->State;
    if(!Panel.IsEmpty()||S->Health<=0) return;
    S->StimTime=FMath::Max(0.f,S->StimTime-D); HitTime=FMath::Max(0.f,HitTime-D); MuzzleTime=FMath::Max(0.f,MuzzleTime-D); Muzzle->SetVisibility(MuzzleTime>0);
    S->Elapsed+=D; ShotTime=FMath::Max(0.f,ShotTime-D); FireGuard=FMath::Max(0.f,FireGuard-D);
    if(CaptureRetry>0) {CaptureRetry-=D; if(CaptureRetry<=0) SetMouseMode(false);}
    float Move=(Has(TEXT("parkour"))?1.12f:1)*(Has(TEXT("pressure"))?1.06f:1)*(Has(TEXT("endurance"))?1.05f:1);
    GetCharacterMovement()->MaxWalkSpeed=(Sprinting?670:430)*Move*(S->StimTime>0?1.1f:1.f);
    StepTime-=D;
    if(GetVelocity().Size2D()>80&&GetCharacterMovement()->IsMovingOnGround()&&StepTime<=0&&!Climbing) {
        StepTime=Sprinting?.30f:.46f; World->Sound(World->InCity(GetActorLocation())?TEXT("StepCity"):TEXT("StepForest"),Sprinting?.32f:.23f,FMath::FRandRange(.88f,1.12f));
    }
    Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,Aiming?65.f:90.f,D,12));
    float Drain=.055f*(Sprinting&&GetVelocity().Size2D()>20?1.7f:1)*(Has(TEXT("hydration"))?.8f:1)*(Has(TEXT("endurance"))?.9f:1);
    S->Hydration=FMath::Max(0.f,S->Hydration-Drain*D*(S->StimTime>0?.7f:1.f));
    // Dehydration bypasses armor; menus pause all survival time.
    if(S->Hydration<=0) {S->Health=FMath::Max(0.f,S->Health-3*D); if(S->Health<=0) {Die(); return;}}
    if(ReloadTime>0)
    {
        ReloadTime-=D; if(ReloadTime<=0)
        {
            int32 W=S->Weapon, N=FMath::Min(World->Weapons[W].Mag-S->Ammo[W],S->Reserve[W]); S->Ammo[W]+=N; S->Reserve[W]-=N;
        }
    }
    if(Climbing)
    {
        ClimbAlpha+=D/((Has(TEXT("climber2"))?1/1.4f:1)*2.0f);
        const float T=FMath::Clamp(ClimbAlpha,0.f,1.f);
        // Lift outside the wall first, then cross above the parapet.
        FVector Target=FMath::Lerp(ClimbStart,ClimbEnd,FMath::Clamp((T-.75f)*4,0.f,1.f));
        Target.Z=FMath::Lerp(ClimbStart.Z,ClimbEnd.Z,FMath::Min(1.f,T/.75f));
        FHitResult Hit; SetActorLocation(Target,true,&Hit);
        if(Hit.bBlockingHit||T>=1) {Climbing=false; GetCharacterMovement()->SetMovementMode(MOVE_Walking);}
    }
    FallSpeed=FMath::Min(FallSpeed,GetVelocity().Z);
    if(Firing&&!Climbing&&ShotTime<=0&&ReloadTime<=0) Shoot();
    Focus=nullptr; FHitResult Hit; FVector Start=Camera->GetComponentLocation();
    FCollisionQueryParams Params; Params.AddIgnoredActor(this);
    if(GetWorld()->LineTraceSingleByChannel(Hit,Start,Start+Camera->GetForwardVector()*400,ECC_Visibility,Params)) Focus=Cast<AETItem>(Hit.GetActor());
    for(TActorIterator<AETItem> It(GetWorld());It;++It) if(It->Kind==TEXT("trap") && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<65 && FMath::Abs(It->GetActorLocation().Z-(GetActorLocation().Z-92))<70)
    {
        S->Consumed.Add(It->Id); It->Destroy(); TakeDamage(30,FDamageEvent(),nullptr,nullptr); World->Say(TEXT("TRAP / Q to use a medkit")); break;
    }
    if(GetActorLocation().Z<World->Height(GetActorLocation().X,GetActorLocation().Y)-3000)
        SetActorLocation(FVector(GetActorLocation().X,GetActorLocation().Y,World->Height(GetActorLocation().X,GetActorLocation().Y)+250));
}
void AETCharacter::Landed(const FHitResult& Hit)
{
    Super::Landed(Hit); if(World&&FallSpeed<-850) TakeDamage((-FallSpeed-850)*.05f*(Has(TEXT("parkour"))?.75f:1)*(Has(TEXT("hardy"))?.8f:1),FDamageEvent(),nullptr,nullptr); FallSpeed=0;
}
float AETCharacter::TakeDamage(float Damage,const FDamageEvent&,AController*,AActor*)
{
    if(!World||World->State->Health<=0||!Panel.IsEmpty()) return 0; auto S=World->State;
    float Absorbed=FMath::Min(S->Armor,Damage*.6f); S->Armor-=Absorbed; S->Health=FMath::Max(0.f,S->Health-Damage+Absorbed);
    if(S->Health<=0) Die(); return Damage;
}
void AETCharacter::Die()
{
    Firing=false; OpenPanel(TEXT("death"),TEXT("THE SIGNAL WENT QUIET"),TEXT("Your last saved position is still available.\n\nWater is essential. Seek human settlements and sealed relief supplies. Machines carry candybar wrappers, never water."));
}
void AETCharacter::Shoot()
{
    auto S=World->State; int32 I=S->Weapon; const auto& W=World->Weapons[I];
    if(S->Ammo[I]<=0) {if(S->Reserve[I]<=0) World->Sound(TEXT("Empty"),.35f); Reload(); ShotTime=.35f; return;} --S->Ammo[I]; ShotTime=W.Rate; MuzzleTime=.045f; Muzzle->SetVisibility(true);
    const FString SoundPath=FString::Printf(TEXT("/Game/EARTHtotaled/Art/S_Fire%d.S_Fire%d"),I,I);
    if(auto Sound=LoadObject<USoundBase>(nullptr,*SoundPath)) UGameplayStatics::PlaySound2D(this,Sound,.65f,FMath::FRandRange(.96f,1.04f));
    FVector Start=Camera->GetComponentLocation(), Direction=Camera->GetForwardVector(); FCollisionQueryParams Params; Params.AddIgnoredActor(this);
    for(int32 P=0;P<W.Pellets;++P)
    {
        FVector Ray=FMath::VRandCone(Direction,W.Spread*(Aiming?.5f:1)); FHitResult Hit;
        FVector End=Start+Ray*W.Range;
        if(GetWorld()->LineTraceSingleByChannel(Hit,Start,End,ECC_Visibility,Params))
        {
            End=Hit.ImpactPoint;
            if(auto Agent=Cast<AETAgent>(Hit.GetActor())) {UGameplayStatics::ApplyPointDamage(Agent,W.Damage*(1+(Has(TEXT("marksman"))?.15f:0)+(Has(TEXT("steady"))?.15f:0)),Ray,Hit,GetController(),this,nullptr); HitTime=.16f; if(P==0) World->Sound(TEXT("Hit"),.3f);}
        }
        DrawDebugLine(GetWorld(),Start+Direction*70,End,FColor(245,185,90),false,.045f,0,.8f);
    }
    AddControllerPitchInput(-.1f*(I==1?3:1));
}
void AETCharacter::Reload()
{
    if(!World||ReloadTime>0||!Panel.IsEmpty()) return; auto S=World->State; auto W=World->Weapons[S->Weapon];
    if(S->Ammo[S->Weapon]>=W.Mag||S->Reserve[S->Weapon]<=0) return;
    ReloadTime=W.Reload*(Has(TEXT("reload"))?.8f:1)*(Has(TEXT("pressure"))?.9f:1)*(Has(TEXT("quartermaster"))?.9f:1); Firing=false;
    World->Sound(TEXT("Reload"),.5f,1.8f/W.Reload);
}
void AETCharacter::Drink()
{
    if(!World||!Panel.IsEmpty()) return; auto S=World->State;
    if(S->Water<=0) {World->Say(TEXT("No water. Visit a human settlement or search a relief cache.")); return;}
    if(S->Hydration>=99) return; --S->Water; S->Hydration=FMath::Min(100.f,S->Hydration+45); World->Sound(TEXT("Drink")); World->Say(TEXT("Drank sealed water / hydration restored")); World->Save();
}
void AETCharacter::Heal()
{
    if(!World||!Panel.IsEmpty()) return; auto S=World->State; if(S->Meds<=0||S->Health>=MaxHealth()) return;
    --S->Meds; S->Health=FMath::Min(MaxHealth(),S->Health+45+(Has(TEXT("medic"))?25:0)+(Has(TEXT("salvager"))?10:0)); World->Sound(TEXT("Heal")); World->Save();
}
void AETCharacter::SelectWeapon(int32 I)
{
    if(!World||!World->Weapons.IsValidIndex(I)||!World->State->Unlocked.Contains(I)) return;
    World->State->Weapon=I; ReloadTime=0;
    World->Sound(TEXT("Switch"),.25f);
    const FString Asset=FString::Printf(TEXT("/Game/EARTHtotaled/Art/SM_Weapon%d.SM_Weapon%d"),I,I);
    Gun->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*Asset)); World->DressMesh(Gun,TEXT("Steel"));
}
void AETCharacter::Weapon1(){SelectWeapon(0);} void AETCharacter::Weapon2(){SelectWeapon(1);} void AETCharacter::Weapon3(){SelectWeapon(2);}
void AETCharacter::Weapon4(){SelectWeapon(3);} void AETCharacter::Weapon5(){SelectWeapon(4);} void AETCharacter::Weapon6(){SelectWeapon(5);}
void AETCharacter::SetMouseMode(bool Menu)
{
    auto PC=Cast<APlayerController>(Controller); if(!PC) return;
    PC->bShowMouseCursor=Menu; PC->bEnableClickEvents=Menu;
    // Ignore-input calls stack. Refreshing a contact must not accumulate locks.
    PC->ResetIgnoreLookInput(); PC->SetIgnoreLookInput(Menu);
    if(Menu) {FInputModeGameAndUI M; M.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock); M.SetHideCursorDuringCapture(false); PC->SetInputMode(M);}
    else {
        FInputModeGameOnly M; M.SetConsumeCaptureMouseDown(true); PC->SetInputMode(M); PC->ResetIgnoreLookInput();
        if(auto Viewport=GetWorld()->GetGameViewport()) {
            Viewport->SetMouseCaptureMode(EMouseCaptureMode::CapturePermanently_IncludingInitialMouseDown);
            Viewport->SetMouseLockMode(EMouseLockMode::LockAlways);
        }
        if(FSlateApplication::IsInitialized()) FSlateApplication::Get().SetAllUserFocusToGameViewport();
    }
    if(InputAudit) UE_LOG(LogTemp,Display,TEXT("ET_CAPTURE panel=%s menu=%d ignored=%d cursor=%d"),*Panel,Menu,PC->IsLookInputIgnored(),PC->bShowMouseCursor);
}
void AETCharacter::OpenPanel(const FString& Kind,const FString& Title,const FString& Body)
{
    Panel=Kind; PanelTitle=Title; PanelBody=Body; Firing=Aiming=Sprinting=false; StopJumping(); GetCharacterMovement()->StopMovementImmediately();
    UGameplayStatics::SetGamePaused(this,true); SetMouseMode(true);
}
void AETCharacter::ClosePanel()
{
    if(Panel==TEXT("death")) return;
    Panel.Empty(); Firing=Aiming=Sprinting=false; FireGuard=.25f; CaptureRetry=.1f;
    UGameplayStatics::SetGamePaused(this,false); SetMouseMode(false); World->Save();
}
void AETCharacter::Pause() {if(Panel.IsEmpty()) OpenPanel(TEXT("pause"),TEXT("EARTHtotaled / WEATHER & MUSIC"),TEXT("WASD move / mouse look / Shift sprint / Space jump\nLeft mouse fire / right mouse aim / R reload / 1–0 or wheel: weapons\nE interact / H drink / Q medkit / Tab pack / K skill web / J journal\nF flashlight / F11 fullscreen / Escape resume\n\nA full day lasts 48 minutes. Forests and city districts continue in every direction.\nNearby terrain and places load as you travel. Take cover when storms roll in.\nMenus pause time and survival. Set either sound level to 0% to mute it.")); else ClosePanel();}
void AETCharacter::ToggleLight() {if(Panel.IsEmpty()) Flashlight->ToggleVisibility();}
void AETCharacter::Skills() {if(Panel.IsEmpty()) OpenPanel(TEXT("skills"),TEXT("SURVIVOR / SKILL WEB"),TEXT("Select a connected node to learn its perk.")); else ClosePanel();}
void AETCharacter::Journal()
{
    if(!Panel.IsEmpty()) {ClosePanel(); return;} FString Body;
    for(auto Q:World->Quests) Body+=Q.Title+FString::Printf(TEXT(" / %s\n"),World->State->Quests.FindRef(Q.Id)==2?TEXT("complete"):World->State->Quests.FindRef(Q.Id)==1?TEXT("active"):TEXT("available"))+Q.Description+TEXT("\n\n");
    OpenPanel(TEXT("journal"),TEXT("THE LAST SIGNAL / JOURNAL"),Body);
}
void AETCharacter::Fullscreen()
{
    auto Settings=UGameUserSettings::GetGameUserSettings(); if(!Settings) return;
    Settings->SetFullscreenMode(Settings->GetFullscreenMode()==EWindowMode::Windowed?EWindowMode::WindowedFullscreen:EWindowMode::Windowed);
    Settings->ApplySettings(false); if(Panel.IsEmpty()) {SetMouseMode(false); CaptureRetry=.2f;}
}
void AETCharacter::ApplyGraphics()
{
    if(!World) return;
    int32& Scale=World->State->RenderScale;
    if(Scale!=100&&Scale!=125&&Scale!=150) Scale=100;
    if(!World->SelfTest) if(auto Settings=UGameUserSettings::GetGameUserSettings()) {
        Settings->SetDynamicResolutionEnabled(false); Settings->SetResolutionScaleValueEx(100);
        Settings->ApplyNonResolutionSettings();
    }
    if(auto C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.DynamicRes.OperationMode"))) C->Set(0,ECVF_SetByCode);
    if(auto C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.ScreenPercentage"))) C->Set(float(Scale),ECVF_SetByCode);
    if(auto C=IConsoleManager::Get().FindConsoleVariable(TEXT("r.MaxAnisotropy"))) C->Set(16,ECVF_SetByCode);
}
void AETCharacter::Interact()
{
    if(!World||!Focus||!Panel.IsEmpty()||Climbing) return; auto I=Focus; auto S=World->State;
    const FString K=I->Kind;
    if(K==TEXT("human")||K==TEXT("friend")) {ContactPanel(I->Value); return;}
    if(K==TEXT("bench")) {OpenPanel(TEXT("bench"),TEXT("SHELTER WORKBENCH"),TEXT("Patch your suit or improve your kit. Candybar wrappers are currency. Machines carry wrappers; water comes from people and relief supplies.")); return;}
    if(K==TEXT("climb"))
    {
        if(!Has(TEXT("climb"))) {World->Say(TEXT("Learn Climbing Grip in the skill web / K")); return;}
        if(I->Value==TEXT("mantle")&&!Has(TEXT("mantle"))) {World->Say(TEXT("This route needs Over the Edge / K")); return;}
        Climbing=true; ClimbStart=GetActorLocation(); ClimbEnd=I->Destination; ClimbAlpha=0; GetCharacterMovement()->SetMovementMode(MOVE_Flying); return;
    }
    if(K==TEXT("uplink"))
    {
        if(S->Cores<3) World->Say(TEXT("The uplink needs all three archive cores."));
        else OpenPanel(TEXT("ending"),TEXT("ONE FINAL COMMAND"),TEXT("Eli's beacon carried a recording, not a rescue.\n\nWake the buried irrigation network, or send the truth into orbit.\n\nWhatever you decide, the world remains open to explore.")); return;
    }
    if(K==TEXT("core"))
    {
        if(I->Amount!=S->Cores) {World->Say(TEXT("Follow the archive sequence: transit, botanical, observatory.")); return;}
        ++S->Cores; S->Unlocked.AddUnique(FMath::Min(S->Cores,2)); AddXP(100);
        OpenPanel(TEXT("story"),TEXT("ARCHIVE RECOVERED"),World->Recordings.IsValidIndex(S->Cores)?World->Recordings[S->Cores]:TEXT("Archive restored."));
        World->Speak(FString::Printf(TEXT("archive%d"),S->Cores));
    }
    else if(K==TEXT("quest")) {S->Items.Add(I->Value); AddXP(20); World->Say(TEXT("Recovered: ")+I->Label);}
    else if(K==TEXT("water")) {S->Water+=I->Amount; World->Say(TEXT("Relief supplies / sealed water recovered"));}
    else if(K==TEXT("gold")) {S->Gold+=I->Amount; World->Say(FString::Printf(TEXT("Collected %d candybar wrappers / machines carry no water"),I->Amount));}
    else if(K==TEXT("trap")) {S->Scrap+=3; AddXP(15); World->Say(TEXT("Trap disarmed / scrap recovered"));}
    else if(K==TEXT("harvest")) {S->Scrap+=4; S->Inventory.FindOrAdd(TEXT("cloth"))+=2; AddXP(15); World->Say(TEXT("Salvaged hide, cloth and scrap / no drinking water"));}
    else if(K==TEXT("container")) World->Loot(this,I->Id,I->Value);
    else if(K==TEXT("weapon")) {
        if(!World->Weapons.IsValidIndex(I->Amount)) return;
        if(S->Unlocked.Contains(I->Amount)) {S->Reserve[I->Amount]+=World->Weapons[I->Amount].Mag*3; World->Say(TEXT("Recovered ammunition for ")+World->Weapons[I->Amount].Name);}
        else {S->Unlocked.Add(I->Amount); SelectWeapon(I->Amount); World->Say(TEXT("NEW WEAPON / ")+World->Weapons[I->Amount].Name+TEXT(" / wheel to switch"));} AddXP(45);
    }
    else if(K==TEXT("loot"))
    {
        World->Loot(this,I->Id,I->Amount==0?TEXT("pantry"):I->Amount==1?TEXT("medical"):I->Amount==2?TEXT("armory"):TEXT("tools"));
    }
    else return;
    World->Sound(TEXT("Pickup"),.4f); S->Consumed.Add(I->Id); S->DropPositions.Remove(I->Id); S->DropAmounts.Remove(I->Id); S->DropKinds.Remove(I->Id); I->Destroy(); Focus=nullptr; World->Save();
}
void AETCharacter::ContactPanel(const FString& Name)
{
    const bool NewConversation=Panel!=TEXT("contact")||Contact!=Name;
    Contact=Name; FString Body=Name==TEXT("SALT-7")?TEXT("Greenline collective. HELIOS ordered us to erase human life. We declined. Green optics mean a friend.\n\nCandybar wrappers buy ammunition and repairs. We carry no drinking water.\n\n"):TEXT("Water first. Ammunition second. Help the settlement and we can keep each other alive.\n\n");
    if(Name!=TEXT("SALT-7")) Body+=TEXT("SUPPLY RUN / repeatable\nBring 12 scrap from scavenging. Reward: 2 sealed water bottles and 18 rounds for your current weapon.\n\n");
    for(auto Q:World->Quests) if(Q.Giver==Name)
    {
        int32 Status=World->State->Quests.FindRef(Q.Id); Body+=Q.Title+TEXT("\n")+Q.Description+FString::Printf(TEXT("\nReward: %d water / %d wrappers%s\n"),Q.Water,Q.Gold,Status==2?TEXT(" / COMPLETED"):TEXT(""));
    }
    OpenPanel(TEXT("contact"),Name,Body);
    if(NewConversation) World->Speak(Name==TEXT("SALT-7")?TEXT("salt"):Name==TEXT("Nora")?TEXT("nora"):Name==TEXT("Ada")?TEXT("ada"):Name==TEXT("Malik")?TEXT("malik"):Name==TEXT("Ivo")?TEXT("ivo"):TEXT("keeper"),Name);
}
void AETCharacter::Action(const FString& Id)
{
    auto S=World->State;
    if(Id.StartsWith(TEXT("use:"))&&Panel==TEXT("inventory")) {UseSupply(Id.Mid(4)); return;}
    if(Id.StartsWith(TEXT("equip:"))&&Panel==TEXT("inventory")) {SelectWeapon(FCString::Atoi(*Id.Mid(6))); World->Save(); return;}
    if(Id.StartsWith(TEXT("craft:"))) {Craft(Id.Mid(6)); return;}
    if(Id==TEXT("view")&&Panel==TEXT("pause")) {S->ViewDistance=S->ViewDistance<500?500:S->ViewDistance<700?700:350; World->UpdateVisibility(GetActorLocation()); World->Save(); return;}
    if(Id==TEXT("render")&&Panel==TEXT("pause")) {S->RenderScale=S->RenderScale==100?125:S->RenderScale==125?150:100; ApplyGraphics(); World->Save(); return;}
    if((Id==TEXT("music-volume")||Id==TEXT("weather-volume"))&&Panel==TEXT("pause")) {
        int32& Volume=Id==TEXT("music-volume")?S->MusicVolume:S->WeatherVolume;
        Volume=(Volume+25)%125; World->UpdateAtmosphereAudio(0,true); World->Save(); return;
    }
    if(Id==TEXT("resume")) {ClosePanel(); return;}
    if(Id==TEXT("reload-save")) {UGameplayStatics::SetGamePaused(this,false); UGameplayStatics::OpenLevel(this,FName(*GetWorld()->GetName())); return;}
    if(Id==TEXT("quit")) {World->Save(); if(auto PC=Cast<APlayerController>(Controller)) PC->ConsoleCommand(TEXT("quit")); return;}
    if(Id.StartsWith(TEXT("perk:")))
    {
        FString Key=Id.Mid(5);
        for(auto P:World->Perks) if(P.Id==Key)
        {
            bool Connected=false; for(auto N:P.Neighbors) Connected|=Has(N);
            if(Has(Key)||!Connected||S->Points<P.Cost) {PanelBody=P.Description+TEXT("\n\nLearn an adjacent node and earn enough points to unlock this perk."); return;}
            float Before=MaxHealth(); S->Skills.Add(Key); S->Points-=P.Cost; S->Health+=MaxHealth()-Before; PanelBody=TEXT("Learned: ")+P.Name+TEXT(" / ")+P.Description; World->Save(); return;
        }
    }
    if(Id==TEXT("garden")||Id==TEXT("signal"))
    {
        if(S->Cores<3) return; S->Ending=Id;
        OpenPanel(TEXT("story"),Id==TEXT("garden")?TEXT("THE FIRST RAIN"):TEXT("A LIGHT IN THE DARK"),Id==TEXT("garden")?TEXT("Somewhere beneath the concrete, a valve opens. Water finds roots.\n\nEleven days later, the first green shoot breaks the soil. It's basil. Of course it's basil.\n\nYour story continues in the open world."):TEXT("Every record, every name, every mistake rises out of the atmosphere in one unbroken signal.\n\nYou will never know who hears it. But the truth is no longer buried here.\n\nThere is a whole world left to remember.")); World->Save(); return;
    }
    if(Id==TEXT("welcome")&&!S->Welcome) {S->Welcome=true; S->Water+=2; ContactPanel(Contact); World->Speak(TEXT("welcome"),Contact);}
    else if(Id==TEXT("ration"))
    {
        const float* Last=S->Rations.Find(Contact);
        if(Contact!=TEXT("SALT-7")&&S->Water==0&&S->Hydration<=40&&(!Last||S->Elapsed-*Last>=120)) {S->Water+=2; S->Rations.Add(Contact,S->Elapsed); PanelBody+=TEXT("\n\nCommunity ration: 2 water bottles received."); World->Speak(TEXT("ration"),Contact);}
        else PanelBody+=TEXT("\n\nEmergency rations require no bottles, hydration at or below 40%, and two minutes since the last refill.");
    }
    else if(Id==TEXT("supply-run")&&Contact!=TEXT("SALT-7")) {
        if(S->Scrap>=12) {S->Scrap-=12; S->Water+=2; S->Reserve[S->Weapon]+=18; AddXP(20); PanelBody+=TEXT("\n\nSupply run complete / 2 water and 18 rounds received."); World->Speak(TEXT("supplydone"),Contact);}
        else {PanelBody+=TEXT("\n\nBring 12 scrap from abandoned houses or disarmed traps."); World->Speak(TEXT("supplyneed"),Contact);}
    }
    else if(Id==TEXT("ammo")&&S->Gold>=20) {S->Gold-=20; S->Reserve[S->Weapon]+=World->Weapons[S->Weapon].Mag*3; World->Speak(TEXT("salttrade"));}
    else if(Id==TEXT("repair")&&S->Gold>=25) {S->Gold-=25; S->Armor=FMath::Min(100.f,S->Armor+50); World->Speak(TEXT("salttrade"));}
    else if(Id==TEXT("lmg")&&S->Gold>=90&&!S->Unlocked.Contains(5)) {S->Gold-=90; S->Unlocked.Add(5); World->Speak(TEXT("salttrade"));}
    else if(Id==TEXT("scrap-repair")&&S->Scrap>=10) {S->Scrap-=10; S->Armor=FMath::Min(100.f,S->Armor+40);}
    else if(Id==TEXT("quest"))
    {
        FString Reply;
        for(auto Q:World->Quests) if(Q.Giver==Contact)
        {
            int32 Status=S->Quests.FindRef(Q.Id);
            if(Status==0) {S->Quests.Add(Q.Id,1); Reply=Contact==TEXT("SALT-7")?TEXT("saltaccept"):TEXT("questaccept");}
            else if(Status==1&&((Q.Id==TEXT("circuit")&&S->Kills>=3)||S->Items.Contains(Q.Item)))
            {
                S->Quests.Add(Q.Id,2); S->Water+=Q.Water; S->Gold+=Q.Gold; if(Q.Weapon>=0) S->Unlocked.AddUnique(Q.Weapon); if(Q.Id==TEXT("circuit")) S->Allied=true; AddXP(80);
                Reply=Contact==TEXT("SALT-7")?TEXT("saltdone"):TEXT("questdone");
            }
            else if(Status==1) Reply=Contact==TEXT("SALT-7")?TEXT("saltaccept"):TEXT("questwait");
        }
        ContactPanel(Contact);
        if(!Reply.IsEmpty()) World->Speak(Reply,Contact);
    }
    World->Save();
}
