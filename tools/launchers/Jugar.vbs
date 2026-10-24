Set sh = CreateObject("WScript.Shell")
Set fs = CreateObject("Scripting.FileSystemObject")
root = fs.GetParentFolderName(fs.GetParentFolderName(fs.GetParentFolderName(WScript.ScriptFullName)))
sh.Run Chr(34) & fs.BuildPath(root, "Jugar.exe") & Chr(34), 1, False
