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
Function StopAnimation(Actor akTarget, Float fTransitionSeconds = 1.0) Native Global
Function SyncAnimations(Actor[] akTargets) Native Global
Function StopSyncing(Actor akTarget) Native Global
Function StartSequence(Actor akTarget, SequencePhase[] sPhases, Bool bLoop) Native Global
Bool Function AdvanceSequence(Actor akTarget, Bool bSmooth) Native Global
String Function GetCurrentAnimation(Actor akTarget) Native Global