Set sh = CreateObject("WScript.Shell")
Set fs = CreateObject("Scripting.FileSystemObject")
sh.Run Chr(34) & fs.BuildPath(fs.GetParentFolderName(WScript.ScriptFullName), "Jugar.exe") & Chr(34), 1, False
