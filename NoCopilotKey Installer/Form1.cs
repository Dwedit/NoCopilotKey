using System;
using System.Collections.Generic;
using System.ComponentModel;
using System.Data;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Text;
using System.Threading.Tasks;
using System.Windows.Forms;
using TaskScheduler;

namespace NoCopilotKey_Installer
{
    public partial class Form1 : Form
    {
        public Form1()
        {
            InitializeComponent();
        }

        private void Form1_Load(object sender, EventArgs e)
        {
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.pictureBox1.Image = System.Drawing.SystemIcons.Shield.ToBitmap();
            this.pictureBox1.Left = this.optProgramFiles.Left + this.optProgramFiles.Width + 0;
            this.pictureBox1.Top = this.optProgramFiles.Top + (this.optProgramFiles.Height - this.pictureBox1.Height) / 2;
            string programFilesDirectory = Installer.GetProgramFilesAppDirectory();
            string userProgramFilesDirectory = Installer.GetUserProgramFilesAppDirectory();

            this.toolTip1.AutoPopDelay = 32767;
            this.toolTip1.InitialDelay = 200;
            this.toolTip1.ReshowDelay = 200;
            this.toolTip1.SetToolTip(optProgramFiles, "When installed as Administrator, the program will be installed into " + programFilesDirectory + "," + Environment.NewLine +
                "and a scheduled task will be created to automatically run the program as admin.");
            this.toolTip1.SetToolTip(optUserProgramFiles, "When installing as a regular user, the program will be installed into " + userProgramFilesDirectory + "," + Environment.NewLine +
                "and a Startup shortcut will be created in your Start Menu to automatically run the program.");

            RefreshButtons();
        }

        void RefreshButtons()
        {
            this.uninstallButton.Enabled = Installer.CanUninstall();
            bool isInstalled = false;
            if (Installer.IsInstalledToProgramFiles())
            {
                optUserProgramFiles.Enabled = false;
                optProgramFiles.Enabled = true;
                optProgramFiles.Checked = true;
                isInstalled = true;
            }
            else if (Installer.IsInstalledToUserProgramFiles())
            {
                optProgramFiles.Enabled = false;
                optUserProgramFiles.Enabled = true;
                optUserProgramFiles.Checked = true;
                isInstalled = true;
            }
            else
            {
                optUserProgramFiles.Enabled = true;
                optProgramFiles.Enabled = true;
            }
            if (isInstalled)
            {
                var args = Installer.GetArgs();
                string customVKey = Installer.GetCustomVKey(args);
                if (string.IsNullOrEmpty(customVKey))
                {
                    customVKey = "VK_RCONTROL";
                }
                this.selectedVKey = customVKey;
                this.selectedFriendlyName = KeySelectionForm.GetFriendlyNameForVKey(customVKey);
                this.lblKey.Text = this.selectedFriendlyName;
                this.chkRemapViaRegistry.Checked = Installer.UseRegistryRemap(args);
            }
            //if (!isInstalled)
            //{
            //    var scancode = KeyBindings.GetKeyMapping(KeyBindings.SCANCODE_F23);
            //    if (scancode != 0)
            //    {
            //        chkRemapViaRegistry.Checked = true;
            //        var vkey = KeySelectionForm.ScancodeToVKey(scancode);
            //        this.SelectCustomKey(vkey);
            //    }
            //    else
            //    {
            //        chkRemapViaRegistry.Checked = false;
            //    }
            //}

            installButton.Text = "Install";
            if (isInstalled)
            {
                var installedVersion = Installer.GetInstalledVersion();
                var myVersion = new Version(Application.ProductVersion);
                if (installedVersion != null)
                {
                    string versionString = installedVersion.ToString();
                    label1.Text = "Version " + versionString + "is currently installed." + Environment.NewLine + "To change whether the program is installed as Administrator or Normal User, uninstall the program first.";
                    if (myVersion > installedVersion)
                    {
                        installButton.Text = "Upgrade";
                    }
                    if (myVersion < installedVersion)
                    {
                        installButton.Text = "Downgrade";
                    }
                    if (myVersion == installedVersion)
                    {
                        installButton.Text = "Apply";
                    }
                }
                else
                {
                    label1.Text = "Program is currently installed." + Environment.NewLine + "To change the installation mode, uninstall the program first.";
                }

            }
            else
            {
                label1.Text = "To support applications which run as Administrator, NoCopilotKey must be installed to run as Administrator.";
            }
        }

        static bool IsInstalledToProgramFiles()
        {
            string programFilesDirectory = Environment.GetFolderPath(Environment.SpecialFolder.ProgramFiles);
            string noCopilotKeyDirectory = Path.Combine(programFilesDirectory, "NoCopilotKey");
            if (Directory.Exists(noCopilotKeyDirectory))
            {
                return File.Exists(Path.Combine(noCopilotKeyDirectory, "NoCopilotKey.exe")) ||
                    File.Exists(Path.Combine(noCopilotKeyDirectory, "NoCopilotKey Installer.exe"));
            }
            return false;
        }

        private void uninstallButton_Click(object sender, EventArgs e)
        {
            Uninstall();
        }

        void Uninstall()
        {
            int exitCode = 1;
            bool needAdmin = Installer.UninstallNeedsAdmin();
            bool installerNeedsToDeleteItself = false;
            //check if installer needs to delete itself
            string installDirectory = "";
            if (Installer.IsInstalledToProgramFiles())
            {
                installDirectory = Installer.GetProgramFilesAppDirectory();
            }
            else if (Installer.IsInstalledToUserProgramFiles())
            {
                installDirectory = Installer.GetUserProgramFilesAppDirectory();
            }
            if (Application.ExecutablePath.StartsWith(installDirectory, StringComparison.OrdinalIgnoreCase))
            {
                installerNeedsToDeleteItself = true;
            }
            
            if (!(needAdmin && !Installer.IsAdmin()))
            {
                exitCode = Program.Main2(new string[] { "--uninstall" });
            }
            else
            {
                var process = Installer.LaunchInstaller2(new string[] { "--uninstall" }, needAdmin);
                if (installerNeedsToDeleteItself)
                {
                    //exit immediately so uninstaller can delete itself
                    Environment.Exit(0);
                }
                else
                {
                    if (process != null)
                    {
                        this.Enabled = false;
                        while (!process.WaitForExit(10))
                        {
                            Application.DoEvents();
                        }
                        exitCode = process.ExitCode;
                        this.Enabled = true;
                    }
                }
            }
            if (exitCode == 0)
            {
                MessageBox.Show(this, "Uninstall Successful", "NoCopilotKey", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                MessageBox.Show(this, "Uninstall Failed", "NoCopilotKey", MessageBoxButtons.OK, MessageBoxIcon.Stop);
            }
            RefreshButtons();
        }

        private void installButton_Click(object sender, EventArgs e)
        {
            Install();
        }

        void Install()
        {
            bool needAdmin = false;
            List<string> args = new List<string>();
            if (this.optProgramFiles.Checked)
            {
                args.Add("--install-to-program-files");
                args.Add("--register-as-scheduled-task");
                needAdmin = true;
            }
            else if (this.optUserProgramFiles.Checked)
            {
                args.Add("--install-to-user-program-files");
                args.Add("--register-as-startup-item");
            }
            if (!Installer.IsAdmin() && KeyBindings.GetInitialF23Mapping() == 0 && KeyBindings.GetCurrentF23Mapping() == 0)
            {
                var dialogResult = MessageBox.Show("The F23 key is currently disabled by registry.  NoCopilotKey will not work unless the F23 key is enabled.  Enable the F23 Key? (Requires Admin)", Application.ProductName, MessageBoxButtons.YesNoCancel, MessageBoxIcon.Question);
                if (dialogResult == DialogResult.Yes)
                {
                    needAdmin = true;
                }
                else
                {
                    return;
                }
            }
            if (this.chkRemapViaRegistry.Enabled && this.chkRemapViaRegistry.Checked)
            {
                args.Add("--registry-remap");
            }
            if (this.selectedVKey != "VK_RCONTROL")
            {
                args.Add("--key");
                args.Add(this.selectedVKey);
            }

            int exitCode = 1;
            if (!(needAdmin && !Installer.IsAdmin()))
            {
                exitCode = Program.Main2(args.ToArray());
            }
            else
            {
                var process = Installer.LaunchInstaller2(args.ToArray(), needAdmin);
                if (process != null)
                {
                    this.Enabled = false;
                    while (!process.WaitForExit(10))
                    {
                        Application.DoEvents();
                    }
                    exitCode = process.ExitCode;
                    this.Enabled = true;
                }
            }
            if (exitCode == 0)
            {
                MessageBox.Show(this, "Installation Successful", "NoCopilotKey", MessageBoxButtons.OK, MessageBoxIcon.Information);
            }
            else
            {
                MessageBox.Show(this, "Installation Failed", "NoCopilotKey", MessageBoxButtons.OK, MessageBoxIcon.Stop);
            }
            RefreshButtons();
        }

        private void optProgramFiles_CheckedChanged(object sender, EventArgs e)
        {
            UpdateCheckboxes();
        }

        private void optUserProgramFiles_CheckedChanged(object sender, EventArgs e)
        {
            UpdateCheckboxes();
        }

        void UpdateCheckboxes()
        {
            if (optUserProgramFiles.Checked)
            {
                chkRemapViaRegistry.Enabled = false;
            }
            else
            {
                chkRemapViaRegistry.Enabled = true;
            }
        }

        private void selectKeyButton_Click(object sender, EventArgs e)
        {
            SelectCustomKey();
        }

        string selectedVKey = "VK_RCONTROL";
        string selectedFriendlyName = "Right Ctrl";

        void SelectCustomKey(uint vkey)
        {
            this.selectedVKey = KeySelectionForm.GetVKeyName(vkey);
            this.selectedFriendlyName = KeySelectionForm.GetFriendlyNameForVKey(this.selectedVKey);
            this.lblKey.Text = this.selectedFriendlyName;
        }

        void SelectCustomKey()
        {
            using (var keySelectionForm = new KeySelectionForm())
            {
                keySelectionForm.SelectedVKey = selectedVKey;
                if (keySelectionForm.ShowDialog() == DialogResult.OK)
                {
                    this.selectedVKey = keySelectionForm.SelectedVKey;
                    this.selectedFriendlyName = keySelectionForm.SelectedFriendlyName;
                    this.lblKey.Text = this.selectedFriendlyName;
                }
            }
        }
    }
}
