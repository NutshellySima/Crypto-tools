function Change-HduHost() {
    if (!([Security.Principal.WindowsPrincipal][Security.Principal.WindowsIdentity]::GetCurrent()).IsInRole([Security.Principal.WindowsBuiltInRole] "Administrator")) { Start-Process powershell.exe "-NoProfile -ExecutionPolicy Bypass -Command `"iex ((new-object net.webclient).DownloadString('https://raw.githubusercontent.com/NutshellySima/Scripts/master/hdu.ps1'))`"" -Verb RunAs; exit }
    (Get-Content -Path "C:\Windows\System32\drivers\etc\hosts") -notmatch "218.75.123.182" | Out-File "C:\Windows\System32\drivers\etc\hosts"
    Add-Content -Path "C:\Windows\System32\drivers\etc\hosts" -Value "`n218.75.123.182 bestcoder.hdu.edu.cn`n218.75.123.182 acm.hdu.edu.cn"
}
Change-HduHost
