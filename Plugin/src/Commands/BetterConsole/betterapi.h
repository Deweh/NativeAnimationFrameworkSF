/* betterapi.h - the public API of BetterConsole */
/* https://www.nexusmods.com/starfield/mods/3683 */
/* https://github.com/SomeCrazyGuy/Starfield-Console-Replacer */
/* Author: Seth Royer */
/* License: http://unlicense.org/ */
// You can always reach out to me if you have any questions or need help
// integrating betterapi into your mod. I'm almost always available in the
// Constellation by v2 discord or nexusmods.

// Always use the latest betterapi.h file from github!
// Sometimes small hacks or fixes are put here to resolve
// issues in the published mod between releases. You
// can find the latest version here:
// https://raw.githubusercontent.com/SomeCrazyGuy/Starfield-Console-Replacer/master/betterapi.h

///////////////////////////////////////////////////////////////////////////////
//                 Table Of Contents
///////////////////////////////////////////////////////////////////////////////
//    1) The Unlicense
//    2) Release History
//    3) About this file
//    4) Quick Start Guide
//    5) Configuring Better API
//    6) What are SFSE and ASI mods?
//    7) The betterapi philosophy
//    8) Thread safety
//    9) What is userdata?
//    10) Typedefs
//    11) Enums
//    12) Callback Types
//    13) Callback API
//    14) Config API
//    15) Hook API
//    16) Graphics API
//    17) LogBuffer API
//    18) C Library API
//    19) Game Console API
//    20) Wrapped Windows API
//    21) File I/O API
//    22) BetterConsole API
//    23) SFSE Minimal Interface

///////////////////////////////////////////////////////////////////////////////
//                 1) The Unlicense
///////////////////////////////////////////////////////////////////////////////
//
// This is free and unencumbered software released into the public domain.
//
// Anyone is free to copy, modify, publish, use, compile, sell, or
// distribute this software, either in source code form or as a compiled
// binary, for any purpose, commercial or non-commercial, and by any
// means.
//
// In jurisdictions that recognize copyright laws, the author or authors
// of this software dedicate any and all copyright interest in the
// software to the public domain. We make this dedication for the benefit
// of the public at large and to the detriment of our heirs and
// successors. We intend this dedication to be an overt act of
// relinquishment in perpetuity of all present and future rights to this
// software under copyright law.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
// EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
// MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT.
// IN NO EVENT SHALL THE AUTHORS BE LIABLE FOR ANY CLAIM, DAMAGES OR
// OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
// ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
// OTHER DEALINGS IN THE SOFTWARE.
//
// For more information, please refer to <http://unlicense.org/>

///////////////////////////////////////////////////////////////////////////////
//                 2) Release History
///////////////////////////////////////////////////////////////////////////////
//
// 1.3.0 and older - no longer supported!
//
// 1.3.1 - 2024-06-09 Starfield v1.2.30
//         First version using the new entrypoint "BetterConsoleReceiver"
//
// 1.4.0 - 2024-06-14 Starfield v1.2.32
//         No public API changes
//

///////////////////////////////////////////////////////////////////////////////
//                 3) About this file
///////////////////////////////////////////////////////////////////////////////
//
// This is the public API for the Starfield mod BetterConsole:
// <https://www.nexusmods.com/starfield/mods/3683>
//
// This file is a zero dependency header only library in the style of STB
// libraries: <https://github.com/nothings/stb>
//
// As such it is written in a way that shows the <s>limitations</s> <i>features</i>
// of the C programming language. This is all an excuse to say: "if it looks like
// something is being done the hard or wrong way, don't worry, it is intentional"
//
// BetterConsole itself is mostly written in C with some C++ features sprinkled in
// for flavor, but the public API (betterapi.h) is intended to be plain C99 code
// and include only two standard C headers: "stdint.h" and "stdbool.h". "betterapi.h"
// itself calls no external functions: not from the game, not the windows api,
// not even standard library functions. It is trivial to integrate into any other
// mod as it is a single file and there is no library to link with. Betterapi uses
// a callback system to mediate between your mod and BetterConsole. Betterapi is
// always a soft dependency which means that your mod can use BetterConsole APIs
// when they are available, but the lack of BetterConsole in the load order does
// not break your mod. The way this works is by exporting a function
// "BetterConsoleReceiver" which, if BetterConsole is loaded, will be called
// automatically just after Starfield creates a DirectX graphics context.
// BetterConsoleReceiver is generated by this file when you define the macro
// BETTERAPI_IMPLEMENTATION before including "betterapi.h". Because this function
// is not just declared but fully implemented in this header you must only define
// the BETTERAPI_IMPLEMENTATION macro in a single translation unit (.c or .cpp file)
// of your mod project to avoid linker errors about "multiple definition of function"
// As part of the glue code BetterConsoleReceiver will perform version and
// compatibility checking with the BetterConsole runtime component before calling
// your entrypoint for the API: "OnBetterConsoleLoad". Please read through the quick
// start guide to see an example of how you would implement "OnBetterConsoleLoad".

///////////////////////////////////////////////////////////////////////////////
//                 4) Quick Start Guide
///////////////////////////////////////////////////////////////////////////////
// Adding BetterConsole features to your mod is very easy, you should be able
// to get a basic GUI up and running within minutes. The "betterapi.h" file can
// be included anywhere in your project and the only build requirement is to
// enable the implementation of the BetterConsole "glue code" by defining
// BETTERAPI_IMPLEMENTATION before including "betterapi.h" in *one* .c or .cpp
// file. The only code you need to write is the "OnBetterConsoleLoad" function.
// OnBetterConsoleLoad is called by BetterConsole when Starfield begins drawing
// to the screen. This function will be used to register your mod with BetterConsole.
// Take a look at this example for a basic plugin or build the example yourself
// by defining BETTERAPI_BUILTIN_EXAMPLE before including "betterapi.h" in an empty
// .c or .cpp file.
#ifdef BETTERAPI_BUILTIN_EXAMPLE
#	undef BETTERAPI_BUILTIN_EXAMPLE

// Create the implementation glue code, this can only be done once per project
#	define BETTERAPI_IMPLEMENTATION

// The example builds a combination ASI and SFSE plugin by enabling the minimal
// SFSE glue code also available in betterapi
#	define BETTERAPI_ENABLE_SFSE_MINIMAL

// Use betterapi, you can include this anywhere you need to access hte features
#	include "betterapi.h"

// It is usually convenient to make aliases for the betterconsole API structures
static const struct better_api_t*  API = NULL;
static const struct simple_draw_t* UI = NULL;

// Forward declarations of callback functions used in this example
void MyDrawCallback(void*);
void MyConfigCallback(ConfigAction);
void MyHotkeyCallback(uintptr_t);

// Global plugin state
static uint32_t ButtonClickCounter = 0;

// Usually for hotkey requests you want your hotkey userdata to be an enum or a
// pointer to som hotkey-specific data.
enum MyHotkeyAction
{
	MHA_Reset,
	MHA_Click,
	MHA_ClickTwice
};

// This function will be called automatically when BetterConsole is loaded and
// your plugin is compatible with the runing version of BetterConsole.
static int OnBetterConsoleLoad(const struct better_api_t* better_api)
{
	API = better_api;
	UI = API->SimpleDraw;

	// The first step is to register your plugin name with BetterConsole
	// The name used here will show up in the GUI so that the user can
	// recognize your mod.
	RegistrationHandle my_mod_handle = API->Callback->RegisterMod("Example");

	// Everything else is optional, but likely you will want to also
	// register a draw callback so your mod shows up in the UI.
	API->Callback->RegisterDrawCallback(my_mod_handle, &MyDrawCallback);

	// There are several types of callbacks depending on what kind of event
	// you want to make a handler for. This line registers a configuration
	// callback.
	API->Callback->RegisterConfigCallback(my_mod_handle, &MyConfigCallback);

	// The hotkey feature is activated in two steps, first register a callback:
	API->Callback->RegisterHotkeyCallback(my_mod_handle, &MyHotkeyCallback);

	// Then for each hotkey action you want to have, request a hotkey for it.
	// The specific key combination used to activate a hotkey is set by the
	// user in the "Mod Menu" > "Hotkeys" tab of BetterConsole.
	API->Callback->RequestHotkey(my_mod_handle, "Reset Click Counter", MHA_Reset);
	API->Callback->RequestHotkey(my_mod_handle, "Add 1 Click", MHA_Click);
	API->Callback->RequestHotkey(my_mod_handle, "Add 2 Clicks", MHA_ClickTwice);

	// return 0 if your plugin loaded correctly or
	// return any positive number to indicate a failure
	return 0;
}

// The draw callback is called whenever your plugin needs to draw something.
// There are many widgets available, but for simplicity lets make a basic example.
void MyDrawCallback(void*)
{
	UI->Text("Hello World!");
	if (UI->Button("Click Me")) {
		++ButtonClickCounter;
	}
	UI->Text("You pressed the button %u times", ButtonClickCounter);
}

// The config callback is called when the "BetterConsoleConfig.txt" file is loaded
// (read action), when the file is saved (write action), or when the user is in
// "Mod Menu" > "Settings" and selects your mod to configure it (edit action).
void MyConfigCallback(ConfigAction action)
{
	// The Config api offers default actions for certain types (like uint32_t)
	API->Config->ConfigU32(action, "Button Click Couter", &ButtonClickCounter);
}

// Hotkeys are global, they can be activated even when BetterConsole is not shown.
// Only one draw callback per mod is allowed, but you can use the userdata parameter
// of the callback to tell which action was requested.
void MyHotkeyCallback(uintptr_t userdata)
{
	enum MyHotkeyAction act = (enum MyHotkeyAction)userdata;

	if (act == MHA_Reset) {
		ButtonClickCounter = 0;
	} else if (act == MHA_Click) {
		ButtonClickCounter++;
	} else if (act == MHA_ClickTwice) {
		ButtonClickCounter += 2;
	}
}

// The following allows the example plugin to also be an sfse plugin
// more details are in the sfse section
// Step 1) Export this struct so sfse knows your DLL is compatible
DLLEXPORT SFSEPluginVersionData SFSEPlugin_Version = {
	1,                               // SFSE api version, 1 is current
	1,                               // Plugin api version, 1 is current
	"BetterConsole Example Plugin",  // Mod/Plugin Name (limit: 255 characters)
	"Linuxversion",                  // Mod Author(s)   (limit: 255 characters)
	// Address Independance:
	1,  // 0 - hardcoded offsets (game version specific)
		// 1 - signature scanning (not version specific)
	// Structure Independance:
	1,  // 0 - relies on specific game structs
	// 1 - mod does not care if game structs change
	// Compatible Game Versions:
	{
		// A list of up to 15 game versions
		MAKE_VERSION(1, 11, 36),  // This means compatible with 1.11.36
		0                         // The list must be terminated with 0
	},                            // if address & structure independent
	// then this is minimum version required
	0,    // 0 = does not rely on any specific sfse version
	0, 0  // reserved fields, must be 0
};

// Step 2) Export this function so sfse knows to load your dll.
//         Doing anything inside the function is optional.
DLLEXPORT bool SFSEPlugin_Load(const SFSEInterface* sfse) { return true; }

// Need to undef this for the example because the example needs to include itself
// and thus would create the implmentation functions twice
#	undef BETTERAPI_IMPLEMENTATION
#endif  // BETTERAPI_BUILTIN_EXAMPLE

#ifndef BETTERAPI_API_H /* Prevent multiple includes of same file */
#	define BETTERAPI_API_H

// This is used to convert version numbers like 1.3.1 into a single number
// that can be compared, follows the same scheme as sfse and the game
#	define MAKE_VERSION(MAJOR, MINOR, BUILD) ((((MAJOR)&0xFF) << 24) | (((MINOR)&0xFF) << 16) | (((BUILD)&0xFFF) << 4))

///////////////////////////////////////////////////////////////////////////////
//                 5) Configuring Better API
///////////////////////////////////////////////////////////////////////////////
// Without configuration, "betterapi.h" will use the most compatible and lowest
// supported feature set. It is recommended to always use the lowest feature
// level necessary for your mod to function to maintain compatibility with
// BetterConsole versions in the past as well as the future. As new versions
// of BetterConsole are released, new API functions become available that were
// not in previous versions. At the minimum, your mod should support at least
// one previous feature level to allow mod collections that include an older
// release of BetterConsole to catch up to newer game releases.

// By default betterapi.h only enables the base featureset
// from version 1.3.1 (which itself is almost 100%) compatible
// with the first version released. To access newer functions
// and capabilities you will need to define the configuration
// variable BETTERAPI_FEATURE_LEVEL to a specific release of
// BetterConsole like MAKE_VERSION(1,4,1) for version 1.4.1
#	ifndef BETTERAPI_FEATURE_LEVEL
#		define BETTERAPI_FEATURE_LEVEL MAKE_VERSION(0, 0, 0)
#	endif  // BETTERAPI_FEATURE_LEVEL

// You might see the use of BETTERAPI_DEVELOPMENT_FEATURES in the code below.
// these are experimental features that might be in the next version of
// betterconsole but are not part of any current release on nexusmods.
// If you are developing a mod using the betterconsole api you MUST NOT define
// BETTERAPI_DEVELOPMENT_FEATURES, as the resulting plugin will not be compatible
// with the version of betterconsole that everyone is using.
//#define BETTERAPI_DEVELOPMENT_FEATURES

///////////////////////////////////////////////////////////////////////////////
//                 6) What are SFSE and ASI mods?
///////////////////////////////////////////////////////////////////////////////
// If your reading this file you should already know? But just so we are all on
// the same page: SFSE and ASI mods are DLL files that are loaded into the game
// using a loader. The main difference between an SFSE and ASI mods is that ASI
// mods are loaded into the game using the ASI loader which just runs the winapi
// function LoadLibrary() on any file named *.asi in the Plugins/ folder. SFSE
// mods are loaded into the game using the SFSE loader which calls the same
// LoadLibrary() function as ASI mods, but offers a more complete interface for
// the game. ASI loaders are generic, just about every game has an ASI loader
// available for running mods, but SFSE is specific to Starfield. Due to the
// specificity of the SFSE loader, its possible to interact with many game
// functions and data structures that you must reverse-engineer youself in ASI
// mods.
//
// But what do the mods do and how do they work?
// You may be familiar with EXE programs and DLL libraries. When writing programs
// your code is converted into an executable file that calls function from dll
// libraries:
// [PROGRAM calls CreateWindow()] -------------> [DLL implments CreateWindow()]
// *This is an example of normal execution
//
// when you write a mod, you are performing the reverse operation, you are writing
// a program that compiles to a DLL and manipulating the functions of other DLLs
// or, with SFSE, using the functions of the game as if the game is the DLL and
// your mod is the EXE:
// [PROGRAM calls CreateWindow()] -> [MOD hooks CreateWindow()] -> [DLL implments CreateWindow()]
// *This is an example of DLL injection, the intent is to modify the window created by the game
// *for example, to change the window name, or set window flags, or adjust resolution more than
// *the game allows.
//
// [PROGRAM implements ExecuteConsole()] <--[MOD calls ExecuteConsole()]
// *This is an example of using the reverse engineered functions of the game to provide
// *a mod that ability to run commands through the in-game console. SFSE acts as
// *a centralized repository for reverse engineered functions that mod authors can use.
//
// [PROGRAM implements ExecuteConsole()] <---> [MOD hooks ExecuteConsole()]
// *Instead of calling ExecuteConsole(), a mod could hook it. This is what BetterConsole
// *does to provide a combination of execution and logging of all commands run even by the
// *game internally.
//
// BetterConsole can be used with SFSE or ASI mods by providing its own API to any other
// loaded DLL that exports a specific function "BetterConsoleReceiver"
//
//   [  B e t t e r C o n s o l e ]
//   |                            |
//   v                            v
// [GAME] <-------------------> [MOD]
//   |                            |
//   v                            v
// ExecuteConsole()      BetterConsoleReceiver(api*) -> SimpleDraw, Hook, Console, etc...
// PrintConsole()
// DX12_Present()

///////////////////////////////////////////////////////////////////////////////
//                 7) The betterapi philosophy
///////////////////////////////////////////////////////////////////////////////
// On a philisophical level if its possible to export an API using plain C99 and
// include only standard C headers, then your API is deemed simple enough. And if
// you cannot describe an interface in C then you cannot truely understand your own
// code. In addition, almost every programming language has some type of C
// compatible foreign function interface, this allows you to write mods in almost
// any programming language albeit with some challenges.
#	include <stdbool.h>
#	include <stdint.h>

// Implements the api version checks
// Dont worry, I'll undef this at the end of the file, nobody
// wants single letter macros polluting the namespace.
#	ifdef V
#		error "V is already defined!"
#	endif  // V
#	define V(MAJOR, MINOR, PATCH) (MAKE_VERSION(MAJOR, MINOR, PATCH) <= (BETTERAPI_FEATURE_LEVEL))

#	ifdef BETTERAPI_DEVELOPMENT_FEATURES
#		pragma message("BETTERAPI_DEVELOPMENT_FEATURES is defined, any plugin using the api will not be compatible the nexusmods release of betterconsole.")
#		undef BETTERAPI_FEATURE_LEVEL
#		define BETTERAPI_FEATURE_LEVEL MAKE_VERSION(9, 9, 9)
#	endif

///////////////////////////////////////////////////////////////////////////////
//                 8) Thread safety
///////////////////////////////////////////////////////////////////////////////
// There is no thread safety! All of the betterapi functions are operating within
// the game's idxgiswapchain::present call as the game is trying submit a
// frame to the gpu. Do not use any of these apis from a separate thread unless
// it is explicitly marked as safe to do so and please keep it quick - we don't
// want to slow down the game. I will add an api for a separate task thread to
// do sig scanning or other slow tasks in a future update.

///////////////////////////////////////////////////////////////////////////////
//                 9) What is userdata?
///////////////////////////////////////////////////////////////////////////////
// Several of the APIs in BetterConsole take a "userdata" argument
// since we are exposing all these capabilities in plain C99 we
// need a way to express a generic type or piece of data that you
// want to work with. Often this will show up in the SimpleDraw functions.
// Lets look at an example of why and how you would do this...
//
// say you have an array of pointers to some struct:
// struct GameType {
//      ...
//      uint32_t formid;
//      ...
//      const char* name;
//      ...
// } *data_array[100] = some_pointer;
//
// how would you make a selectable list of these in a gui that knows nothing
// about GameType? The answer is userdata and a callback! Let's look at an
// example using the SimpleDraw SelectionList function:
//
// Create a to_string function for GameType:
//
// const char* gametype_to_string(const void* userdata, uint32_t index, char* out_tmp, uint32_t tmp_size) {
//      const GameType** pGT = (const GameType**)userdata;
//      const GameType* GT = pGT[index];
//      snprintf(out_tmp, tmp_size, "FormID: %X, Name: %s", GT->formid, GT->name);
//      return out_tmp;
// }
//
// Then make it a selectable list:
//
// static uint32_t selected_item = 0;
// bool was_clicked = API->SimpleDraw->SelectionList(&selected_item, (const void*)data_array, 100, gametype_to_string);
//
// Now the user can scroll through and click on an entry in the list without
//              the UI code ever needing to know what the heck a GameType is.
// Note: selections lists dont always start at the first index, in our example above
//       there were 100 items in the list, but selectionlist will only call gametype_to_string
//       on the items that are visible, if the selection list can only show 20 items
//       your gametype_to_string function may only be called for say index 53-73.
//       So make sure your data is random access iterable.

///////////////////////////////////////////////////////////////////////////////
//                 10) Typedefs
///////////////////////////////////////////////////////////////////////////////

// When you register a mod you receive a handle to your mod
// this handle can be used to register additional functionality
// like hotkeys or draw callbacks
typedef uint32_t RegistrationHandle;

// Opaque handle for the log buffer system
typedef uint32_t LogBufferHandle;

// while many compilers let you cast a function pointer to a void*
// the C standard only lets you cast a function pointer to another
// function pointer. This also helps document the API better.
typedef void (*FUNC_PTR)(void);

///////////////////////////////////////////////////////////////////////////////
//                 11) Enums
///////////////////////////////////////////////////////////////////////////////
// The settings callback tells you what type of action to perform from these options:
// ConfigAction_Read - betterconsole finished reading the settings file
//                     and now you can try loading any settings you saved
// ConfigAction_Write - betterconsole is about to save all settings to
//                      the settings file
// ConfigAction_Edit - the user is in the settings menu and wants to adjust a setting
//                     show a UI to the user to edit the setting
typedef enum ConfigAction
{
	ConfigAction_Read,
	ConfigAction_Write,
	ConfigAction_Edit,
} ConfigAction;

///////////////////////////////////////////////////////////////////////////////
//                 12) Callback Types
///////////////////////////////////////////////////////////////////////////////

// To show a GUI you must register a draw callback with this signature.
//
// `imgui_context` can be ignored if using the SimpleDraw API, but if you link with
//    imgui directly this can be used to create more complex user interfaces but you
//    must call ImGui::SetCurrentContext(imgui_context) first in your draw callback
//    and match the version of imgui used by betterconsole exactly.
//
// Note: this callback will be called every frame when betterconsole is open and
//       the tab for your mod is selected.
typedef void (*DRAW_CALLBACK)(void* imgui_context);

// if your application would like to take advantage of the built-in settings
// registry you will need to provide a callback function that is called when
// the settings file is loaded, saved, or when the user is in the settings menu
// and selects your mod. The Settings API provides automatic read/write/edit
// functionality if the built-in types work for you.
// `read_write_or_edit` is one of: ConfigAction_Read, ConfigAction_Write, or
//                      ConfigAction_Edit
typedef void (*CONFIG_CALLBACK)(ConfigAction read_write_or_edit);

// This is a function called when the user presses a hotkey combination that
// matches the hotkey set for your plugin. Setting a hotkey is done by the user
// in the hotkeys menu. Hotkeys are saved automatically in the settings registry
// so a set hotkey combo is preserved across sessions. Only one hotkey callback
// per mod can be registered, but you can request hotkeys multiple times with
// different user data.
//
// `userdata` is any arbitrary data that you want to pass to the callback usually
//            to help identify the hotkey if multiple hotkeys are requested
typedef void (*HOTKEY_CALLBACK)(uintptr_t userdata);

// For the simpledraw selectionlist, this is a function called to provide text for
// a single entry in a selectable list of items. This function will be called every
// frame for every *visible* item in the list. The return value should be a null
// terminated string that will be shown in the UI.
//
// `userdata` is any arbitrary data that you want to pass to the callback
//
// `index` is the index of the item in the list to "stringify"
//
// `out_buffer` is provided as a temporary buffer that can be used
//              to store the result of snprintf() or similar since
//              freeing a heap allocation made within the callback
//              is a very difficult task.
typedef const char* (*CALLBACK_SELECTIONLIST)(const void* userdata, uint32_t index, char* out_buffer, uint32_t out_buffer_size);

// For the simpledraw table renderer, used to build the UI shown in tables.
// This callback will be called for each *visible* cell in the table (unseen cells
// will not be rendered and thus will not call this function). Tables are drawn
// first left to right for each column, then from top to bottom for each row.
//
// `table_userdata` is any arbitrary data that you want to pass to the callback and
//                  should be used for the data you wish to draw in the table
//
// `current_row` is the index of the current row in the table that is being drawn
//
// `current_column` is the index of the current column in the table that is being drawn
// TODO: next version update this to use unsigned integers and possibly use uintptr_t
//       instead of void* for table_userdata
typedef void (*CALLBACK_TABLE)(uintptr_t table_userdata, int current_row, int current_column);

///////////////////////////////////////////////////////////////////////////////
//                 13) Callback API
///////////////////////////////////////////////////////////////////////////////

// This is the main api you will use to add betterconsole integration to your mod
struct callback_api_t
{
	// Register your mod with betterconsole.
	// The returned handle can be used to register additional functionality
	// such as hotkeys or draw callbacks.
	//
	// `mod_name` should uniquely identify your mod and conform to the following:
	//                 - must be less than 32 characters
	//                 - must have a length of >3 characters
	//                 - must not begin or end with whitespace
	//
	// Almost all mods will need to use this to interact with betterconsole.
	RegistrationHandle (*RegisterMod)(const char* mod_name);

	// Register a function to show your mod's user interface.
	//
	// `handle` is your registration handle from a previous call to RegisterMod
	//
	// `draw_callback` will be called every frame when the following conditions are met:
	//                 - the BetterConsole UI is open
	//                 - your mod is the one in focus
	//
	// Usually, you would utilize the SimpleDraw API to create your UI, advanced
	// users may link with imgui directly and utilize the imgui context provided
	// as the first parameter to the draw callback. If using imgui directly, you
	// must call ImGui::SetCurrentContext() in the draw callback before using
	// any imgui functions AND you must match the version of imgui with the
	// version used by betterconsole.
	void (*RegisterDrawCallback)(RegistrationHandle handle, DRAW_CALLBACK draw_callback);

	// Register a configuration callback
	//
	// `handle` is your registration handle from a previous call to RegisterMod
	//
	// `config_callback` will be called when the settings file is loaded (read event)
	//                   saved (write event) or when the user is in the setting menu
	//                   and selects your mod in the UI (edit event)
	void (*RegisterConfigCallback)(RegistrationHandle handle, CONFIG_CALLBACK config_callback);

	// Register a hotkey callback
	//
	// `handle` is your registration handle from a previous call to RegisterMod
	//
	//  `hotkey_callback` will be called when the user presses the hotkey combo
	//                    set for your plugin. to have an effect, you must also
	//                    call RequestHotkey. This is a two-step process because
	//                    your plugin can have only one hotkey callback but you
	//                    request any number of hotkeys with different userdata.
	void (*RegisterHotkeyCallback)(RegistrationHandle handle, HOTKEY_CALLBACK hotkey_callback);

	// Request a hotkey handler, the user configures the hotkey in the hotkeys menu
	//
	// `handle` is the handle you created when registering your mod
	//
	// `hotkey_name` is the name of the hotkey in the ui, use a descriptive name
	//               less than the 32 character internal buffer. this string is
	//               copied to an internal buffer so feel free to use a generated
	//               name via snprintf() or similar on stack allocated memory.
	//
	// `userdata` is optional extra data that is sent to the hotkey callback.
	//            if you want to request multiple hotkeys this parameter is
	//            how you would differentiate them.
	void (*RequestHotkey)(RegistrationHandle handle, const char* hotkey_name, uintptr_t userdata);

#	ifdef BETTERAPI_DEVELOPMENT_FEATURES
	// Register an about page draw callback.
	//
	// `handle` is the handle you created when registering your mod
	//
	// `about_callback` is a draw callback that you can use to show the user infomation
	//                  about your mod like version, changelog, and copyright or
	//                  provide links to a file (like a log file) or url
	//                  (like nexusmods page, discord, or github repo)
	void (*RegisterAboutPage)(RegistrationHandle handle, DRAW_CALLBACK about_callback);
#	endif
};

///////////////////////////////////////////////////////////////////////////////
//                 14) Config API
///////////////////////////////////////////////////////////////////////////////

/// This API is used for saving and loading data to a configuration file.
struct config_api_t
{
	// this handles read / write / edit on 32-bit unsigned integers
	// if a config value for key_name is not found, value is not modified
	// this way you can initialize all variables to a default value
	void (*ConfigU32)(ConfigAction action, const char* key_name, uint32_t* value);

#	ifdef BETTERAPI_DEVELOPMENT_FEATURES
	// dev - 1.4.0
	// read, write, and edit for string values
	// `in_out_buffer` is a pointer to a buffer that will be read, written, or edited
	// `buffer_size` is the length of the bufer in bytes
	// ConfigString stops on the first null character of the input or output
	// unlike other config functions, this function does write to the output if lookup fails on read (first byte becomes null)
	// and sets "in_out_buffer[buffer_size - 1] = 0" on action write
	void (*ConfigString)(ConfigAction action, const char* key_name, char* in_out_buffer, uint32_t buffer_size);

	// dev - 1.4.1
	// read, write, and edit for boolean values
	// `out_value` is a pointer to a boolean variable that will be read, written, or edited
	void (*ConfigBool)(ConfigAction action, const char* key_name, bool* out_value);

	// dev - 1.4.1
	// read, write, and edit for float values
	// `out_value` is a pointer to a float variable that will be read, written, or edited
	void (*ConfigFloat)(ConfigAction action, const char* key_name, float* out_value);

	// dev - 1.4.1
	// read or write fixed size arbitrary data
	// arbitrary data is stored hex encoded in the config file, this config option should be used
	// to store larger or more complex data like POD structs
	// unlike other config functions, this one needs to report if the data was parsed correctly
	// this function returns false if any non-hex characters were encountered while decoding or
	// if the size of the encoded data does not match the `data_size` parameter
	// since config functions do not allocate, returning false on a read event means that `out_data`
	// may have been overwritten with garbage values and should not be trusted
	bool (*ConfigData)(ConfigAction action, const char* key_name, void* out_data, uint32_t data_size);
#	endif  // BETTERAPI_DEVELOPMENT_FEATURES
};

///////////////////////////////////////////////////////////////////////////////
//                 15) Hook API
///////////////////////////////////////////////////////////////////////////////

// This api deals with hooking functions and vtables
struct hook_api_t
{
	// Hook old_func so it is redirected to new_func
	// returns a function pointer to call old_func after the hook
	// uses the minhook library internally
	FUNC_PTR (*HookFunction)
	(FUNC_PTR old_func, FUNC_PTR new_func);

	// Hook a vtable function for all instances of a class
	// returns the old function pointer
	FUNC_PTR (*HookVirtualTable)
	(void* class_instance, unsigned method_index, FUNC_PTR new_func);

	// Relocate an offset from imagebase
	// sfse offers a better version of this, but if you are using the minimal api (see below)
	// then you might want to use this
	void* (*Relocate)(unsigned imagebase_offset);

	// Write memory
	// same as Relocate, use the sfse version if you can
	bool (*WriteMemory)(void* dest, const void* src, unsigned size);

	// get the address of a function through starfield's import address table
	FUNC_PTR (*GetProcAddressFromIAT)
	(const char* dll_name, const char* func_name);

	// hook a function in the Import Address Table of starfield
	FUNC_PTR (*HookFunctionIAT)
	(const char* dll_name, const char* func_name, const FUNC_PTR new_function);

	// !EXPERIMENTAL API! AOB scan the exe memory and return the first match
	void* (*AOBScanEXE)(const char* signature);

#	ifdef BETTERAPI_DEVELOPMENT_FEATURES
	// build a wall so high that nobody can see the top from the highest ladder
	// https://devblogs.microsoft.com/oldnewthing/20110310-00/?p=11253
	void** (*LiterallyReplaceEntireVtable)(void*** ppClassInstance, uint32_t method_count);
#	endif  // BETTERAPI_DEVELOPMENT_FEATURES
};

///////////////////////////////////////////////////////////////////////////////
//                 16) Graphics API
///////////////////////////////////////////////////////////////////////////////

// This API allows you to create a basic mod menu without linking to imgui.
// if you play your cards right, you can write an entire mod without any other
// headers or libraries... not even the standard library!
struct simple_draw_t
{
	// Draw a separator line that fills the horizontal space
	void (*Separator)(void);

	// Draw some text, supports format specifiers
	void (*Text)(const char* fmt, ...);

	// Draw a button, returns true if the button was clicked
	// Example:
	// if (SimpleDraw->Button("Hello")) { /* do something on click */ }
	bool (*Button)(const char* name);

	// Draw a checkbox, this widget needs to store persistent state
	// usually you would pass a static bool as a pointer
	// returns true if the state was changed this frame
	// Example:
	// static bool state = false;
	// if (SimpleDraw->Checkbox("Hello", &state)) { /* do something on state change */ }
	bool (*Checkbox)(const char* name, bool* state);

	// Draw a text input widget for a single line of text
	// `buffer` will be used to store the text the user types
	// `true_on_enter` determines if the widget returns true on every character
	//                 typed or only when enter is pressed
	// Example:
	// static char buffer[32];
	// if (SimpleDraw->InputText("Hello", buffer, sizeof(buffer), true)) { /* do something on enter */ }
	bool (*InputText)(const char* name, char* buffer, uint32_t buffer_size, bool true_on_enter);

	// Split the remainaing window space into a left and right region.
	// the left region occupies a `left_size` (eg: .5) fraction of the screen
	// the remaining space is reserved for the HBoxRight() side
	// `min_size_em` is a safety minimum number of charactes wide that the
	// HBoxLeft region can be and overrides the `left_size` if it is smaller.
	// If you call HBoxLeft, you must also call HBoxRight and HBoxEnd
	// HBox and VBox regions can be nested for more complex layouts but
	// are more performance intensive than most other SimpleDraw functions.
	void (*HBoxLeft)(float left_size, float min_size_em);

	// Move to the right side region after a call to HBoxLeft
	// no argument needed, the size of the region is the remaining
	// space not taken up by HBoxLeft
	void (*HBoxRight)();

	// Finish the HBoxLeft and HBoxRight regions
	// no argument needed, this signals the end of the region
	// and resumes normal layout
	void (*HBoxEnd)();

	// This is the same as hbox, but for vertically separated spaces
	void (*VboxTop)(float top_size, float min_size_em);
	void (*VBoxBottom)();
	void (*VBoxEnd)();

	// Display an int number editor as a draggable slider that
	// can be double-clicked to edit the value directly.
	// `min` and `max` define the range of the *value
	// the return value is true if the value was changed this frame
	bool (*DragInt)(const char* name, int* value, int min, int max);

	// Follows the same rules as the sliderint above
	bool (*DragFloat)(const char* name, float* value, float min, float max);

	// Use the remaining vertical space to render the logbuffer
	// `handle` is the logbuffer handle to render
	// `scroll_to_bottom` if true, force the logbuffer region to scroll
	//                    from its current position to the end (newest data)
	void (*ShowLogBuffer)(LogBufferHandle handle, bool scroll_to_bottom);

	// Use the remaining vertical space to render a filtered logbuffer
	// `handle` is the logbuffer handle to render
	// `lines` is an array of line numbers to display
	// `line_count` is the number of lines to display from the `lines` array
	// `scroll_to_bottom` if true, force the logbuffer region to scroll
	//                    from its current position to the end (newest data)
	void (*ShowFilteredLogBuffer)(LogBufferHandle handle, const uint32_t* lines, uint32_t line_count, bool scroll_to_bottom);

	// Show a selectable list of text items.
	// returns true when the user click on an item and the selection changes.
	// `selection` is the index of the selected item and should be a static variable
	// `userdata` is any arbitrary data that you want to pass to the callback
	// `count` is the number of items in the list
	// `to_string` is a function that converts an index and userdata into a string
	//             an is called for each *visible* item in the list every frame.
	// Proper clipping of non-visible items is done automatically so large lists
	// can be drawn in a performant manner.
	// Usually you would want to contain this in an HBox or VBox otherwise
	// it will take up all remaining horizontal and vertical space.
	bool (*SelectionList)(uint32_t* selection, const void* userdata, uint32_t count, CALLBACK_SELECTIONLIST to_string);

	// Draw a table one cell at a time through the `draw_cell` callback
	// `header_labels` is an array of strings that are the column headers
	// `header_count` is the number of strings in the `header_labels` array
	//                this is assumed to be the same as the count of columns
	// `table_userdata` is any arbitrary data that you want to pass to the callback
	// `row_count` is the number of rows in the table, the total number of cells in
	//             the table is row_count * header_count
	// `draw_cell` is a callback that is called for each *visible* cell in the table
	// Proper vertical and horizontal clipping is done to accelerate even very large tables
	void (*Table)(const char* const* const header_labels, uint32_t header_count, uintptr_t table_userdata, uint32_t row_count, CALLBACK_TABLE draw_cell);

	// draw tabs that can be switched between
	void (*TabBar)(const char* const* const headers, uint32_t header_count, int* state);

	// draw buttons in a row, similar to a toolbar
	// returns -1 if no button was pressed this frame (common case)
	// otherwise returns the index of the button pressed this frame
	// `labels` is an array of strings that are the button labels
	// `label_count` is the number of strings in the `labels` array
	//               this is assumed to be the count of buttons
	// if there is not enough horizontal space available the buttons
	// out of bounds will wrap to the next row.
	int (*ButtonBar)(const char* const* const labels, uint32_t label_count);

	// show a tooltip marker that looks like "(?)"
	// when the user hovers over the marker `text` will be shown
	// in a little popup
	void (*Tip)(const char* text);

	// place multiple simpledraw elements on the same line
	void (*SameLine)();

#	ifdef BETTERAPI_DEVELOPMENT_FEATURES
	// Draw a radiobutton, returns true if the button was selected this frame
	//
	// `selection_group` is the group this radiobutton belongs to.
	//                   use the address of a static variable here.
	//                   the value is the currently selected radiobutton.
	//
	// `current_id`      the id of the current button in a group.
	//                   set this to 0 every frame before drawing the
	//                   first button in a group. no need for a static variable
	bool (*RadioButton)(const char* text, uint32_t* selection_group, uint32_t* current_id);

	// Show a small button that invokes the "open" shell action on click
	//
	// `display_text` This is the text shown inside the button
	//
	// `url_or_path` This is the link or file path to open using the default
	//               handler (like a web browser or notepad)
	void (*LinkButton)(const char* display_text, const char* url_or_path);
#	endif
};

///////////////////////////////////////////////////////////////////////////////
//                 17) LogBuffer API
///////////////////////////////////////////////////////////////////////////////

// the logbuffer API is deprecated and will be changed in a future release
struct log_buffer_api_t
{
	// Create a handle, needed to identify the log buffer
	// name is the name of the logbuffer as it shows up in the UI
	// path is an optional path to a logfile to append with logs
	LogBufferHandle (*Create)(const char* name, const char* path);

	// Get the number of lines stored in the buffer
	uint32_t (*GetLineCount)(LogBufferHandle);

	// Retrieve a specific line for storage
	const char* (*GetLine)(LogBufferHandle, uint32_t line);

	// Log a line of text to the buffer
	void (*Append)(LogBufferHandle, const char* log_line);

	// Restore the log buffer from a file, the file is then used for appending
	LogBufferHandle (*Restore)(const char* name, const char* filename);

#	if V(1, 4, 1)
	// Clear the log buffer
	void (*Clear)(LogBufferHandle handle);
#	endif
};

///////////////////////////////////////////////////////////////////////////////
//                 18) C Library API
///////////////////////////////////////////////////////////////////////////////

// this api is just a small selection of the most commonly needed standard library functions
// using this api means that your plugin might not need to link with the standard library at all
struct std_api_t
{
	// allocate `bytes` of ram, return NULL on error
	void* (*malloc)(size_t bytes);

	// free a previously allocated block of ram
	void (*free)(void* malloc_buffer);

	// printf style formatting into `buffer` of `buffer_size` bytes, returns <0 on error or bytes used
	int (*snprintf)(char* buffer, size_t buffer_size, const char* fmt, ...);

	// copy `bytes` of memory from `src` to `dest`, return dest
	void* (*memcpy)(void* dest, const void* src, size_t bytes);

	// set `bytes` of memory in `dest` to `value`, return dest, value must be <256
	void* (*memset)(void* dest, int value, size_t bytes);
};

///////////////////////////////////////////////////////////////////////////////
//                 19) Game Console API
///////////////////////////////////////////////////////////////////////////////

struct console_api_t
{
	// run `command` on the in-game console
	// max length of `command` is limited to 512 bytes by the game
	// NOTE: the game wont run console commands until the main menu
	//       has finished loading otherwise it freezes waiting
	//       for the console to be ready (if your compiling shaders
	//       the game could seem frozen for several minutes)
	// there should be a way to know when the console is ready, but
	// as I dont link with sfse, addresslibrary, commonlibsf or anything
	// I have to figure it out myself. Maybe see when startingconsolecommand
	// is run?

	// Version 1.4.1 - copy command to internal buffer to take const char*
	//                 and string literals, not API breaking for old plugins
	//                 running on newer betterconsole
#	if V(1, 4, 1)
	void (*RunCommand)(const char* command);
#	else
	void (*RunCommand)(char* command);
#	endif

#	if BETTERAPI_DEVELOPMENT_FEATURES
	// Pause or unpause the game
	//
	// `paused` true to pause the game
	//          false to unpause
	void (*SetGamePaused)(bool paused);
#	endif
};

#	if BETTERAPI_DEVELOPMENT_FEATURES
// 1.4.1 - this api is experimental
///////////////////////////////////////////////////////////////////////////////
//                 20) Wrapped Windows API
///////////////////////////////////////////////////////////////////////////////
// this api is just a small selection of commonly needed windows functions
// using this api means that your plugin might not need to link with the windows api at all
// this api is wrapped to only use types from the c standard library
// internally there is extensive error checking, assertions, and debug logging
// All string parameters are assumed to be utf-8 and functions taking string arguments
// are appended with "UTF" to avoid conflict with the macros defined by the windows header
// I/O is no wrapped by this API.
struct windows_api_t
{
	// returns false on error
	// VirtualProtect - see https://docs.microsoft.com/en-us/windows/win32/api/memoryapi/nf-memoryapi-virtualprotect
	bool (*VirtualProtect)(void* address, size_t size, uint32_t new_protect, uint32_t* old_protect);

	// returns NULL on error
	// GetModuleHandle - see https://docs.microsoft.com/en-us/windows/win32/api/libloaderapi/nf-libloaderapi-getmodulehandlea
	void* (*GetModuleHandleUTF)(const char* module_name);

	// returns false on error
	// ShellExecute - see https://docs.microsoft.com/en-us/windows/win32/api/shellapi/nf-shellapi-shellexecutea
	bool (*ShellExecuteUTF)(const char* type, const char* resource_path);

	// MessageBox - see https://docs.microsoft.com/en-us/windows/win32/api/winuser/nf-winuser-messageboxa
	void (*MessageBoxUTF)(const char* message, const char* window_title);

	// returns true if debugger is attached
	// IsDebuggerPresent - see https://docs.microsoft.com/en-us/windows/win32/api/dbghelp/nf-dbghelp-isdebuggerpresent
	bool (*IsDebuggerPresent)();

	// Sleep - see https://docs.microsoft.com/en-us/windows/win32/api/synchapi/nf-synchapi-sleep
	void (*Sleep)(uint32_t milliseconds);

	// returns NULL on error
	// GetProcAddress - see https://docs.microsoft.com/en-us/windows/win32/api/winbase/nf-winbase-getprocaddress
	void* (*GetProcAddress)(void* module_name, const char* function_name);
};
#	endif

#	if BETTERAPI_DEVELOPMENT_FEATURES
///////////////////////////////////////////////////////////////////////////////
//                 21) File I/O API
///////////////////////////////////////////////////////////////////////////////
struct file_io_api_t
{
	// create a directory if it does not exist
	// returns false on error
	// `directory_name` is the name of the directory
	// `relative_dir` is one of the following:
	//      NULL - `directory_name` is an absolute path
	//      "Starfield" - `directory_name` is relative to the folder containing starfield.exe
	//      "BetterConsole" - `directory_name` is relative to the folder containing BetterConsole
	bool (*CreateDirectory)(const char* relative_dir, const char* directory_name);

	// open a file
	// returns NULL on error or a file handle (as void*) on success
	// `relative_dir` is one of the following:
	//      NULL - `filename` is an absolute path
	//      "Starfield" - `filename` is relative to the folder containing starfield.exe
	//      "BetterConsole" - `filename` is relative to the folder containing BetterConsole
	// `mode` is one of the following:
	//      'r' - the file is opened read only
	//      'w' - the file is opened for reading and writing (the file is truncated if it already exists)
	//      'a' - the file is opened for reading and writing (the file is appended if it already exists)
	void* (*Open)(const char* relative_dir, const char* filename, char mode);

	// close a file
	// returns false on error
	// `file` is a file handle (as void*)
	bool (*Close)(void* file);

	// get the size of a file
	// returns false on error
	// `file` is a file handle (as void*)
	// `size` is a pointer to a uint64_t that receives the file size
	bool (*Size)(void* file_handle, uint64_t* size);

	// seek to an offset in a file
	// returns false on error
	// `file` is a file handle (as void*)
	// `offset` is the offset to seek to
	bool (*Seek)(void* file_handle, int64_t offset);

	// tell the current offset in a file
	// returns false on error
	// `file` is a file handle (as void*)
	// `offset` is a pointer to a uint64_t that receives the offset
	bool (*Tell)(void* file_handle, uint64_t* offset);

	// write to a file
	// returns false on error
	// `file` is a file handle (as void*)
	// `buffer` is a pointer to the data to write
	// `size` is the number of bytes to write
	bool (*Write)(void* file_handle, const void* buffer, uint32_t size);

	// read from a file
	// returns false on error
	// `file` is a file handle (as void*)
	// `buffer` is a pointer to the data to read
	// `size` is the number of bytes to read
	bool (*Read)(void* file_handle, void* buffer, uint32_t size);

	// flush any unwritten data to disk
	// returns false on error
	// `file` is a file handle (as void*)
	bool (*Flush)(void* file_handle);

	// formatted write to a file
	// returns false on error
	// `file` is a file handle (as void*)
	// `format` is a printf style format string
	// NOTE: the internal formatting buffer is 1024 bytes,
	// attempting to write more than that will result in an error
	bool (*Print)(void* file_handle, const char* format, ...);
};
#	endif

///////////////////////////////////////////////////////////////////////////////
//                 22) BetterConsole API
///////////////////////////////////////////////////////////////////////////////

// This is all the above structs wrapped up in one place
// why pointers to apis instead of the api itself? so that
// I may extend any api without changing the size of BetterAPI
// this helps with forwards compatibility and i wont need to
// update the BETTERAPI_VERSION as often.
typedef struct better_api_t
{
	const struct hook_api_t*       Hook;
	const struct log_buffer_api_t* LogBuffer;
	const struct simple_draw_t*    SimpleDraw;
	const struct callback_api_t*   Callback;
	const struct config_api_t*     Config;
	const struct std_api_t*        Stdlib;
	const struct console_api_t*    Console;
#	if BETTERAPI_DEVELOPMENT_FEATURES
	const struct windows_api_t* Windows;
	const struct file_io_api_t* FileIO;
#	endif
} BetterAPI;

// Open to feedback:
// in the future, instead of having one version of the api
// and one version of the betterapi struct, pass in a
// service locator and resolve dependencies on a more
// granular level that does not rely on struct layout
#	if BETTERAPI_VERSION > 1
enum ServiceType
{
	ServiceType_Callback,
	ServiceType_Hook,
	// etc...
};

// Get a pointer to an api struct.
// This allows betterconsole to return different versions of the api
//      depending on mod compatibility.
typedef const void* (*BetterServiceLocator)(enum ServiceType type, uint32_t version);

//then instead of providing a betterapi pointer, only send the locator and version data:
DLLEXPORT int BetterConsoleReceiver2(BetterServiceLocator locator, uint32_t betterconsole_api_level)
{
	static better_api_t api;
	api.Callback = locator(ServiceType_Callback, 0);  //something like this?
	// we could have #defines for BETTERAPI_ENABLE_CONFIG or something
	// so you only enable the parts of the api that you use, less chance of
	// breaking compatibility if we only incrementally update unpopular APIs
	return OnBetterConsoleLoad(&api);
}
#	endif

// Dont keep single letter macros around outside this file
#	ifdef V
#		undef V
#	endif  // V

#endif  // !BETTERAPI_API_H

///////////////////////////////////////////////////////////////////////////////
//                 23) SFSE Minimal Interface
///////////////////////////////////////////////////////////////////////////////

// For people that want to port Cheat Engine or ASI mods to sfse without including
// sfse code into the project, define BETTERAPI_ENABLE_SFSE_MINIMAL before
// including betterapi.h, this should get you 90% of the way to a
// plug and play sfse plugin with no brain required. Take a look at the example
// plugin for an example of how to do this.

#ifdef BETTERAPI_ENABLE_SFSE_MINIMAL
#	ifndef BETTERAPI_SFSE_MINIMAL
#		define BETTERAPI_SFSE_MINIMAL

typedef uint32_t PluginHandle;

typedef enum SFSEEnumeration_t
{
	InterfaceID_Invalid = 0,
	InterfaceID_Messaging,
	InterfaceID_Trampoline,

	MessageType_SFSE_PostLoad = 0,
	MessageType_SFSE_PostPostLoad,
} InterfaceID;

typedef struct SFSEPluginVersionData_t
{
	uint32_t dataVersion;
	uint32_t pluginVersion;
	char     name[256];
	char     author[256];
	uint32_t addressIndependence;
	uint32_t structureIndependence;
	uint32_t compatibleVersions[16];
	uint32_t seVersionRequired;
	uint32_t reservedNonBreaking;
	uint32_t reservedBreaking;
} SFSEPluginVersionData;

typedef struct SFSEPluginInfo_t
{
	uint32_t    infoVersion;
	const char* name;
	uint32_t    version;
} SFSEPluginInfo;

typedef struct SFSEInterface_t
{
	uint32_t sfseVersion;
	uint32_t runtimeVersion;
	uint32_t interfaceVersion;
	void* (*QueryInterface)(uint32_t id);
	PluginHandle (*GetPluginHandle)(void);
	SFSEPluginInfo* (*GetPluginInfo)(const char* name);
} SFSEInterface;

typedef struct SFSEMessage_t
{
	const char* sender;
	uint32_t    type;
	uint32_t    dataLen;
	void*       data;
} SFSEMessage;

typedef void (*SFSEMessageEventCallback)(SFSEMessage* msg);

typedef struct SFSEMessagingInterface_t
{
	uint32_t interfaceVersion;
	bool (*RegisterListener)(PluginHandle listener, const char* sender, SFSEMessageEventCallback handler);
	bool (*Dispatch)(PluginHandle sender, uint32_t messageType, void* data, uint32_t dataLen, const char* receiver);
} SFSEMessagingInterface;

#	endif  // !BETTERAPI_SFSE_MINIMAL
#endif

#ifdef BETTERAPI_IMPLEMENTATION
#	undef BETTERAPI_IMPLEMENTATION
#	ifdef __cplusplus
#		define DLLEXPORT extern "C" __declspec(dllexport)
#	else
#		define DLLEXPORT __declspec(dllexport)
#	endif  // __cplusplus
static int OnBetterConsoleLoad(const struct better_api_t* betterapi);
// In a future API version we need to return a value from this function
// it makes sense to know and log if a mod can't load
DLLEXPORT void BetterConsoleReceiver(const struct better_api_t* api)
{
	OnBetterConsoleLoad(api);
}
#endif  // BETTERAPI_IMPLEMENTATION