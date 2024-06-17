Scriptname NAF

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