using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace NoCopilotKey_Installer
{
    public partial class KeySelectionForm : Form
    {
        public static string GetFriendlyNameForVKey(string vkey)
        {
            int i = Array.IndexOf(vkeyTable, vkey);
            if (i >= 0 && i < vkeyFriendlyNames.Length)
            {
                string friendlyName = vkeyFriendlyNames[i];
                if (!String.IsNullOrEmpty(friendlyName))
                {
                    return friendlyName;
                }
            }
            return vkey;
        }

        public static string GetVKeyName(uint vkey)
        {
            if (vkey >= 0 && vkey < vkeyTable.Length)
            {
                return vkeyTable[vkey];
            }
            return "";
        }

        public string SelectedVKey
        {
            get
            {
                var selectedItem = this.listBox1.SelectedItem as MyListItem;
                if (selectedItem == null) return "";
                return selectedItem.VKey;
            }
            set
            {
                for (int i = 0; i < this.listBox1.Items.Count; i++)
                {
                    var item = this.listBox1.Items[i] as MyListItem;
                    if (item != null)
                    {
                        if (item.VKey.Equals(value, StringComparison.OrdinalIgnoreCase))
                        {
                            this.listBox1.SelectedIndex = i;
                            return;
                        }
                    }
                }
            }
        }
        public string SelectedFriendlyName
        {
            get
            {
                var selectedItem = this.listBox1.SelectedItem as MyListItem;
                if (selectedItem == null) return "";
                return selectedItem.FriendlyName;
            }
        }

        public KeySelectionForm()
        {
            InitializeComponent();
            InitializeListItems();
        }

        private void KeySelectionForm_Load(object sender, EventArgs e)
        {

        }

        public static uint VKeyNameToVKey(string customVKey)
        {
            for (int i = 0; i < vkeyTable.Length; i++)
            {
                if (String.Equals(vkeyTable[i], customVKey, StringComparison.OrdinalIgnoreCase))
                {
                    return (uint)i;
                }
            }
            return 0;
        }

        class MyListItem
        {
            public int Index;
            public string VKey;
            public string FriendlyName;
            public override string ToString()
            {
                return FriendlyName;
            }
        }

        private void InitializeListItems()
        {
            List<MyListItem> items = new List<MyListItem>();
            items.Add(CreateListItem(0xA3)); //Right Ctrl
            items.Add(CreateListItem(0x5D)); //Application (Menu key)
            items.Add(CreateListItem(0x5C)); //Right Windows
            for (int i = 0; i < vkeyTable.Length; i++)
            {
                var listItem = CreateListItem(i);
                if (!String.IsNullOrEmpty(listItem.FriendlyName))
                {
                    items.Add(listItem);
                }
            }
            this.listBox1.BeginUpdate();
            this.listBox1.Items.AddRange(items.ToArray());
            this.listBox1.EndUpdate();
        }

        static MyListItem CreateListItem(int i)
        {
            MyListItem listItem = new MyListItem();
            listItem.Index = i;
            if (i >= 0 && i < vkeyTable.Length)
            {
                listItem.VKey = vkeyTable[i];
            }
            if (i >= 0 && i < vkeyFriendlyNames.Length)
            {
                listItem.FriendlyName = vkeyFriendlyNames[i];
            }
            return listItem;
        }

        static readonly string[] vkeyTable = new string[]
        {
            "",
            "VK_LBUTTON",
            "VK_RBUTTON",
            "VK_CANCEL",
            "VK_MBUTTON",
            "VK_XBUTTON1",
            "VK_XBUTTON2",
            "",
            "VK_BACK",
            "VK_TAB",
            "",
            "",
            "VK_CLEAR",
            "VK_RETURN",
            "",
            "",
            "VK_SHIFT",
            "VK_CONTROL",
            "VK_MENU",
            "VK_PAUSE",
            "VK_CAPITAL",
            "VK_KANA",
            "VK_IME_ON",
            "VK_JUNJA",
            "VK_FINAL",
            "VK_KANJI",
            "VK_IME_OFF",
            "VK_ESCAPE",
            "VK_CONVERT",
            "VK_NONCONVERT",
            "VK_ACCEPT",
            "VK_MODECHANGE",
            "VK_SPACE",
            "VK_PRIOR",
            "VK_NEXT",
            "VK_END",
            "VK_HOME",
            "VK_LEFT",
            "VK_UP",
            "VK_RIGHT",
            "VK_DOWN",
            "VK_SELECT",
            "VK_PRINT",
            "VK_EXECUTE",
            "VK_SNAPSHOT",
            "VK_INSERT",
            "VK_DELETE",
            "VK_HELP",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "A",
            "B",
            "C",
            "D",
            "E",
            "F",
            "G",
            "H",
            "I",
            "J",
            "K",
            "L",
            "M",
            "N",
            "O",
            "P",
            "Q",
            "R",
            "S",
            "T",
            "U",
            "V",
            "W",
            "X",
            "Y",
            "Z",
            "VK_LWIN",
            "VK_RWIN",
            "VK_APPS",
            "",
            "VK_SLEEP",
            "VK_NUMPAD0",
            "VK_NUMPAD1",
            "VK_NUMPAD2",
            "VK_NUMPAD3",
            "VK_NUMPAD4",
            "VK_NUMPAD5",
            "VK_NUMPAD6",
            "VK_NUMPAD7",
            "VK_NUMPAD8",
            "VK_NUMPAD9",
            "VK_MULTIPLY",
            "VK_ADD",
            "VK_SEPARATOR",
            "VK_SUBTRACT",
            "VK_DECIMAL",
            "VK_DIVIDE",
            "VK_F1",
            "VK_F2",
            "VK_F3",
            "VK_F4",
            "VK_F5",
            "VK_F6",
            "VK_F7",
            "VK_F8",
            "VK_F9",
            "VK_F10",
            "VK_F11",
            "VK_F12",
            "VK_F13",
            "VK_F14",
            "VK_F15",
            "VK_F16",
            "VK_F17",
            "VK_F18",
            "VK_F19",
            "VK_F20",
            "VK_F21",
            "VK_F22",
            "VK_F23",
            "VK_F24",
            "VK_NAVIGATION_VIEW",
            "VK_NAVIGATION_MENU",
            "VK_NAVIGATION_UP",
            "VK_NAVIGATION_DOWN",
            "VK_NAVIGATION_LEFT",
            "VK_NAVIGATION_RIGHT",
            "VK_NAVIGATION_ACCEPT",
            "VK_NAVIGATION_CANCEL",
            "VK_NUMLOCK",
            "VK_SCROLL",
            "VK_OEM_FJ_JISHO",
            "VK_OEM_FJ_MASSHOU",
            "VK_OEM_FJ_TOUROKU",
            "VK_OEM_FJ_LOYA",
            "VK_OEM_FJ_ROYA",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "VK_LSHIFT",
            "VK_RSHIFT",
            "VK_LCONTROL",
            "VK_RCONTROL",
            "VK_LMENU",
            "VK_RMENU",
            "VK_BROWSER_BACK",
            "VK_BROWSER_FORWARD",
            "VK_BROWSER_REFRESH",
            "VK_BROWSER_STOP",
            "VK_BROWSER_SEARCH",
            "VK_BROWSER_FAVORITES",
            "VK_BROWSER_HOME",
            "VK_VOLUME_MUTE",
            "VK_VOLUME_DOWN",
            "VK_VOLUME_UP",
            "VK_MEDIA_NEXT_TRACK",
            "VK_MEDIA_PREV_TRACK",
            "VK_MEDIA_STOP",
            "VK_MEDIA_PLAY_PAUSE",
            "VK_LAUNCH_MAIL",
            "VK_LAUNCH_MEDIA_SELECT",
            "VK_LAUNCH_APP1",
            "VK_LAUNCH_APP2",
            "",
            "",
            "VK_OEM_1",
            "VK_OEM_PLUS",
            "VK_OEM_COMMA",
            "VK_OEM_MINUS",
            "VK_OEM_PERIOD",
            "VK_OEM_2",
            "VK_OEM_3",
            "",
            "",
            "VK_GAMEPAD_A",
            "VK_GAMEPAD_B",
            "VK_GAMEPAD_X",
            "VK_GAMEPAD_Y",
            "VK_GAMEPAD_RIGHT_SHOULDER",
            "VK_GAMEPAD_LEFT_SHOULDER",
            "VK_GAMEPAD_LEFT_TRIGGER",
            "VK_GAMEPAD_RIGHT_TRIGGER",
            "VK_GAMEPAD_DPAD_UP",
            "VK_GAMEPAD_DPAD_DOWN",
            "VK_GAMEPAD_DPAD_LEFT",
            "VK_GAMEPAD_DPAD_RIGHT",
            "VK_GAMEPAD_MENU",
            "VK_GAMEPAD_VIEW",
            "VK_GAMEPAD_LEFT_THUMBSTICK_BUTTON",
            "VK_GAMEPAD_RIGHT_THUMBSTICK_BUTTON",
            "VK_GAMEPAD_LEFT_THUMBSTICK_UP",
            "VK_GAMEPAD_LEFT_THUMBSTICK_DOWN",
            "VK_GAMEPAD_LEFT_THUMBSTICK_RIGHT",
            "VK_GAMEPAD_LEFT_THUMBSTICK_LEFT",
            "VK_GAMEPAD_RIGHT_THUMBSTICK_UP",
            "VK_GAMEPAD_RIGHT_THUMBSTICK_DOWN",
            "VK_GAMEPAD_RIGHT_THUMBSTICK_RIGHT",
            "VK_GAMEPAD_RIGHT_THUMBSTICK_LEFT",
            "VK_OEM_4",
            "VK_OEM_5",
            "VK_OEM_6",
            "VK_OEM_7",
            "VK_OEM_8",
            "",
            "VK_OEM_AX",
            "VK_OEM_102",
            "VK_ICO_HELP",
            "VK_ICO_00",
            "VK_PROCESSKEY",
            "VK_ICO_CLEAR",
            "VK_PACKET",
            "",
            "VK_OEM_RESET",
            "VK_OEM_JUMP",
            "VK_OEM_PA1",
            "VK_OEM_PA2",
            "VK_OEM_PA3",
            "VK_OEM_WSCTRL",
            "VK_OEM_CUSEL",
            "VK_OEM_ATTN",
            "VK_OEM_FINISH",
            "VK_OEM_COPY",
            "VK_OEM_AUTO",
            "VK_OEM_ENLW",
            "VK_OEM_BACKTAB",
            "VK_ATTN",
            "VK_CRSEL",
            "VK_EXSEL",
            "VK_EREOF",
            "VK_PLAY",
            "VK_ZOOM",
            "VK_NONAME",
            "VK_PA1",
            "VK_OEM_CLEAR",
            "",
        };

        static readonly string[] vkeyFriendlyNames = new string[]
        {
            "",
            "",
            "",
            "Break",
            "",
            "",
            "",
            "",
            "Backspace",
            "Tab",
            "",
            "",
            "Num 5 (without num lock)",
            "Enter",
            "",
            "",
            "Shift",
            "Ctrl",
            "Alt",
            "Pause",
            "Caps Lock",
            "",
            "",
            "",
            "",
            "",
            "",
            "Esc",
            "",
            "",
            "",
            "",
            "Space",
            "Page Up",
            "Page Down",
            "End",
            "Home",
            "Left",
            "Up",
            "Right",
            "Down",
            "",
            "",
            "",
            "Sys Req",
            "Insert",
            "Delete",
            "Help",
            "0",
            "1",
            "2",
            "3",
            "4",
            "5",
            "6",
            "7",
            "8",
            "9",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "A",
            "B",
            "C",
            "D",
            "E",
            "F",
            "G",
            "H",
            "I",
            "J",
            "K",
            "L",
            "M",
            "N",
            "O",
            "P",
            "Q",
            "R",
            "S",
            "T",
            "U",
            "V",
            "W",
            "X",
            "Y",
            "Z",
            "Left Windows",
            "Right Windows",
            "Application (Menu key)",
            "",
            "Sleep",
            "Num 0",
            "Num 1",
            "Num 2",
            "Num 3",
            "Num 4",
            "Num 5",
            "Num 6",
            "Num 7",
            "Num 8",
            "Num 9",
            "Num *",
            "Num +",
            "",
            "Num -",
            "Num Del",
            "Num /",
            "F1",
            "F2",
            "F3",
            "F4",
            "F5",
            "F6",
            "F7",
            "F8",
            "F9",
            "F10",
            "F11",
            "F12",
            "F13",
            "F14",
            "F15",
            "F16",
            "F17",
            "F18",
            "F19",
            "F20",
            "F21",
            "F22",
            "F23",
            "F24",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "Pause",
            "Scroll Lock",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "Left Shift",
            "Right Shift",
            "Left Ctrl",
            "Right Ctrl",
            "Left Alt",
            "Right Alt",
            "Browser Back",
            "Browser Forward",
            "Browser Refresh",
            "Browser Stop",
            "Broswer Search",
            "Browser Favorites",
            "Browser Home",
            "Volume Mute",
            "Volume Down",
            "Volume Up",
            "Media Next Track",
            "Media Previous Track",
            "Media Stop",
            "Media Play/Pause",
            "Launch Mail",
            "Launch Media Select",
            "Launch App 1",
            "Launch App 2",
            "",
            "",
            "; (semicolon)",
            "= (equals)",
            ", (comma)",
            "- (minus)",
            ". (period)",
            "/ (slash)",
            "` (backquote)",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "[ (open square brace)",
            "\\ (backslash)",
            "] (closed square brace)",
            "' (apostrophe)",
            "",
            "",
            "",
            "\\ (left backslash)",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "",
            "Backtab",
            "",
            "",
            "",
            "",
            "",
            "Zoom",
            "",
            "",
            "",
            "",
        };

        public static uint ScancodeToVKey(ushort scancode)
        {
            const uint VK_PAUSE = 0x13;
            const uint MAPVK_VSC_TO_VK_EX = 3;
            uint result = MapVirtualKeyExW(scancode, MAPVK_VSC_TO_VK_EX);
            if (scancode == 0x45)
            {
                return VK_PAUSE;
            }
            return result;
        }

        public static ushort VKeyToScancode(uint vkey, uint flags = 0)
        {
            const uint MAPVK_VK_TO_VSC_EX = 4;
            const uint VK_PRIOR = 0x33;
            const uint VK_DOWN = 0x28;
            const uint VK_INSERT = 0x2D;
            const uint VK_DELETE = 0x2E;
            const uint VK_NUMLOCK = 0x90;
            const uint VK_RSHIFT = 0xA1;
            const uint VK_RETURN = 0x0D;
            const uint VK_SNAPSHOT = 0x2C;
            const uint VK_PAUSE = 0x13;

            const uint NotExtendedKey = 4;
            const uint IsExtendedKey = 5;

            uint result = MapVirtualKeyExW(vkey, MAPVK_VK_TO_VSC_EX);
            if ((vkey >= VK_PRIOR && vkey <= VK_DOWN) ||
                vkey == VK_INSERT || vkey == VK_DELETE ||
                vkey == VK_NUMLOCK || vkey == VK_RSHIFT)
            {
                if (flags != NotExtendedKey)
                {
                    //Normal arrow keys, Page Up keys, etc (not numpad keys)
                    return (ushort)(result | 0xE000);
                }
                else
                {
                    //Numpad keys with num lock off
                    return (ushort)result;
                }
            }
            if (flags == IsExtendedKey)
            {
                //When extended key flag is set, it's Numpad Enter instead of regular enter.
                if (vkey == VK_RETURN)
                {
                    return (ushort)(result | 0xE000);
                }
                //When extended key flag is set, it's the sysreq key instead of the printscreen key
                if (vkey == VK_SNAPSHOT)
                {
                    return 0xE037;
                }
            }
            //VK_PAUSE has the wrong scancode for some reason?
            if (vkey == VK_PAUSE && (flags == NotExtendedKey))
            {
                return 0x45;
            }
            return (ushort)result;
        }

        [DllImport("user32.dll", CallingConvention = CallingConvention.Winapi, ExactSpelling = true)]
        static extern uint MapVirtualKeyExW(uint code, uint mapType, IntPtr localeHandle = default);
    }
}
