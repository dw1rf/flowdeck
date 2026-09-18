#define AppVersion GetEnv("FLOWDECK_VERSION")
#if AppVersion == ""
  #define AppVersion "0.2.0"
#endif
[Setup]
AppId={{B144862C-6C53-4A4E-AD31-82BA6F942E77}
AppName=FlowDeck
AppVersion={#AppVersion}
AppPublisher=FlowDeck
DefaultDirName={localappdata}\Programs\FlowDeck
DefaultGroupName=FlowDeck
OutputDir=..\dist
OutputBaseFilename=FlowDeck-Setup-x64
Compression=lzma2
SolidCompression=yes
PrivilegesRequired=lowest
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
WizardStyle=modern
UninstallDisplayIcon={app}\flowdeck.exe
ChangesAssociations=no
CloseApplications=yes
RestartApplications=no

[Files]
Source: "..\dist\FlowDeck-x64\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs

[Icons]
Name: "{group}\FlowDeck"; Filename: "{app}\flowdeck.exe"
Name: "{autodesktop}\FlowDeck"; Filename: "{app}\flowdeck.exe"; Tasks: desktopicon

[Tasks]
Name: "desktopicon"; Description: "Create desktop shortcut"; GroupDescription: "Additional shortcuts:"

[Run]
Filename: "{app}\flowdeck.exe"; Description: "Start FlowDeck"; Flags: nowait postinstall skipifsilent
