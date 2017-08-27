$vsixSource = "https://github.com/csoltenborn/GoogleTestAdapter/releases/download/v0.10.1/GoogleTestAdapter-0.10.1.vsix"
$vsixTarget = "$($env:USERPROFILE)\GoogleTestAdapter-0.10.1.vsix"
(New-Object Net.WebClient).DownloadFile($vsixSource, $vsixTarget)
$vsixInstallerPath = "C:\Program Files (x86)\Microsoft Visual Studio\2017\Community\Common7\IDE"
"`"$vsixInstallerPath\VSIXInstaller.exe`" /q /a $vsixTarget" | out-file ".\install-vsix.cmd" -Encoding ASCII
& .\install-vsix.cmd