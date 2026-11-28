using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.Drawing;
using System.IO;
using System.Linq;
using System.Net;
using System.Reflection;
using System.Threading;
using System.Threading.Tasks;
using System.Windows.Forms;

[assembly: AssemblyTitle("EARTHtotaled Setup")]
[assembly: AssemblyProduct("EARTHtotaled")]
[assembly: AssemblyFileVersion("1.19.1.0")]
[assembly: AssemblyVersion("1.19.1.0")]

namespace Earthtotaled.Setup {
    internal sealed class SetupForm : Form {
        readonly Manager manager=Manager.Standard();
        readonly ComboBox choices=new ComboBox{DropDownStyle=ComboBoxStyle.DropDownList,Dock=DockStyle.Fill};
        readonly Label note=new Label{Dock=DockStyle.Fill,AutoSize=false};
        readonly Button install=new Button{Text="Install and play",AutoSize=true,Enabled=false};
        readonly Button close=new Button{Text="Close",AutoSize=true};
        readonly ProgressBar progress=new ProgressBar{Dock=DockStyle.Fill,Style=ProgressBarStyle.Blocks};
        readonly string selected,old,origin;readonly int wait;
        CancellationTokenSource operation;
        bool closeWhenIdle;
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
            Controls.Add(grid);
            close.Click+=(s,e)=>{if(operation==null)Close();else Cancel();};
            install.Click+=async(s,e)=>{if(choices.SelectedItem==null)await LoadChoices();else await Apply();};
            FormClosing+=(s,e)=>{if(operation!=null){e.Cancel=true;closeWhenIdle=true;Cancel();}};
            Shown+=async(s,e)=>await LoadChoices();
        }
        void Status(string message) {
            if(IsDisposed||Disposing)return;
            if(InvokeRequired){BeginInvoke(new Action<string>(Status),message);return;}
            if(operation!=null&&operation.IsCancellationRequested)return;
            note.Text=message;
            int percent=0,space=message.LastIndexOf(' ');
            bool measured=message.EndsWith("%")&&space>=0&&int.TryParse(message.Substring(space+1).TrimEnd('%'),out percent);
            progress.Style=measured||operation==null?ProgressBarStyle.Blocks:ProgressBarStyle.Marquee;
            if(measured)progress.Value=Math.Max(0,Math.Min(100,percent));
            if(!measured||percent%25==0) {
                try{Directory.CreateDirectory(manager.Root);File.AppendAllText(Path.Combine(manager.Root,"setup.log"),DateTime.Now.ToString("s")+"  "+message+Environment.NewLine);}
                catch(IOException){}catch(UnauthorizedAccessException){}
            }
        }
        CancellationToken StartOperation() {
            operation=new CancellationTokenSource();
            choices.Enabled=false;install.Enabled=false;close.Enabled=true;close.Text="Cancel";
            progress.Value=0;progress.Style=ProgressBarStyle.Marquee;
            return operation.Token;
        }
        void EndOperation() {
            operation.Dispose();operation=null;progress.Style=ProgressBarStyle.Blocks;progress.Value=0;
            choices.Enabled=choices.Items.Count>0;install.Enabled=true;close.Text="Close";close.Enabled=true;
        }
        void Cancel() {
            if(operation.IsCancellationRequested)return;
            note.Text="Cancelling at the next safe step…";close.Enabled=false;operation.Cancel();
        }
        async Task LoadChoices() {
            if(operation!=null)return;
            var cancel=StartOperation();choices.Items.Clear();
            bool autoInstall=false;string message;
            try {
                Status("Loading Windows releases from GitHub…");
                var releases=await Catalog.Read(cancel,Status);
                cancel.ThrowIfCancellationRequested();
                foreach(var release in releases)choices.Items.Add(release);
                var requested=releases.FirstOrDefault(r=>r.Tag==selected);
                choices.SelectedItem=requested??releases[0];
                autoInstall=requested!=null;
                install.Text="Install and play";
                message=selected!=null&&requested==null?
                    selected+" is unavailable. Choose Install and play to use "+releases[0].Tag+", or close Setup.":
                    "Setup backs up your survivor slots and installs missing Microsoft runtimes. Older versions use a separate copy of your saves.";
            } catch(OperationCanceledException){message="Release check cancelled.";install.Text="Retry releases";}
              catch(Exception e){message=e.Message;install.Text="Retry releases";}
            finally{EndOperation();}
            if(closeWhenIdle){Close();return;}
            Status(message);
            if(autoInstall)await Apply();
        }
        async Task Apply() {
            var r=choices.SelectedItem as Release;if(operation!=null||r==null)return;
            var cancel=StartOperation();bool done=false;string message;
            try {
                await GameSessions.WaitForSaves(manager,wait,Status,cancel);
                string sibling=Path.Combine(Path.GetDirectoryName(Assembly.GetExecutingAssembly().Location),"EARTHtotaled-Unreal-Windows-"+r.Tag+".zip");
                var latest=((Release)choices.Items[0]).Number;
                await Task.Run(()=>manager.Install(r,old,latest,sibling,Status,Runtime.Ensure,true,cancel,origin));
                if(Runtime.RestartRecommended) {MessageBox.Show(this,"Installation is complete. Restart Windows, then open EARTHtotaled from Start.","EARTHtotaled Setup",MessageBoxButtons.OK,MessageBoxIcon.Information);}
                else if(!closeWhenIdle&&!cancel.IsCancellationRequested)manager.Play();
                done=true;message=r.Tag+" installed.";
            } catch(OperationCanceledException){message="Update cancelled. You can retry or close Setup.";}
              catch(Exception e){message=e.Message;}
            finally{EndOperation();}
            Status(message);install.Text="Retry install";
            if(done||closeWhenIdle)Close();
        }
    }
    internal static class Program {
        [STAThread] static void Main(string[] args) {
            Manager.EnableLongPaths();
            if(args.Length==1&&args[0]=="--refresh-launcher") {
                // Older installers copy themselves into Start. Refresh it after the selected game opens.
                var manager=Manager.Standard();
                for(int attempt=0;attempt<4;attempt++) {
                    try {manager.RefreshLauncher(Assembly.GetExecutingAssembly().Location);return;}
                    catch(IOException){}catch(UnauthorizedAccessException){}
                    Thread.Sleep(350);
                }
                return;
            }
            try {
                string name=Path.GetFileName(Assembly.GetExecutingAssembly().Location);
                if(name.Equals("EARTHtotaled.exe",StringComparison.OrdinalIgnoreCase)&&args.Length==0&&File.Exists(Manager.Standard().Pointer)) {Manager.Standard().Play();return;}
                ServicePointManager.SecurityProtocol=SecurityProtocolType.Tls12;
                Application.EnableVisualStyles();Application.SetCompatibleTextRenderingDefault(false);
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
