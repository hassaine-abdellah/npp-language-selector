//This file is part of Language Selector.
//Copyright (C) 2025 Abdellah Hassaine.
#ifndef PLUGINDEFINITION_H
#define PLUGINDEFINITION_H


#include <windows.h>
#include <string>
#include <vector>



//
// All definitions of plugin interface
//
#include "PluginInterface.h"

//-------------------------------------//
//-- STEP 1. DEFINE YOUR PLUGIN NAME --//
//-------------------------------------//
// Here define your plugin name
//
const TCHAR NPP_PLUGIN_NAME[] = TEXT("Language Selector");

//-----------------------------------------------//
//-- STEP 2. DEFINE YOUR PLUGIN COMMAND NUMBER --//
//-----------------------------------------------//
//
// Here define the number of your plugin commands
//
const int nbFunc = 3;


//
//Initialization of your plugin commands
//
void commandMenuInit();


//
// Function which sets your command 
//
bool setCommand(size_t index, TCHAR *cmdName, PFUNCPLUGINCMD pFunc, ShortcutKey *sk = NULL, bool check0nInit = false);

//
// Your plugin command functions
//

void ShowLanguageDialog();

void ShowSettingsDialog();

void About();


#endif //PLUGINDEFINITION_H
