using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Diagnostics;
using System.IO;
using System.Linq;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading.Tasks;
using System.Windows.Forms;

namespace NoCopilotKey_Installer
{
    static class Program
    {
        /// <summary>
        /// The main entry point for the application.
        /// </summary>
        [STAThread]
        public static void Main()
        {
            Application.EnableVisualStyles();
            Application.SetCompatibleTextRenderingDefault(false);

            var args = Environment.GetCommandLineArgs();
            int exitCode = Main2(args);
            Environment.ExitCode = exitCode;
        }
        public static int Main2(string[] args)
        {
            KeyBindings.UpdateVolatile();
            InstallationMode installationMode = InstallationMode.Undefined;
            AutoRunMode autoRunMode = AutoRunMode.Undefined;

            bool doUninstall = false;
            bool doStop = false;
            bool doRegistryRemap = false;
            bool cleanupKeyMappings = false;
            string customVKey = "";

            if (args.Contains("--install-to-program-files"))
            {
                installationMode = InstallationMode.InstallToProgramFiles;
            }
            else if (args.Contains("--install-to-user-program-files"))
            {
                installationMode = InstallationMode.InstallToUserProgramFiles;
            }
            else if (args.Contains("--leave-exe-here"))
            {
                installationMode = InstallationMode.LeaveExeHere;
            }

            if (args.Contains("--register-as-scheduled-task"))
            {
                autoRunMode = AutoRunMode.ScheduledTask;
            }
            else if (args.Contains("--register-as-startup-item"))
            {
                autoRunMode = AutoRunMode.StartupItem;
            }
            else if (args.Contains("--no-auto-run"))
            {
                autoRunMode = AutoRunMode.NoAutoRun;
            }

            if (args.Contains("--cleanup-key-remappings"))
            {
                if (Installer.IsAdmin())
                {
                    bool okay = Installer.CleanUpKeyRemappings();
                    if (!okay) return -1;
                    return 0;
                }
                else
                {
                    Installer.RestartAsAdmin();
                }
            }

            if (args.Contains("--uninstall"))
            {
                doUninstall = true;
            }
            if (args.Contains("--stop"))
            {
                doStop = true;
            }
            if (args.Contains("--registry-remap"))
            {
                doRegistryRemap = true;
            }
            int i = Array.IndexOf(args, "--key");
            if (i >= 0 && i + 1 < args.Length)
            {
                customVKey = args[i + 1];
            }

            if (!doUninstall && !doStop && installationMode != InstallationMode.Undefined && autoRunMode != AutoRunMode.Undefined)
            {
                bool installOkay = Installer.Install(installationMode, autoRunMode, doRegistryRemap, customVKey);
                if (!installOkay) return 1;
                return 0;
            }

            if (doUninstall)
            {
                bool uninstallOkay = Installer.Uninstall();
                if (!uninstallOkay) return 1;
                return 0;
            }

            if (doStop)
            {
                var status = Installer.StopProgram();
                if (status.HasFlag(Installer.StopProgramStatus.FailedToStop)) return 1;
                if (status == Installer.StopProgramStatus.NotFound) return 2;
                return 0;
            }

            var currentProcess = Process.GetCurrentProcess();
            {
                var instances = Process.GetProcessesByName(currentProcess.ProcessName);
                foreach (var instance in instances)
                {
                    if (instance.Id != currentProcess.Id && instance.MainWindowHandle != IntPtr.Zero && 
                        Application.ExecutablePath.Equals(Installer.GetProcessFullName(instance), StringComparison.OrdinalIgnoreCase))
                    {
                        SetForegroundWindow(instance.MainWindowHandle);
                        return 1;
                    }
                }
                foreach (var instance in instances)
                {
                    instance.Dispose();
                }
            }
            Application.Run(new Form1());
            return Environment.ExitCode;
        }

        [DllImport("user32.dll", CallingConvention = CallingConvention.Winapi)]
        [return: MarshalAs(UnmanagedType.Bool)]
        static extern bool SetForegroundWindow(IntPtr hWnd);
    }
}
