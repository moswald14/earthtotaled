using System;
using System.Drawing;
using System.IO;
using System.Reflection;
using System.Runtime.InteropServices;
using System.Threading;
using System.Windows.Forms;
using Microsoft.Web.WebView2.Core;
using Microsoft.Web.WebView2.WinForms;

static class Bootstrap {
    [DllImport("kernel32.dll", CharSet=CharSet.Unicode)] static extern bool SetDllDirectory(string path);
    [STAThread] static void Main() {
        try {
            var folder = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"EARTHtotaled","Runtime","1.0.4314.35");
            Directory.CreateDirectory(folder);
            Extract("CoreDll",Path.Combine(folder,"Microsoft.Web.WebView2.Core.dll"));
            Extract("FormsDll",Path.Combine(folder,"Microsoft.Web.WebView2.WinForms.dll"));
            Extract("LoaderDll",Path.Combine(folder,"WebView2Loader.dll"));
            SetDllDirectory(folder);
            AppDomain.CurrentDomain.AssemblyResolve += (sender,args) => {
                var name = new AssemblyName(args.Name).Name;
                if (name != "Microsoft.Web.WebView2.Core" && name != "Microsoft.Web.WebView2.WinForms") return null;
                return Assembly.LoadFrom(Path.Combine(folder,name+".dll"));
            };
            // Reflection keeps SDK types out of this entry point's initial JIT.
            Assembly.GetExecutingAssembly().GetType("Program").GetMethod("Run",BindingFlags.Static|BindingFlags.Public).Invoke(null,null);
        } catch (Exception ex) { MessageBox.Show("EARTHtotaled could not start.\n\n"+(ex.InnerException??ex).Message,"EARTHtotaled"); }
    }
    static void Extract(string resource,string file) {
        using (var input=Assembly.GetExecutingAssembly().GetManifestResourceStream(resource))
        using (var memory=new MemoryStream()) {
            input.CopyTo(memory);var bytes=memory.ToArray();
            if (File.Exists(file)) {
                var existing=File.ReadAllBytes(file);bool same=existing.Length==bytes.Length;
                if(same)for(int i=0;i<bytes.Length;i++)if(existing[i]!=bytes[i]){same=false;break;}
                if(same)return;
            }
            File.WriteAllBytes(file,bytes);
        }
    }
}

static class Program {
    internal const string AppId = "CoralSorta.EARTHtotaled";
    [DllImport("shell32.dll")] static extern int SetCurrentProcessExplicitAppUserModelID(string id);
    [DllImport("user32.dll")] static extern bool SetProcessDPIAware();
    [DllImport("user32.dll", CharSet=CharSet.Unicode)] static extern IntPtr FindWindow(string cls, string title);
    [DllImport("user32.dll")] static extern bool SetForegroundWindow(IntPtr window);
    public static void Run() {
        bool first;
        using (var mutex = new Mutex(true, "Local\\" + AppId, out first)) {
            if (!first) { SetForegroundWindow(FindWindow(null, "EARTHtotaled")); return; }
            SetProcessDPIAware(); SetCurrentProcessExplicitAppUserModelID(AppId);
            Application.EnableVisualStyles(); Application.SetCompatibleTextRenderingDefault(false);
            Application.Run(new GameWindow());
        }
    }
}

sealed class GameWindow : Form {
    readonly WebView2 web = new WebView2();
    readonly Label loading = new Label();
    bool fullscreen, closing;
    DesktopMouse mouse;
    Rectangle windowBounds;
    public GameWindow() {
        Text = "EARTHtotaled"; MinimumSize = new Size(800, 500);
        BackColor = Color.FromArgb(17,28,24); StartPosition = FormStartPosition.CenterScreen;
        ClientSize = new Size(1280,720);
        using (var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("GameIcon")) Icon = new Icon(stream);
        web.Dock = DockStyle.Fill; web.DefaultBackgroundColor = BackColor; Controls.Add(web);
        loading.Text = "EARTHtotaled\nOpening the last signal…"; loading.ForeColor = Color.FromArgb(216,238,140);
        loading.BackColor = BackColor; loading.Font = new Font("Segoe UI",24); loading.TextAlign = ContentAlignment.MiddleCenter;
        loading.Dock = DockStyle.Fill; Controls.Add(loading); loading.BringToFront();
        Shown += async (s,e) => {
            SetFullscreen(true);
            try {
                var data = Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData),"EARTHtotaled");
                var content = Path.Combine(data,"Game"); Directory.CreateDirectory(content);
                byte[] game;
                using (var stream = Assembly.GetExecutingAssembly().GetManifestResourceStream("GameHtml"))
                using (var memory = new MemoryStream()) { stream.CopyTo(memory); game = memory.ToArray(); }
                var htmlPath = Path.Combine(content,"index.html");
                // Stable application origin and profile retain saves across game updates.
                File.WriteAllBytes(htmlPath, game);
                var env = await CoreWebView2Environment.CreateAsync(null,Path.Combine(data,"Profile"));
                await web.EnsureCoreWebView2Async(env);
                mouse = new DesktopMouse(this,web,
                    captured => SendMouseMessage("{\"type\":\"earth:capture\",\"captured\":"+(captured?"true":"false")+"}"),
                    (dx,dy) => SendMouseMessage("{\"type\":\"earth:mouse\",\"dx\":"+dx+",\"dy\":"+dy+"}"));
                web.CoreWebView2.WebMessageReceived += (sender,args) => {
                    if(args.Source!="https://earthtotaled.local/index.html")return;
                    string command;try{command=args.TryGetWebMessageAsString();}catch{return;}
                    if(command=="earth:capture"&&!closing)mouse.Capture();
                    if(command=="earth:release")mouse.Release();
                };
                using(var bridge=Assembly.GetExecutingAssembly().GetManifestResourceStream("DesktopBridge"))
                using(var reader=new StreamReader(bridge))await web.CoreWebView2.AddScriptToExecuteOnDocumentCreatedAsync(reader.ReadToEnd());
                web.CoreWebView2.ProcessFailed += (sender,args) => mouse.Release();
                web.SizeChanged += (sender,args) => mouse.UpdateBounds();
                web.CoreWebView2.SetVirtualHostNameToFolderMapping("earthtotaled.local",content,CoreWebView2HostResourceAccessKind.DenyCors);
                web.CoreWebView2.Settings.AreDefaultContextMenusEnabled = false;
                web.CoreWebView2.Settings.AreDevToolsEnabled = false;
                web.CoreWebView2.Settings.IsStatusBarEnabled = false;
                web.CoreWebView2.Settings.IsZoomControlEnabled = false;
                web.CoreWebView2.Settings.AreBrowserAcceleratorKeysEnabled = false;
                web.CoreWebView2.NavigationStarting += (sender,args) => {
                    if (!args.Uri.StartsWith("https://earthtotaled.local/",StringComparison.OrdinalIgnoreCase)) args.Cancel = true;
                };
                web.CoreWebView2.NewWindowRequested += (sender,args) => { args.Handled = true; };
                web.CoreWebView2.ContainsFullScreenElementChanged += (sender,args) => SetFullscreen(web.CoreWebView2.ContainsFullScreenElement);
                web.KeyDown += (sender,args) => {
                    if (args.KeyCode == Keys.F11 || (args.KeyCode == Keys.Enter && ModifierKeys.HasFlag(Keys.Alt))) {
                        args.Handled = true; args.SuppressKeyPress = true; BeginInvoke((Action)(()=>SetFullscreen(!fullscreen)));
                    }
                    if (args.KeyCode == Keys.F4 && ModifierKeys.HasFlag(Keys.Alt)) {
                        args.Handled = true; args.SuppressKeyPress = true; BeginInvoke((Action)Close);
                    }
                };
                web.CoreWebView2.NavigationCompleted += (sender,args) => {
                    if (args.IsSuccess) { loading.Visible = false; web.Focus(); }
                    else loading.Text = "The game could not load.\nClose and reopen EARTHtotaled.";
                };
                web.Source = new Uri("https://earthtotaled.local/index.html");
            } catch (Exception ex) {
                SetFullscreen(false);
                MessageBox.Show(this,"EARTHtotaled could not start.\n\n"+ex.Message+"\n\nMicrosoft Edge WebView2 Runtime is required.","EARTHtotaled",MessageBoxButtons.OK,MessageBoxIcon.Error);
                closing = true; Close();
            }
        };
        FormClosing += async (s,e) => {
            if(mouse!=null)mouse.Release();
            if (closing || web.CoreWebView2 == null) return;
            e.Cancel = true; closing = true;
            try { await web.CoreWebView2.ExecuteScriptAsync("window.dispatchEvent(new Event('beforeunload'));"); } catch { }
            BeginInvoke((Action)Close);
        };
        FormClosed += (s,e) => {if(mouse!=null)mouse.Dispose();};
        Deactivate += (s,e) => {if(mouse!=null)mouse.Release();};
    }
    void SendMouseMessage(string message){
        if(IsDisposed||web.CoreWebView2==null)return;
        try{web.CoreWebView2.PostWebMessageAsJson(message);}catch{ }
    }
    protected override void WndProc(ref Message message){
        if(message.Msg==0x00FF&&mouse!=null)mouse.Read(message.LParam);
        base.WndProc(ref message);
    }
    void SetFullscreen(bool enabled) {
        if (fullscreen == enabled) return;
        if (enabled) { windowBounds = Bounds; WindowState = FormWindowState.Normal; FormBorderStyle = FormBorderStyle.None; Bounds = Screen.FromControl(this).Bounds; }
        else { FormBorderStyle = FormBorderStyle.Sizable; Bounds = windowBounds; }
        fullscreen = enabled;
    }
}
