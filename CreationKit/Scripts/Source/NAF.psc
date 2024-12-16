Scriptname NAF extends ScriptObject Native

Struct SequencePhase
    ; If set to -1, will loop infinitely. If set to 0, indicates a play-once/non-looping animation.
	Int numLoops = 0
    ; The amount of time to take to blend from the previous phase to this phase, in seconds. Does not affect phase timing, only visuals.
    Float transitionTime = 1.0
    ; The path to the animation file, starting from the Starfield/Data/NAF folder. i.e. Starfield/Data/NAF/CustomAnim.glb would just be CustomAnim.glb
    ; This also works with blend graph (.bt) files.
	String filePath
EndStruct

; Plays an animation file directly on an actor. The file path starts from the Starfield/Data/NAF folder. i.e. Starfield/Data/NAF/CustomAnim.glb would just be CustomAnim.glb
; This also works with blend graph (.bt) files.
Function PlayAnimation(Actor akTarget, String asAnim, Float fTransitionSeconds = 1.0) Native Global

; This is a conveinence function for starting a sequence with one phase that simply plays the given animation once then stops.
Function PlayAnimationOnce(Actor akTarget, String asAnim, Float fTransitionSeconds = 1.0) Native Global

; Returns true if the actor is playing a NAF animation.
Bool Function StopAnimation(Actor akTarget, Float fTransitionSeconds = 1.0) Native Global

; Synchronizes multiple actors' animation times, sequence phase indexes, blend graph variables, and root positions.
; Requires all provided actors to be currently playing a NAF animation.
;
; The actor at index 0 will be made the "owner" of the synchronization. All actors will be forced to the owner's root position.
; Make sure all actors are in the same worldspace before calling this function, or else the positioning may cause unexpected results.
; Changes to animation speed, sequence phase & blend graph variables should be done to the owner. Changes to non-owners will not do anything.
;
; This synchronization will persist until an actor stops playing any NAF animations, or has StopSyncing called on them.
Function SyncAnimations(Actor[] akTargets) Native Global

; Cancels any synchronization currently being performed on the provided actor.
; If the actor is the synchronization "owner", then all actors synced to this one will also stop syncing.
Function StopSyncing(Actor akTarget) Native Global

; Starts a sequence of animations on an actor, given an array of phases.
Function StartSequence(Actor akTarget, SequencePhase[] sPhases, Bool bLoop) Native Global

; Returns true if the actor currently has a sequence.
; Note: Internally, this function sets a flag on the actor that is checked when their animation updates.
; So, if they're not currently loaded (i.e. in a different worldspace), their sequence phase won't change until they become loaded.
; To instantly set the phase regardless of whether the actor is loaded or not, use SetSequencePhase.
Bool Function AdvanceSequence(Actor akTarget, Bool bSmooth) Native Global

; Returns true if the actor currently has a sequence.
Bool Function SetSequencePhase(Actor akTarget, Int iPhase) Native Global

; Returns -1 if the actor does not currently have a sequence.
Int Function GetSequencePhase(Actor akTarget) Native Global

; Sets whether or not NAF should lock an actor's world position. Only works while the actor is playing a NAF animation.
; This defaults to true when an animation is started on an actor. If set to false, the actor will be able to move
; while the animation is playing.
Function SetPositionLocked(Actor akTarget, Bool bLocked) Native Global

; Sets an actor's position in the world. Only works while the actor is playing a NAF animation & their position is locked.
; NAF will force the actor to stay at this position until the animation is stopped or they move to a different cell.
; This value will be overriden if the actor's position is being synchronized to another actor.
Function SetActorPosition(Actor akTarget, Float fX, Float fY, Float fZ) Native Global

; Returns true if the actor is playing a NAF animation. Only persists for the current animation.
; 100.0 is normal speed.
Bool Function SetAnimationSpeed(Actor akTarget, Float fSpeed) Native Global

; Returns 0 if the actor is not playing a NAF animation.
; Note: The value returned from this might be slightly different than SetAnimationSpeed due to float imprecision.
; 100.0 is normal speed.
Float Function GetAnimationSpeed(Actor akTarget) Native Global

; Returns an empty string if the actor is not playing a NAF animation.
String Function GetCurrentAnimation(Actor akTarget) Native Global

; Returns true if the actor is currently playing a blend graph and the blend graph has a variable with the provided name.
; This function will always return false if the provided actor is unloaded (i.e. not in the current worldspace),
; as blend graphs are not kept in memory when an actor is unloaded.
Bool Function SetBlendGraphVariable(Actor akTarget, String asName, Float fValue) Native Global

; Returns 0.0f if the actor is not playing a blend graph, or the blend graph does not have a variable with the provided name.
; This function will always return 0.0f if the provided actor is unloaded (i.e. not in the current worldspace),
; as blend graphs are not kept in memory when an actor is unloaded.
Float Function GetBlendGraphVariable(Actor akTarget, String asName) Native Global

;; EVENTS
; Important: Registrations only last for the current game session, so you will need to re-register whenever the PlayerLoadGame event occurs.
; These events work like the RegisterForExternalEvent() function from F4SE. You have to make a function in your script with the correct params,
; and give the function name when registering. The params for each event are commented above the respective registration function.

; Event Params: (ObjectReference akTarget, Int iPhase, String sName)
; Event for when the animation for a sequence phase has fully loaded and begun playing.
Function RegisterForPhaseBegin(ScriptObject sScript, String sFunctionName) Native Global

; Event Params: (ObjectReference akTarget, String sName)
; Event for when an actor ends their current sequence. This will also occur if a sequence animation fails to load, as that causes the sequence to end itself.
Function RegisterForSequenceEnd(ScriptObject sScript, String sFunctionName) Native Global

Function UnregisterForPhaseBegin(ScriptObject sScript) Native Global

Function UnregisterForSequenceEnd(ScriptObject sScript) Native Global