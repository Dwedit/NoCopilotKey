using Microsoft.Win32;
using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.IO;
using System.Linq;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;

namespace NoCopilotKey_Installer
{
    [StructLayout(LayoutKind.Sequential)]
    struct KeyBindingsHeader
    {
        [DefaultValue(0)]
        public int HeaderVersion;
        [DefaultValue(0)]
        public int HeaderFlags;
        public int NumberOfEntries;
        //Then there's an `int[]` for the remap entries, and `int terminator = 0`.
    }

    [StructLayout(LayoutKind.Sequential)]
    struct KeyBindingsEntry
    {
        public ushort SourceKey;
        public ushort DestinationKey;
        public int ToInt32()
        {
            return (int)((uint)SourceKey | ((uint)DestinationKey << 16));
        }
        public static KeyBindingsEntry[] FromByteArray(byte[] bytes, int startPosition, int byteCount)
        {
            if (byteCount + startPosition > bytes.Length || byteCount < 0 || startPosition < 0)
            {
                throw new ArgumentOutOfRangeException();
            }

            var arr = new KeyBindingsEntry[byteCount / 4];
            for (int i = 0; i < arr.Length; i++)
            {
                byte b0 = bytes[i * 4 + 0 + startPosition];
                byte b1 = bytes[i * 4 + 1 + startPosition];
                byte b2 = bytes[i * 4 + 2 + startPosition];
                byte b3 = bytes[i * 4 + 3 + startPosition];
                arr[i].SourceKey = (ushort)(b0 + (b1 << 8));
                arr[i].DestinationKey = (ushort)(b2 + (b3 << 8));
            }
            return arr;
        }
        public static void ToByteArray(KeyBindingsEntry[] arr, byte[] bytes, int startPosition)
        {
            if (startPosition + arr.Length * 4 > bytes.Length || startPosition < 0)
            {
                throw new ArgumentOutOfRangeException();
            }
            for (int i = 0; i < arr.Length; i++)
            {
                ushort a = arr[i].SourceKey;
                ushort b = arr[i].DestinationKey;
                bytes[i * 4 + 0 + startPosition] = (byte)(a & 0xFF);
                bytes[i * 4 + 1 + startPosition] = (byte)((a >> 8) & 0xFF);
                bytes[i * 4 + 2 + startPosition] = (byte)(b & 0xFF);
                bytes[i * 4 + 3 + startPosition] = (byte)((b >> 8) & 0xFF);
            }
        }
    }

    class KeyBindings
    {
        public static KeyBindingsEntry[] ReadFromRegistry()
        {
            try
            {
                using (var regKey = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Control\Keyboard Layout"))
                {
                    if (regKey == null) return null;
                    var bytes = regKey.GetValue("Scancode Map") as byte[];
                    if (bytes == null) return null;
                    var ms = new MemoryStream(bytes);
                    var br = new BinaryReader(ms);
                    int headerVersion = br.ReadInt32();
                    int headerFlags = br.ReadInt32();
                    int numberOfEntries = br.ReadInt32();
                    if (headerVersion != 0 || headerFlags != 0)
                    {
                        return null;
                    }
                    if ((ulong)numberOfEntries * 4 + 16 > (ulong)bytes.Length)
                    {
                        return null;
                    }
                    int entriesPosition = (int)br.BaseStream.Position;
                    br.BaseStream.Position += numberOfEntries * 4;
                    int terminator = br.ReadInt32();
                    if (terminator != 0)
                    {
                        return null;
                    }
                    return KeyBindingsEntry.FromByteArray(bytes, entriesPosition, numberOfEntries * 4);
                }
            }
            catch
            {
                return null;
            }
        }

        public static bool WriteToRegistry(KeyBindingsEntry[] entries)
        {
            try
            {
                using (var regKey = Registry.LocalMachine.OpenSubKey(@"SYSTEM\CurrentControlSet\Control\Keyboard Layout", true))
                {
                    if (entries == null)
                    {
                        regKey.DeleteValue("Scancode Map");
                        return true;
                    }
                    byte[] bytes = new byte[entries.Length * 4 + 16];
                    var ms = new MemoryStream(bytes);
                    var bw = new BinaryWriter(ms);
                    bw.Write((int)0); //headerVersion
                    bw.Write((int)0); //headerFlags
                    bw.Write((int)entries.Length); //numberOfEntries
                    for (int i = 0; i < entries.Length; i++)
                    {
                        bw.Write((int)entries[i].ToInt32());
                    }
                    bw.Write((int)0); //terminator
                    regKey.SetValue("Scancode Map", bytes, RegistryValueKind.Binary);
                    return true;
                }
            }
            catch
            {
                return false;
            }
        }

        public static bool RegisterRemappedKey(ushort scancodeToRemap, ushort scancodeToChangeTo, out bool changed)
        {
            changed = false;
            if (scancodeToRemap == 0) return false;
            var _entries = ReadFromRegistry();
            if (_entries == null) return false;
            var entries = new List<KeyBindingsEntry>(_entries);
            bool found = false;
            for (int i = 0; i < entries.Count; i++)
            {
                if (entries[i].SourceKey == scancodeToRemap)
                {
                    found = true;
                    if (entries[i].DestinationKey != scancodeToChangeTo)
                    {
                        if (scancodeToChangeTo == scancodeToRemap)
                        {
                            entries.RemoveAt(i);
                        }
                        else
                        {
                            entries[i] = new KeyBindingsEntry()
                            {
                                SourceKey = scancodeToRemap,
                                DestinationKey = scancodeToChangeTo
                            };
                        }
                        changed = true;
                    }
                    break;
                }
            }
            if (!found && scancodeToChangeTo != scancodeToRemap)
            {
                entries.Add(new KeyBindingsEntry()
                {
                    SourceKey = scancodeToRemap,
                    DestinationKey = scancodeToChangeTo
                });
                changed = true;
            }
            return WriteToRegistry(entries.ToArray());
        }

        public static bool HaveKeyMapping(ushort scancode)
        {
            var entries = ReadFromRegistry();
            if (entries == null) return false;
            return entries.Any(e => e.SourceKey == scancode);
        }
    }
}
