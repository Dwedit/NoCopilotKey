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

        public static bool Install(InstallationMode installationMode, AutoRunMode autoRunMode)
        {
            bool needAdmin = installationMode == InstallationMode.InstallToProgramFiles || autoRunMode == AutoRunMode.ScheduledTask;
            if (!IsAdmin() && needAdmin)
            {
                RestartAsAdmin();
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
                var task = ScheduledTask.CreateScheduledTask(exePath, "NoCopilotKey", "Dan Weiss (www.dwedit.org)", "Changes Copilot keyboard key into right ctrl key");
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
                    Shortcut.CreateShortcut(lnkFileName, exePath);
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
                    subkey.SetValue("DisplayVersion", "1.0.3.0");
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
                MessageBox.Show("Failed to stop other running instances");
            }
            Process.Start(exePath);
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

        [DllImport("kernel32.dll", SetLastError = true)]
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
    }
}
