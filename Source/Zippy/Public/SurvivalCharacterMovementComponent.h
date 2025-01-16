#pragma once

#include "CoreMinimal.h"
#include "Zippy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "SurvivalCharacterMovementComponent.generated.h"

/**
 * Delegate broadcast when the character starts a dash. 
 * Typically used to trigger events or effects at dash start.
 */
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FDashStartDelegate);

/**
 * Custom Movement Modes used to handle extended movement states
 * such as Sliding, Proning, Wall Running, etc.
 */
UENUM(BlueprintType)
enum ECustomMovementMode
{
	/**
	 * No custom movement mode set. Hidden in editor.
	 */
	CMOVE_None UMETA(Hidden),

	/**
	 * Sliding movement mode, typically low friction with certain gravity.
	 */
	CMOVE_Slide UMETA(DisplayName = "Slide"),

	/**
	 * Prone movement mode, slower movement, different collision behavior.
	 */
	CMOVE_Prone UMETA(DisplayName = "Prone"),

	/**
	 * Wall running movement mode, used when running along the side of a wall.
	 */
	CMOVE_WallRun UMETA(DisplayName = "Wall Run"),

	/**
	 * Hanging movement mode, used for grabbing onto ledges or surfaces.
	 */
	CMOVE_Hang UMETA(DisplayName = "Hang"),

	/**
	 * Climbing movement mode, used for climbing vertical surfaces.
	 */
	CMOVE_Climb UMETA(DisplayName = "Climb"),

	/**
	 * The maximum custom movement mode value. Hidden in editor.
	 */
	CMOVE_MAX UMETA(Hidden),
};

DECLARE_LOG_CATEGORY_EXTERN(LogSurvivalCharacterMovement, Log, All);

/**
 * Custom Character Movement Component that extends UCharacterMovementComponent
 * with additional movement modes (slide, prone, wallrun, etc.) and functionalities
 * (dash, mantle, etc.).
 * Safe variable are vars that are synced from client to server and are also synced on a correction safe variables
 * should be used for anything that's important to reproduce the same movement on the server and should be saved into a saved move.
 * Compressed flags are the actual data sent to the server.
 */
UCLASS()
class ZIPPY_API USurvivalCharacterMovementComponent : public UCharacterMovementComponent
{
	GENERATED_BODY()

	/**
	 * A saved move struct for custom movement, extending FSavedMove_Character
	 * to store extra flags like sprint, dash, slide, etc.
	 */
	class FSavedMove_SurvivalCharacter : public FSavedMove_Character
	{
	public:

		/**
		 * Custom bit-flags for representing extended movement states
		 * beyond the default jump/crouch flags.
		 */
		enum CompressedFlags
		{
			/** Indicates that the player is sprinting. */
			FLAG_Sprint		= 0x10,

			/** Indicates that the player is dashing. */
			FLAG_Dash		= 0x20,

			/** Indicates that the player wants to slide. */
			FLAG_Slide  	= 0x40,

			/** A spare custom flag if needed (e.g., for another movement state). Currently not used. */
			FLAG_Custom_3	= 0x80,
		};

		/**
		 * Default constructor for FSavedMove_SurvivalCharacter.
		 * Initializes all custom movement flags to zero.
		 */
		FSavedMove_SurvivalCharacter();

		/**
		 * Returns whether this saved move can be combined with another
		 * saved move, to reduce bandwidth usage by merging moves that
		 * share the same key states (e.g. sprint, dash).
		 * @param NewMove The other move to compare with.
		 * @param InCharacter The character using this movement component.
		 * @param MaxDelta The maximum time step for combining moves.
		 * @return True if moves can combine; otherwise false.
		 */
		virtual bool CanCombineWith(const FSavedMovePtr& NewMove, ACharacter* InCharacter, float MaxDelta) const override;

		/**
		 * Resets this saved move's data, clearing out all recorded state.
		 */
		virtual void Clear() override;

		/**
		 * Gets all standard flags plus any custom flags (e.g., sprint, slide).
		 * @return The compressed flag bits representing current move state.
		 */
		virtual uint8 GetCompressedFlags() const override;

		/**
		 * Called to set up this saved move using data from the character.
		 * @param C The character associated with this move.
		 * @param InDeltaTime The time step for this move.
		 * @param NewAccel The new acceleration vector.
		 * @param ClientData Prediction data that tracks client movement.
		 */
		virtual void SetMoveFor(ACharacter* C, float InDeltaTime, FVector const& NewAccel, FNetworkPredictionData_Client_Character& ClientData) override;

		/**
		 * Called just before this move is played back on the client.
		 * Allows us to restore custom flags into the movement component.
		 * @param C The character that will use this move.
		 */
		virtual void PrepMoveFor(ACharacter* C) override;
		
		/** Whether the Zippy jump input was pressed (custom jump). */
		uint8 Saved_bPressedZippyJump : 1;

		/** Whether the player wants to sprint. */
		uint8 Saved_bWantsToSprint : 1;

		/** Whether the player wants to slide. */
		uint8 Saved_bWantsToSlide : 1;

		/** Whether the player wants to dash. */
		uint8 Saved_bWantsToDash : 1;

		/** Whether the character had root motion applied from an animation. */
		uint8 Saved_bHadAnimRootMotion : 1;

		/** Whether a special transition (e.g., mantle) has finished on this tick. */
		uint8 Saved_bTransitionFinished : 1;

		/** Previous crouch state, used to detect transitions. */
		uint8 Saved_bPrevWantsToCrouch : 1;

		/** Whether the player wants to prone. */
		uint8 Saved_bWantsToProne : 1;

		/** Tracks wall-running direction (true if right side, false if left). */
		uint8 Saved_bWallRunIsRight : 1;
		
	};

	/**
	 * Custom prediction data container for client moves, allocating FSavedMove_SurvivalCharacter.
	 */
	class FNetworkPredictionData_Client_SurvivalCharacter : public FNetworkPredictionData_Client_Character
	{
	public:
		
		/**
		 * Constructor that passes the ClientMovement reference
		 * to the parent class.
		 * @param ClientMovement The movement component on this client.
		 */
		FNetworkPredictionData_Client_SurvivalCharacter(const UCharacterMovementComponent& ClientMovement);

		typedef FNetworkPredictionData_Client_Character Super;

		/**
		 * Allocates a new FSavedMove_SurvivalCharacter for capturing
		 * custom movement data each client tick.
		 * @return A pointer to the newly created saved move.
		 */
		virtual FSavedMovePtr AllocateNewMove() override;
		
	};


protected:

#pragma region Movement Settings

	#pragma region Sprint
	
	/** Maximum sprint speed when Safe_bWantsToSprint is true and the character is walking. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Walking", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxSprintSpeed = 750.f;

	#pragma endregion
	
	#pragma region Slide

	/** Weather we can slide off a ledge if false we will attempt to stop from directly sliding of a ledge */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide")
	bool bCanSlideOffOfLedges = true;
	
	/** Minimum horizontal speed needed to initiate a slide. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MinSlideSpeed = 400.f;

	/** Maximum speed you can move at while sliding. NOT WORKING ATM */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxSlideSpeed = 400.f;

	/** Impulse added to the character upon entering a slide. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float SlideEnterImpulse = 400.f;
	
	/** The max speed that the slide initial impulse can be applied if above this speed it will not be applied */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxSlideImpulseSpeed = 700.f;

	/** Additional gravity factor for pulling the character down while sliding. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide", meta=(ClampMin="0", UIMin="0"))
	float SlideGravityForce = 4000.f;

	/** Reduces friction during slide to allow for more slippery movement. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide", meta=(ClampMin="0", UIMin="0"))
	float SlideFrictionFactor = 0.06f;

	/** Braking deceleration applied while sliding. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Slide", meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationSliding = 1000.f;

	#pragma endregion

	#pragma region Prone
	
	/** Duration the player needs to hold crouch to enter prone (seconds). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Prone", meta=(ClampMin="0", UIMin="0", Units="s"))
	float ProneEnterHoldDuration = 0.2f;

	/** Impulse added when transitioning from slide to prone. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Prone", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float ProneSlideEnterImpulse = 300.f;

	/** Maximum speed the character can move while prone. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Prone", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxProneSpeed = 300.f;

	/** Braking deceleration used while proning. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Prone", meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationProning = 2500.f;

	#pragma endregion

	#pragma region Dash
	
	/** The cooldown duration after dashing, preventing immediate re-dash. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Dash", meta=(ClampMin="0", UIMin="0", Units="s"))
	float DashCooldownDuration = 1.f;

	/** The cooldown duration for the server to accept a new dash (slightly shorter than client). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Dash", meta=(ClampMin="0", UIMin="0", Units="s"))
	float AuthDashCooldownDuration = 0.9f;

	/** Montage played when starting a dash. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Dash|Montages")
	UAnimMontage* DashMontage;

	#pragma endregion

	#pragma region Mantle
	
	/** The maximum distance to check in front for a mantle surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MantleMaxDistance = 200;

	/** Extra vertical reach above the capsule half-height for mantling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MantleReachHeight = 50;

	/** Minimum depth of the ledge to be mantleable. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MinMantleDepth = 30;

	/** Minimum angle from vertical to consider a surface valid for mantling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle", meta=(ClampMin="0", UIMin="0", Units="degrees"))
	float MantleMinWallSteepnessAngle = 75;

	/** Maximum angle from vertical allowed for a valid mantle surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle", meta=(ClampMin="0", UIMin="0", Units="degrees"))
	float MantleMaxSurfaceAngle = 40;

	/** Maximum alignment angle between the character's facing and the mantle surface normal. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle", meta=(ClampMin="0", UIMin="0", Units="degrees"))
	float MantleMaxAlignmentAngle = 45;

	/** Animation montage for a tall mantle (climbing a high ledge). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle|Montages")
	UAnimMontage* TallMantleMontage;

	/** Transitional montage played right before a tall mantle is fully active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle|Montages")
	UAnimMontage* TransitionTallMantleMontage;

	/** Montage played on remote proxies for tall mantling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle|Montages")
	UAnimMontage* ProxyTallMantleMontage;

	/** Animation montage for a short mantle (lower ledge). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle|Montages")
	UAnimMontage* ShortMantleMontage;

	/** Transitional montage played right before a short mantle is fully active. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle|Montages")
	UAnimMontage* TransitionShortMantleMontage;

	/** Montage played on remote proxies for short mantling. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Mantle|Montages")
	UAnimMontage* ProxyShortMantleMontage;

	#pragma endregion

	#pragma region Wall Run
	
	/** Minimum speed required to initiate a wall run. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MinWallRunSpeed = 200.f;

	/** Maximum horizontal velocity allowed during wall running. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxWallRunSpeed = 800.f;

	/** Maximum upward velocity allowed during wall running. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxVerticalWallRunSpeed = 200.f;

	/** Angle threshold to pull away from the wall (based on movement input). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run", meta=(ClampMin="0", UIMin="0", ForceUnits="degrees"))
	float WallRunPullAwayAngle = 75;

	/** Force that keeps the character pinned to the wall during a wall run. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run", meta=(ClampMin="0", UIMin="0"))
	float WallAttractionForce = 200.f;

	/** Minimum height off the ground required to initiate a wall run. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MinWallRunHeight = 50.f;
	
	/** Force added to the character when jumping off a wall run. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float WallJumpOffForce = 300.f;
	
	/** A curve controlling gravity scaling during wall run, typically set up so it’s lower at certain input angles. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Wall Run")
	UCurveFloat* WallRunGravityScaleCurve;

	#pragma endregion

	#pragma region Climbing/Hanging

	/** Montage used as a transition before entering hang mode. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Climb")
	UAnimMontage* TransitionHangMontage;

	/** Montage for performing a wall jump (when on a climbable or hangable ledge). */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Climb")
	UAnimMontage* WallJumpMontage;

	/** Force added to the character when executing a wall jump from hang/climb. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Climb", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float WallJumpForce = 400.f;

	/** Maximum speed while climbing a surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Climb", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float MaxClimbSpeed = 300.f;

	/** Braking deceleration applied while climbing. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Climb", meta=(ClampMin="0", UIMin="0"))
	float BrakingDecelerationClimbing = 1000.f;

	/** How far in front of the capsule to check for a climbable surface. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite, Category="Character Movement: Climb", meta=(ClampMin="0", UIMin="0", ForceUnits="cm/s"))
	float ClimbReachDistance = 200.f;

	#pragma endregion

#pragma endregion

	/** A cached pointer to the owning AZippyCharacter, set during InitializeComponent. */
	UPROPERTY(Transient)
	AZippyCharacter* ZippyCharacterOwner;

	/** Whether the character wants to sprint. Mirrors Saved_bWantsToSprint. */
	bool Safe_bWantsToSprint;

	/** Whether the character wants to slide. Mirrors Saved_bWantsToSlide. */
	bool Safe_bWantsToSlide;

	/** Whether the character wants to prone. Mirrors Saved_bWantsToProne. */
	bool Safe_bWantsToProne;

	/** Whether the character wants to dash. Mirrors Saved_bWantsToDash. */
	bool Safe_bWantsToDash;

	/** Whether the character had root-motion last frame. Mirrors Saved_bHadAnimRootMotion. */
	bool Safe_bHadAnimRootMotion;

	/** Caches the previous frame's crouch state, used for transition logic. */
	bool Safe_bPrevWantsToCrouch;

	/** Time at which dash was started, used for cooldown checks. */
	float DashStartTime;

	/** Timer handle for entering prone after holding crouch for a certain duration. */
	FTimerHandle TimerHandle_EnterProne;

	/** Timer handle for dash cooldown logic. */
	FTimerHandle TimerHandle_DashCooldown;

	/** Whether a transition (mantle, etc.) has finished this frame. Mirrors Saved_bTransitionFinished. */
	bool Safe_bTransitionFinished;

	/** A shared pointer to the Root Motion Source used for transitions (like mantling). */
	TSharedPtr<FRootMotionSource_MoveToForce> TransitionRMS;

	/** Name identifying the type of transition (e.g., "Mantle", "Hang") currently in progress. */
	FString TransitionName;

	/** Montage that is queued to play immediately after a root-motion transition completes. */
	UPROPERTY(Transient)
	UAnimMontage* TransitionQueuedMontage;

	/** Play speed for the queued transition montage, based on velocity or other logic. */
	float TransitionQueuedMontageSpeed;

	/** Root-motion source ID returned by ApplyRootMotionSource, for removing later. */
	int TransitionRMS_ID;

	/** Used in wall-running to indicate which side of the wall the character is on. */
	bool Safe_bWallRunIsRight;

	/** Accumulator used on the server to track total location error from client corrections. */
	float AccumulatedClientLocationError = 0.f;

	/** Debugging counter for total ticks executed. */
	int64 TickCount = 0;

	/** Debugging counter for how many client corrections have occurred. */
	int32 CorrectionCount = 0;

	/** Debugging counter for the total bits sent to the server (used to calculate bandwidth usage). */
	int64 TotalBitsSent = 0;

	/** Whether a dash is triggered on the proxy (replicated to non-owning clients). */
	UPROPERTY(ReplicatedUsing=OnRep_Dash)
	bool Proxy_bDash;

	/** Whether a short mantle is triggered on the proxy (replicated to non-owning clients). */
	UPROPERTY(ReplicatedUsing=OnRep_ShortMantle)
	bool Proxy_bShortMantle;

	/** Whether a tall mantle is triggered on the proxy (replicated to non-owning clients). */
	UPROPERTY(ReplicatedUsing=OnRep_TallMantle)
	bool Proxy_bTallMantle;

public:

	/**
	 * Broadcast when a dash is triggered. Useful for UI/FX hooks.
	 */
	UPROPERTY(BlueprintAssignable)
	FDashStartDelegate DashStartDelegate;

public:

	/**
	 * Default constructor to initialize settings like bCanCrouch and bitwriter resizing.
	 */
	USurvivalCharacterMovementComponent();

	/**
	 * Called each tick; increments debug counters, displays on-screen debug info (if desired),
	 * and then calls the parent TickComponent.
	 * @param DeltaTime The time delta for this tick.
	 * @param TickType The kind of ticking this actor is using.
	 * @param ThisTickFunction Pointer to tick function struct.
	 */
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	/**
	 * Overridden from UActorComponent. Called when component is initialized, 
	 * here we cache the owning AZippyCharacter pointer.
	 */
	virtual void InitializeComponent() override;

public:

	/**
	 * Returns a pointer to the client prediction data (FNetworkPredictionData_Client_SurvivalCharacter),
	 * allocating one if it doesn’t already exist.
	 * @return A pointer to the FNetworkPredictionData_Client for this character.
	 */
	virtual FNetworkPredictionData_Client* GetPredictionData_Client() const override;

	/**
	 * Returns true if the character is considered on the ground.
	 * We treat slide and prone modes as 'on ground' too.
	 * @return True if on ground or in a ground-based custom mode.
	 */
	virtual bool IsMovingOnGround() const override;

	/**
	 * Checks if crouching is allowed in the current movement state.
	 * @return True if parent says crouching is allowed AND we are on the ground.
	 */
	virtual bool CanCrouchInCurrentState() const override;

	/**
	 * Returns the max speed (e.g., sprint speed, slide speed) based on the current movement mode.
	 * @return The maximum speed the character can travel at this frame.
	 */
	virtual float GetMaxSpeed() const override;

	/**
	 * Returns the appropriate braking deceleration based on the current movement mode
	 * (slide, prone, climb, etc.) or defaults to parent if not in a custom mode.
	 * @return The max deceleration for the current mode.
	 */
	virtual float GetMaxBrakingDeceleration() const override;

	/**
	 * Checks if the character can attempt a jump. Includes logic for wall run, hang, climb states.
	 * @return True if the character is permitted to attempt a jump right now.
	 */
	virtual bool CanAttemptJump() const override;

	/**
	 * Handles jump input, including custom logic for wall jump or jumping off hang/climb.
	 * @param bReplayingMoves Indicates if we are replaying saved moves on client correction.
	 * @return True if the jump was started; false otherwise.
	 */
	virtual bool DoJump(bool bReplayingMoves) override;

	/**
	 * Checks if the player can move of a ledge in its current movement mode.
	 * @return Can we move off this ledge
	 */
	virtual bool CanWalkOffLedges() const override;

protected:
	/**
	 * Decodes custom compressed flags (e.g., sprint, dash, slide) from network packets.
	 * @param Flags The packed flags from the server.
	 */
	virtual void UpdateFromCompressedFlags(uint8 Flags) override;

	/**
	 * Called on client correction from server, increments correction count and
	 * calls the parent method to finalize correction logic.
	 * @param ClientData The client’s prediction data.
	 * @param TimeStamp The client timestamp for this correction.
	 * @param NewLocation The corrected location from the server.
	 * @param NewVelocity The corrected velocity from the server.
	 * @param NewBase The movement base assigned by the server.
	 * @param NewBaseBoneName The bone name if the new base is a skeletal mesh.
	 * @param bHasBase True if the character has a valid movement base.
	 * @param bBaseRelativePosition True if the client location is relative to the base.
	 * @param ServerMovementMode The movement mode assigned by the server.
	 * @param ServerGravityDirection The gravity direction used by the server (if non-default).
	 */
	virtual void OnClientCorrectionReceived(
		FNetworkPredictionData_Client_Character& ClientData,
		float TimeStamp,
		FVector NewLocation,
		FVector NewVelocity,
		UPrimitiveComponent* NewBase,
		FName NewBaseBoneName,
		bool bHasBase,
		bool bBaseRelativePosition,
		uint8 ServerMovementMode,
		FVector ServerGravityDirection) override;

public:
	/**
	 * Called just before the main movement logic for this frame, after input is collected 
	 * but before Phys* is executed. Handles transitions into custom movement states.
	 * @param DeltaSeconds The time delta for this tick.
	 */
	virtual void UpdateCharacterStateBeforeMovement(float DeltaSeconds) override;

	/**
	 * Called right after the main movement logic, used to finalize states (like stopping root motion).
	 * @param DeltaSeconds The time delta for this tick.
	 */
	virtual void UpdateCharacterStateAfterMovement(float DeltaSeconds) override;

protected:
	/**
	 * Called every frame if MovementMode == MOVE_Custom. We switch on
	 * CustomMovementMode to call the appropriate physics function.
	 * @param deltaTime Time step for this movement update.
	 * @param Iterations Count of how many sub-steps have been processed so far.
	 */
	virtual void PhysCustom(float deltaTime, int32 Iterations) override;

	/**
	 * Called every frame after movement is updated, to record the current crouch state
	 * or perform any additional movement updates before finishing the tick.
	 * @param DeltaSeconds Time step for this movement update.
	 * @param OldLocation The location of the character prior to movement update.
	 * @param OldVelocity The velocity of the character prior to movement update.
	 */
	virtual void OnMovementUpdated(float DeltaSeconds, const FVector& OldLocation, const FVector& OldVelocity) override;

	/**
	 * Called when movement mode changes from one state (e.g., walking) to another 
	 * (e.g., custom). Handles transitions like entering/exiting slide, prone, etc.
	 * @param PreviousMovementMode The mode we are leaving.
	 * @param PreviousCustomMode The custom mode we are leaving if PreviousMovementMode==MOVE_Custom.
	 */
	virtual void OnMovementModeChanged(EMovementMode PreviousMovementMode, uint8 PreviousCustomMode) override;

	/**
	 * Validates client position against the server, accumulates location error
	 * for debugging, and then calls the parent method.
	 * @param ClientTimeStamp The client timestamp for the move.
	 * @param DeltaTime How long this move took on the client.
	 * @param Accel The acceleration input for this move.
	 * @param ClientWorldLocation The location the client reported.
	 * @param RelativeClientLocation The relative client location (if base relative).
	 * @param ClientMovementBase The component the client believes they're standing on.
	 * @param ClientBaseBoneName The bone name for that base if it’s a skeletal mesh.
	 * @param ClientMovementMode The movement mode the client is using.
	 * @return True if there is an error, false otherwise.
	 */
	virtual bool ServerCheckClientError(float ClientTimeStamp, float DeltaTime, const FVector& Accel, const FVector& ClientWorldLocation,
		 const FVector& RelativeClientLocation, UPrimitiveComponent* ClientMovementBase, FName ClientBaseBoneName, uint8 ClientMovementMode) override;

	/**
	 * Used for packing custom movement data into a bitstream before sending it
	 * to the server. This allows partial manual serialization of movement data,
	 * including extra flags for sprint, slide, dash, etc.
	 */
	FNetBitWriter SurvivalServerMoveBitWriter;
	
	/**
	 * Packs custom move data into a bitstream for server RPC, including our extra slide/sprint/etc. flags.
	 * @param NewMove The new saved move we are sending.
	 * @param PendingMove Any pending saved move the server also needs.
	 * @param OldMove The oldest saved move.
	 */
	virtual void CallServerMovePacked(const FSavedMove_Character* NewMove, const FSavedMove_Character* PendingMove, const FSavedMove_Character* OldMove) override;

private:
	/**
	 * Sets up the process of entering a slide by setting bWantsToCrouch to true, 
	 * setting bOrientRotationToMovement to false, and adding the slide impulse 
	 * if we are not moving faster than the max slide speed. Also checks for a valid floor.
	 * @param PrevMode Previous movement mode.
	 * @param PrevCustomMode The previous custom movement mode.
	 */
	void EnterSlide(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);

	/**
	 * Handles logic when exiting slide. Resets crouch flags and re-enables rotation to movement.
	 */
	FORCEINLINE void ExitSlide();

	/**
	 * Checks if conditions are met to start sliding (adequate speed, valid floor, etc.).
	 * @return True if character can transition to slide mode.
	 */
	bool CanSlide() const;

	/**
	 * Physics logic for sliding, including custom friction, gravity, and transitions out of slide.
	 * @param deltaTime The time step for this tick.
	 * @param Iterations Count of how many sub-steps have been processed so far.
	 */
	void PhysSlide(float deltaTime, int32 Iterations);

	/**
	 * Called when the player tries to enter prone. We set Safe_bWantsToProne to true for the next check.
	 */
	FORCEINLINE void OnTryEnterProne() { Safe_bWantsToProne = true; }

	/**
	 * Server RPC to confirm entering prone. Avoids cheating or mismatch across clients/servers.
	 */
	UFUNCTION(Server, Reliable)
	void Server_EnterProne();

	/**
	 * Sets the state to prone (if valid), sets crouch to true, and applies an impulse if transitioning from slide.
	 * @param PrevMode Previous movement mode.
	 * @param PrevCustomMode The previous custom movement mode.
	 */
	void EnterProne(EMovementMode PrevMode, ECustomMovementMode PrevCustomMode);

	/**
	 * Handles logic when exiting prone. Typically just resets any flags.
	 */
	FORCEINLINE void ExitProne();

	/**
	 * Checks conditions to transition into prone. Generally requires the character to be sliding or walking + crouching.
	 * @return True if the character can prone.
	 */
	FORCEINLINE bool CanProne() const;

	/**
	 * Physics logic for proning, including custom deceleration, friction, or transitions out of prone.
	 * @param deltaTime The time step for this tick.
	 * @param Iterations Count of how many sub-steps have been processed so far.
	 */
	void PhysProne(float deltaTime, int32 Iterations);

	/**
	 * Called after the dash button is pressed while dash is on cooldown. 
	 * When this finishes, Safe_bWantsToDash is set to true so the character can dash again.
	 */
	FORCEINLINE void OnDashCooldownFinished();

	/**
	 * Checks if the character can start a dash (e.g., not crouched, or is falling).
	 * @return True if conditions are valid for a dash.
	 */
	FORCEINLINE bool CanDash() const;

	/**
	 * Actually executes the dash by setting flying mode, playing the montage, 
	 * and broadcasting the DashStartDelegate.
	 */
	void PerformDash();

	/**
	 * Attempts a mantle by checking for a ledge in front and above the player,
	 * verifying clearance, and initiating a root-motion transition if valid.
	 * @return True if a mantle is successfully started.
	 */
	bool TryMantle();

	/**
	 * Calculates the starting location for the root-motion mantle move,
	 * choosing between short or tall mantle offsets.
	 * @param FrontHit The initial trace impact on the wall/ledge face.
	 * @param SurfaceHit The trace hit that found the top surface to climb onto.
	 * @param bTallMantle True if we are performing a tall mantle.
	 * @return The position to move the character to with root motion.
	 */
	FVector GetMantleStartLocation(FHitResult FrontHit, FHitResult SurfaceHit, bool bTallMantle) const;

	/**
	 * Attempts to initiate a wall run by checking side traces, velocity, etc.
	 * @return True if a wall run is successfully started.
	 */
	bool TryWallRun();

	/**
	 * Physics logic for wall running, including limiting velocity direction,
	 * applying wall attraction, partial gravity, etc.
	 * @param deltaTime The time step for this tick.
	 * @param Iterations Count of how many sub-steps have been processed so far.
	 */
	void PhysWallRun(float deltaTime, int32 Iterations);

	/**
	 * Checks if the character can hang onto a ledge or climb point by line tracing forward.
	 * If valid, initiates a root-motion transition.
	 * @return True if a hang is successfully started.
	 */
	bool TryHang();

	/**
	 * Checks if the character can climb by tracing forward, verifying no floor beneath, etc.
	 * @return True if a climb is successfully started.
	 */
	bool TryClimb();

	/**
	 * Physics logic for climbing, updating velocity in line with the wall surface, 
	 * applying wall attraction, and transitioning out if invalid.
	 * @param deltaTime The time step for this tick.
	 * @param Iterations Count of how many sub-steps have been processed so far.
	 */
	void PhysClimb(float deltaTime, int32 Iterations);

	/**
	 * @return True if this component is currently on the server (HasAuthority).
	 */
	FORCEINLINE bool IsServer() const;

	/**
	 * @return The capsule radius of the owning character.
	 */
	FORCEINLINE float CapR() const;

	/**
	 * @return The capsule half-height of the owning character.
	 */
	FORCEINLINE float CapHH() const;

public:

	/**
	 * Signals that the player wants to sprint. Sets the Safe_bWantsToSprint flag.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StartSprint();

	/**
	 * Signals that the player released sprint. Unsets the Safe_bWantsToSprint flag.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StopSprint();

	/**
	 * Signals that the player wants to slide. Sets the Safe_bWantsToSlide flag.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StartSlide();

	/**
	 * Signals that the player no longer wants to slide. Unsets the Safe_bWantsToSlide flag.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StopSlide();

	/** @TODO Replace this function by just overriding the Crouch/Uncrouch function in the character class.
	 * Toggles crouch and sets a timer to enter prone if crouch is held long enough.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StartCrouch();

	/** @TODO Replace this function by just overriding the Crouch/Uncrouch function in the character class.
	 * Cancels the timer for entering prone if crouch is released.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StopCrouch();

	/**
	 * Initiates dash logic if not on cooldown, or starts a timer to try again 
	 * if the cooldown will expire soon.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StartDash();

	/**
	 * Clears the dash cooldown timer, unsets the dash flag if dash input is released.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StopDash();

	/**
	 * Signals that the player wants to climb or continue climbing if possible. 
	 * Sets bWantsToCrouch if in midair or already climbing.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StartClimb();

	/**
	 * Unsets bWantsToCrouch, effectively stopping climb/hang if pressed.
	 */
	UFUNCTION(BlueprintCallable)
	FORCEINLINE void StopClimb();

	/**
	 * Checks if we’re in a specific custom movement mode.
	 * @param InCustomMovementMode The custom mode to check.
	 * @return True if currently in that custom mode.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsCustomMovementMode(ECustomMovementMode InCustomMovementMode) const;

	/**
	 * Checks if we’re in a specific standard movement mode (walking, falling, etc.).
	 * @param InMovementMode The movement mode to check.
	 * @return True if currently in that mode.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsMovementMode(EMovementMode InMovementMode) const;

	/**
	 * @return True if we are currently in wall run mode.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsWallRunning() const { return IsCustomMovementMode(CMOVE_WallRun); }

	/**
	 * @return True if the wall we’re running along is on our right side.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool WallRunningIsRight() const { return Safe_bWallRunIsRight; }

	/**
	 * @return True if we are currently in hanging mode.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsHanging() const { return IsCustomMovementMode(CMOVE_Hang); }

	/**
	 * @return True if we are currently in climbing mode.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsClimbing() const { return IsCustomMovementMode(CMOVE_Climb); }

	/**
	 * @return True if we are currently in sliding mode.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsSliding() const { return IsCustomMovementMode(CMOVE_Slide); }

	/**
	 * @return True if we are currently in prone mode.
	 */
	UFUNCTION(BlueprintPure)
	FORCEINLINE bool IsProne() const { return IsCustomMovementMode(CMOVE_Prone); }

public:

	/**
	 * Called to gather properties that need replication. 
	 * Ensures dash and mantle booleans replicate to non-owning clients.
	 * @param OutLifetimeProps The array of FLifetimeProperty definitions to update.
	 */
	virtual void GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const override;

private:
	/**
	 * Called when Proxy_bDash is replicated to remote clients.
	 * Plays the dash montage and broadcasts the dash start event.
	 */
	UFUNCTION()
	void OnRep_Dash();

	/**
	 * Called when Proxy_bShortMantle is replicated to remote clients.
	 * Plays the ProxyShortMantleMontage.
	 */
	UFUNCTION()
	void OnRep_ShortMantle();

	/**
	 * Called when Proxy_bTallMantle is replicated to remote clients.
	 * Plays the ProxyTallMantleMontage.
	 */
	UFUNCTION()
	void OnRep_TallMantle();
};
