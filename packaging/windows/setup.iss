#ifndef AppVersion
  #error AppVersion is required
#endif
[Setup]
AppId=TFX.TelegramEffects
AppName=TFX
AppVersion={#AppVersion}
DefaultDirName={localappdata}\Programs\TFX
PrivilegesRequired=lowest
DisableProgramGroupPage=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
OutputDir=..\..
OutputBaseFilename=TFX-setup-x64
SetupIconFile=..\..\assets\tfx-icon.ico
UninstallDisplayIcon={app}\bin\tfx.exe
Compression=lzma2/fast
SolidCompression=yes
CloseApplications=yes
RestartApplications=no

[Files]
Source: "..\..\dist\*"; DestDir: "{app}"; Flags: recursesubdirs ignoreversion

[Icons]
Name: "{userprograms}\TFX"; Filename: "{app}\bin\tfx.exe"

[Run]
Filename: "{app}\bin\tfx.exe"; Description: "Start TFX"; Flags: postinstall nowait skipifsilent
