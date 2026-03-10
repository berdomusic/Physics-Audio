WIP

Case study on audio implementation for physics simulating objects. 

The physics audio system provides realistic, procedurally driven sound feedback for physical interactions in the game world. It dynamically responds to motion based on real-time physics calculations.
Implementation is mainly in C++, with some functions exposed to blueprints. All other systems (pickup, UI, input etc.) were be treated as non-essential placeholders, 
thus they are implemented with mixed Blueprint/C++ methodology for faster iteration.

-------------------------------------------------------------------------------------------------------
Core Concepts


-------------------------------------------------------------------------------------------------------
UPAPhysicsAudioComponent : UAkComponent
-------------------------------------------------------------------------------------------------------

#
The system uses UPAPhysicsAudioComponent derived from the UAkComponent class.
#
Tick is disabled by default and enabled only when the component is active.
#
RTPC updates occur only when values change beyond a tolerance threshold.
#
Distance-based culling prevents audio processing for inaudible events.

#
The component can attach to two types of primitives:

Simulating Components
Mass is automatically taken from the physics body.

Non-Simulating Components
Must provide a mass value in audio properties.

#
The component tracks three primary physical states:

Impact: Sudden collision events with configurable thresholds.
Rolling: Continuous rotation while in contact with a surface.
Sliding: Linear motion across a surface without rotation.

Apart from that, impacts from projectiles and object destruction are handled separately.

#
State updates occur both on tick and on component hit, with a flag system preventing double-processing within a single frame.
States use cooldowns to prevent audio overlap and excessive triggering. These cooldowns either use default values or customized per-event values.

-------------------------------------------------------------------------------------------------------
Physics Audio Subsystem
-------------------------------------------------------------------------------------------------------

The Physics Audio Subsystem has the following functions exposed to blueprints:
 
    int32 GetCurrentPoolSize()
    bool IsPhysicsAudioEnabled()
    void EnablePhysicsAudio(bool bAsync, int32 InPoolSize = 10);
    void DisablePhysicsAudio();
    bool CanEnablePhysicsAudio() const;
    TArray<FVector> GetListenersPositions()
    void GetPhysicsAudioComponentCount(int32& Available, int32& Active, int32& PendingReturn) const;

-------------------------------------------------------------------------------------------------------
Object Pooling 
Instead of creating and destroying audio components repeatedly, they are taken from and returned to a pool spawned when physics audio is enabled. They can be spawned synchronously or asynchronously (1 per frame).

-------------------------------------------------------------------------------------------------------
Class Separation
The physics actor and its audio component are independent and do not communicate with each other. Audio components are attached to primitive components instead, so any primitive will do. All attachments are handled via the subsystem.

-------------------------------------------------------------------------------------------------------
Data-Driven Design
All setup including audio events, loading, and cooldowns is configured via data tables.

-------------------------------------------------------------------------------------------------------
Async Loading
Audio components and events are loaded asynchronously by default. The system also allows forcing synchronous loading when needed.

-------------------------------------------------------------------------------------------------------
Manual Attachment
The example provides implementation of an audio component as a default subobject of an actor (in this case, a projectile). These components are not part of the pooling system.