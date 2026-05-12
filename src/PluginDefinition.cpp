//This file is part of Language Selector.
//Copyright (C) 2025 Abdellah Hassaine.

#include "resource.h"                   // Resource IDs
#include "PluginDefinition.h"           // Notepad++ plugin definitions
#include "menuCmdID.h"                  // Notepad++ menu command IDs
#include <windows.h>                    // Windows API headers
#include <string>                       // C++ string library
#include <vector>                       // C++ vector container
#include <algorithm>                    // C++ algorithms (sort, remove)
#include <ctime>                        // C++ time functions
#include <fstream>                      // C++ file stream operations
#include <sstream>                      // C++ string stream operations
#include <commctrl.h>                   // Windows common controls

#pragma comment(lib, "comctl32.lib")    // Link with common controls library

using namespace std;

#define nbFunc 3
FuncItem funcItem[nbFunc];

HINSTANCE g_hInstance = NULL;
NppData nppData;
static WNDPROC g_originalNppProc = NULL;
static std::vector<int> g_shownBuffers;
static HFONT hComboFont = NULL;
static HFONT hBoldFont = NULL;


static bool g_isShuttingDown = false;

#define CONFIG_FILE L"LanguageSelector.ini"
#define DEFAULT_FAV_LIMIT 5
#define DEFAULT_AUTO_SHOW true
static int  g_stripHeight = 45;
int g_maxFavorites = DEFAULT_FAV_LIMIT;
bool g_autoShowOnNewTab = DEFAULT_AUTO_SHOW;

static wstring g_currentLanguageName;
static int g_currentDisplayIndex = -1;

// Function prototypes
void RepopulateComboBox(HWND hCombo);
void InitializeLanguages();
void UpdateDisplayOrder();
int CountFavorites();
int FindOldestFavorite();
void PruneExcessFavorites();
int FindDisplayIndexByName(const wstring& name);
void SaveConfiguration();
void LoadConfiguration();
wstring GetConfigFilePath();

wstring GetConfigFilePath() {
    wchar_t configPath[MAX_PATH] = { 0 };
    ::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);
    wstring path(configPath);
    path += L"\\";
    path += CONFIG_FILE;
    return path;
}

void commandMenuInit() {
    setCommand(0, TEXT("Select Language"), ShowLanguageDialog, NULL, false);
    setCommand(1, TEXT("Settings"), ShowSettingsDialog, NULL, false);
    setCommand(2, TEXT("About"), About, NULL, false);
}

bool setCommand(size_t index, TCHAR* cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey* sk, bool check0nInit) {
    if (index >= nbFunc) return false;
    if (!pFunc) return false;
    lstrcpy(funcItem[index]._itemName, cmdName);
    funcItem[index]._pFunc = pFunc;
    funcItem[index]._init2Check = check0nInit;
    funcItem[index]._pShKey = sk;
    return true;
}

struct Language {
    wstring displayName;
    int id;
    bool isFavorite;
    time_t favoriteTime;

    Language(const wchar_t* name, int langId, bool favorite = false)
        : displayName(name), id(langId), isFavorite(favorite), favoriteTime(0) {
        if (favorite) {
            favoriteTime = time(NULL);
        }
    }

    wstring GetDisplayName() const {
        return displayName;
    }
};

vector<Language> allLanguages;
vector<int> displayOrder;

void InitializeLanguages() {
    allLanguages.clear();
    allLanguages.push_back(Language(L"Normal text", L_TEXT, false));
    allLanguages.push_back(Language(L"PHP", L_PHP, false));
    allLanguages.push_back(Language(L"C", L_C, false));
    allLanguages.push_back(Language(L"C++", L_CPP, false));
    allLanguages.push_back(Language(L"C#", L_CS, false));
    allLanguages.push_back(Language(L"Objective-C", L_OBJC, false));
    allLanguages.push_back(Language(L"Java", L_JAVA, false));
    allLanguages.push_back(Language(L"RC", L_RC, false));
    allLanguages.push_back(Language(L"HTML", L_HTML, false));
    allLanguages.push_back(Language(L"XML", L_XML, false));
    allLanguages.push_back(Language(L"Makefile", L_MAKEFILE, false));
    allLanguages.push_back(Language(L"Pascal", L_PASCAL, false));
    allLanguages.push_back(Language(L"Batch", L_BATCH, false));
    allLanguages.push_back(Language(L"ini", L_INI, false));
    allLanguages.push_back(Language(L"NFO", L_ASCII, false));
    allLanguages.push_back(Language(L"udf", L_USER, false));
    allLanguages.push_back(Language(L"ASP", L_ASP, false));
    allLanguages.push_back(Language(L"SQL", L_SQL, false));
    allLanguages.push_back(Language(L"Visual Basic", L_VB, false));
    allLanguages.push_back(Language(L"Embedded JS", L_JS_EMBEDDED, false));
    allLanguages.push_back(Language(L"CSS", L_CSS, false));
    allLanguages.push_back(Language(L"Perl", L_PERL, false));
    allLanguages.push_back(Language(L"Python", L_PYTHON, false));
    allLanguages.push_back(Language(L"Lua", L_LUA, false));
    allLanguages.push_back(Language(L"TeX", L_TEX, false));
    allLanguages.push_back(Language(L"Fortran free form", L_FORTRAN, false));
    allLanguages.push_back(Language(L"Shell", L_BASH, false));
    allLanguages.push_back(Language(L"ActionScript", L_FLASH, false));
    allLanguages.push_back(Language(L"NSIS", L_NSIS, false));
    allLanguages.push_back(Language(L"TCL", L_TCL, false));
    allLanguages.push_back(Language(L"Lisp", L_LISP, false));
    allLanguages.push_back(Language(L"Scheme", L_SCHEME, false));
    allLanguages.push_back(Language(L"Assembly", L_ASM, false));
    allLanguages.push_back(Language(L"Diff", L_DIFF, false));
    allLanguages.push_back(Language(L"Properties file", L_PROPS, false));
    allLanguages.push_back(Language(L"PostScript", L_PS, false));
    allLanguages.push_back(Language(L"Ruby", L_RUBY, false));
    allLanguages.push_back(Language(L"Smalltalk", L_SMALLTALK, false));
    allLanguages.push_back(Language(L"VHDL", L_VHDL, false));
    allLanguages.push_back(Language(L"KiXtart", L_KIX, false));
    allLanguages.push_back(Language(L"AutoIt", L_AU3, false));
    allLanguages.push_back(Language(L"CAML", L_CAML, false));
    allLanguages.push_back(Language(L"Ada", L_ADA, false));
    allLanguages.push_back(Language(L"Verilog", L_VERILOG, false));
    allLanguages.push_back(Language(L"MATLAB", L_MATLAB, false));
    allLanguages.push_back(Language(L"Haskell", L_HASKELL, false));
    allLanguages.push_back(Language(L"Inno Setup", L_INNO, false));
    allLanguages.push_back(Language(L"Internal Search", L_SEARCHRESULT, false));
    allLanguages.push_back(Language(L"CMake", L_CMAKE, false));
    allLanguages.push_back(Language(L"YAML", L_YAML, false));
    allLanguages.push_back(Language(L"COBOL", L_COBOL, false));
    allLanguages.push_back(Language(L"Gui4Cli", L_GUI4CLI, false));
    allLanguages.push_back(Language(L"D", L_D, false));
    allLanguages.push_back(Language(L"PowerShell", L_POWERSHELL, false));
    allLanguages.push_back(Language(L"R", L_R, false));
    allLanguages.push_back(Language(L"JSP", L_JSP, false));
    allLanguages.push_back(Language(L"CoffeeScript", L_COFFEESCRIPT, false));
    allLanguages.push_back(Language(L"json", L_JSON, false));
    allLanguages.push_back(Language(L"JavaScript", L_JAVASCRIPT, false));
    allLanguages.push_back(Language(L"Fortran fixed form", L_FORTRAN_77, false));
    allLanguages.push_back(Language(L"BaanC", L_BAANC, false));
    allLanguages.push_back(Language(L"S-Record", L_SREC, false));
    allLanguages.push_back(Language(L"Intel HEX", L_IHEX, false));
    allLanguages.push_back(Language(L"Tektronix extended HEX", L_TEHEX, false));
    allLanguages.push_back(Language(L"Swift", L_SWIFT, false));
    allLanguages.push_back(Language(L"ASN.1", L_ASN1, false));
    allLanguages.push_back(Language(L"AviSynth", L_AVS, false));
    allLanguages.push_back(Language(L"BlitzBasic", L_BLITZBASIC, false));
    allLanguages.push_back(Language(L"PureBasic", L_PUREBASIC, false));
    allLanguages.push_back(Language(L"FreeBasic", L_FREEBASIC, false));
    allLanguages.push_back(Language(L"Csound", L_CSOUND, false));
    allLanguages.push_back(Language(L"Erlang", L_ERLANG, false));
    allLanguages.push_back(Language(L"ESCRIPT", L_ESCRIPT, false));
    allLanguages.push_back(Language(L"Forth", L_FORTH, false));
    allLanguages.push_back(Language(L"LaTeX", L_LATEX, false));
    allLanguages.push_back(Language(L"MMIXAL", L_MMIXAL, false));
    allLanguages.push_back(Language(L"Nim", L_NIM, false));
    allLanguages.push_back(Language(L"Nncrontab", L_NNCRONTAB, false));
    allLanguages.push_back(Language(L"OScript", L_OSCRIPT, false));
    allLanguages.push_back(Language(L"REBOL", L_REBOL, false));
    allLanguages.push_back(Language(L"registry", L_REGISTRY, false));
    allLanguages.push_back(Language(L"Rust", L_RUST, false));
    allLanguages.push_back(Language(L"Spice", L_SPICE, false));
    allLanguages.push_back(Language(L"txt2tags", L_TXT2TAGS, false));
    allLanguages.push_back(Language(L"Visual Prolog", L_VISUALPROLOG, false));
    allLanguages.push_back(Language(L"TypeScript", L_TYPESCRIPT, false));
    allLanguages.push_back(Language(L"json5", L_JSON5, false));
    allLanguages.push_back(Language(L"mssql", L_MSSQL, false));
    allLanguages.push_back(Language(L"GDScript", L_GDSCRIPT, false));
    allLanguages.push_back(Language(L"Hollywood", L_HOLLYWOOD, false));
    allLanguages.push_back(Language(L"Go", L_GOLANG, false));
    allLanguages.push_back(Language(L"Raku", L_RAKU, false));
    allLanguages.push_back(Language(L"TOML", L_TOML, false));
    allLanguages.push_back(Language(L"SAS", L_SAS, false));
    allLanguages.push_back(Language(L"ErrorList", L_ERRORLIST, false));
    allLanguages.push_back(Language(L"External", L_EXTERNAL, false));
}

int FindOldestFavorite() {
    time_t oldestTime = time(NULL);
    int oldestIndex = -1;
    for (int i = 0; i < static_cast<int>(allLanguages.size()); i++) {
        if (allLanguages[i].isFavorite && allLanguages[i].favoriteTime <= oldestTime) {
            oldestTime = allLanguages[i].favoriteTime;
            oldestIndex = i;
        }
    }
    return oldestIndex;
}

void UpdateDisplayOrder() {
    displayOrder.clear();
    vector<int> favoriteIndices;
    vector<int> regularIndices;

    for (int i = 0; i < static_cast<int>(allLanguages.size()); i++) {
        if (allLanguages[i].isFavorite) {
            favoriteIndices.push_back(i);
        }
        else {
            regularIndices.push_back(i);
        }
    }

    sort(favoriteIndices.begin(), favoriteIndices.end(),
        [&](int a, int b) {
            if (allLanguages[a].favoriteTime != allLanguages[b].favoriteTime) {
                return allLanguages[a].favoriteTime > allLanguages[b].favoriteTime;
            }
            return allLanguages[a].displayName < allLanguages[b].displayName;
        });

    sort(regularIndices.begin(), regularIndices.end(),
        [&](int a, int b) {
            return allLanguages[a].displayName < allLanguages[b].displayName;
        });

    displayOrder.insert(displayOrder.end(), favoriteIndices.begin(), favoriteIndices.end());
    displayOrder.insert(displayOrder.end(), regularIndices.begin(), regularIndices.end());
}

void RepopulateComboBox(HWND hCombo) {
    if (!hCombo) return;

    SendMessage(hCombo, CB_RESETCONTENT, 0, 0);

    int favoriteCount = CountFavorites();

    if (favoriteCount > 0) {
        SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"Favourites");
        int headerIndex = static_cast<int>(SendMessage(hCombo, CB_GETCOUNT, 0, 0) - 1);
        SendMessage(hCombo, CB_SETITEMDATA, headerIndex, (LPARAM)HEADER_FAVORITES);
    }

    for (int displayIdx = 0; displayIdx < static_cast<int>(displayOrder.size()); displayIdx++) {
        int actualIdx = displayOrder[displayIdx];
        Language& lang = allLanguages[actualIdx];
        if (lang.isFavorite) {
            wstring displayName = lang.GetDisplayName();
            LRESULT itemIndex = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)displayName.c_str());
            SendMessage(hCombo, CB_SETITEMDATA, itemIndex, (LPARAM)lang.id);
        }
    }

    SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"All languages");
    int allHeaderIndex = static_cast<int>(SendMessage(hCombo, CB_GETCOUNT, 0, 0) - 1);
    SendMessage(hCombo, CB_SETITEMDATA, allHeaderIndex, (LPARAM)HEADER_ALL);

    for (int displayIdx = 0; displayIdx < static_cast<int>(displayOrder.size()); displayIdx++) {
        int actualIdx = displayOrder[displayIdx];
        Language& lang = allLanguages[actualIdx];
        if (!lang.isFavorite) {
            wstring displayName = lang.GetDisplayName();
            LRESULT itemIndex = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)displayName.c_str());
            SendMessage(hCombo, CB_SETITEMDATA, itemIndex, (LPARAM)lang.id);
        }
    }

    if (!g_currentLanguageName.empty()) {
        int count = static_cast<int>(SendMessage(hCombo, CB_GETCOUNT, 0, 0));
        for (int i = 0; i < count; i++) {
            wchar_t itemText[256] = { 0 };
            SendMessage(hCombo, CB_GETLBTEXT, i, (LPARAM)itemText);
            LRESULT itemData = SendMessage(hCombo, CB_GETITEMDATA, i, 0);
            if (itemData == HEADER_FAVORITES || itemData == HEADER_ALL)
                continue;
            if (g_currentLanguageName == itemText) {
                SendMessage(hCombo, CB_SETCURSEL, i, 0);
                g_currentDisplayIndex = i;
                break;
            }
        }
    }

    SendMessage(hCombo, CB_SETTOPINDEX, 0, 0);
}

void LoadConfiguration() {
    wstring configFile = GetConfigFilePath();
    ifstream file(configFile);

    if (file.is_open()) {
        string line;
        vector<int> favoriteIDs;

        while (getline(file, line)) {
            istringstream iss(line);
            string key, value;
            if (getline(iss, key, '=') && getline(iss, value)) {
                if (key == "MaxFavorites") {
                    g_maxFavorites = stoi(value);
                    if (g_maxFavorites < 1) g_maxFavorites = 1;
                    if (g_maxFavorites > 10) g_maxFavorites = 10;
                }
                else if (key == "AutoShowOnNewTab") {
                    g_autoShowOnNewTab = (value == "1" || value == "true");
                }
                else if (key == "FavoriteIDs") {
                    istringstream idStream(value);
                    string idStr;
                    while (getline(idStream, idStr, ',')) {
                        if (!idStr.empty()) {
                            favoriteIDs.push_back(stoi(idStr));
                        }
                    }
                }
            }
        }
        file.close();

        if (allLanguages.empty()) {
            InitializeLanguages();
        }

        if (!favoriteIDs.empty()) {
            for (auto& lang : allLanguages) {
                lang.isFavorite = false;
                lang.favoriteTime = 0;
            }
            time_t currentTime = time(NULL);
            for (int id : favoriteIDs) {
                for (auto& lang : allLanguages) {
                    if (lang.id == id) {
                        lang.isFavorite = true;
                        lang.favoriteTime = currentTime--;
                        break;
                    }
                }
            }
            UpdateDisplayOrder();
        }
    }
    else {
        if (allLanguages.empty()) {
            InitializeLanguages();
        }
    }
}

void SaveConfiguration() {
    wstring configFile = GetConfigFilePath();
    ofstream file(configFile);

    if (file.is_open()) {
        file << "MaxFavorites=" << g_maxFavorites << endl;
        file << "AutoShowOnNewTab=" << (g_autoShowOnNewTab ? "1" : "0") << endl;
        file << "FavoriteIDs=";
        bool first = true;
        for (const auto& lang : allLanguages) {
            if (lang.isFavorite) {
                if (!first) file << ",";
                file << lang.id;
                first = false;
            }
        }
        file << endl;
        file.close();
    }
}

int CountFavorites() {
    int count = 0;
    for (const auto& lang : allLanguages) {
        if (lang.isFavorite) count++;
    }
    return count;
}

void PruneExcessFavorites() {
    int currentFavorites = CountFavorites();
    while (currentFavorites > g_maxFavorites) {
        int oldestIndex = FindOldestFavorite();
        if (oldestIndex != -1) {
            allLanguages[oldestIndex].isFavorite = false;
            allLanguages[oldestIndex].favoriteTime = 0;
            currentFavorites--;
        }
        else {
            break;
        }
    }
    UpdateDisplayOrder();
}

void ApplyLanguageToCurrentDoc(int langId) {
    int currentBufferId = static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0));
    ::SendMessage(nppData._nppHandle, NPPM_SETCURRENTLANGTYPE, (WPARAM)currentBufferId, (LPARAM)langId);
}

int FindDisplayIndexByName(const wstring& name) {
    for (size_t i = 0; i < displayOrder.size(); i++) {
        if (allLanguages[displayOrder[i]].displayName == name) {
            return static_cast<int>(i);
        }
    }
    return -1;
}


// -------------------------------------------------------------------------
// Main language dialog procedure
// -------------------------------------------------------------------------
INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    static HWND hCombo = NULL;
    static HWND hCheckFavorite = NULL;

    UINT dpi = GetDpiForWindow(hwnd);
    double dpiScale = dpi / 96.0;
    switch (msg) {
    case WM_CTLCOLORLISTBOX: {
        HWND hList = (HWND)lParam;
        if (hList == GetDlgItem(hwnd, IDC_COMBO_LANG) ||
            hList == (HWND)SendMessage(hCombo, CBEM_GETCOMBOCONTROL, 0, 0)) {
            SendMessage(hCombo, CB_SETTOPINDEX, 0, 0);
        }
        return (INT_PTR)GetSysColorBrush(COLOR_WINDOW);
    }

    case WM_INITDIALOG: {
        if (allLanguages.empty()) InitializeLanguages();
        UpdateDisplayOrder();

        hCombo = GetDlgItem(hwnd, IDC_COMBO_LANG);
        hCheckFavorite = GetDlgItem(hwnd, IDC_CHECK_FAVORITE);
              // making the 'favourite' check box font
        LOGFONT lfCheck = { 0 };
        HFONT hCheckFont = CreateFontIndirect(&lfCheck);
        SendMessage(hCheckFavorite, WM_SETFONT, (WPARAM)hCheckFont, TRUE);
        SetProp(hwnd, L"CheckFont", (HANDLE)hCheckFont);
        g_stripHeight = (int)(45 * dpiScale);
        // Create fonts
        NONCLIENTMETRICS ncm = { sizeof(ncm) };
        if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {
            if (hBoldFont) { DeleteObject(hBoldFont);  hBoldFont = NULL; }
            if (hComboFont) { DeleteObject(hComboFont); hComboFont = NULL; }

            LOGFONT lf = ncm.lfMessageFont;
            lf.lfHeight = (LONG)(lf.lfHeight * 1.2); //make dropdown menu text larger
            hComboFont = CreateFontIndirect(&lf);
            lf.lfWeight = FW_SEMIBOLD;
            hBoldFont = CreateFontIndirect(&lf);
          

        }
        SendMessage(hCombo, CB_SETMINVISIBLE, (WPARAM)10, 0);
        RepopulateComboBox(hCombo);

        // Set initial selection
        int firstSelectable = CountFavorites() > 0 ? 1 : 0;
        SendMessage(hCombo, CB_SETCURSEL, firstSelectable, 0);
        g_currentDisplayIndex = firstSelectable;

        if (!displayOrder.empty()) {
            LRESULT langID = SendMessage(hCombo, CB_GETITEMDATA, firstSelectable, 0);
            if (langID != HEADER_FAVORITES && langID != HEADER_ALL) {
                for (int i = 0; i < static_cast<int>(allLanguages.size()); i++) {
                    if (allLanguages[i].id == langID) {
                        g_currentLanguageName = allLanguages[i].displayName;
                        break;
                    }
                }
            }
        }

        if (g_currentDisplayIndex >= 0) {
            LRESULT langID = SendMessage(hCombo, CB_GETITEMDATA, g_currentDisplayIndex, 0);
            if (langID != HEADER_FAVORITES && langID != HEADER_ALL) {
                for (int i = 0; i < static_cast<int>(allLanguages.size()); i++) {
                    if (allLanguages[i].id == langID) {
                        SendMessage(hCheckFavorite, BM_SETCHECK,
                            allLanguages[i].isFavorite ? BST_CHECKED : BST_UNCHECKED, 0);
                        break;
                    }
                }
            }
        }


        if (hCheckFont) {
            DeleteObject(hCheckFont);
            RemoveProp(hwnd, L"CheckFont");
        }
        return TRUE;
    }

    case WM_MEASUREITEM: {
        MEASUREITEMSTRUCT* pItemMetrics = (MEASUREITEMSTRUCT*)lParam;
        if (pItemMetrics->CtlID == IDC_COMBO_LANG) {
            LRESULT itemData = SendMessage(GetDlgItem(hwnd, IDC_COMBO_LANG),
                CB_GETITEMDATA, pItemMetrics->itemID, 0);
            bool isFavorite = false;
            for (const auto& lang : allLanguages) {
                if (lang.id == (int)itemData && lang.isFavorite) {
                    isFavorite = true;
                    break;
                }
            }
            pItemMetrics->itemHeight = (int)(25 * dpiScale); // height of the item element (linespacing)
            return TRUE;
        }
        break;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        WORD code = HIWORD(wParam);

        if (id == IDC_COMBO_LANG && code == CBN_DROPDOWN) {
            PostMessage(hwnd, WM_APP + 1, 0, 0);   //This condition ensures that when the list drops down, the header is at the top, not scrolled down.
            return TRUE;
        }

        if (id == IDC_COMBO_LANG && code == CBN_SELCHANGE) {
            int newDisplayIndex = static_cast<int>(SendMessage(hCombo, CB_GETCURSEL, 0, 0));
            LRESULT itemData = SendMessage(hCombo, CB_GETITEMDATA, newDisplayIndex, 0);

            if (itemData == HEADER_FAVORITES || itemData == HEADER_ALL) {
                int count = static_cast<int>(SendMessage(hCombo, CB_GETCOUNT, 0, 0));
                int nextIndex = newDisplayIndex + 1;
                while (nextIndex < count) {
                    LRESULT nextData = SendMessage(hCombo, CB_GETITEMDATA, nextIndex, 0);
                    if (nextData != HEADER_FAVORITES && nextData != HEADER_ALL) {
                        SendMessage(hCombo, CB_SETCURSEL, nextIndex, 0);
                        newDisplayIndex = nextIndex;
                        itemData = nextData;
                        break;
                    }
                    nextIndex++;
                }
            }

            if (itemData != HEADER_FAVORITES && itemData != HEADER_ALL) {
                g_currentDisplayIndex = newDisplayIndex;
                for (int i = 0; i < static_cast<int>(allLanguages.size()); i++) {
                    if (allLanguages[i].id == itemData) {
                        g_currentLanguageName = allLanguages[i].displayName;
                        SendMessage(hCheckFavorite, BM_SETCHECK,
                            allLanguages[i].isFavorite ? BST_CHECKED : BST_UNCHECKED, 0);
                        break;
                    }
                }
            }
            return TRUE;
        }

        if (id == IDC_CHECK_FAVORITE && code == BN_CLICKED) {
            if (g_currentDisplayIndex >= 0 && !g_currentLanguageName.empty()) {
                int actualIndex = -1;
                for (int i = 0; i < static_cast<int>(allLanguages.size()); i++) {
                    if (allLanguages[i].displayName == g_currentLanguageName) {
                        actualIndex = i;
                        break;
                    }
                }
                if (actualIndex != -1) {
                    bool wasFavorite = allLanguages[actualIndex].isFavorite;
                    if (!wasFavorite) {
                        int currentFavorites = CountFavorites();
                        if (currentFavorites >= g_maxFavorites) {
                            int oldestIndex = FindOldestFavorite();
                            if (oldestIndex != -1) {
                                allLanguages[oldestIndex].isFavorite = false;
                                allLanguages[oldestIndex].favoriteTime = 0;
                            }
                        }
                        allLanguages[actualIndex].isFavorite = true;
                        allLanguages[actualIndex].favoriteTime = time(NULL);
                    }
                    else {
                        allLanguages[actualIndex].isFavorite = false;
                        allLanguages[actualIndex].favoriteTime = 0;
                    }
                    UpdateDisplayOrder();
                    RepopulateComboBox(hCombo);
                    SendMessage(hCheckFavorite, BM_SETCHECK,
                        allLanguages[actualIndex].isFavorite ? BST_CHECKED : BST_UNCHECKED, 0);
                }
            }
            return TRUE;
        }

        if (id == IDOK) {
            if (hCombo && g_currentDisplayIndex >= 0) {
                LRESULT langID = SendMessage(hCombo, CB_GETITEMDATA, g_currentDisplayIndex, 0);
                if (langID != HEADER_FAVORITES && langID != HEADER_ALL) {
                    ApplyLanguageToCurrentDoc(static_cast<int>(langID));
                }
            }
            SaveConfiguration();
            DestroyWindow(hwnd);
            return TRUE;
        }

        if (id == IDCANCEL) {
            DestroyWindow(hwnd);
            return TRUE;
        }
        break;
    }

    case WM_APP + 1: {
        SendMessage(hCombo, CB_SETTOPINDEX, 0, 0);
        return TRUE;
    }

    case WM_DRAWITEM: {
        LPDRAWITEMSTRUCT dis = (LPDRAWITEMSTRUCT)lParam;
        if (dis->CtlID == IDC_COMBO_LANG) {
            if (dis->itemID == (UINT)-1) return FALSE;

            wchar_t buffer[256];
            SendMessage(dis->hwndItem, CB_GETLBTEXT, dis->itemID, (LPARAM)buffer);
            LRESULT itemData = SendMessage(dis->hwndItem, CB_GETITEMDATA, dis->itemID, 0);

            bool isHeader = (itemData == HEADER_FAVORITES || itemData == HEADER_ALL);

            HBRUSH hBrush;
            if (isHeader) {
                hBrush = CreateSolidBrush(RGB(240, 240, 240));
                SetTextColor(dis->hDC, RGB(100, 100, 100));
            }
            else if (dis->itemState & ODS_SELECTED) {
                hBrush = CreateSolidBrush(GetSysColor(COLOR_HIGHLIGHT));
                SetTextColor(dis->hDC, GetSysColor(COLOR_HIGHLIGHTTEXT));
            }
            else {
                hBrush = CreateSolidBrush(GetSysColor(COLOR_WINDOW));
                SetTextColor(dis->hDC, GetSysColor(COLOR_WINDOWTEXT));
            }
            FillRect(dis->hDC, &dis->rcItem, hBrush);
            DeleteObject(hBrush);

            HFONT hOldFont = NULL;
            if (isHeader) {
                LOGFONT lf = { 0 };
                GetObject(hComboFont, sizeof(LOGFONT), &lf);  // base it on hComboFont
                lf.lfHeight = (LONG)(lf.lfHeight * 0.8);     // slightly smaller header font size
                lf.lfItalic = TRUE;
                HFONT hHeaderFont = CreateFontIndirect(&lf);
                hOldFont = (HFONT)SelectObject(dis->hDC, hHeaderFont);
            }
            else {
                bool isFavorite = false;
                for (const auto& lang : allLanguages) {
                    if (lang.id == itemData && lang.isFavorite) {
                        isFavorite = true;
                        break;
                    }

                }
                hOldFont = (HFONT)SelectObject(dis->hDC,
                    (isFavorite && hBoldFont) ? hBoldFont : hComboFont);

            }

            SetBkMode(dis->hDC, TRANSPARENT);
            RECT textRect = dis->rcItem;
            textRect.left += 5;
            DrawTextW(dis->hDC, buffer, -1, &textRect,
                DT_LEFT | DT_VCENTER | DT_SINGLELINE);

            if (isHeader) {
                HFONT hHeaderFont = (HFONT)SelectObject(dis->hDC, hOldFont);
                DeleteObject(hHeaderFont);
            }
            else if (hOldFont) {
                SelectObject(dis->hDC, hOldFont);
            }

            if ((dis->itemState & ODS_FOCUS) && !isHeader) {
                DrawFocusRect(dis->hDC, &dis->rcItem);
            }
            return TRUE;
        }
        break;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // Draw grey strip at the bottom
        RECT rcStrip = rcClient;
        rcStrip.top = rcStrip.bottom - g_stripHeight;

        HBRUSH hBrush = CreateSolidBrush(RGB(230, 230, 230));
        FillRect(hdc, &rcStrip, hBrush);
        DeleteObject(hBrush);

        // Draw a subtle top border line on the strip
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, rcStrip.left, rcStrip.top, NULL);
        LineTo(hdc, rcStrip.right, rcStrip.top);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        EndPaint(hwnd, &ps);
        return FALSE;  // FALSE lets Windows paint controls on top
    }
    case WM_CTLCOLORDLG: {
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;

        SetBkColor(hdc, RGB(255, 255, 255));
        return (INT_PTR)GetStockObject(WHITE_BRUSH);

        
    }
    case WM_CLOSE:
        DestroyWindow(hwnd);
        return TRUE;
    }

    return FALSE;
}
// -------------------------------------------------------------------------
// Settings dialog procedure
// -------------------------------------------------------------------------
INT_PTR CALLBACK SettingsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM) {
    static HWND hLimitCombo = NULL;
    static HWND hAutoShowCheck = NULL;
    static int  originalLimit = g_maxFavorites;
    static bool originalAutoShow = g_autoShowOnNewTab;

    switch (msg) {
    case WM_INITDIALOG: {
        hLimitCombo = GetDlgItem(hwnd, IDC_LIMIT_COMBO);
        hAutoShowCheck = GetDlgItem(hwnd, IDC_AUTOSHOW_CHECK);
        UINT dpi = GetDpiForWindow(hwnd);
        double dpiScale = dpi / 96.0;
        g_stripHeight = (int)(45 * dpiScale);
        for (int i = 1; i <= 10; i++) {
            wstring limitStr = to_wstring(i);
            SendMessage(hLimitCombo, CB_ADDSTRING, 0, (LPARAM)limitStr.c_str());
            SendMessage(hLimitCombo, CB_SETITEMDATA, i - 1, (LPARAM)i);
        }
        SendMessage(hLimitCombo, CB_SETCURSEL, g_maxFavorites - 1, 0);
        SendMessage(hAutoShowCheck, BM_SETCHECK,
            g_autoShowOnNewTab ? BST_CHECKED : BST_UNCHECKED, 0);

        originalLimit = g_maxFavorites;
        originalAutoShow = g_autoShowOnNewTab;
        return TRUE;
    }

    case WM_COMMAND: {
        WORD id = LOWORD(wParam);
        if (id == IDC_APPLY_BTN || id == IDOK) {
            int newLimit = static_cast<int>(SendMessage(hLimitCombo, CB_GETITEMDATA,
                (WPARAM)SendMessage(hLimitCombo, CB_GETCURSEL, 0, 0), 0));
            bool newAutoShow = (SendMessage(hAutoShowCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);
            int oldLimit = g_maxFavorites;
            g_maxFavorites = newLimit;
            g_autoShowOnNewTab = newAutoShow;
            if (newLimit < oldLimit && !allLanguages.empty()) {
                PruneExcessFavorites();
            }
            SaveConfiguration();
            DestroyWindow(hwnd);
        }
        else if (id == IDCANCEL) {
            g_maxFavorites = originalLimit;
            g_autoShowOnNewTab = originalAutoShow;
            DestroyWindow(hwnd);
        }
        return TRUE;
    }
    case WM_PAINT: {
        PAINTSTRUCT ps;
        HDC hdc = BeginPaint(hwnd, &ps);
        RECT rcClient;
        GetClientRect(hwnd, &rcClient);

        // Draw grey strip at the bottom
        RECT rcStrip = rcClient;
        rcStrip.top = rcStrip.bottom - g_stripHeight;

        HBRUSH hBrush = CreateSolidBrush(RGB(230, 230, 230));
        FillRect(hdc, &rcStrip, hBrush);
        DeleteObject(hBrush);

        // Draw a subtle top border line on the strip
        HPEN hPen = CreatePen(PS_SOLID, 1, RGB(210, 210, 210));
        HPEN hOldPen = (HPEN)SelectObject(hdc, hPen);
        MoveToEx(hdc, rcStrip.left, rcStrip.top, NULL);
        LineTo(hdc, rcStrip.right, rcStrip.top);
        SelectObject(hdc, hOldPen);
        DeleteObject(hPen);

        EndPaint(hwnd, &ps);
        return FALSE;  // FALSE lets Windows paint controls on top
    }
    case WM_CTLCOLORDLG: {
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }
    case WM_CTLCOLORSTATIC: {
        HDC hdc = (HDC)wParam;
        SetBkColor(hdc, RGB(255, 255, 255));
        return (INT_PTR)GetStockObject(WHITE_BRUSH);
    }
    }

    return FALSE;
}

// -------------------------------------------------------------------------
// Show dialogs
// -------------------------------------------------------------------------
void ShowLanguageDialog() {
    if (allLanguages.empty()) InitializeLanguages();
    HWND hDlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_LANGUAGE_DIALOG),
        nppData._nppHandle, DialogProc, 0);
    if (!hDlg) return;
    ShowWindow(hDlg, SW_SHOW);
    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

void ShowSettingsDialog() {
    HWND hDlg = CreateDialogParam(g_hInstance, MAKEINTRESOURCE(IDD_SETTINGS_DIALOG),
        nppData._nppHandle, SettingsDialogProc, 0);
    if (!hDlg) return;
    ShowWindow(hDlg, SW_SHOW);
    MSG msg;
    while (IsWindow(hDlg) && GetMessage(&msg, NULL, 0, 0)) {
        if (!IsDialogMessage(hDlg, &msg)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    }
}

HRESULT CALLBACK TaskDialogCallbackProc(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData) {
    (void)hwnd; (void)wParam; (void)dwRefData;
    if (uNotification == TDN_HYPERLINK_CLICKED) {
        std::wstring link = (LPCWSTR)lParam;
        if (link == L"website") {
            ShellExecute(NULL, L"open", L"https://github.com/hassaine-abdellah/npp-language-selector/", NULL, NULL, SW_SHOWNORMAL);
        }
        else if (link == L"email") {
            ShellExecute(NULL, L"open", L"mailto:hassaine_abdellah@univ-blida.dz", NULL, NULL, SW_SHOWNORMAL);
        }
    }
    return S_OK;
}

void About() {
    TASKDIALOGCONFIG config = { 0 };
    config.cbSize = sizeof(TASKDIALOGCONFIG);
    config.hwndParent = nppData._nppHandle;
    config.dwFlags = TDF_ENABLE_HYPERLINKS | TDF_ALLOW_DIALOG_CANCELLATION;
    config.pszWindowTitle = L"About Language Selector";
    config.pszMainInstruction = L"Language Selector Plugin";
   config.pszContent = 
    L"Version: 1.2\n"
    L"Copyright Abdellah Hassaine, May 2026\n\n"
    L"Website: <a href=\"website\">https://github.com/hassaine-abdellah/npp-language-selector/</a>\n"
    L"Contact: <a href=\"email\">hassaine_abdellah@univ-blida.dz</a>\n\n"
    L"Description: This plug-in allows you to quickly select the programming language for the current tab, "
    L"and by consequence the corresponding syntax highlighting.\n\n"
    L"Features:\n"
    L"• Auto-show the selection dialogue on new tabs from a drop-down list containing all the supported languages.\n"
    L"• Pin your favourite languages on top of the list for quick access.\n"
    L"• Configurable favourite list size (when favourites are full, the oldest one is removed).\n\n"
    L"The author welcomes your suggestions to improve the plugin.\n";config.pfCallback = TaskDialogCallbackProc;
    config.dwCommonButtons = TDCBF_OK_BUTTON;
    TaskDialogIndirect(&config, NULL, NULL, NULL);
}

// -------------------------------------------------------------------------
// Notepad++ plugin exports
// -------------------------------------------------------------------------

LRESULT CALLBACK NppSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam) {
    if (msg == WM_APP + 100) {
        SetTimer(hwnd, 9001, 300, NULL);  // this is a small trick to impose a 300ms delay before the ShowLanguageDialog() is fired, in order to give Notepad++ enough time to fully render before selection dialogue appears. Without it, the dialogue would show without the full appearence of Notepad++ GUI elements.
        return 0;
    }
    if (msg == WM_TIMER && wParam == 9001) {
        KillTimer(hwnd, 9001);
        ShowLanguageDialog();
        return 0;
    }
    return CallWindowProc(g_originalNppProc, hwnd, msg, wParam, lParam);
}
extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) {
    nppData = notpadPlusData;
    g_originalNppProc = (WNDPROC)SetWindowLongPtr(nppData._nppHandle,
        GWLP_WNDPROC, (LONG_PTR)NppSubclassProc);
    commandMenuInit();
    LoadConfiguration();
}

extern "C" __declspec(dllexport) const TCHAR * getName() {
    return TEXT("Language Selector");
}

extern "C" __declspec(dllexport) FuncItem * getFuncsArray(int* nbF) {
    *nbF = nbFunc;
    return funcItem;
}

extern "C" __declspec(dllexport) void beNotified(SCNotification * notifyCode) {
    static bool nppReady = false;
    if (!notifyCode) return;

    switch (notifyCode->nmhdr.code) {

    case NPPN_READY:
        nppReady = true;
        g_isShuttingDown = false;
        g_shownBuffers.clear();
        if (g_autoShowOnNewTab && !g_isShuttingDown) { // A condition for when the settings checkbox 'show on new tabs' is ticked AND the app is not shutting down (because Notepad++ reactivates in some sort all the tabs when it is shutsdown, and so when you close it it mimics the opening of all new tabs, so I had to exclude this case)
            int currentBufferId = static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0));
            wchar_t fileExt[MAX_PATH] = { 0 };
            ::SendMessage(nppData._nppHandle, NPPM_GETEXTPART, MAX_PATH, (LPARAM)fileExt);
            if (wcslen(fileExt) == 0) { // This line is to ensure that the automatic display of dialogue is only for new tabs (whose extension part has 0 length)
                g_shownBuffers.push_back(currentBufferId);
                // ShowLanguageDialog();
                PostMessage(nppData._nppHandle, WM_APP + 100, 0, 0);
            }
            else {
                g_shownBuffers.push_back(currentBufferId);
            }
        }
        return;

    case NPPN_BEFORESHUTDOWN:
        g_isShuttingDown = true;
        nppReady = false;
        return;
    }

    if (g_isShuttingDown) return;
    if (!nppReady) return;
    if (!g_autoShowOnNewTab) return;

    if (notifyCode->nmhdr.code == NPPN_BUFFERACTIVATED) {
        int bufferId = static_cast<int>(::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0));
        for (int id : g_shownBuffers) {
            if (id == bufferId) return;
        }
        wchar_t fileExt[MAX_PATH] = { 0 };
        ::SendMessage(nppData._nppHandle, NPPM_GETEXTPART, MAX_PATH, (LPARAM)fileExt);
        if (wcslen(fileExt) > 0) {
            g_shownBuffers.push_back(bufferId);
            return;
        }
        g_shownBuffers.push_back(bufferId);
        ShowLanguageDialog();
    }

    if (notifyCode->nmhdr.code == NPPN_FILEBEFORECLOSE) {
        int closingId = static_cast<int>(notifyCode->nmhdr.idFrom);
        g_shownBuffers.erase(
            std::remove(g_shownBuffers.begin(), g_shownBuffers.end(), closingId),
            g_shownBuffers.end());
    }
}

extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(HANDLE hModule, DWORD reason, LPVOID /*lpReserved*/) {
    switch (reason) {
    case DLL_PROCESS_ATTACH:
        g_hInstance = (HINSTANCE)hModule;
        LoadConfiguration();
        break;
    }
    return TRUE;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/, WPARAM /*wParam*/, LPARAM /*lParam*/) {
    return 0;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {
    return TRUE;
}
#endif
