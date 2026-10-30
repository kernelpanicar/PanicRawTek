; =============================================================================
;  PANIC RAW TEK — instalador para Windows (Inno Setup 6)
;  Compilar:  ISCC.exe /DMyAppVersion=3.1.0 /DArtDir=..\..\build\PanicRawTek_artefacts\Release PanicRawTek.iss
; =============================================================================

#ifndef MyAppVersion
  #define MyAppVersion "3.2.0"
#endif
#ifndef ArtDir
  #define ArtDir "..\..\build\PanicRawTek_artefacts\Release"
#endif

#define MyAppName      "Panic Raw Tek"
#define MyAppPublisher "Panic Audio"
#define MyAppExe       "Panic Raw Tek.exe"

[Setup]
AppId={{6B0D6F1E-3C1A-4E7B-9F5D-50A1C0DE7E31}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
VersionInfoVersion={#MyAppVersion}
VersionInfoDescription={#MyAppName} Installer
DefaultDirName={autopf}\{#MyAppPublisher}\{#MyAppName}
DefaultGroupName={#MyAppPublisher}
DisableProgramGroupPage=yes
OutputDir=..\..\dist
OutputBaseFilename=Panic Raw Tek {#MyAppVersion} Windows Setup
SetupIconFile=PanicRawTek.ico
UninstallDisplayIcon={app}\{#MyAppExe}
UninstallDisplayName={#MyAppName}
WizardStyle=modern
WizardImageFile=wizard.bmp,wizard_2x.bmp
WizardSmallImageFile=wizard_small.bmp,wizard_small_2x.bmp
Compression=lzma2/max
SolidCompression=yes
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=admin
CloseApplications=yes
MinVersion=10.0

[Languages]
Name: "spanish"; MessagesFile: "compiler:Languages\Spanish.isl"
Name: "english"; MessagesFile: "compiler:Default.isl"

[CustomMessages]
spanish.CompVST3=Plug-in VST3 (Ableton Live, FL Studio, Bitwig, Reaper, Cubase...)
english.CompVST3=VST3 plug-in (Ableton Live, FL Studio, Bitwig, Reaper, Cubase...)
spanish.CompApp=App standalone (tocar sin DAW)
english.CompApp=Standalone app (play without a DAW)
spanish.TypeFull=Instalación completa
english.TypeFull=Full installation
spanish.TypeCustom=Personalizada
english.TypeCustom=Custom
spanish.Done=Panic Raw Tek quedó instalado.%n%nEn Ableton Live: Settings > Plug-Ins > activá "Use VST3 Plug-In System Folders" y hacé click en Rescan.%nLo vas a encontrar en Plug-Ins > Panic Audio > Panic Raw Tek.
english.Done=Panic Raw Tek is installed.%n%nIn Ableton Live: Settings > Plug-Ins > enable "Use VST3 Plug-In System Folders" and click Rescan.%nYou'll find it under Plug-Ins > Panic Audio > Panic Raw Tek.

[Types]
Name: "full";   Description: "{cm:TypeFull}"
Name: "custom"; Description: "{cm:TypeCustom}"; Flags: iscustom

[Components]
Name: "vst3"; Description: "{cm:CompVST3}"; Types: full custom
Name: "app";  Description: "{cm:CompApp}";  Types: full

[InstallDelete]
; reemplazar instalaciones anteriores limpio
Type: filesandordirs; Name: "{commoncf64}\VST3\Panic Raw Tek.vst3"
Type: filesandordirs; Name: "{commoncf64}\VST3\RawTek.vst3"

[Files]
Source: "{#ArtDir}\VST3\Panic Raw Tek.vst3\*"; DestDir: "{commoncf64}\VST3\Panic Raw Tek.vst3"; Components: vst3; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#ArtDir}\Standalone\{#MyAppExe}";    DestDir: "{app}"; Components: app; Flags: ignoreversion
Source: "..\..\README.md";                     DestDir: "{app}"; DestName: "README.txt"; Flags: ignoreversion
Source: "..\..\Resources\Fonts\LICENSE-*.txt"; DestDir: "{app}\licenses"; Flags: ignoreversion

[Icons]
Name: "{autoprograms}\{#MyAppName}";          Filename: "{app}\{#MyAppExe}"; Components: app
Name: "{autoprograms}\Desinstalar {#MyAppName}"; Filename: "{uninstallexe}"

[UninstallDelete]
Type: filesandordirs; Name: "{commoncf64}\VST3\Panic Raw Tek.vst3"

[Run]
Filename: "{app}\{#MyAppExe}"; Description: "{cm:LaunchProgram,{#MyAppName}}"; Flags: nowait postinstall skipifsilent unchecked; Components: app

[Code]
procedure CurStepChanged(CurStep: TSetupStep);
begin
  if (CurStep = ssDone) and (not WizardSilent) then
    MsgBox(ExpandConstant('{cm:Done}'), mbInformation, MB_OK);
end;
