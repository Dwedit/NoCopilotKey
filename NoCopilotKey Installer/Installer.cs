using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace NoCopilotKey_Installer
{
    public enum InstallationMode
    {
        Undefined = 0,
        LeaveExeHere,
        InstallToProgramFiles,
        InstallToUserProgramFiles,
    }

    public enum AutoRunMode
    {
        Undefined = 0,
        NoAutoRun,
        ScheduledTask,
        StartupItem,
    }

    public static class Installer
    {
        public static string GetProgramFilesAppDirectory()
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles), "NoCopilotKey");
        }

        public static string GetUserProgramFilesAppDirectory()
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData), "Programs", "NoCopilotKey");
        }

        public static string GetStartupShortcutPath()
        {
            return Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.Startup), "NoCopilotKey.lnk");
        }

        public static bool Install(InstallationMode installationMode, AutoRunMode autoRunMode, bool doRegistryRemap, string customVKey)
        {
            bool needAdmin = installationMode == InstallationMode.InstallToProgramFiles || autoRunMode == AutoRunMode.ScheduledTask;
            bool isAdmin = IsAdmin();
            if (!isAdmin && needAdmin)
            {
                RestartAsAdmin();
            }
            ushort currentF23Mapping = KeyBindings.GetCurrentF23Mapping();
            ushort initialF23Mapping = KeyBindings.GetInitialF23Mapping();
            bool haveF23Mapping = currentF23Mapping != 0xFFFF;
            bool haveInitialF23Mapping = initialF23Mapping != 0xFFFF;
            doRegistryRemap = needAdmin && doRegistryRemap;

            //validate that customVKey is a recognized key - blank it out if it's not
            if (!String.IsNullOrEmpty(customVKey))
            {
                string friendlyName = KeySelectionForm.GetFriendlyNameForVKey(customVKey);
                if (String.IsNullOrEmpty(friendlyName))
                {
                    customVKey = "";
                }
            }
            string customVKey2 = customVKey;
            if (String.IsNullOrEmpty(customVKey2))
            {
                customVKey2 = "VK_RCONTROL";
            }

            //bool keyMappingChanged = false;
            //if (haveF23Mapping)
            //{
            //    ushort currentF23Mapping = KeyBindings.GetCurrentF23Mapping();
            //    ushort selectedScancode = KeySelectionForm.VKeyToScancode(KeySelectionForm.VKeyNameToVKey(customVKey2));
            //    if (currentF23Mapping != selectedScancode)
            //    {
            //        keyMappingChanged = true;
            //    }
            //}

            bool wantToRemoveF23Mapping = false;

            if (!doRegistryRemap && haveF23Mapping && isAdmin)
            {
                DialogResult dialogResult;
                dialogResult = MessageBox.Show(
                    "The F23 key is currently remapped, but the setting to remap F23 is turned off." + Environment.NewLine +
                    "Remove the key mapping from the registry?", Application.ProductName, MessageBoxButtons.YesNoCancel);
                if (dialogResult == DialogResult.Yes)
                {
                    wantToRemoveF23Mapping = true;
                }
                if (dialogResult == DialogResult.Cancel)
                {
                    return false;
                }
            }
            if (wantToRemoveF23Mapping)
            {
                bool removed = KeyBindings.RemoveRemappedKey(KeyBindings.SCANCODE_F23);
                if (!removed)
                {
                    MessageBox.Show("Failed to unmap F23 key", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Error);
                }
            }

            string targetDirectory = "";
            if (installationMode == InstallationMode.InstallToProgramFiles)
            {
                targetDirectory = GetProgramFilesAppDirectory();
            }
            else if (installationMode == InstallationMode.InstallToUserProgramFiles)
            {
                targetDirectory = GetUserProgramFilesAppDirectory();
            }
            else if (installationMode == InstallationMode.LeaveExeHere)
            {
                targetDirectory = Application.StartupPath;
            }
            else
            {
                throw new ArgumentOutOfRangeException(nameof(installationMode));
            }
            string exePath = Path.Combine(targetDirectory, "NoCopilotKey.exe");
            string arguments = "";
            if (doRegistryRemap || !String.IsNullOrEmpty(customVKey))
            {
                if (doRegistryRemap)
                {
                    if (arguments.Length > 0) arguments += " ";
                    arguments += "--registry-remap";
                }
                if (!String.IsNullOrEmpty(customVKey))
                {
                    if (arguments.Length > 0) arguments += " ";
                    arguments += "--key " + customVKey;
                }
            }
            string installerExePath = Path.Combine(targetDirectory, "NoCopilotKey Installer.exe");

            try
            {
                Directory.CreateDirectory(targetDirectory);
            }
            catch
            {
                MessageBox.Show("Failed to create directory " + targetDirectory, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                return false;
            }
            var stopStatus = StopProgram(exePath);
            if (stopStatus.HasFlag(StopProgramStatus.FailedToStop))
            {
                MessageBox.Show("Failed to stop process " + exePath, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                return false;
            }
            bool exeOkay = ExtractExe(exePath);
            if (!exeOkay)
            {
                MessageBox.Show("Failed to extract main EXE to " + exePath, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                return false;
            }
            if (!installerExePath.Equals(Application.ExecutablePath, StringComparison.OrdinalIgnoreCase))
            {
                try
                {
                    File.Copy(Application.ExecutablePath, installerExePath, true);
                }
                catch
                {
                    MessageBox.Show("Failed to copy installer to " + installerExePath, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    return false;
                }
            }

            if (autoRunMode == AutoRunMode.ScheduledTask)
            {
                var task = ScheduledTask.CreateScheduledTask(exePath, arguments, "NoCopilotKey", "Dan Weiss (www.dwedit.org)", "Changes Copilot keyboard key into right ctrl key");
                if (task == null)
                {
                    MessageBox.Show("Failed to create a scheduled task", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                }
            }
            else if (autoRunMode == AutoRunMode.StartupItem)
            {
                string lnkFileName = GetStartupShortcutPath();
                try
                {
                    Shortcut.CreateShortcut(lnkFileName, exePath, arguments);
                }
                catch
                {
                    MessageBox.Show("Failed to create shortcut " + lnkFileName, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                }
            }

            string registryPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\NoCopilotKey";
            Microsoft.Win32.RegistryKey subkey = null;
            if (installationMode == InstallationMode.InstallToProgramFiles)
            {
                subkey = Microsoft.Win32.Registry.LocalMachine.CreateSubKey(registryPath);
            }
            else if (installationMode == InstallationMode.InstallToUserProgramFiles)
            {
                subkey = Microsoft.Win32.Registry.CurrentUser.CreateSubKey(registryPath);
            }
            if (subkey != null)
            {
                try
                {
                    subkey.SetValue("DisplayName", "NoCopilotKey");
                    subkey.SetValue("DisplayVersion", "1.0.4.0");
                    subkey.SetValue("Publisher", "www.dwedit.org");
                    subkey.SetValue("URLInfoAbout", "https://github.com/Dwedit/NoCopilotKey");
                    subkey.SetValue("UninstallString", "\"" + installerExePath + "\" --uninstall");
                }
                catch
                {
                    MessageBox.Show("Failed write uninstallation information to the registry", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                }
            }

            //close other instances
            stopStatus = StopProgram();
            if (stopStatus.HasFlag(StopProgramStatus.FailedToStop))
            {
                MessageBox.Show("Failed to stop other running instances.\r\nIf there is another instance running (such as a renamed EXE), end the process using Task Manager and try installing again.\r\nOtherwise, try restarting or logging off.", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
            }
            Process.Start(exePath, arguments);

            if (!haveF23Mapping && doRegistryRemap)
            {
                MessageBox.Show("Using the registry to remap the F23 Key will take effect once you restart or log off.\r\nUntil then, the program will remap the Copilot key by using only global keyboard hooks.", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Information);
            }

            SetParentProcessForegroundWindow();
            return true;
        }

        public static bool ExtractExe(string targetPath)
        {
            try
            {
                var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("NoCopilotKey_Installer.NoCopilotKey.exe");
                BinaryReader br = new BinaryReader(stream);
                byte[] bytes = br.ReadBytes((int)stream.Length);
                File.WriteAllBytes(targetPath, bytes);
                return true;
            }
            catch (Exception ex)
            {
                return false;
            }
        }

        public static bool CanUninstall()
        {
            return IsInstalledToProgramFiles() || IsInstalledToUserProgramFiles() || IsScheduledTask() || IsStartupItem();
        }

        static bool TryDeleteFile(string fileName)
        {
            try
            {
                if (!File.Exists(fileName))
                {
                    return true;
                }
                File.Delete(fileName);
                return true;
            }
            catch (Exception ex)
            {
                return false;
            }
        }

        static bool TryDeleteDirectory(string directoryName)
        {
            try
            {
                if (!Directory.Exists(directoryName))
                {
                    return true;
                }
                Directory.Delete(directoryName);
                return true;
            }
            catch (Exception ex)
            {
                return false;
            }
        }
        public static bool UninstallNeedsAdmin()
        {
            return IsInstalledToProgramFiles() || IsScheduledTask();
        }

        public static bool Uninstall()
        {
            bool anyFailures = false;
            bool isInstalledToProgramFiles = IsInstalledToProgramFiles();
            bool isScheduledTask = IsScheduledTask();
            bool isInstalledToUserProgramFiles = IsInstalledToUserProgramFiles();
            bool isStartupItem = IsStartupItem();

            bool needAdmin = isInstalledToProgramFiles || isScheduledTask;

            if (needAdmin && !IsAdmin())
            {
                RestartAsAdmin();
            }

            bool haveF23Mapping = needAdmin && KeyBindings.GetCurrentF23Mapping() != 0xFFFF;
            bool haveInitialF23Mapping = needAdmin && KeyBindings.GetInitialF23Mapping() != 0xFFFF;
            bool useRegistryRemap = needAdmin && UseRegistryRemap(GetArgs());

            List<string> filesToDelete = new List<string>();
            List<string> directoriesToDelete = new List<string>();
            bool TryDeleteFile2(string fileNameToDelete)
            {
                if (File.Exists(fileNameToDelete))
                {
                    bool deleted = TryDeleteFile(fileNameToDelete);
                    if (!deleted)
                    {
                        filesToDelete.Add(fileNameToDelete);
                    }
                    return deleted;
                }
                return true;
            }
            bool TryDeleteDirectory2(string directoryNameToDelete)
            {
                if (Directory.Exists(directoryNameToDelete))
                {
                    bool deleted = TryDeleteDirectory(directoryNameToDelete);
                    if (!deleted)
                    {
                        directoriesToDelete.Add(directoryNameToDelete);
                    }
                    return deleted;
                }
                return true;
            }

            var deleteOperations = new[]
            {
                (isInstalledToProgramFiles, GetProgramFilesAppDirectory()),
                (isInstalledToUserProgramFiles, GetUserProgramFilesAppDirectory())
            };
            foreach (var pair in deleteOperations)
            {
                bool doThisOperation = pair.Item1;
                string programDirectory = pair.Item2;
                if (doThisOperation)
                {
                    string exeName = Path.Combine(programDirectory, "NoCopilotKey.exe");
                    string installerExeName = Path.Combine(programDirectory, "NoCopilotKey Installer.exe");
                    string installerExeName2 = Path.Combine(programDirectory, "NoCopilotKey Installer.pdb");
                    string installerExeName3 = Path.Combine(programDirectory, "NoCopilotKey Installer.exe.config");
                    var stopStatus = StopProgram(exeName);
                    if (stopStatus.HasFlag(StopProgramStatus.FailedToStop))
                    {
                        anyFailures = true;
                        MessageBox.Show("Failed to stop process " + exeName, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    }
                    bool deletedExe = TryDeleteFile2(exeName);
                    bool deletedInstaller1 = TryDeleteFile2(installerExeName);
                    bool deletedInstaller2 = TryDeleteFile2(installerExeName2);
                    bool deletedInstaller3 = TryDeleteFile2(installerExeName3);
                    bool deletedDirectory = TryDeleteDirectory2(programDirectory);
                }
            }
            if (isScheduledTask)
            {
                try
                {
                    ScheduledTask.RemoveScheduledTask("NoCopilotKey");
                }
                catch
                {
                    MessageBox.Show("Failed to remove scheduled task.", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    anyFailures = true;
                }
            }
            if (isStartupItem)
            {
                string startupLnk = GetStartupShortcutPath();
                bool deletedShortcut = TryDeleteFile(startupLnk);
                if (!deletedShortcut)
                {
                    MessageBox.Show("Failed to delete shortcut " + startupLnk, Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    anyFailures = true;
                }
            }
            //remove uninstaller from registry
            string registryPath = "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\NoCopilotKey";
            bool uninstallerRemovedFromRegistry = false;
            bool registryKeyExists = false;

            var registryOperations = new[]
            {
                (isInstalledToProgramFiles, Microsoft.Win32.Registry.LocalMachine),
                (isInstalledToUserProgramFiles, Microsoft.Win32.Registry.CurrentUser)
            };
            foreach (var pair in registryOperations)
            {
                bool doThisOperation = pair.Item1;
                var baseRegistryKey = pair.Item2;
                if (doThisOperation)
                {
                    try
                    {
                        var existingKey = baseRegistryKey.OpenSubKey(registryPath, false);
                        if (existingKey != null)
                        {
                            registryKeyExists = true;
                            existingKey.Dispose();
                        }
                        baseRegistryKey.DeleteSubKeyTree(registryPath, true);
                        uninstallerRemovedFromRegistry = true;
                    }
                    catch
                    {
                        uninstallerRemovedFromRegistry = false;
                        if (registryKeyExists) anyFailures = true;
                        MessageBox.Show("Failed to remove uninstallation information from the registry.", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                    }
                }
            }
            if (needAdmin)
            {
                bool wantToRemoveRegistryRemap = false;
                if (haveF23Mapping && useRegistryRemap)
                {
                    wantToRemoveRegistryRemap = true;
                }
                else if (haveF23Mapping)
                {
                    var dialogResult = MessageBox.Show("There is a key remapping for F23 in the registry, but it could have been created by another program.  Do you want to remove it?", Application.ProductName, MessageBoxButtons.YesNo, MessageBoxIcon.Question, MessageBoxDefaultButton.Button1);
                    if (dialogResult == DialogResult.Yes)
                    {
                        wantToRemoveRegistryRemap = true;
                    }
                }
                if (wantToRemoveRegistryRemap)
                {
                    //remove F23 registry remap
                    bool okay = KeyBindings.RegisterRemappedKey(KeyBindings.SCANCODE_F23, KeyBindings.SCANCODE_F23, out bool changed);
                    if (changed)
                    {
                        if (!okay)
                        {
                            MessageBox.Show("Failed to remove F23 key remapping from registry", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Stop);
                        }
                        else
                        {
                            if (haveInitialF23Mapping)
                            {
                                MessageBox.Show("The F23 Key (part of the Copilot Key) will remain remapped until you restart or log off.", Application.ProductName, MessageBoxButtons.OK, MessageBoxIcon.Information);
                            }
                        }
                    }
                }
            }

            if (filesToDelete.Count > 0 || directoriesToDelete.Count > 0)
            {
                ProcessStartInfo startInfo = new ProcessStartInfo(Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.System), "cmd.exe"));
                startInfo.Arguments = "/C sleep 0.5";
                foreach (var deleteFileName in filesToDelete)
                {
                    startInfo.Arguments += " & del \"" + deleteFileName + "\"";
                }
                foreach (var deleteDirectory in directoriesToDelete)
                {
                    startInfo.Arguments += " & rmdir \"" + deleteDirectory + "\"";
                }
                startInfo.CreateNoWindow = true;
                startInfo.UseShellExecute = false;
                //to ensure that working directory is not the uninstallation directory and prevent it from being locked, switch to Windows directory before running the command
                startInfo.WorkingDirectory = Environment.GetFolderPath(Environment.SpecialFolder.Windows);

                Process.Start(startInfo);
                Environment.Exit(0);
            }
            SetParentProcessForegroundWindow();
            return !anyFailures;
        }

        public static bool IsInstalledToDirectory(string directoryName)
        {
            try
            {
                if (Directory.Exists(directoryName))
                {
                    string noCopilotKeyExe = Path.Combine(directoryName, "NoCopilotKey.exe");
                    string noCopilotKeyInstallerExe = Path.Combine(directoryName, "NoCopilotKey Installer.exe");
                    if (File.Exists(noCopilotKeyExe)) return true;
                    if (File.Exists(noCopilotKeyInstallerExe)) return true;
                    if (Directory.EnumerateFileSystemEntries(directoryName).FirstOrDefault() != null) return false;
                    return true;
                }
            }
            catch
            {

            }
            return false;
        }

        public static bool IsInstalledToProgramFiles()
        {
            return IsInstalledToDirectory(GetProgramFilesAppDirectory());
        }
        public static bool IsInstalledToUserProgramFiles()
        {
            return IsInstalledToDirectory(GetUserProgramFilesAppDirectory());
        }
        public static bool IsScheduledTask()
        {
            try
            {
                var scheduledTask = ScheduledTask.GetScheduledTask("NoCopilotKey");
                return scheduledTask != null;
            }
            catch
            {
                return false;
            }
        }
        public static bool IsStartupItem()
        {
            try
            {
                return File.Exists(GetStartupShortcutPath());
            }
            catch
            {
                return false;
            }
        }

        [DllImport("kernel32.dll", SetLastError = true, ExactSpelling = true, CharSet = CharSet.Unicode)]
        static extern IntPtr OpenMutexW(uint dwDesiredAccess, bool bInheritHandle, string lpName);

        [DllImport("kernel32.dll", SetLastError = true, CallingConvention = CallingConvention.Winapi)]
        static extern bool QueryFullProcessImageName([In] IntPtr hProcess, [In] int dwFlags, [Out] StringBuilder lpExeName, ref int lpdwSize);

        public static string GetProcessFullName(Process process)
        {
            int capacity = 32767;
            StringBuilder sb = new StringBuilder(capacity);
            QueryFullProcessImageName(process.Handle, 0, sb, ref capacity);
            return sb.ToString();
        }

        [Flags]
        public enum StopProgramStatus
        {
            NotFound = 0,
            Success = 1,
            FailedToStop = 2,
        }

        public static StopProgramStatus StopProgram(string exePath)
        {
            StopProgramStatus status = 0;
            try
            {
                var processes = Process.GetProcessesByName("NoCopilotKey");
                foreach (var process in processes)
                {
                    string exeName = GetProcessFullName(process);
                    if (exePath.Equals(exeName, StringComparison.OrdinalIgnoreCase))
                    {
                        process.Kill();
                        process.WaitForExit();
                        status |= StopProgramStatus.Success;
                    }
                }
            }
            catch
            {
                status |= StopProgramStatus.FailedToStop;
            }
            return status;
        }

        public static StopProgramStatus StopProgram()
        {
            StopProgramStatus status = 0;
            try
            {
                var processes = Process.GetProcessesByName("NoCopilotKey");
                foreach (var process in processes)
                {
                    process.Kill();
                    process.WaitForExit();
                    status |= StopProgramStatus.Success;
                }
            }
            catch
            {
                status |= StopProgramStatus.FailedToStop;
            }
            //check if the mutex is still present (from running a renamed EXE)
            const uint SYNCHRONIZE = 0x00100000;
            IntPtr hMutex = OpenMutexW(SYNCHRONIZE, false, "Mutex for NoCopilotKey");
            if (hMutex != IntPtr.Zero)
            {
                status |= StopProgramStatus.FailedToStop;
            }
            return status;
        }

        public static bool LaunchAsAdmin(string[] args = null)
        {
            return LaunchInstaller2(args, true) != null;
        }

        public static bool LaunchInstaller(string[] args = null, bool admin = false)
        {
            var process = LaunchInstaller2(args, admin);
            return process != null;
        }

        public static Process LaunchInstaller2(string[] args = null, bool admin = false)
        {
            if (args == null)
            {
                args = Environment.GetCommandLineArgs().Skip(1).ToArray();
            }

            var startInfo = new ProcessStartInfo(Application.ExecutablePath);
            startInfo.Arguments = string.Join(" ", args);
            if (admin && !IsAdmin())
            {
                startInfo.Verb = "runas";
            }
            startInfo.UseShellExecute = true;
            Process otherProcess = null;
            try
            {
                otherProcess = Process.Start(startInfo);
            }
            catch (Exception ex)
            {
                return null;
            }
            return otherProcess;
        }

        public static void RestartAsAdmin(string[] args = null)
        {
            bool success = LaunchAsAdmin(args);
            if (!success) Environment.Exit(1);
            Environment.Exit(0);
        }

        public static bool IsAdmin()
        {
            System.Security.Principal.WindowsIdentity identity = System.Security.Principal.WindowsIdentity.GetCurrent();
            System.Security.Principal.WindowsPrincipal principal = new System.Security.Principal.WindowsPrincipal(identity);
            return principal.IsInRole(System.Security.Principal.WindowsBuiltInRole.Administrator);
        }

        public static string[] GetArgs()
        {
            string args = "";
            string lnkPath = GetStartupShortcutPath();
            if (File.Exists(lnkPath))
            {
                args = Shortcut.GetShortcutArguments(lnkPath);
            }
            else if (IsScheduledTask())
            {
                args = ScheduledTask.GetScheduledTaskArguments("NoCopilotKey");
            }
            if (String.IsNullOrEmpty(args))
            {
                return Array.Empty<string>();
            }
            return args.Split(' ');
        }

        public static string GetCustomVKey(string[] args)
        {
            int i = Array.IndexOf(args, "--key");
            if (i >= 0 && i + 1 < args.Length)
            {
                string vkey = args[i + 1];
                string friendlyName = KeySelectionForm.GetFriendlyNameForVKey(vkey);
                if (!String.IsNullOrEmpty(friendlyName))
                {
                    return vkey;
                }
            }
            return "";
        }
        public static bool UseRegistryRemap(string [] args)
        {
            return args.Contains("--registry-remap");
        }

        static bool SetParentProcessForegroundWindow()
        {
            int parentProcessId = GetParentProcessId();
            if (parentProcessId == 0) return false;
            //If the parent process is this executable, then make that the active window
            using (var parentProcess = Process.GetProcessById(parentProcessId))
            {
                string fileName = parentProcess.MainModule.FileName;
                string myFileName = Process.GetCurrentProcess().MainModule.FileName;
                if (String.Equals(myFileName, fileName, StringComparison.OrdinalIgnoreCase))
                {
                    return SetForegroundWindow(parentProcess.Id);
                }
                return false;
            }
        }

        static int GetParentProcessId()
        {
            try
            {
                PROCESS_BASIC_INFORMATION processBasicInformation = new PROCESS_BASIC_INFORMATION();
                int returnLength = 0;
                int status = NtQueryInformationProcess(Process.GetCurrentProcess().Handle, 0, ref processBasicInformation, Marshal.SizeOf<PROCESS_BASIC_INFORMATION>(), out returnLength);
                if (status != 0)
                {
                    return 0;
                }
                return (int)processBasicInformation.InheritedFromUniqueProcessId;
            }
            catch (Exception ex)
            {
                return 0;
            }
        }

        struct PROCESS_BASIC_INFORMATION
        {
            public IntPtr Reserved1;
            public IntPtr PebBaseAddress;
            public IntPtr Reserved2_0;
            public IntPtr Reserved2_1;
            public IntPtr UniqueProcessId;
            public IntPtr InheritedFromUniqueProcessId;
        }
        
        [DllImport("ntdll.dll", CallingConvention = CallingConvention.Winapi)]
        private static extern int NtQueryInformationProcess(IntPtr processHandle, int processInformationClass, ref PROCESS_BASIC_INFORMATION processInformation, int processInformationLength, out int returnLength);

        static bool SetForegroundWindow(int processId)
        {
            try
            {
                using (var process = Process.GetProcessById(processId))
                {
                    if (process != null)
                    {
                        IntPtr mainWindowHandle = process.MainWindowHandle;
                        if (mainWindowHandle != IntPtr.Zero)
                        {
                            return SetForegroundWindow(mainWindowHandle) != 0;
                        }
                    }
                }
            }
            catch (Exception ex)
            {

            }
            return false;
        }

        [DllImport("user32.dll", CallingConvention = CallingConvention.Winapi, ExactSpelling = true)]
        static extern int SetForegroundWindow(IntPtr hwnd);

        public static Version GetInstalledVersion()
        {
            string appDir;

            if (IsInstalledToProgramFiles())
            {
                appDir = GetProgramFilesAppDirectory();
            }
            else if (IsInstalledToUserProgramFiles())
            {
                appDir = GetUserProgramFilesAppDirectory();
            }
            else
            {
                return null;
            }
            try
            {
                string exePath = Path.Combine(appDir, "NoCopilotKey.exe");
                FileVersionInfo fileVersion = FileVersionInfo.GetVersionInfo(exePath);
                return new Version(fileVersion.FileVersion);
            }
            catch (Exception ex)
            {
                return null;
            }
        }
    }
}
