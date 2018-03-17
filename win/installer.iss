#define MyAppName "Moolticute"
#define MyAppID "{329B8184-21E7-463E-B1A6-789657616680}"
#define MyAppPublisher "Moolticute"
#define MyAppURL "http://themooltipass.com/"
; #define MyAppVersion "1.0"

[Setup]
; NOTE: The value of AppId uniquely identifies this application.
; Do not use the same AppId value in installers for other applications.
; (To generate a new GUID, click Tools | Generate GUID inside the IDE.)
AppId={{#MyAppID}
AppName={#MyAppName}
AppVersion={#MyAppVersion}
;AppVerName={#MyAppName} {#MyAppVersion}
AppPublisher={#MyAppPublisher}
AppPublisherURL={#MyAppURL}
AppSupportURL={#MyAppURL}
AppUpdatesURL={#MyAppURL}
DefaultDirName={userpf}\{#MyAppName}
DefaultGroupName=Moolticute
DisableProgramGroupPage=no
OutputDir=build
OutputBaseFilename=moolticute_setup_{#MyAppVersion}
Compression=lzma
SolidCompression=no
AppCopyright=Copyright (c) The Mooltipass Team
WizardSmallImageFile=WizModernSmallImage-IS.bmp
SetupIconFile=Setup.ico
UninstallDisplayIcon={app}\moolticute.exe
MinVersion=0,5.01sp3
PrivilegesRequired=lowest

[Languages]
Name: "english"; MessagesFile: "compiler:Default.isl"

[Tasks]
Name: "desktopicon"; Description: "{cm:CreateDesktopIcon}"; GroupDescription: "{cm:AdditionalIcons}"

[Files]
Source: "C:\moolticute_build\*"; DestDir: "{app}"; Flags: ignoreversion recursesubdirs createallsubdirs; Components: moolticute;

[Icons]
Name: "{group}\Moolticute"; Filename: "{app}\moolticute.exe"; Components: moolticute
Name: "{group}\{cm:UninstallProgram,{#MyAppName}}"; Filename: "{uninstallexe}"
Name: "{commondesktop}\Moolticute"; Filename: "{app}\moolticute.exe"; Components: moolticute; Tasks: desktopicon
Name: "{group}\Mooltipass"; Filename: "http://themooltipass.com"; Components: moolticute; IconFilename: "{app}\question.ico";
Name: "{group}\Moolticute Github"; Filename: "http://github.com/mooltipass/moolticute"; Components: moolticute; IconFilename: "{app}\question.ico";

[Types]
Name: "Full"; Description: "Full installation"

[Components]
Name: "moolticute"; Description: "Moolticute"; Types: Full

[Run]
Filename: "{app}\moolticute.exe"; WorkingDir: "{app}"; Description: "Start Moolticute"; Flags: postinstall nowait skipifsilent runascurrentuser

[Registry]
Root: "HKCU"; Subkey: "SOFTWARE\Microsoft\Windows\CurrentVersion\Run"; ValueType: string; ValueName: "Moolticute"; ValueData: "{app}\moolticute.exe --autolaunched"; Flags: uninsdeletevalue

[Code]
function IsAppRunning(const FileName : string): Boolean;
var
    FSWbemLocator: Variant;
    FWMIService   : Variant;
    FWbemObjectSet: Variant;
begin
    Result := false;
    FSWbemLocator := CreateOleObject('WBEMScripting.SWBEMLocator');
    FWMIService := FSWbemLocator.ConnectServer('', 'root\CIMV2', '', '');
    FWbemObjectSet := FWMIService.ExecQuery(Format('SELECT Name FROM Win32_Process Where Name="%s"',[FileName]));
    Result := (FWbemObjectSet.Count > 0);
    FWbemObjectSet := Unassigned;
    FWMIService := Unassigned;
    FSWbemLocator := Unassigned;
end;
 
function CompareVersion(str1, str2: String): Integer;
var
  temp1, temp2: String;
begin
    temp1 := str1;
    temp2 := str2;
	  if temp1 <> temp2 then
	  begin
	    Result := -1;
    end
    else
	  begin
	    Result := 0;
	  end;
end;
 
function InitializeSetup(): Boolean;
var
  oldVersion: String;
  uninstaller: String;
  ErrorCode: Integer;
begin
  
  Result := true;
  if IsAppRunning( 'moolticute.exe' ) or IsAppRunning( 'moolticuted.exe' ) then
  BEGIN
    ShellExec('', ExpandConstant('{sys}\taskkill.exe'),'/f /im moolticute.exe', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
    ShellExec('', ExpandConstant('{sys}\taskkill.exe'),'/f /im moolticuted.exe', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  END

  if RegKeyExists(HKEY_LOCAL_MACHINE,
    'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppID}_is1') then
  begin
    RegQueryStringValue(HKEY_LOCAL_MACHINE,
      'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppID}_is1',
      'DisplayVersion', oldVersion);
    if (CompareVersion(oldVersion, '{#MyAppVersion}') < 0) then
    begin
      if MsgBox('Version ' + oldVersion + ' of Moolticute is already installed. Do you want to replace this version with {#MyAppVersion}?',
        mbConfirmation, MB_YESNO) = IDNO then
      begin
        Result := False;
      end
      else
      begin
          RegQueryStringValue(HKEY_LOCAL_MACHINE,
            'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppID}_is1',
            'UninstallString', uninstaller);
          ShellExec('', uninstaller, 'SILENT', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
          if (ErrorCode <> 0) then
          begin
            MsgBox( 'Failed to uninstall Moolticute version ' + oldVersion + '. Please restart Windows and run setup again.',
             mbError, MB_OK );
            Result := False;
          end
          else
          begin
            Result := True;
          end;
      end;
    end
    else
    begin
      MsgBox('Version ' + oldVersion + ' of Moolticute is already installed. This installer will exit.',
        mbInformation, MB_OK);
      Result := False;
    end;
  end
  else
  if RegKeyExists(HKEY_CURRENT_USER,
    'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppID}_is1') then
  begin
    RegQueryStringValue(HKEY_CURRENT_USER,
      'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppID}_is1',
      'DisplayVersion', oldVersion);
    if (CompareVersion(oldVersion, '{#MyAppVersion}') < 0) then
    begin
      if MsgBox('Version ' + oldVersion + ' of Moolticute is already installed. Do you want to replace this version with {#MyAppVersion}?',
        mbConfirmation, MB_YESNO) = IDNO then
      begin
        Result := False;
      end
      else
      begin
          RegQueryStringValue(HKEY_CURRENT_USER,
            'SOFTWARE\Microsoft\Windows\CurrentVersion\Uninstall\{#MyAppID}_is1',
            'UninstallString', uninstaller);
          ShellExec('', uninstaller, 'SILENT', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
          if (ErrorCode <> 0) then
          begin
            MsgBox( 'Failed to uninstall Moolticute version ' + oldVersion + '. Please restart Windows and run setup again.',
             mbError, MB_OK );
            Result := False;
          end
          else
          begin
            Result := True;
          end;
      end;
    end
    else
    begin
      MsgBox('Version ' + oldVersion + ' of Moolticute is already installed. This installer will exit.',
        mbInformation, MB_OK);
      Result := False;
    end;
  end
end;
 
function InitializeUninstall(): Boolean;
var
  ErrorCode: Integer;
begin
  Result := true;
   if IsAppRunning( 'moolticute.exe' ) or IsAppRunning( 'moolticuted.exe' ) then
  BEGIN
    ShellExec('', ExpandConstant('{sys}\taskkill.exe'),'/f /im moolticute.exe', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
    ShellExec('', ExpandConstant('{sys}\taskkill.exe'),'/f /im moolticuted.exe', '', SW_HIDE, ewWaitUntilTerminated, ErrorCode);
  END
end;

