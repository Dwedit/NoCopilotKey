#pragma once

//Test mode: Make backquote act as copilot key  (for testing on a keyboard which doesn't have a copilot key)
//Also allows testing invalid sequences
#define TEST 0

//Debug mode: adds a console and logging (enabled for debug builds)
#define DEBUG _DEBUG

//Whether to try to handle invalid key sequences
#define HANDLE_INVALID 1

//Whether to include Alt + Ctrl + Del code (Smart App Control doesn't like it)
#define USE_SAS 0

//Whether to make a build which does not interfere with the keyboard and only displays debug log messages
#define DO_NOTHING 0

//Whether to reinstall the keyboard hook every 1 second (to override programs which install their own key hooks, such as Remote Desktop Connection)
#define REINSTALL_HOOK 1

//Whether to use Raw Input to look for keys that got past the keyboard hook (useless)
#define USE_RAW_INPUT 0

//Whether to watch the active window
#define WATCH_ACTIVE_WINDOW 1

//Whether to use the registry to remap F23
#define ENABLE_REGISTRY_REMAPPING 1

//Whether to allow custom keys
#define ENABLE_CUSTOM_KEY 1
