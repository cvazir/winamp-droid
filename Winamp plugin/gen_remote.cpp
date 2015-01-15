/*

Winamp generic plugin template code.
This code should be just the basics needed to get a plugin up and running.
You can then expand the code to build your own plugin.

Updated details compiled June 2009 by culix, based on the excellent code examples
and advice of forum members Kaboon, kichik, baafie, burek021, and bananskib.
Thanks for the help everyone!

*/

#include "stdafx.h"
#include <windows.h>
#include "gen_remote.h"
#include <stdio.h>
#include "CRemote.h"



// these are callback functions/events which will be called by Winamp
int  init(void);
void config(void);
void quit(void);





// this structure contains plugin information, version, name...
// GPPHDR_VER is the version of the winampGeneralPurposePlugin (GPP) structure
winampGeneralPurposePlugin plugin = {
	GPPHDR_VER,  // version of the plugin, defined in "gen_myplugin.h"
	PLUGIN_NAME, // name/title of the plugin, defined in "gen_myplugin.h"
	init,        // function name which will be executed on init event
	config,      // function name which will be executed on config event
	quit,        // function name which will be executed on quit event
	0,           // handle to Winamp main window, loaded by winamp when this dll is loaded
	0            // hinstance to this dll, loaded by winamp when this dll is loaded
};
CRemote *c;


// event functions follow

int init() {
	//MessageBox(plugin.hwndParent, L"Init event triggered for gen_myplugin. Plugin installed successfully!", L"", MB_OK);

	c = new CRemote{ plugin };

	
	return 0;
}

void config() {
	//A basic messagebox that tells you the 'config' event has been triggered.
	//You can change this later to do whatever you want (including nothing)
	//MessageBox(plugin.hwndParent, L"Config event triggered for gen_myplugin.", L"", MB_OK);
	NDEReader n(L"C:/Users/vlad/AppData/Roaming/Winamp/Plugins/ml/main.dat", L"C:/Users/vlad/AppData/Roaming/Winamp/Plugins/ml/main.idx");

	library l = n.load();
}

void quit() {
	//A basic messagebox that tells you the 'quit' event has been triggered.
	//If everything works you should see this message when you quit Winamp once your plugin has been installed.
	//You can change this later to do whatever you want (including nothing)
	//MessageBox(0, L"Quit event triggered for gen_myplugin.", L"", MB_OK);

	delete c;
	return;
}

// This is an export function called by winamp which returns this plugin info.
// We wrap the code in 'extern "C"' to ensure the export isn't mangled if used in a CPP file.
extern "C" __declspec(dllexport) winampGeneralPurposePlugin * winampGetGeneralPurposePlugin() {
	return &plugin;
}