Scriptname NAF extends ScriptObject Native

Struct SequencePhase
    ; If set to -1, will loop infinitely. If set to 0, indicates a play-once/non-looping animation.
	Int numLoops = 0
    ; The amount of time to take to blend from the previous phase to this phase, in seconds. Does not affect phase timing, only visuals.
    Float transitionTime = 1.0
    ; The path to the animation file, starting from the Starfield/Data/NAF folder. i.e. Starfield/Data/NAF/CustomAnim.glb would just be CustomAnim.glb
	String filePath
EndStruct

Function PlayAnimation(Actor akTarget, String asAnim, Float fTransitionSeconds = 1.0) Native Global

; Returns true if the actor is playing a NAF animation.
Bool Function StopAnimation(Actor akTarget, Float fTransitionSeconds = 1.0) Native Global

Function SyncAnimations(Actor[] akTargets) Native Global

Function StopSyncing(Actor akTarget) Native Global

Function StartSequence(Actor akTarget, SequencePhase[] sPhases, Bool bLoop) Native Global

; Returns true if the actor currently has a sequence.
Bool Function AdvanceSequence(Actor akTarget, Bool bSmooth) Native Global

; Returns true if the actor currently has a sequence.
Bool Function SetSequencePhase(Actor akTarget, Int iPhase) Native Global

; Returns -1 if the actor does not currently have a sequence.
Int Function GetSequencePhase(Actor akTarget) Native Global

; Returns true if the actor is playing a NAF animation. Only persists for the current animation.
; 100.0 is normal speed.
Bool Function SetAnimationSpeed(Actor akTarget, Float fSpeed) Native Global

; Returns 0 if the actor is not playing a NAF animation.
; Note: The value returned from this might be slightly different than SetAnimationSpeed due to float imprecision.
; 100.0 is normal speed.
Float Function GetAnimationSpeed(Actor akTarget, Float fSpeed) Native Global

; Returns an empty string if the actor is not playing a NAF animation.
String Function GetCurrentAnimation(Actor akTarget) Native Global