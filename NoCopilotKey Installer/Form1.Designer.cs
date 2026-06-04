
namespace NoCopilotKey_Installer
{
    partial class Form1
    {
        /// <summary>
        /// Required designer variable.
        /// </summary>
        private System.ComponentModel.IContainer components = null;

        /// <summary>
        /// Clean up any resources being used.
        /// </summary>
        /// <param name="disposing">true if managed resources should be disposed; otherwise, false.</param>
        protected override void Dispose(bool disposing)
        {
            if (disposing && (components != null))
            {
                components.Dispose();
            }
            base.Dispose(disposing);
        }

        #region Windows Form Designer generated code

        /// <summary>
        /// Required method for Designer support - do not modify
        /// the contents of this method with the code editor.
        /// </summary>
        private void InitializeComponent()
        {
            this.components = new System.ComponentModel.Container();
            System.ComponentModel.ComponentResourceManager resources = new System.ComponentModel.ComponentResourceManager(typeof(Form1));
            this.optProgramFiles = new System.Windows.Forms.RadioButton();
            this.groupBox1 = new System.Windows.Forms.GroupBox();
            this.pictureBox1 = new System.Windows.Forms.PictureBox();
            this.optUserProgramFiles = new System.Windows.Forms.RadioButton();
            this.uninstallButton = new System.Windows.Forms.Button();
            this.installButton = new System.Windows.Forms.Button();
            this.label1 = new System.Windows.Forms.Label();
            this.chkRemapViaRegistry = new System.Windows.Forms.CheckBox();
            this.selectKeyButton = new System.Windows.Forms.Button();
            this.lblKey = new System.Windows.Forms.Label();
            this.toolTip1 = new System.Windows.Forms.ToolTip(this.components);
            this.groupBox1.SuspendLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).BeginInit();
            this.SuspendLayout();
            // 
            // optProgramFiles
            // 
            this.optProgramFiles.AutoSize = true;
            this.optProgramFiles.Checked = true;
            this.optProgramFiles.Location = new System.Drawing.Point(6, 19);
            this.optProgramFiles.Name = "optProgramFiles";
            this.optProgramFiles.Size = new System.Drawing.Size(159, 17);
            this.optProgramFiles.TabIndex = 0;
            this.optProgramFiles.TabStop = true;
            this.optProgramFiles.Text = "Install to run as &Administrator";
            this.optProgramFiles.UseVisualStyleBackColor = true;
            this.optProgramFiles.CheckedChanged += new System.EventHandler(this.optProgramFiles_CheckedChanged);
            // 
            // groupBox1
            // 
            this.groupBox1.Controls.Add(this.pictureBox1);
            this.groupBox1.Controls.Add(this.optUserProgramFiles);
            this.groupBox1.Controls.Add(this.optProgramFiles);
            this.groupBox1.Location = new System.Drawing.Point(12, 12);
            this.groupBox1.Name = "groupBox1";
            this.groupBox1.Size = new System.Drawing.Size(186, 67);
            this.groupBox1.TabIndex = 0;
            this.groupBox1.TabStop = false;
            this.groupBox1.Text = "Install Mode:";
            // 
            // pictureBox1
            // 
            this.pictureBox1.Location = new System.Drawing.Point(163, 20);
            this.pictureBox1.Name = "pictureBox1";
            this.pictureBox1.Size = new System.Drawing.Size(16, 16);
            this.pictureBox1.SizeMode = System.Windows.Forms.PictureBoxSizeMode.StretchImage;
            this.pictureBox1.TabIndex = 2;
            this.pictureBox1.TabStop = false;
            // 
            // optUserProgramFiles
            // 
            this.optUserProgramFiles.AutoSize = true;
            this.optUserProgramFiles.Location = new System.Drawing.Point(6, 42);
            this.optUserProgramFiles.Name = "optUserProgramFiles";
            this.optUserProgramFiles.Size = new System.Drawing.Size(154, 17);
            this.optUserProgramFiles.TabIndex = 1;
            this.optUserProgramFiles.Text = "Install to run as regular &user";
            this.optUserProgramFiles.UseVisualStyleBackColor = true;
            this.optUserProgramFiles.CheckedChanged += new System.EventHandler(this.optUserProgramFiles_CheckedChanged);
            // 
            // uninstallButton
            // 
            this.uninstallButton.Location = new System.Drawing.Point(42, 207);
            this.uninstallButton.Name = "uninstallButton";
            this.uninstallButton.Size = new System.Drawing.Size(75, 23);
            this.uninstallButton.TabIndex = 5;
            this.uninstallButton.Text = "Uninstall";
            this.uninstallButton.UseVisualStyleBackColor = true;
            this.uninstallButton.Click += new System.EventHandler(this.uninstallButton_Click);
            // 
            // installButton
            // 
            this.installButton.Location = new System.Drawing.Point(123, 207);
            this.installButton.Name = "installButton";
            this.installButton.Size = new System.Drawing.Size(75, 23);
            this.installButton.TabIndex = 6;
            this.installButton.Text = "Install";
            this.installButton.UseVisualStyleBackColor = true;
            this.installButton.Click += new System.EventHandler(this.installButton_Click);
            // 
            // label1
            // 
            this.label1.Location = new System.Drawing.Point(7, 147);
            this.label1.Name = "label1";
            this.label1.Size = new System.Drawing.Size(195, 57);
            this.label1.TabIndex = 4;
            this.label1.Text = "To support applications which run as Administrator, NoCopilotKey must be installe" +
    "d to run as Administrator.\r\n";
            // 
            // chkRemapViaRegistry
            // 
            this.chkRemapViaRegistry.AutoSize = true;
            this.chkRemapViaRegistry.Location = new System.Drawing.Point(12, 85);
            this.chkRemapViaRegistry.Name = "chkRemapViaRegistry";
            this.chkRemapViaRegistry.Size = new System.Drawing.Size(186, 30);
            this.chkRemapViaRegistry.TabIndex = 1;
            this.chkRemapViaRegistry.Text = "&Remap F23 via Windows Registry\r\n(Mouseover for more details)";
            this.toolTip1.SetToolTip(this.chkRemapViaRegistry, resources.GetString("chkRemapViaRegistry.ToolTip"));
            this.chkRemapViaRegistry.UseVisualStyleBackColor = true;
            // 
            // selectKeyButton
            // 
            this.selectKeyButton.Location = new System.Drawing.Point(10, 118);
            this.selectKeyButton.Name = "selectKeyButton";
            this.selectKeyButton.Size = new System.Drawing.Size(75, 23);
            this.selectKeyButton.TabIndex = 3;
            this.selectKeyButton.Text = "Select &Key...";
            this.selectKeyButton.UseVisualStyleBackColor = true;
            this.selectKeyButton.Click += new System.EventHandler(this.selectKeyButton_Click);
            // 
            // lblKey
            // 
            this.lblKey.AutoSize = true;
            this.lblKey.Location = new System.Drawing.Point(91, 123);
            this.lblKey.Name = "lblKey";
            this.lblKey.Size = new System.Drawing.Size(50, 13);
            this.lblKey.TabIndex = 7;
            this.lblKey.Text = "Right Ctrl";
            // 
            // toolTip1
            // 
            this.toolTip1.AutoPopDelay = 32767;
            this.toolTip1.InitialDelay = 550;
            this.toolTip1.ReshowDelay = 110;
            // 
            // Form1
            // 
            this.AcceptButton = this.installButton;
            this.AutoScaleDimensions = new System.Drawing.SizeF(6F, 13F);
            this.AutoScaleMode = System.Windows.Forms.AutoScaleMode.Font;
            this.ClientSize = new System.Drawing.Size(210, 241);
            this.Controls.Add(this.lblKey);
            this.Controls.Add(this.selectKeyButton);
            this.Controls.Add(this.chkRemapViaRegistry);
            this.Controls.Add(this.label1);
            this.Controls.Add(this.installButton);
            this.Controls.Add(this.uninstallButton);
            this.Controls.Add(this.groupBox1);
            this.FormBorderStyle = System.Windows.Forms.FormBorderStyle.FixedSingle;
            this.MaximizeBox = false;
            this.MinimizeBox = false;
            this.Name = "Form1";
            this.Text = "NoCopilotKey Installer";
            this.Load += new System.EventHandler(this.Form1_Load);
            this.groupBox1.ResumeLayout(false);
            this.groupBox1.PerformLayout();
            ((System.ComponentModel.ISupportInitialize)(this.pictureBox1)).EndInit();
            this.ResumeLayout(false);
            this.PerformLayout();

        }

        #endregion

        private System.Windows.Forms.RadioButton optProgramFiles;
        private System.Windows.Forms.GroupBox groupBox1;
        private System.Windows.Forms.PictureBox pictureBox1;
        private System.Windows.Forms.RadioButton optUserProgramFiles;
        private System.Windows.Forms.Button uninstallButton;
        private System.Windows.Forms.Button installButton;
        private System.Windows.Forms.Label label1;
        private System.Windows.Forms.CheckBox chkRemapViaRegistry;
        private System.Windows.Forms.Button selectKeyButton;
        private System.Windows.Forms.Label lblKey;
        private System.Windows.Forms.ToolTip toolTip1;
    }
}