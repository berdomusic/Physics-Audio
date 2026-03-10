WIP

Case study on audio implementation for physics simulating objects

The physics audio system provides realistic, procedurally driven sound feedback for physical interactions in the game world. It dynamically responds to motion based on real-time physics calculations.

Core Concepts

Custom AkComponent

The system uses UPAPhysicsAudioComponent derived from the UAkComponent class.
Tick is disabled by default and enabled only when the component is active.
RTPC updates occur only when values change beyond a tolerance threshold.
Distance-based culling prevents audio processing for inaudible events.
The component can attach to two types of primitives:

Simulating Components
Mass is automatically taken from the physics body.

Non-Simulating Components
Must provide a mass value in audio properties.

The component tracks three primary physical states:

Impact: Sudden collision events with configurable thresholds.
Rolling: Continuous rotation while in contact with a surface.
Sliding: Linear motion across a surface without rotation.

Apart from that, impacts from projectiles and object destruction are handled separately.

State updates occur both on tick and on component hit, with a flag system preventing double-processing within a single frame.
States use cooldowns to prevent audio overlap and excessive triggering. These cooldowns either use default values or customized per-event values.


Object pooling 
Instead of creating and destroying audio components repeatedly, they are taken from and returned to a pool spawned when physics audio is enabled. They can be spawned synchronously or asynchronously (1 per frame).

Class separation 
The physics actor and its audio component are independent and do not communicate with each other. Audio components are attached to primitive components instead, so any primitive will do. All attachements are handled via subsystem.

Data-driven design
All setup including audio events, loading, and cooldowns is configured via data tables.

Async loading
Audio components and events are loaded asynchronously by default. The system also allows forcing synchronous loading when needed.

Manual attachement
The example provides implementation of audio component as a default subobject of an actor (in this case, projectile). These components are not parts of pooling system.