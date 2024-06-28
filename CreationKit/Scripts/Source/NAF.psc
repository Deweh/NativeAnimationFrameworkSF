Scriptname NAF extends ScriptObject

Struct SequencePhase
    ; If set to -1, will loop infinitely. If set to 0, indicates a play-once/non-looping animation.
	Int numLoops = 0
    ; The amount of time to take to blend from the previous phase to this phase, in seconds. Does not affect phase timing, only visuals.
    Float transitionTime = 1.0
    ; The path to the animation file, starting from the Starfield/Data/NAF folder. i.e. Starfield/Data/NAF/CustomAnim.glb would just be CustomAnim.glb
	String filePath
EndStruct

Function PlayAnimation(Actor akTarget, String asAnim) Global
    Debug.ExecuteConsole("naf s play " + asAnim + " " + Utility.IntToHex(akTarget.GetFormID()))
EndFunction

Function StopAnimation(Actor akTarget) Global
    Debug.ExecuteConsole("naf s stop " + Utility.IntToHex(akTarget.GetFormID()))
EndFunction

Function SyncAnimations(Actor[] akTargets) Global
    Int i = 0
    Int size = akTargets.Length
    String cmd = "naf s sync"
    While i < size
        cmd += " " + Utility.IntToHex(akTargets[i].GetFormID())
        i = i + 1
    EndWhile
    Debug.ExecuteConsole(cmd)
EndFunction

Function StopSyncing(Actor akTarget) Global
    Debug.ExecuteConsole("naf s stopsync " + Utility.IntToHex(akTarget.GetFormID()))
EndFunction

Function StartSequence(Actor akTarget, SequencePhase[] sPhases, Bool bLoop) Global
    Int i = 0
    Int size = sPhases.Length
    String cmd = "naf s startseq " + Utility.IntToHex(akTarget.GetFormID()) + " " + bLoop
    While i < size
        cmd += " " + sPhases[i].filePath + " " + sPhases[i].numLoops + " " + sPhases[i].transitionTime
        i = i + 1
    EndWhile
    Debug.ExecuteConsole(cmd)
EndFunction

Function AdvanceSequence(Actor akTarget, Bool bSmooth) Global
    Debug.ExecuteConsole("naf s advseq " + bSmooth + " " + Utility.IntToHex(akTarget.GetFormID()))
EndFunction