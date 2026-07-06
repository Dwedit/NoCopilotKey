# NoCopilotKey
A tiny program that changes the Copilot keyboard key back into the right Ctrl key.  Unlike other solutions, this one does not interfere with the Left Shift or Left Windows keys.

# Installation
Download the release from the [Releases page](https://github.com/Dwedit/NoCopilotKey/releases/latest).

Run "NoCopilotKey Installer.exe" to install the program.  After the program is installed, it will start running now, and also automatically run on startup or login.

To stop NoCopilotKey, use Task Manager to end the program, or uninstall the program.

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

## What exactly is the Copilot Key

The Copilot Key acts as Left Windows, Left Shift, then F23 in that order.  (The F23 key is not normally found on keyboards.)

When the Copilot Key is released, F23 is released first, then Left Shift, then Left Windows.

## The features provided by Windows

The function `SetWindowsHookEx` allows a program to install a global low-level keyboard hook.  Whenever a key is pressed (or released), every program with a global keyboard hook gets to see the keypress (or release), and the program will decide whether the key event should be blocked or allowed.  This API has been around since Windows 2000, and is normally used to create global hotkeys.

The function `SendInput` allows a program to inject keyboard input into the system.  This same functionality is used by the Windows On-Screen Keyboard.

By combining these features, we can create a key-remapping program that supports a key which presses multiple keys.

## Detecting the three-key sequence vs someone using the normal keys:

The copilot key quickly presses its three key sequence, but you could also be pressing the first two keys on your own keyboard.  You could also be pressing Left Shift while holding down Copilot, or pressing Copilot while holding down Left Shift.

In order to distinguish between the real keyboard keys and the Copilot Key, we follow a sequence:

[ Idle ] → [ Left Windows Key ] → [ Left Shift ] → [ F23 ]

We start at Idle.

Whenever we see a keypress for the correct key, we temporarily block the key, then advance to the next step.

These will break the sequence:
 * Other Key Press
 * Any Key Release
 * 0.1 second timer expires

When we see any of those, we know it's not the Copilot Key, and we take these actions:
 * Replay all temporarily blocked keys
 * Ensure the incoming keypress happens after the keys are replayed
 * Return to Idle
 * If the pressed key was Left Windows Key, handle it immediately.

Whenever we reach the Left Windows Key step, we start the 0.1 second timer.  This timer is cancelled whenever we return to idle.

When we reach F23, we have completed the sequence.  We take these actions:
 * Inject a Right Ctrl keypress.
 * Temporarily blocked keys remain blocked
 * Return to Idle
 * Sequence for releasing keys advances to F23 step (see below)

##Second sequence for releasing the Copilot Key

There is also another sequence when releasing the Copilot Key, but once you release the F23 key, the other two keys follow immediately.  This makes the sequence more simple, since it can't be interrupted.

[ Idle ] → [ F23 ] → [ Left Shift ] → [ Left Windows ] → [ Idle ]

When F23 is pressed (from the other sequence), we are now waiting for F23 to be released.  A key release is only blocked if we're at the correct step, so you can still release Left Shift as normal while holding down Copilot key.

 * F23 Released:
   * Inject Right Ctrl release
   * Block F23 release
 * Left Shift released:
   * Block left shift release
 * Left Windows released:
   * Block left windows release

# Version History

1.0.4.0
 * Includes scancode in injected keyboard actions.
 * You can pick a custom key instead of Right Ctrl.
 * Option to remap F23 using the registry.
 * Supports running on systems where the F23 key has been remapped.

1.0.3.1
 * When running as non-admin, held ctrl key could get stuck if you switched to an admin window before releasing the key.  Now held ctrl is automatically unstuck the next time you switch back to a non-admin window.
 * Fix installation on non-English systems

1.0.3.0
 * Now supports broken key sequences (such as Left Windows Key missing, or Left Shift pressed before Left Windows Key)
 * Keyboard hook reattaches every second to ensure that other applications (such as Remote Desktop Connection) do not override the hook

1.0.2.0
 * No longer calls SendInput from within the hook code, improving responsiveness
 * Quick presses of Windows Key and another key should now be in the correct order

1.0.1.2
 * Installer can now correctly close the program to upgrade it (forgot to wait for process to finish closing)
 * Installer no longer lets you install as both Administrator and regular user.
 * Other cleanup for installer

1.0.1.1
 * Fixed logic error in tracking the keyboard state

1.0.1.0
 * New separate installer can configure the program to run automatically as Admin, or install as a limited user.
 * Installer can also uninstall the program.
 * Removed shortcut code from the main exe.

1.0.0.4
 * Fixed startup entry

1.0.0.3
 * Added feature to create Startup entry (This is hard to do yourself on Windows 11)
 * Fixed Game Bar appearing if you tapped and released Left Windows Key then pressed G at any time afterwards
 * Code cleanup and changing code for running other hooks

1.0.0.2
 * Changed code for running other hooks

1.0.0.1
 * Initial Release
