using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Threading.Tasks;
using System.Windows.Forms;

[assembly: AssemblyTitle("EARTHtotaled Setup")]
[assembly: AssemblyProduct("EARTHtotaled")]
[assembly: AssemblyFileVersion("1.13.0.0")]
[assembly: AssemblyVersion("1.13.0.0")]

namespace Earthtotaled.Setup {
    internal sealed class SetupForm : Form {
        readonly Manager manager=Manager.Standard();
        readonly ComboBox choices=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,Dock=DockStyle.Fill};
        readonly Label note=new Label{Dock=DockStyle.Fill,AutoSize=false};
        readonly Button install=new Button{Text="Install and play",AutoSize=true,Enabled=false};
        readonly Button close=new Button{Text="Close",AutoSize=true};
        readonly ProgressBar progress=new ProgressBar{Dock=DockStyle.Fill,Style=ProgressBarStyle.Blocks};
        readonly string selected,old,origin;readonly int wait;
        bool busy;
        internal SetupForm(string tag,string from,int pid,string source) {
            selected=tag;old=from;wait=pid;origin=source;Text="EARTHtotaled   |   Install and update";StartPosition=FormStartPosition.CenterScreen;
            Icon=Icon.ExtractAssociatedIcon(Assembly.GetExecutingAssembly().Location);
            Font=new Font("Segoe UI",10);AutoScaleMode=AutoScaleMode.Dpi;ClientSize=new Size(570,320);MinimumSize=new Size(510,340);
            var grid=new TableLayoutPanel{Dock=DockStyle.Fill,Padding=new Padding(22),ColumnCount=1,RowCount=6};
            foreach(int h in new[]{42,33,38,88,17,44}) grid.RowStyles.Add(new RowStyle(SizeType.Absolute,h));
            grid.Controls.Add(new Label{Text="EARTHtotaled",Font=new Font("Segoe UI",19,FontStyle.Bold),AutoSize=true});
            grid.Controls.Add(new Label{Text="Choose a Windows release from GitHub",AutoSize=true});
            grid.Controls.Add(choices);grid.Controls.Add(note);grid.Controls.Add(progress);
            var buttons=new FlowLayoutPanel{Dock=DockStyle.Fill,FlowDirection=FlowDirection.RightToLeft};buttons.Controls.Add(close);buttons.Controls.Add(install);grid.Controls.Add(buttons);
            Controls.Add(grid);close.Click+=(s,e)=>Close();install.Click+=async(s,e)=>await Apply();
            FormClosing+=(s,e)=>{if(busy)e.Cancel=true;};Shown+=async(s,e)=>await LoadChoices();
        }
        void Status(string s){if(IsDisposed)return;if(InvokeRequired){BeginInvoke(new Action<string>(Status),s);return;}note.Text=s;}
        async Task LoadChoices() {
            try {
                Status("Contacting GitHub…");var releases=await Catalog.Read();if(releases.Count==0)throw new IOException("No compatible Windows releases were found.");
                foreach(var r in releases)choices.Items.Add(r);
                var chosen=selected==null?releases[0]:releases.FirstOrDefault(r=>r.Tag==selected);
                if(chosen==null)throw new IOException("The selected release is unavailable on GitHub.");
                choices.SelectedItem=chosen;install.Enabled=true;
                Status("Setup installs missing Microsoft runtimes and backs up all survivor slots. Older versions use a separate copy. Windows may ask for permission to install a runtime.");
                if(selected!=null) await Apply();
            } catch(Exception e){Status(e.GetBaseException().Message);}
        }
        async Task Apply() {
            var r=choices.SelectedItem as Release;if(busy||r==null)return;
            busy=true;install.Enabled=false;choices.Enabled=false;close.Enabled=false;progress.Style=ProgressBarStyle.Marquee;
            bool done=false;
            try {
                if(wait>0) await Task.Run(()=>{try{using(var p=Process.GetProcessById(wait)){if(!p.HasExited&&p.ProcessName.Equals("EARTHtotaled-Win64-Shipping",StringComparison.OrdinalIgnoreCase)&&!p.WaitForExit(90000))throw new IOException("Close the game and retry.");}}catch(ArgumentException){}});
                foreach(var p in Process.GetProcessesByName("EARTHtotaled-Win64-Shipping")) using(p) {if(!p.HasExited)throw new IOException("Save and close EARTHtotaled, then choose Install and play again.");}
                string sibling=Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),"EARTHtotaled-Unreal-Windows-"+r.Tag+".zip");
                var latest=((Release)choices.Items[0]).Number;
                await Task.Run(()=>manager.Install(r,old,latest,sibling,Status,Runtime.Ensure,true));
                manager.RememberPortable(origin);
                if(Runtime.RestartRecommended) {MessageBox.Show(this,"Installation is complete. Restart Windows, then open EARTHtotaled from Start.","EARTHtotaled Setup",MessageBoxButtons.OK,MessageBoxIcon.Information);}
                else manager.Play();
                done=true;Status("Installation complete.");
            } catch(Exception e){Status(e.GetBaseException().Message);}
            finally{busy=false;install.Enabled=true;choices.Enabled=true;close.Enabled=true;progress.Style=ProgressBarStyle.Blocks;}
            if(done)Close();
        }
    }
    internal static class Program {
        [STAThread] static void Main(string[] args) {
            Manager.EnableLongPaths();
            ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
            Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
            try {
                string name=Path.GetFileName(Assembly.GetExecutingAssembly().Location);
                if(name.Equals("EARTHtotaled.exe",StringComparison.OrdinalIgnoreCase)&&args.Length==0&&File.Exists(Manager.Standard().Pointer)) {Manager.Standard().Play();return;}
                string tag=null,from=null,origin=null;int pid=0;
                for(int i=0;i<args.Length;i++) {
                    if(args[i]=="--manage")continue;
                    if(i+1>=args.Length)throw new ArgumentException("Incomplete setup option.");
                    string action=args[i],value=args[++i];
                    if(action=="--wait"&&int.TryParse(value,out pid)&&pid>0)continue;
                    if(action=="--origin"){origin=value;continue;}
                    if(!Catalog.TagPattern.IsMatch(value))throw new ArgumentException("Invalid release version.");
                    if(action=="--install")tag=value;else if(action=="--from")from=value;else throw new ArgumentException("Unknown setup option.");
                }
                Application.Run(new SetupForm(tag,from,pid,origin));
            } catch(Exception e){MessageBox.Show(e.GetBaseException().Message,"EARTHtotaled Setup",MessageBoxButtons.OK,MessageBoxIcon.Error);}
        }
    }
}
