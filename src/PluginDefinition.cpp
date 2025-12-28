//This file is part of Language Selector.
//Copyright (C) 2025 Abdellah Hassaine.
#define IDC_COMBO_LANG     1001        // ID for language combo box control
#define IDC_STATIC_LABEL   1002        // ID for static text label
#define IDC_CHECK_FAVORITE 1003        // ID for favorite checkbox
#define IDC_FAV_LIMIT_COMBO 1006       // ID for favorite limit COMBO BOX (changed from EDIT)
#define IDC_AUTO_SHOW_CHECK 1007       // ID for auto-show checkbox
#define IDC_APPLY_BTN      1008        // ID for Apply button
#define IDC_CANCEL_BTN     1009        // ID for Cancel button
#include "PluginDefinition.h"           // Notepad++ plugin definitions
#include "menuCmdID.h"                  // Notepad++ menu command IDs
#include <windows.h>                    // Windows API headers
#include <string>                       // C++ string library
#include <vector>                       // C++ vector container
#include <algorithm>                    // C++ algorithms (sort, remove)
#include <ctime>                        // C++ time functions (to implement the FIFO principle in favorites)
#include <fstream>                      // C++ file stream operations
#include <sstream>                      // C++ string stream operations
#include <commctrl.h>                   // Windows common controls

#pragma comment(lib, "comctl32.lib")    // Link with common controls library


using namespace std;                    // Use standard namespace
#define nbFunc 3                        // Number of plugin menu commands
FuncItem funcItem[nbFunc];              // Array of plugin function items

HINSTANCE g_hInstance = NULL;           // DLL instance handle
NppData nppData;                        // Notepad++ data structure

static std::vector<int> g_shownBuffers; // Track buffers where selection dialog was shown

static bool g_isShuttingDown = false;
#define CONFIG_FILE L"LanguageSelector.ini"  // Configuration filename

#define DEFAULT_FAV_LIMIT 5             // Default maximum favorites count
#define DEFAULT_AUTO_SHOW true          // Default auto-show setting

int g_maxFavorites = DEFAULT_FAV_LIMIT; // Current maximum favorites limit
bool g_autoShowOnNewTab = DEFAULT_AUTO_SHOW; // Current auto-show setting

wstring GetConfigFilePath() {
	wchar_t configPath[MAX_PATH] = { 0 };
	::SendMessage(nppData._nppHandle, NPPM_GETPLUGINSCONFIGDIR, MAX_PATH, (LPARAM)configPath);

	wstring path(configPath);
	path += L"\\";
	path += CONFIG_FILE;
	return path;
}




void commandMenuInit()                  // Initialize plugin menu commands
{
	setCommand(0, TEXT("Select Language"), ShowLanguageDialog, NULL, false);  // Adds "Select Language" menu item
	setCommand(1, TEXT("Settings"), ShowSettingsDialog, NULL, false);         // Adds "Settings" menu item
	setCommand(2, TEXT("About"), About, NULL, false);         // Adds "About" menu item
}

bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk, bool check0nInit)  // Helper to set menu commands
{
	if (index >= nbFunc)                // Validate index bounds
		return false;

	if (!pFunc)                         // Validate function pointer
		return false;

	lstrcpy(funcItem[index]._itemName, cmdName);  // Copy command name
	funcItem[index]._pFunc = pFunc;     // Store function pointer
	funcItem[index]._init2Check = check0nInit;  // Store check state
	funcItem[index]._pShKey = sk;       // Store shortcut key

	return true;                        // Success
}

struct Language {                       // Structure representing a language
	wstring displayName;                // Language display name
	int id;                             // Notepad++ language ID
	bool isFavorite;                    // Favorite status flag
	time_t favoriteTime;                // Time when added as favorite

	Language(const wchar_t* name, int langId, bool favorite = false)  // Simplified constructor
		: displayName(name), id(langId), isFavorite(favorite), favoriteTime(0) {
		if (favorite) {                 // If marked as favorite
			favoriteTime = time(NULL);  // Set current time as favorite time
		}
	}


	wstring GetDisplayName() const {    // Get display name with star for favorites
		if (isFavorite) {               // If language is a favorite
			return L"★ " + displayName; // Return name with star prefix
		}
		return displayName;             // Return name
	}
};

vector<Language> allLanguages;          // All available languages
vector<int> displayOrder;               // Indices for display order (favorites first)
void InitializeLanguages() {            // Initialize language list with defaults
	allLanguages.clear();               // Clear existing languages
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
int FindOldestFavorite() {              // Find oldest favorite by timestamp
	time_t oldestTime = time(NULL);     // Initialize with current time
	int oldestIndex = -1;               // Default: no favorite found

	for (int i = 0; i < (int)allLanguages.size(); i++) {  // Iterate through all languages
		if (allLanguages[i].isFavorite && allLanguages[i].favoriteTime <= oldestTime) {  // Check if favorite and older
			oldestTime = allLanguages[i].favoriteTime;  // Update oldest time
			oldestIndex = i;             // Update oldest index
		}
	}
	return oldestIndex;                  // Return index of oldest favorite
}

void UpdateDisplayOrder() {             // Update display order with favorites first
	displayOrder.clear();               // Clear current display order

	vector<int> favoriteIndices;        // Store indices of favorite languages
	vector<int> regularIndices;         // Store indices of regular languages

	for (int i = 0; i < (int)allLanguages.size(); i++) {  // Categorize languages
		if (allLanguages[i].isFavorite) {  // If language is favorite
			favoriteIndices.push_back(i);  // Add to favorites list
		}
		else {                           // If language is not favorite
			regularIndices.push_back(i); // Add to regular list
		}
	}

	sort(favoriteIndices.begin(), favoriteIndices.end(),  // Sort favorites
		[&](int a, int b) {             // Lambda comparator
		if (allLanguages[a].favoriteTime != allLanguages[b].favoriteTime) {  // If favorite times differ
			return allLanguages[a].favoriteTime > allLanguages[b].favoriteTime; // Newest first
		}
		return allLanguages[a].displayName < allLanguages[b].displayName;  // Alphabetical tie-breaker
	});

	sort(regularIndices.begin(), regularIndices.end(),  // Sort regular languages alphabetically
		[&](int a, int b) {
		return allLanguages[a].displayName < allLanguages[b].displayName;  // Alphabetical order
	});

	displayOrder.insert(displayOrder.end(), favoriteIndices.begin(), favoriteIndices.end());  // Add favorites first
	displayOrder.insert(displayOrder.end(), regularIndices.begin(), regularIndices.end());    // Add regular languages
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
					if (g_maxFavorites < 1) g_maxFavorites = 1; //just to make sure no one misses with the configuration file manually
					if (g_maxFavorites > 10) g_maxFavorites = 10;
				}
				else if (key == "AutoShowOnNewTab") {
					g_autoShowOnNewTab = (value == "1" || value == "true");
				}
				else if (key == "FavoriteIDs") {
					// Parse comma-separated favorite IDs
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

		// Initialize languages
		if (allLanguages.empty()) {
			InitializeLanguages();
		}

		// Apply saved favorites
		if (!favoriteIDs.empty()) {
			// First, clear all favorites
			for (auto& lang : allLanguages) {
				lang.isFavorite = false;
				lang.favoriteTime = 0;
			}

			// Then set the saved favorites with current timestamp
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
		// No config file - use defaults
		if (allLanguages.empty()) {
			InitializeLanguages();
		}
	}
}

void SaveConfiguration() {
	wstring configFile = GetConfigFilePath();
	ofstream file(configFile);

	if (file.is_open()) {
		// Save basic settings
		file << "MaxFavorites=" << g_maxFavorites << endl;
		file << "AutoShowOnNewTab=" << (g_autoShowOnNewTab ? "1" : "0") << endl;

		// Save favorite language IDs
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
int CountFavorites() {                  // Count number of favorite languages
	int count = 0;                      // Initialize counter
	for (const auto& lang : allLanguages) {  // Iterate through languages
		if (lang.isFavorite) {          // If language is favorite
			count++;                     // Increment counter
		}
	}
	return count;                       // Return favorite count
}

void PruneExcessFavorites() {           // Remove excess favorites when limit reduced
	int currentFavorites = CountFavorites();  // Get current favorite count

	while (currentFavorites > g_maxFavorites) {  // While over limit
		int oldestIndex = FindOldestFavorite();  // Find oldest favorite
		if (oldestIndex != -1) {        // If found
			allLanguages[oldestIndex].isFavorite = false;  // Remove favorite status
			allLanguages[oldestIndex].favoriteTime = 0;     // Clear favorite time
			currentFavorites--;          // Decrement count
		}
		else {                           // No favorite found (shouldn't happen)
			break;                       // Safety break
		}
	}

	UpdateDisplayOrder();                // Update display order after pruning
}


void ApplyLanguageToCurrentDoc(int langId) {  // Apply language to current document
	int currentBufferId = (int)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);  // Get current buffer ID
	::SendMessage(nppData._nppHandle, NPPM_SETCURRENTLANGTYPE,  // Set language type
		(WPARAM)currentBufferId, (LPARAM)langId);  // For current buffer
}


 int FindDisplayIndexByName(const wstring& name) {  // Find display index by language name
	 for (size_t i = 0; i < displayOrder.size(); i++) {
		 if (allLanguages[displayOrder[i]].displayName == name) {
			 return (int)i;
		 }
	 }
	 return -1;
 }
HWND CreateButton(HWND hParent, const wchar_t* text, int x, int y, int width, int height, int id) {  // Create button control
	return CreateWindow(                // Create Windows button
		L"BUTTON",                      // Button class
		text,                           // Button text
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_PUSHBUTTON,  // Window styles
		x, y, width, height,            // Position and size
		hParent,                        // Parent window
		(HMENU)(INT_PTR)id,             // Control ID
		g_hInstance,                    // Instance handle
		NULL);                          // No extra data
}

HWND CreateCheckbox(HWND hParent, const wchar_t* text, int x, int y, int width, int height, int id) {  // Create checkbox control
	return CreateWindow(                // Create Windows checkbox
		L"BUTTON",                      // Button class (used for checkboxes)
		text,                           // Checkbox text
		WS_CHILD | WS_VISIBLE | WS_TABSTOP | BS_AUTOCHECKBOX,  // Window styles
		x, y, width, height,            // Position and size
		hParent,                        // Parent window
		(HMENU)(INT_PTR)id,             // Control ID
		g_hInstance,                    // Instance handle
		NULL);                          // No extra data
}

INT_PTR CALLBACK SettingsDialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM) {  // Settings dialog procedure
	static HWND hLimitCombo = NULL;    // Combo box for favorite limit (changed from hLimitEdit)
	static HWND hAutoShowCheck = NULL;  // Checkbox for auto-show setting
	static int originalLimit = g_maxFavorites;  // Store original limit for cancel
	static bool originalAutoShow = g_autoShowOnNewTab;  // Store original auto-show for cancel

	switch (msg) {                      // Handle window messages
	case WM_INITDIALOG: {               // Dialog initialization
		HFONT hFont = NULL;                  // Custom font
		NONCLIENTMETRICS ncm = { sizeof(ncm) };  // System metrics
		if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {  // Get metrics
			hFont = CreateFontIndirect(&ncm.lfMessageFont);  // Create system font
		}

		HWND hLimitLabel = CreateWindow(L"STATIC", L"Maximum favorite items:",  // Create label
			WS_CHILD | WS_VISIBLE | SS_LEFT,  // Window styles
			20, 15, 200, 25,            // Position and size
			hwnd, NULL, g_hInstance, NULL);  // Parent and instance
		 SendMessage(hLimitLabel, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font if available

		// Create combo box instead of edit control
		hLimitCombo = CreateWindowEx(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",  // Create combo box
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS,  // Window styles
			180, 12, 50, 200,           // Position and size
			hwnd, (HMENU)IDC_FAV_LIMIT_COMBO, g_hInstance, NULL);  // Control ID
		 SendMessage(hLimitCombo, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		// Populate combo box with numbers 1-10
		for (int i = 1; i <= 10; i++) {
			wstring limitStr = to_wstring(i);
			SendMessage(hLimitCombo, CB_ADDSTRING, 0, (LPARAM)limitStr.c_str());
			SendMessage(hLimitCombo, CB_SETITEMDATA, i - 1, (LPARAM)i);  // Store the value
		}

		// Select current limit
		int selectIndex = g_maxFavorites - 1;  // Convert to 0-based index
		SendMessage(hLimitCombo, CB_SETCURSEL, selectIndex, 0);


		hAutoShowCheck = CreateCheckbox(hwnd, L"Shows automatically on new tabs",  // Create checkbox
			20, 45, 250, 25, IDC_AUTO_SHOW_CHECK);  // Position, size, ID
		 SendMessage(hAutoShowCheck, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		SendMessage(hAutoShowCheck, BM_SETCHECK,  // Set checkbox state
			g_autoShowOnNewTab ? BST_CHECKED : BST_UNCHECKED, 0);  // Based on setting

		HWND hApplyBtn = CreateButton(hwnd, L"Apply",  // Create Apply button
			50, 80, 80, 32, IDC_APPLY_BTN);  // Position, size, ID
		 SendMessage(hApplyBtn, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		HWND hCancelBtn = CreateButton(hwnd, L"Cancel",  // Create Cancel button
			170, 80, 80, 32, IDC_CANCEL_BTN);  // Position, size, ID
		 SendMessage(hCancelBtn, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		originalLimit = g_maxFavorites;   // Store original limit
		originalAutoShow = g_autoShowOnNewTab;  // Store original auto-show

		RECT rc;                         // Dialog rectangle
		GetWindowRect(hwnd, &rc);        // Get dialog bounds
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);  // Get screen width
		int screenHeight = GetSystemMetrics(SM_CYSCREEN); // Get screen height
		int dialogWidth = rc.right - rc.left;  // Calculate dialog width
		int dialogHeight = rc.bottom - rc.top; // Calculate dialog height
		int x = (screenWidth - dialogWidth) / 2;  // Center X position
		int y = (screenHeight - dialogHeight) / 2; // Center Y position
		SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);  // Center dialog


		return TRUE;                     // Initialization successful
	}

	case WM_COMMAND: {                   // Handle command messages
		WORD id = LOWORD(wParam);        // Get control ID

		if (id == IDC_APPLY_BTN || id == IDOK) {       // Apply button clicked, or, enter is pressed
			// Get selected limit from combo box
			int newLimit = (int)SendMessage(hLimitCombo, CB_GETITEMDATA,
				(WPARAM)SendMessage(hLimitCombo, CB_GETCURSEL, 0, 0), 0);
			bool newAutoShow = (SendMessage(hAutoShowCheck, BM_GETCHECK, 0, 0) == BST_CHECKED);  // Get checkbox state

			int oldLimit = g_maxFavorites;        // Store old limit

			g_maxFavorites = newLimit;            // Update limit
			g_autoShowOnNewTab = newAutoShow;     // Update auto-show setting

			if (newLimit < oldLimit && !allLanguages.empty()) {  // If limit reduced
				PruneExcessFavorites();            // Remove excess favorites
			}

			SaveConfiguration();
			EndDialog(hwnd, 1);                   // Close dialog with success
		}
		else if (id == IDCANCEL || id == IDC_CANCEL_BTN) { // Cancel clicked, or Escape pressed
		
			g_maxFavorites = originalLimit;   // Restore original limit
			g_autoShowOnNewTab = originalAutoShow;  // Restore original auto-show
			EndDialog(hwnd, 0);               // Close dialog with cancel
		}
		return TRUE;                         // Message handled
		break;                                  // Break from switch
	}

	case WM_DESTROY: {                         // Window being destroyed

		break;                                  // Break from switch
	}
	}

	return FALSE;                             // Message not handled
}

INT_PTR CALLBACK DialogProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM) {  // Language dialog procedure
	static HWND hCombo = NULL;               // Combo box control
	static HWND hCheckFavorite = NULL;       // Favorite checkbox
	static int currentDisplayIndex = -1;     // Current selected display index
	static wstring currentLanguageName;      // Current language name

	switch (msg) {                           // Handle window messages
	case WM_INITDIALOG: {                    // Dialog initialization

		if (allLanguages.empty()) {          // If languages not initialized
			InitializeLanguages();            // Initialize languages
		}

		UpdateDisplayOrder();                 // Update display order

		SetWindowText(hwnd, L"Select Language");  // Set dialog title
		HFONT hFont = NULL;                  // Custom font
		NONCLIENTMETRICS ncm = { sizeof(ncm) };  // System metrics
		if (SystemParametersInfo(SPI_GETNONCLIENTMETRICS, sizeof(ncm), &ncm, 0)) {  // Get metrics
			hFont = CreateFontIndirect(&ncm.lfMessageFont);  // Create system font
		}


		wstring labelText = L"Select language for current tab:";  // Label text
		HWND hLabel = CreateWindow(L"STATIC", labelText.c_str(),  // Create label
			WS_CHILD | WS_VISIBLE | SS_LEFT,  // Window styles
			20, 20, 400, 25,            // Position and size
			hwnd, (HMENU)IDC_STATIC_LABEL, g_hInstance, NULL);  // Parent and ID
		 SendMessage(hLabel, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		hCombo = CreateWindowEx(WS_EX_CLIENTEDGE, L"COMBOBOX", L"",  // Create combo box
			WS_CHILD | WS_VISIBLE | WS_TABSTOP | CBS_DROPDOWNLIST | CBS_HASSTRINGS | WS_VSCROLL,  // Styles
			20, 50, 150, 100,           // Position and size
			hwnd, (HMENU)IDC_COMBO_LANG, g_hInstance, NULL);  // Parent and ID

		 SendMessage(hCombo, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font
		 SendMessage(hCombo, CB_SETMINVISIBLE, (WPARAM)10, 0);
		int separatorAdded = -1;            // Track if separator was added
		int favoriteCount = CountFavorites();  // Get favorite count

		for (int displayIdx = 0; displayIdx < (int)displayOrder.size(); displayIdx++) {  // Populate combo box
			int actualIdx = displayOrder[displayIdx];  // Get actual index
			Language& lang = allLanguages[actualIdx];  // Get language reference

			if (!lang.isFavorite && favoriteCount > 0 && displayIdx == favoriteCount) {  // Add separator after favorites
				SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"────────────");  // Add separator string
				int separatorIndex = displayIdx;  // Store separator index
				SendMessage(hCombo, CB_SETITEMDATA, separatorIndex, (LPARAM)-10);  // Mark as separator
				separatorAdded = separatorIndex;  // Update separator flag
			}

			wstring displayName = lang.GetDisplayName();  // Get display name
			LRESULT itemIndex = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)displayName.c_str());  // Add to combo

			SendMessage(hCombo, CB_SETITEMDATA, itemIndex, (LPARAM)lang.id);  // Store language ID
		}

		int firstSelectable = 0;            // First selectable item index
		if (separatorAdded == 0) {          // If separator is first item
			firstSelectable = 1;            // Skip to second item
		}
		SendMessage(hCombo, CB_SETCURSEL, firstSelectable, 0);  // Set selection
		currentDisplayIndex = firstSelectable;  // Store current index

		if (!displayOrder.empty()) {        // If items exist
			LRESULT langID = SendMessage(hCombo, CB_GETITEMDATA, firstSelectable, 0);  // Get language ID
			if (langID != -10) {            // If not separator
				for (int i = 0; i < (int)allLanguages.size(); i++) {  // Find language
					if (allLanguages[i].id == langID) {  // If ID matches
						currentLanguageName = allLanguages[i].displayName;  // Store name
						break;              // Break loop
					}
				}
			}
		}

		hCheckFavorite = CreateCheckbox(hwnd, L"Favorite",  // Create favorite checkbox
			200, 40, 70, 50, IDC_CHECK_FAVORITE);  // Position, size, ID
		 SendMessage(hCheckFavorite, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		if (currentDisplayIndex >= 0) {     // If item selected
			LRESULT langID = SendMessage(hCombo, CB_GETITEMDATA, currentDisplayIndex, 0);  // Get language ID
			if (langID != -10) {            // If not separator
				for (int i = 0; i < (int)allLanguages.size(); i++) {  // Find language
					if (allLanguages[i].id == langID) {  // If ID matches
						SendMessage(hCheckFavorite, BM_SETCHECK,  // Set checkbox state
							allLanguages[i].isFavorite ? BST_CHECKED : BST_UNCHECKED, 0);  // Based on favorite status
						break;              // Break loop
					}
				}
			}
		}

		HWND hOkButton = CreateButton(hwnd, L"OK",  // Create OK button
			50, 100, 80, 32, IDOK);         // Position, size, ID
		 SendMessage(hOkButton, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		HWND hCancelButton = CreateButton(hwnd, L"Cancel",  // Create Cancel button
			170, 100, 80, 32, IDCANCEL);    // Position, size, ID
		 SendMessage(hCancelButton, WM_SETFONT, (WPARAM)hFont, TRUE);  // Set font

		RECT rc;                            // Dialog rectangle
		GetWindowRect(hwnd, &rc);           // Get dialog bounds
		int screenWidth = GetSystemMetrics(SM_CXSCREEN);  // Screen width
		int screenHeight = GetSystemMetrics(SM_CYSCREEN); // Screen height
		int dialogWidth = rc.right - rc.left;  // Dialog width
		int dialogHeight = rc.bottom - rc.top; // Dialog height
		int x = (screenWidth - dialogWidth) / 2;  // Center X
		int y = (screenHeight - dialogHeight) / 2; // Center Y
		SetWindowPos(hwnd, NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);  // Center dialog


		return TRUE;                        // Initialization successful
	}

	case WM_COMMAND: {                      // Handle command messages
		WORD id = LOWORD(wParam);           // Control ID
		WORD code = HIWORD(wParam);         // Notification code

		if (id == IDC_COMBO_LANG && code == CBN_SELCHANGE) {  // Combo box selection changed
			int newDisplayIndex = (int)SendMessage(hCombo, CB_GETCURSEL, 0, 0);  // Get new selection
			LRESULT itemData = SendMessage(hCombo, CB_GETITEMDATA, newDisplayIndex, 0);  // Get item data
			if (itemData == -10) {      // If separator
				if (newDisplayIndex < (int)SendMessage(hCombo, CB_GETCOUNT, 0, 0) - 1) {  // If not last item
					newDisplayIndex++;   // Skip to next item
					SendMessage(hCombo, CB_SETCURSEL, newDisplayIndex, 0);  // Set selection
				}
				else if (newDisplayIndex > 0) {  // If not first item
					newDisplayIndex--;   // Skip to previous item
					SendMessage(hCombo, CB_SETCURSEL, newDisplayIndex, 0);  // Set selection
				}
				itemData = SendMessage(hCombo, CB_GETITEMDATA, newDisplayIndex, 0);  // Get new item data
			}

			if (itemData != -10) {      // If not separator
				currentDisplayIndex = newDisplayIndex;  // Update current index

				for (int i = 0; i < (int)allLanguages.size(); i++) {  // Find language
					if (allLanguages[i].id == itemData) {  // If ID matches
						currentLanguageName = allLanguages[i].displayName;  // Update name

						SendMessage(hCheckFavorite, BM_SETCHECK,  // Update checkbox
							allLanguages[i].isFavorite ? BST_CHECKED : BST_UNCHECKED, 0);  // Based on favorite status
						break;          // Break loop
					}
				}
			}

			return TRUE;                    // Message handled
		}

		if (id == IDC_CHECK_FAVORITE && code == BN_CLICKED) {  // Favorite checkbox clicked
			if (currentDisplayIndex >= 0 && !currentLanguageName.empty()) {  // If valid selection
				int actualIndex = -1;       // Store actual index
				for (int i = 0; i < (int)allLanguages.size(); i++) {  // Find language by name
					if (allLanguages[i].displayName == currentLanguageName) {  // If name matches
						actualIndex = i;     // Store index
						break;              // Break loop
					}
				}

				if (actualIndex != -1) {    // If language found
					bool wasFavorite = allLanguages[actualIndex].isFavorite;  // Get current favorite status

					if (!wasFavorite) {     // If not currently favorite
						int currentFavorites = CountFavorites();  // Count favorites
						if (currentFavorites >= g_maxFavorites) {  // If at max capacity
							int oldestIndex = FindOldestFavorite();  // Find oldest favorite
							if (oldestIndex != -1) {  // If found
								allLanguages[oldestIndex].isFavorite = false;  // Remove favorite status
								allLanguages[oldestIndex].favoriteTime = 0;     // Clear time
							}
						}

						allLanguages[actualIndex].isFavorite = true;  // Set as favorite
						allLanguages[actualIndex].favoriteTime = time(NULL);  // Set current time
					}
					else {                   // If currently favorite
						allLanguages[actualIndex].isFavorite = false;  // Remove favorite status
						allLanguages[actualIndex].favoriteTime = 0;     // Clear time
					}

					UpdateDisplayOrder();     // Update display order

					SendMessage(hCombo, CB_RESETCONTENT, 0, 0);  // Clear combo box

					int separatorAdded = -1;  // Track separator
					int favoriteCount = CountFavorites();  // Get favorite count

					for (int displayIdx = 0; displayIdx < (int)displayOrder.size(); displayIdx++) {  // Repopulate
						int actualIdx = displayOrder[displayIdx];  // Get actual index
						Language& lang = allLanguages[actualIdx];  // Get language

						if (!lang.isFavorite && favoriteCount > 0 && displayIdx == favoriteCount) {  // Add separator
							SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)L"────────────");  // Add separator
							int separatorIndex = displayIdx;  // Store index
							SendMessage(hCombo, CB_SETITEMDATA, separatorIndex, (LPARAM)-10);  // Mark as separator
							separatorAdded = separatorIndex;  // Update flag
						}

						wstring displayName = lang.GetDisplayName();  // Get display name
						LRESULT itemIndex = SendMessage(hCombo, CB_ADDSTRING, 0, (LPARAM)displayName.c_str());  // Add item

						SendMessage(hCombo, CB_SETITEMDATA, itemIndex, (LPARAM)lang.id);  // Store language ID
					}

					int newDisplayIndex = FindDisplayIndexByName(currentLanguageName);  // Find new position
					if (newDisplayIndex != -1) {  // If found
						if (separatorAdded != -1 && newDisplayIndex >= separatorAdded) {  // Adjust for separator
							newDisplayIndex++;   // Skip separator
						}
						SendMessage(hCombo, CB_SETCURSEL, newDisplayIndex, 0);  // Set selection
						currentDisplayIndex = newDisplayIndex;  // Update index

						LRESULT langID = SendMessage(hCombo, CB_GETITEMDATA, newDisplayIndex, 0);  // Get language ID
						if (langID != -10) {      // If not separator
							for (int i = 0; i < (int)allLanguages.size(); i++) {  // Find language
								if (allLanguages[i].id == langID) {  // If ID matches
									SendMessage(hCheckFavorite, BM_SETCHECK,  // Update checkbox
										allLanguages[i].isFavorite ? BST_CHECKED : BST_UNCHECKED, 0);  // Based on status
									break;      // Break loop
								}
							}
						}
					}
				}
			}
			return TRUE;                    // Message handled
		}

		if (id == IDOK) {                   // OK button clicked
			if (hCombo && currentDisplayIndex >= 0) {  // If combo exists and item selected
				LRESULT langID = SendMessage(hCombo, CB_GETITEMDATA, currentDisplayIndex, 0);  // Get language ID
				if (langID != -10) {        // If not separator
					ApplyLanguageToCurrentDoc((int)langID);  // Apply language to document
				}
			}
			SaveConfiguration();
			EndDialog(hwnd, 0);             // Close dialog
			return TRUE;                    // Message handled

		}

		if (id == IDCANCEL) {               // Cancel button clicked
			EndDialog(hwnd, 0);             // Close dialog
			return TRUE;                    // Message handled
		}

		break;                              // Break from switch
	}

	case WM_CLOSE:                          // Window close (X button)
		EndDialog(hwnd, 0);                 // Close dialog
		return TRUE;                       // Message handled
	}

	return FALSE;                           // Message not handled
}

void ShowLanguageDialog() {                 // Show language selection dialog
	if (allLanguages.empty()) {             // If languages not initialized
		InitializeLanguages();              // Initialize languages
	}

	HWND hDialog = CreateWindowEx(          // Create dialog window
		WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,  // Extended styles
		L"#32770",                          // Dialog class name
		L"Select Language",                 // Window title
		WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME,  // Window styles
		CW_USEDEFAULT, CW_USEDEFAULT, 320, 200,  // Position and size
		nppData._nppHandle,                // Parent window (Notepad++)
		NULL,                              // No menu
		g_hInstance,                       // Instance handle
		NULL);                             // No extra data

	SetWindowLongPtr(hDialog, DWLP_DLGPROC, (LONG_PTR)DialogProc);  // Set dialog procedure

	SendMessage(hDialog, WM_INITDIALOG, 0, 0);  // Initialize dialog

	ShowWindow(hDialog, SW_SHOW);          // Show window
	UpdateWindow(hDialog);                 // Update window
}

void ShowSettingsDialog() {                // Show settings dialog
	HWND hWnd = CreateWindowEx(            // Create dialog window
		WS_EX_DLGMODALFRAME | WS_EX_WINDOWEDGE,  // Extended styles
		L"#32770",                          // Dialog class name
		L"Language Selector Settings",      // Window title
		WS_POPUP | WS_CAPTION | WS_SYSMENU | DS_MODALFRAME | DS_SETFOREGROUND,  // Window styles
		CW_USEDEFAULT, CW_USEDEFAULT, 300, 160,  // Position and size
		nppData._nppHandle,                // Parent window (Notepad++)
		NULL,                              // No menu
		g_hInstance,                       // Instance handle
		NULL);                             // No extra data

	SetWindowLongPtr(hWnd, DWLP_DLGPROC, (LONG_PTR)SettingsDialogProc);  // Set dialog procedure

	SendMessage(hWnd, WM_INITDIALOG, 0, 0);  // Initialize dialog

	ShowWindow(hWnd, SW_SHOW);             // Show window
	UpdateWindow(hWnd);                    // Update window

	MSG msg;                               // Message structure
	while (IsWindow(hWnd) && GetMessage(&msg, NULL, 0, 0)) {  // Message loop
		if (!IsDialogMessage(hWnd, &msg)) {  // If not dialog message
			TranslateMessage(&msg);        // Translate message
			DispatchMessage(&msg);         // Dispatch message
		}
	}
}


HRESULT CALLBACK TaskDialogCallbackProc(HWND hwnd, UINT uNotification, WPARAM wParam, LPARAM lParam, LONG_PTR dwRefData) {
	(void)hwnd; (void)wParam; (void)dwRefData;

	if (uNotification == TDN_HYPERLINK_CLICKED) {
		// convert the lParam to a string to see which link was clicked
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
		L"Version: 1.0\n"
		L"Copyright © Abdellah Hassaine, December 2025\n\n"//28th of Dec precisely, Algiers, at my desk.
		L"Website: <a href=\"website\">https://github.com/hassaine-abdellah/npp-language-selector/</a>\n"
		L"Contact: <a href=\"email\">hassaine_abdellah@univ-blida.dz</a>\n\n"
		L"Description: This plug-in allows you to quickly select the programming language for the current tab, and by consequence the corresponding syntax highlighting.\n\n"
		L"Features:\n"
		L"• Select programming language from dropdown menu.\n"
		L"• Auto-show selection dialog on new tabs (can be turned off).\n"
		L"• Pin favorite languages on top of the list for quick access.\n  (when favorites are full, the oldest one (first in) is put out).\n"
		L"• Configurable favorite list size.\n\n"
		L"The author gladly welcomes your feedback and suggestions.\n"
		L"\n";

	config.pfCallback = TaskDialogCallbackProc;
	config.dwCommonButtons = TDCBF_OK_BUTTON;

	TaskDialogIndirect(&config, NULL, NULL, NULL);
}


extern "C" __declspec(dllexport) void setInfo(NppData notpadPlusData) { 
	nppData = notpadPlusData;            // Store Notepad++ data
	commandMenuInit();                    // Initialize menu commands
	LoadConfiguration();
}

extern "C" __declspec(dllexport) const TCHAR* getName() {  // Get plugin name
	return TEXT("Language Selector");   // Return plugin name
}

extern "C" __declspec(dllexport) FuncItem* getFuncsArray(int* nbF) {  // Get function array
	*nbF = nbFunc;                      // Set number of functions
	return funcItem;                     // Return function array
}

extern "C" __declspec(dllexport) void beNotified(SCNotification* notifyCode) {  // Notification handler (to decide whether to activate on opened buffers)
	static bool nppReady = false;        // To track Notepad++ readiness, so selection dialog waits until Npp has fully launched
	if (!notifyCode) return;                     // Validate parameters

	// Handle Notepad++ state notifications
	switch (notifyCode->nmhdr.code) {
	case NPPN_READY:                         // Notepad++ fully initialized
		nppReady = true;                   // Set ready flag
		g_isShuttingDown = false;            // Reset shutdown flag
		g_shownBuffers.clear();              // Clear shown buffers

		// Show dialog for current buffer if needed
		if (g_autoShowOnNewTab && !g_isShuttingDown) {
			int currentBufferId = (int)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);

			// Check if file has extension
			wchar_t fileExt[MAX_PATH] = { 0 };
			::SendMessage(nppData._nppHandle, NPPM_GETEXTPART, MAX_PATH, (LPARAM)fileExt);

			if (wcslen(fileExt) == 0) {      // No extension
				g_shownBuffers.push_back(currentBufferId); // Mark as shown
				ShowLanguageDialog();				// show dialogue
			}
			else {
				g_shownBuffers.push_back(currentBufferId);  // Mark as shown
			}
		}
		return;

	case NPPN_BEFORESHUTDOWN:                      // Notepad++ is shutting down
		g_isShuttingDown = true;
		nppReady = false;
		return;
	}

	// If Notepad++ is shutting down, ignore all other notifications
	if (g_isShuttingDown) return;

	// If Notepad++ isn't ready yet, ignore buffer activations
	// (We'll handle the current buffer after NPPN_READY)
	if (!nppReady) return;

	// Only process auto-show if enabled
	if (!g_autoShowOnNewTab) return;

	// Normal handling after N++ is ready
	if (notifyCode->nmhdr.code == NPPN_BUFFERACTIVATED) {
		int bufferId = (int)::SendMessage(nppData._nppHandle, NPPM_GETCURRENTBUFFERID, 0, 0);

		// Skip if already shown
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
		int closingId = (int)notifyCode->nmhdr.idFrom;
		g_shownBuffers.erase(std::remove(g_shownBuffers.begin(),
			g_shownBuffers.end(), closingId), g_shownBuffers.end());
	}
}
extern "C" __declspec(dllexport) BOOL APIENTRY DllMain(
	HANDLE hModule, DWORD reason, LPVOID /*lpReserved*/) {

	switch (reason) {
	case DLL_PROCESS_ATTACH:
		g_hInstance = (HINSTANCE)hModule;
		// Load configuration when plugin starts
		LoadConfiguration(); 
		break;
	}

	return TRUE;
}

extern "C" __declspec(dllexport) LRESULT messageProc(UINT /*Message*/,  // Message processor
	WPARAM /*wParam*/,
	LPARAM /*lParam*/) {
	return 0;
}

#ifdef UNICODE
extern "C" __declspec(dllexport) BOOL isUnicode() {  // Unicode support check
	return TRUE;                          // Plugin supports Unicode
}
#endif