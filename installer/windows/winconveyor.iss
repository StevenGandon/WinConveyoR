#ifndef AppVersion
  #define AppVersion "0.0.0"
#endif
#ifndef NumericVersion
  #define NumericVersion "0.0.0"
#endif
#ifndef SourceDir
  #define SourceDir "..\..\dist\dist-windows-amd64"
#endif
#ifndef LicenseFile
  #define LicenseFile "..\..\LICENSE"
#endif
#ifndef OutputDir
  #define OutputDir "..\..\installer-out"
#endif

#define AppName "WinConveyoR"

[Setup]
AppId={{8B3F5C1E-4A7D-4E2B-9C61-2D5A7E0F13B4}
AppName={#AppName}
AppVersion={#AppVersion}
AppVerName={#AppName} {#AppVersion}
AppPublisher={#AppName}
VersionInfoVersion={#NumericVersion}
VersionInfoProductName={#AppName}
DefaultDirName={autopf}\{#AppName}
UsePreviousAppDir=yes
DisableProgramGroupPage=yes
LicenseFile={#LicenseFile}
OutputDir={#OutputDir}
OutputBaseFilename={#AppName}-{#AppVersion}-windows-amd64-setup
Compression=lzma2/ultra64
SolidCompression=yes
WizardStyle=modern
ArchitecturesAllowed=x64compatible
ArchitecturesInstallIn64BitMode=x64compatible
PrivilegesRequired=lowest
PrivilegesRequiredOverridesAllowed=dialog commandline
ChangesEnvironment=yes
UninstallDisplayName={#AppName} {#AppVersion}
UninstallDisplayIcon={app}\bin\wcr.exe

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Types]
Name: "full";    Description: "Full installation"
Name: "compact"; Description: "Minimal installation (CLI and library only)"
Name: "custom";  Description: "Custom installation"; Flags: iscustom

[Components]
Name: "core";     Description: "Command-line tools and runtime library (required)"; Types: full compact custom; Flags: fixed
Name: "server";   Description: "Mirror server (wcrmirror)"; Types: full
Name: "devtools"; Description: "Developer tools (wcrpackager, wcrgenkey, header_gen) and C headers"; Types: full

[Tasks]
Name: "addtopath"; Description: "Add {#AppName} to the &PATH environment variable"; GroupDescription: "System integration:"
Name: "devenv";    Description: "Set &WINCONVEYOR_HOME (headers in %WINCONVEYOR_HOME%\include, libraries in %WINCONVEYOR_HOME%\lib)"; GroupDescription: "System integration:"; Components: devtools

[Files]
Source: "{#SourceDir}\bin\wcr.exe";    DestDir: "{app}\bin"; Components: core; Flags: ignoreversion

Source: "{#SourceDir}\lib\*";     DestDir: "{app}\lib"; Components: core; Flags: ignoreversion recursesubdirs createallsubdirs
Source: "{#SourceDir}\lib\*.dll"; DestDir: "{app}\bin"; Components: core; Flags: ignoreversion
Source: "{#LicenseFile}"; DestDir: "{app}"; DestName: "LICENSE.txt"; Components: core; Flags: ignoreversion

Source: "{#SourceDir}\bin\wcrmgr.exe"; DestDir: "{app}\bin"; Components: server; Flags: ignoreversion
Source: "{#SourceDir}\bin\wcrmirror.exe"; DestDir: "{app}\bin"; Components: server; Flags: ignoreversion
Source: "{#SourceDir}\bin\wcrpackager.exe"; DestDir: "{app}\bin"; Components: server; Flags: ignoreversion
Source: "{#SourceDir}\bin\wcrgenkey.exe";   DestDir: "{app}\bin"; Components: server; Flags: ignoreversion

Source: "{#SourceDir}\bin\header_gen.exe";  DestDir: "{app}\bin"; Components: devtools; Flags: ignoreversion
Source: "{#SourceDir}\include\*"; DestDir: "{app}\include"; Components: devtools; Flags: ignoreversion recursesubdirs createallsubdirs

[Code]
function EnvRoot(): Integer;
begin
  if IsAdminInstallMode then
    Result := HKEY_LOCAL_MACHINE
  else
    Result := HKEY_CURRENT_USER;
end;

function EnvSubKey(): String;
begin
  if IsAdminInstallMode then
    Result := 'SYSTEM\CurrentControlSet\Control\Session Manager\Environment'
  else
    Result := 'Environment';
end;

procedure EnvAddPath(Dir: String);
var
  Paths: String;
begin
  if not RegQueryStringValue(EnvRoot(), EnvSubKey(), 'Path', Paths) then
    Paths := '';
  { already present? }
  if Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Paths) + ';') > 0 then
    Exit;
  if (Paths <> '') and (Paths[Length(Paths)] <> ';') then
    Paths := Paths + ';';
  Paths := Paths + Dir;
  RegWriteExpandStringValue(EnvRoot(), EnvSubKey(), 'Path', Paths);
end;

procedure EnvRemovePath(Dir: String);
var
  Paths: String;
  P: Integer;
begin
  if not RegQueryStringValue(EnvRoot(), EnvSubKey(), 'Path', Paths) then
    Exit;
  P := Pos(';' + Uppercase(Dir) + ';', ';' + Uppercase(Paths) + ';');
  if P = 0 then
    Exit;
  Delete(Paths, P, Length(Dir) + 1);
  RegWriteExpandStringValue(EnvRoot(), EnvSubKey(), 'Path', Paths);
end;

procedure CurStepChanged(CurStep: TSetupStep);
begin
  if CurStep = ssPostInstall then
  begin
    if WizardIsTaskSelected('addtopath') then
      EnvAddPath(ExpandConstant('{app}\bin'));
    if WizardIsTaskSelected('devenv') then
      RegWriteStringValue(EnvRoot(), EnvSubKey(), 'WINCONVEYOR_HOME', ExpandConstant('{app}'));
  end;
end;

procedure CurUninstallStepChanged(CurUninstallStep: TUninstallStep);
begin
  if CurUninstallStep = usPostUninstall then
  begin
    EnvRemovePath(ExpandConstant('{app}\bin'));
    RegDeleteValue(EnvRoot(), EnvSubKey(), 'WINCONVEYOR_HOME');
  end;
end;
