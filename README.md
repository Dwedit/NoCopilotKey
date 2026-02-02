# NoCopilotKey
A tiny program that changes the Copilot keyboard key back into the right Ctrl key.

# Usage
Download the release from the [Releases page](/releases/).

Extract all files from the ZIP to the directory you want to keep the program in.

Run "Install NoCopilotKey.bat" to create a Start Menu entry for Startup, this will make the program automatically run.

When you run the program, you won't see anything happen, but it is automatically changing the Copilot key into the right ctrl key.  To close the program, use Task Manager to end NoCopilotKey.

# Why?
Because Microsoft required manufacturers to replace the right Ctrl key with a Copilot key, with no BIOS or Windows setting available to change it back.

Some people actually do use their keyboards, and need a right Ctrl key.  Things I frequently do with the right Ctrl key include:
 * Ctrl + Left/Right to move the cursor between words
 * Ctrl + Shift + Left/Right to select text one word at a time.
 * Ctrl + Enter to add a line to a text box where normal Enter would close the dialog  (Or add a non-paragraph linebreak to a word processor)
 * Ctrl + L to move to the Location bar in a web browser
 * Ctrl + Home/End to move the cursor to the beginning or end of a document
 * Ctrl + P to print
 * Ctrl + N for a new browser window
 * Ctrl + +/- and Ctrl + 0 to control zooming in a web browser
 * Ctrl + [ / ] to move between matching braces in a code editor

# How it works

Pressing the Copilot key acts as pressing keys in this order: Left Windows Key, Left Shift, then F23 (a key not normally found on keyboards).

Releasing the Copilot key is a release of F23, Left Shift, then Left Windows Key in that order.

The function `SetWindowsHookEx` allows a program to install a low-level keyboard hook.  This allows a program to accept or reject key presses for the whole system.

In addition to a keyboard hook accepting or rejecting key presses, the function `SendInput` can synthesize keys being pressed and released.

## Detecting the three-key sequence vs someone using the normal keys:

The program uses simple rules to detect the Copilot key vs. normal use of the Left Windows and Left Shift keys.

When you press Left Windows Key:
 * Left Windows Keystroke is blocked
 * If any of these happens, your keystroke to Left Windows key is replayed:
   * Releasing any key
   * Pressing any key that's not Left Shift
   * 0.1 seconds elapses
If Left Shift becomes pressed after Left Windows Key:
 * Left Shift Keystroke is blocked
 * If any of these happens, your keystrokes to Left Windows Key and Left Shift are replayed:
   * Releasing any key
   * Pressing any key that's not F23
   * 0.1 seconds elapses since Left Windows Key was pressed
If F23 becomes pressed after Left Windows and Left Shift:
 * Keystroke is blocked
 * Right Ctrl becomes pressed (via `SendInput`) unless Right Ctrl is already held down.

Because keystrokes to Left Windows and Left Shift are replayed very quickly, you won't even notice that they were blocked.

Then when you release the Copilot key:
 * Key release of F23 is blocked
 * Right Ctrl becomes released and no longer pressed (via `SendInput`)
 * After F23 key is released, the next key release of Left Shift is blocked (not your real Left Shift key)
 * After Left Shift key is released, the next key release of Left Windows Key is blocked (not your real Left Windows key)
 * After Left Windows key is released, it's done handling the complete keystroke.

# Version History

* 1.0.0.3
 * Added feature to create Startup entry (This is hard to do yourself on Windows 11)
 * Fixed Game Bar appearing if you tapped and released Left Windows Key then pressed G at any time afterwards
 * Code cleanup and changing code for running other hooks
* 1.0.0.2
 * Changed code for running other hooks
* 1.0.0.1
 * Initial Release
