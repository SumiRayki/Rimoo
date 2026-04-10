// WeaselSetup.cpp : main source file for WeaselSetup.exe
//

#include "stdafx.h"

#include "resource.h"
#include "WeaselUtility.h"
#include <thread>
#include <filesystem>
#include <fstream>

#include "InstallOptionsDlg.h"

#include <ShellScalingApi.h>
#pragma comment(lib, "Shcore.lib")
CAppModule _Module;

static int Run(LPTSTR lpCmdLine);
static bool IsProcAdmin();
static int RestartAsAdmin(LPTSTR lpCmdLine);
static void EnsureUserCustomPreferences(const std::wstring& user_dir, bool hant);

int WINAPI _tWinMain(HINSTANCE hInstance,
                     HINSTANCE /*hPrevInstance*/,
                     LPTSTR lpstrCmdLine,
                     int /*nCmdShow*/) {
  HRESULT hRes = ::CoInitialize(NULL);
  ATLASSERT(SUCCEEDED(hRes));

  AtlInitCommonControls(
      ICC_BAR_CLASSES);  // add flags to support other controls

  hRes = _Module.Init(NULL, hInstance);
  ATLASSERT(SUCCEEDED(hRes));

  LANGID langId = get_language_id();
  SetThreadUILanguage(langId);
  SetThreadLocale(langId);

  int nRet = Run(lpstrCmdLine);

  _Module.Term();
  ::CoUninitialize();

  return nRet;
}
int install(bool hant, bool silent);
int uninstall(bool silent);
bool has_installed();
static int finalize_user_preferences(bool hant);

static std::wstring install_dir() {
  WCHAR exe_path[MAX_PATH] = {0};
  GetModuleFileNameW(GetModuleHandle(NULL), exe_path, _countof(exe_path));
  std::wstring dir(exe_path);
  size_t pos = dir.find_last_of(L"\\");
  dir.resize(pos);
  return dir;
}

static std::wstring default_user_dir() {
  WCHAR path[MAX_PATH] = {0};
  ExpandEnvironmentStringsW(L"%APPDATA%\\Rime", path, _countof(path));
  return std::wstring(path);
}

static void UpsertYamlPatchValue(std::string& content,
                                 const std::string& key,
                                 const std::string& value) {
  const std::string line = "  \"" + key + "\": " + value;
  const std::string prefix = "  \"" + key + "\":";
  size_t pos = content.find(prefix);
  if (pos != std::string::npos) {
    size_t end = content.find('\n', pos);
    if (end == std::string::npos) {
      content.replace(pos, content.size() - pos, line);
      content.push_back('\n');
    } else {
      content.replace(pos, end - pos, line);
    }
    return;
  }

  if (content.find("patch:\n") == std::string::npos) {
    if (!content.empty() && content.back() != '\n')
      content.push_back('\n');
    content += "patch:\n";
  }
  if (!content.empty() && content.back() != '\n')
    content.push_back('\n');
  content += line + "\n";
}

static void EnsureUserCustomPreferences(const std::wstring& user_dir, bool hant) {
  namespace fs = std::filesystem;
  const fs::path path = fs::path(user_dir) / L"user.custom.yaml";
  std::string content;

  if (fs::exists(path)) {
    std::ifstream in(path, std::ios::binary);
    content.assign(std::istreambuf_iterator<char>(in),
                   std::istreambuf_iterator<char>());
  }

  if (content.empty()) {
    content =
        "customization:\n"
        "  distribution_code_name: Rimoo\n"
        "  distribution_version: 0.1.0\n"
        "  generator: \"Rimoo::PreviewDefaults\"\n"
        "patch:\n";
  }

  UpsertYamlPatchValue(content, "var/previously_selected_schema",
                       hant ? "luna_pinyin" : "luna_pinyin_simp");
  UpsertYamlPatchValue(content, "var/option/simplification",
                       hant ? "false" : "true");
  UpsertYamlPatchValue(content, "var/option/zh_simp", hant ? "false" : "true");
  UpsertYamlPatchValue(content, "var/option/zh_hans", hant ? "false" : "true");
  UpsertYamlPatchValue(content, "var/option/zh_hant", hant ? "true" : "false");
  UpsertYamlPatchValue(content, "var/option/zh_hant_tw",
                       hant ? "true" : "false");

  std::ofstream out(path, std::ios::binary | std::ios::trunc);
  out.write(content.data(), static_cast<std::streamsize>(content.size()));
}

static int CustomInstall(bool installing) {
  bool hant = false;
  bool silent = false;
  std::wstring user_dir;

  const WCHAR KEY[] = L"Software\\Rime\\Weasel";
  HKEY hKey;
  LSTATUS ret = RegOpenKey(HKEY_CURRENT_USER, KEY, &hKey);
  if (ret == ERROR_SUCCESS) {
    WCHAR value[MAX_PATH];
    DWORD len = sizeof(value);
    DWORD type = 0;
    DWORD data = 0;
    ret =
        RegQueryValueEx(hKey, L"RimeUserDir", NULL, &type, (LPBYTE)value, &len);
    if (ret == ERROR_SUCCESS && type == REG_SZ) {
      user_dir = value;
    }
    len = sizeof(data);
    ret = RegQueryValueEx(hKey, L"Hant", NULL, &type, (LPBYTE)&data, &len);
    if (ret == ERROR_SUCCESS && type == REG_DWORD) {
      hant = (data != 0);
      if (installing)
        silent = true;
    }
    RegCloseKey(hKey);
  }
  bool _has_installed = has_installed();
  if (!silent) {
    InstallOptionsDialog dlg;
    dlg.installed = _has_installed;
    dlg.hant = hant;
    dlg.user_dir = user_dir;
    if (IDOK != dlg.DoModal()) {
      if (!installing)
        return 1;  // aborted by user
    } else {
      hant = dlg.hant;
      user_dir = dlg.user_dir;
      _has_installed = dlg.installed;
    }
  }
  if (!_has_installed)
    if (0 != install(hant, silent))
      return 1;

  if (user_dir.empty()) {
    // default user dir %APPDATA%\Rime
    WCHAR _path[MAX_PATH] = {0};
    ExpandEnvironmentStringsW(L"%APPDATA%\\Rime", _path, _countof(_path));
    user_dir = std::wstring(_path);
  }
  ret = SetRegKeyValue(HKEY_CURRENT_USER, KEY, L"RimeUserDir", user_dir.c_str(),
                       REG_SZ, false);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_BY_IDS(IDS_STR_ERR_WRITE_USER_DIR, IDS_STR_INSTALL_FAILED,
               MB_ICONERROR | MB_OK);
    return 1;
  }
  ret = SetRegKeyValue(HKEY_CURRENT_USER, KEY, L"Hant", (hant ? 1 : 0),
                       REG_DWORD, false);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_BY_IDS(IDS_STR_ERR_WRITE_HANT, IDS_STR_INSTALL_FAILED,
               MB_ICONERROR | MB_OK);
    return 1;
  }
  if (_has_installed) {
    std::wstring dir(install_dir());
    std::thread th([dir]() {
      ShellExecuteW(NULL, NULL, (dir + L"\\WeaselServer.exe").c_str(), L"/q",
                    NULL, SW_SHOWNORMAL);
      Sleep(500);
      ShellExecuteW(NULL, NULL, (dir + L"\\WeaselServer.exe").c_str(), L"",
                    NULL, SW_SHOWNORMAL);
      Sleep(500);
      ShellExecuteW(NULL, NULL, (dir + L"\\WeaselDeployer.exe").c_str(),
                    L"/deploy", NULL, SW_SHOWNORMAL);
    });
    th.detach();
    MSG_BY_IDS(IDS_STR_MODIFY_SUCCESS_INFO, IDS_STR_MODIFY_SUCCESS_CAP,
               MB_ICONINFORMATION | MB_OK);
  }

  return 0;
}

LPCTSTR GetParamByPrefix(LPCTSTR lpCmdLine, LPCTSTR prefix) {
  return (wcsncmp(lpCmdLine, prefix, wcslen(prefix)) == 0)
             ? (lpCmdLine + wcslen(prefix))
             : 0;
}

static int Run(LPTSTR lpCmdLine) {
  constexpr bool silent = true;
  // parameter /? or /help to show commandline args
  if (!wcscmp(L"/?", lpCmdLine) || !wcscmp(L"/help", lpCmdLine)) {
    WCHAR msg[1024] = {0};
    if (LoadString(GetModuleHandle(NULL), IDS_STR_HELP, msg,
                   sizeof(msg) / sizeof(TCHAR))) {
      MessageBox(NULL, msg, L"Rimoo Setup", MB_ICONINFORMATION | MB_OK);
    } else {
      MessageBox(
          NULL,
          L"Usage: WeaselSetup.exe [options]\n"
          L"/? or /help    - Show this help message\n"
          L"/u             - Uninstall Rimoo\n"
          L"/i             - Install Rimoo\n"
          L"/s             - Install Rimoo (Simplified Chinese)\n"
          L"/t             - Install Rimoo (Traditional Chinese)\n"
          L"/ls            - Set Rimoo language to Simplified Chinese\n"
          L"/lt            - Set Rimoo language to Traditional Chinese\n"
          L"/le            - Set Rimoo language to English\n"
          L"/toggleime     - Toggle IME on open/close(ctrl+space)\n"
          L"/toggleascii   - Toggle ASCII on open/close(ctrl+space)\n"
          L"/userdir:<dir> - Set user directory\n",
          L"Rimoo Setup", MB_ICONINFORMATION | MB_OK);
    }
    return 0;
  }
  bool uninstalling = !wcscmp(L"/u", lpCmdLine);
  if (uninstalling) {
    if (IsProcAdmin())
      return uninstall(silent);
    else
      return RestartAsAdmin(lpCmdLine);
  }

  if (auto res = GetParamByPrefix(lpCmdLine, L"/userdir:")) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"RimeUserDir", res, REG_SZ);
  }

  if (!wcscmp(L"/ls", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"Language", L"chs", REG_SZ);
  } else if (!wcscmp(L"/lt", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"Language", L"cht", REG_SZ);
  } else if (!wcscmp(L"/le", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"Language", L"eng", REG_SZ);
  }

  if (!wcscmp(L"/toggleime", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"ToggleImeOnOpenClose", L"yes", REG_SZ);
  }
  if (!wcscmp(L"/toggleascii", lpCmdLine)) {
    return SetRegKeyValue(HKEY_CURRENT_USER, L"Software\\Rime\\weasel",
                          L"ToggleImeOnOpenClose", L"no", REG_SZ);
  }

  if (!IsProcAdmin()) {
    return RestartAsAdmin(lpCmdLine);
  }

  bool hans = !wcscmp(L"/s", lpCmdLine);
  if (hans) {
    int ret = install(false, silent);
    if (ret == 0)
      ret = finalize_user_preferences(false);
    return ret;
  }
  bool hant = !wcscmp(L"/t", lpCmdLine);
  if (hant) {
    int ret = install(true, silent);
    if (ret == 0)
      ret = finalize_user_preferences(true);
    return ret;
  }
  bool installing = !wcscmp(L"/i", lpCmdLine);
  return CustomInstall(installing);
}

static int finalize_user_preferences(bool hant) {
  const WCHAR KEY[] = L"Software\\Rime\\Weasel";
  auto ret = SetRegKeyValue(HKEY_CURRENT_USER, KEY, L"Hant", (hant ? 1 : 0),
                            REG_DWORD, false);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_BY_IDS(IDS_STR_ERR_WRITE_HANT, IDS_STR_INSTALL_FAILED,
               MB_ICONERROR | MB_OK);
    return 1;
  }
  ret = SetRegKeyValue(HKEY_CURRENT_USER, KEY, L"Language",
                       hant ? L"cht" : L"chs", REG_SZ, false);
  if (FAILED(HRESULT_FROM_WIN32(ret))) {
    MSG_BY_IDS(IDS_STR_ERR_WRITE_HANT, IDS_STR_INSTALL_FAILED,
               MB_ICONERROR | MB_OK);
    return 1;
  }
  EnsureUserCustomPreferences(default_user_dir(), hant);
  return 0;
}

// https://learn.microsoft.com/zh-cn/windows/win32/api/securitybaseapi/nf-securitybaseapi-checktokenmembership
bool IsProcAdmin() {
  BOOL b = FALSE;
  SID_IDENTIFIER_AUTHORITY NtAuthority = SECURITY_NT_AUTHORITY;
  PSID AdministratorsGroup;
  b = AllocateAndInitializeSid(&NtAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                               DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                               &AdministratorsGroup);

  if (b) {
    if (!CheckTokenMembership(NULL, AdministratorsGroup, &b)) {
      b = FALSE;
    }
    FreeSid(AdministratorsGroup);
  }

  return (b);
}

int RestartAsAdmin(LPTSTR lpCmdLine) {
  SHELLEXECUTEINFO execInfo{0};
  TCHAR path[MAX_PATH];
  GetModuleFileName(GetModuleHandle(NULL), path, _countof(path));
  execInfo.lpFile = path;
  execInfo.lpParameters = lpCmdLine;
  execInfo.lpVerb = _T("runas");
  execInfo.cbSize = sizeof(execInfo);
  execInfo.nShow = SW_SHOWNORMAL;
  execInfo.fMask = SEE_MASK_NOASYNC | SEE_MASK_NOCLOSEPROCESS;
  execInfo.hwnd = NULL;
  execInfo.hProcess = NULL;
  if (::ShellExecuteEx(&execInfo) && execInfo.hProcess != NULL) {
    ::WaitForSingleObject(execInfo.hProcess, INFINITE);
    DWORD dwExitCode = 0;
    ::GetExitCodeProcess(execInfo.hProcess, &dwExitCode);
    ::CloseHandle(execInfo.hProcess);
    return dwExitCode;
  }
  return -1;
}
