using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Net.Http;
using System.Reflection;
using System.Security.Cryptography;
using System.Text;
using System.Text.RegularExpressions;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;
using Microsoft.Win32;
using System.Runtime.InteropServices;

namespace Earthtotaled.Setup {
    internal sealed class Release {
        public string Tag, Url, Hash;
        public long Size;
        public Version Number { get { return new Version(Tag.Substring(1)); } }
        public override string ToString() { return Tag; }
    }
    internal sealed class Catalog {
        internal const string Api="https://api.github.com/repos/moswald14/earthtotaled/releases";
        internal const string LatestFile="https://github.com/moswald14/earthtotaled/releases/latest/download/EARTHtotaled-Windows-update.json";
        internal const string Assets="https://github.com/moswald14/earthtotaled/releases/download/";
        internal static readonly Regex TagPattern=new Regex(@"^v(0|[1-9]\d{0,6})\.(0|[1-9]\d{0,6})\.(0|[1-9]\d{0,6})$");
        internal static readonly HttpClient Http=MakeClient();
        static HttpClient MakeClient() {
            var http=new HttpClient { Timeout=System.Threading.Timeout.InfiniteTimeSpan };
            http.DefaultRequestHeaders.UserAgent.ParseAdd("EARTHtotaled-Setup/1.19.2");return http;
        }
        internal static Release Parse(Dictionary<string,object> row) {
            string tag=Convert.ToString(row["tag_name"]);
            if(!TagPattern.IsMatch(tag)||(bool)row["draft"]||(bool)row["prerelease"]) return null;
            string name="EARTHtotaled-Unreal-Windows-"+tag+".zip", url=Assets+tag+"/"+name;
            foreach(Dictionary<string,object> a in (System.Collections.IEnumerable)row["assets"]) {
                object digest;
                if((string)a["name"]!=name||(string)a["browser_download_url"]!=url||!a.TryGetValue("digest",out digest)) continue;
                string hash=Convert.ToString(digest);
                if(Regex.IsMatch(hash,"^sha256:[a-f0-9]{64}$")&&Convert.ToInt64(a["size"])>0)
                    return new Release { Tag=tag, Url=url, Hash=hash.Substring(7), Size=Convert.ToInt64(a["size"]) };
            }
            return null;
        }
        internal static Task<List<Release>> Read(CancellationToken cancel=default(CancellationToken),Action<string> status=null) {
            return Read(Http,cancel,status);
        }
        static async Task<Dictionary<string,object>[]> Page(HttpClient http,string url,CancellationToken cancel) {
            using(var deadline=CancellationTokenSource.CreateLinkedTokenSource(cancel)) {
                deadline.CancelAfter(TimeSpan.FromSeconds(14));
                using(var response=await http.GetAsync(url,deadline.Token).ConfigureAwait(false)) {
                    response.EnsureSuccessStatusCode();
                    string body=await response.Content.ReadAsStringAsync().ConfigureAwait(false);
                    return new JavaScriptSerializer{MaxJsonLength=32*1024*1024}.Deserialize<Dictionary<string,object>[]>(body);
                }
            }
        }
        internal static async Task<List<Release>> Read(HttpClient http,CancellationToken cancel,Action<string> status) {
            var found=new List<Release>();
            try {
                for(int page=1;;page++) {
                    var rows=await Page(http,Api+"?per_page=100&page="+page,cancel).ConfigureAwait(false);
                    foreach(var row in rows) {var release=Parse(row);if(release!=null)found.Add(release);}
                    if(rows.Length<100)break;
                }
                if(found.Count!=0)return found.OrderByDescending(r=>r.Number).ToList();
            } catch(Exception){cancel.ThrowIfCancellationRequested();}
            if(status!=null)status("Checking the latest Windows release directly on GitHub…");
            try {
                var rows=await Page(http,LatestFile,cancel).ConfigureAwait(false);
                var latest=rows.Select(Parse).Where(r=>r!=null).OrderByDescending(r=>r.Number).FirstOrDefault();
                if(latest==null)throw new InvalidDataException("No verified Windows package was listed.");
                return new List<Release>{latest};
            } catch(Exception e) {
                cancel.ThrowIfCancellationRequested();
                throw new IOException("Could not load Windows releases from GitHub. Check your connection and choose Retry releases. "+e.Message,e);
            }
        }
    }
    internal sealed class Manager {
        internal readonly string Root, Saved;
        internal Manager(string root,string saved) { Root=Path.GetFullPath(root);Saved=Path.GetFullPath(saved); }
        internal static void EnableLongPaths() {
            AppContext.SetSwitch("Switch.System.IO.UseLegacyPathHandling",false);
            AppContext.SetSwitch("Switch.System.IO.BlockLongPaths",false);
        }
        internal static Manager Standard() {
            var local=Environment.GetFolderPath(Environment.SpecialFolder.LocalApplicationData);
            return new Manager(Path.Combine(local,"Programs","EARTHtotaled"),Path.Combine(local,"EARTHtotaled","Saved"));
        }
        internal string Versions {get{return Path.Combine(Root,"Versions");}}
        internal string Launcher {get{return Path.Combine(Root,"EARTHtotaled.exe");}}
        internal string Pointer {get{return Path.Combine(Root,"active.txt");}}
        internal string[] Active() {return File.Exists(Pointer)?File.ReadAllLines(Pointer):new string[0];}
        internal string Game(string tag){return Path.Combine(Versions,tag,"EARTHtotaled.exe");}
        internal static void CopyTree(string from,string to) {
            if(!Directory.Exists(from)) return;
            if((File.GetAttributes(from)&FileAttributes.ReparsePoint)!=0) throw new IOException("A linked save directory needs a manual backup.");
            Directory.CreateDirectory(to);
            foreach(var f in Directory.GetFiles(from)) {
                if((File.GetAttributes(f)&FileAttributes.ReparsePoint)!=0) throw new IOException("A linked save file needs a manual backup.");
                File.Copy(f,Path.Combine(to,Path.GetFileName(f)),false);
            }
            foreach(var d in Directory.GetDirectories(from)) CopyTree(d,Path.Combine(to,Path.GetFileName(d)));
        }
        static void Snapshot(string from,string to) {
            foreach(var section in new[]{"SaveGames","Mods","Config","SaveExports"}) CopyTree(Path.Combine(from,section),Path.Combine(to,section));
        }
        static void AtomicText(string path,string[] lines) {
            string tmp=path+"."+Guid.NewGuid().ToString("N")+".tmp";
            File.WriteAllLines(tmp,lines);if(File.Exists(path)) File.Replace(tmp,path,null);else File.Move(tmp,path);
        }
        internal static void VerifyArchive(string zip,Release r) {
            if(new FileInfo(zip).Length!=r.Size) throw new IOException("The download is incomplete. Please retry.");
            using(var stream=File.OpenRead(zip)) using(var sha=SHA256.Create())
                if(BitConverter.ToString(sha.ComputeHash(stream)).Replace("-","").ToLowerInvariant()!=r.Hash)
                    throw new IOException("The ZIP checksum differs from GitHub's release record. Please retry.");
        }
        internal static void Unpack(string zip,string stage,CancellationToken cancel=default(CancellationToken)) {
            Directory.CreateDirectory(stage);
            using(var z=ZipFile.OpenRead(zip)) {
                long bytes=0;const string prefix="EARTHtotaled-Unreal/";
                if(z.Entries.Count>24000) throw new InvalidDataException("Too many files in this release.");
                foreach(var e in z.Entries) {
                    cancel.ThrowIfCancellationRequested();
                    string key=e.FullName;
                    if(!key.StartsWith(prefix,StringComparison.Ordinal)||key.IndexOf('\\')>=0||key.Contains(":")) throw new InvalidDataException("Unexpected archive layout.");
                    string relative=key.Substring(prefix.Length);
                    if(relative==""||relative.EndsWith("/")) continue;
                    string[] parts=relative.Split('/');
                    if(parts.Any(x=>x==""||x=="."||x==".."||x.TrimEnd(' ','.')!=x||Regex.IsMatch(x,@"^(con|prn|aux|nul|com\d|lpt\d)(\.|$)",RegexOptions.IgnoreCase)||x.IndexOfAny(Path.GetInvalidFileNameChars())>=0||x.Equals("Saved",StringComparison.OrdinalIgnoreCase))||relative.EndsWith(".sav",StringComparison.OrdinalIgnoreCase))
                        throw new InvalidDataException("Unexpected file in the game archive.");
                    string dest=Path.GetFullPath(Path.Combine(stage,relative));
                    if(!dest.StartsWith(Path.GetFullPath(stage)+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase)) throw new InvalidDataException("Invalid release path.");
                    bytes+=e.Length;if(bytes>12L*1024*1024*1024) throw new InvalidDataException("The release exceeds the installation limit.");
                    Directory.CreateDirectory(Path.GetDirectoryName(dest));e.ExtractToFile(dest,false);
                }
            }
            if(!File.Exists(Path.Combine(stage,"EARTHtotaled.exe"))||!File.Exists(Path.Combine(stage,"EARTHtotaled","Binaries","Win64","EARTHtotaled-Win64-Shipping.exe"))||!Directory.Exists(Path.Combine(stage,"EARTHtotaled","Content","Paks"))) throw new InvalidDataException("The Unreal game is incomplete.");
        }
        internal static async Task Download(HttpClient http,Release release,string zip,Action<string> status,CancellationToken cancel,TimeSpan idle) {
            status("Connecting to the "+release.Tag+" download…");
            using(var deadline=CancellationTokenSource.CreateLinkedTokenSource(cancel)) {
                try {
                    deadline.CancelAfter(idle);
                    using(var response=await http.GetAsync(release.Url,HttpCompletionOption.ResponseHeadersRead,deadline.Token).ConfigureAwait(false)) {
                        response.EnsureSuccessStatusCode();
                        using(var stop=deadline.Token.Register(response.Dispose))
                        using(var input=await response.Content.ReadAsStreamAsync().ConfigureAwait(false))
                        using(var output=File.Create(zip)) {
                            byte[] buffer=new byte[131072];long total=0;int last=-1;
                            status("Downloading "+release.Tag+"   0%");
                            while(true) {
                                deadline.CancelAfter(idle);
                                int n=await input.ReadAsync(buffer,0,buffer.Length,deadline.Token).ConfigureAwait(false);
                                deadline.Token.ThrowIfCancellationRequested();
                                if(n==0)break;
                                total+=n;if(total>release.Size)throw new IOException("The download exceeded the published release size.");
                                await output.WriteAsync(buffer,0,n,cancel).ConfigureAwait(false);
                                int percent=(int)(total*100/release.Size);
                                if(percent!=last){last=percent;status("Downloading "+release.Tag+"   "+percent+"%");}
                            }
                        }
                    }
                } catch(Exception) {
                    cancel.ThrowIfCancellationRequested();
                    if(deadline.IsCancellationRequested)throw new IOException("GitHub stopped sending the download. Check your connection and choose Retry install.");
                    throw;
                }
            }
        }
        internal async Task Install(Release release,string from,Version latest,string sibling,Action<string> status,Action prereqs,bool register,CancellationToken cancel=default(CancellationToken),string portableOrigin=null) {
            cancel.ThrowIfCancellationRequested();
            if(!Catalog.TagPattern.IsMatch(release.Tag)) throw new InvalidDataException("Invalid release.");
            Directory.CreateDirectory(Versions);string target=Path.Combine(Versions,release.Tag);
            string work=Path.Combine(Versions,".stage-"+Guid.NewGuid().ToString("N"));
            using(var locker=new FileStream(Path.Combine(Root,"install.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None)) {
                Directory.CreateDirectory(work);
                try {
                    if(!File.Exists(Path.Combine(target,".release"))||File.ReadAllText(Path.Combine(target,".release"))!=release.Hash||!File.Exists(Game(release.Tag))) {
                        if(Directory.Exists(target)) throw new IOException("This version's installation is incomplete or changed. Rename its folder in Versions before reinstalling.");
                        string zip=Path.Combine(work,"release.zip");
                        if(sibling!=null&&File.Exists(sibling)) {status("Checking the local release ZIP…");zip=sibling;}
                        else await Download(Catalog.Http,release,zip,status,cancel,TimeSpan.FromSeconds(40)).ConfigureAwait(false);
                        cancel.ThrowIfCancellationRequested();
                        status("Verifying the "+release.Tag+" download…");VerifyArchive(zip,release);
                        status("Unpacking "+release.Tag+"…");
                        var staged=Path.Combine(work,"Game");Unpack(zip,staged,cancel);File.WriteAllText(Path.Combine(staged,".release"),release.Hash);
                        Directory.Move(staged,target);
                    }
                    // Complete all prerequisites and backups before selecting a new startup version.
                    cancel.ThrowIfCancellationRequested();
                    status("Checking Microsoft prerequisites. Windows may ask you to approve a runtime installer.");prereqs();
                    cancel.ThrowIfCancellationRequested();
                    var previous=Active();
                    Version high=release.Number;
                    foreach(string old in new[]{from,previous.Length>2?previous[2]:""})
                        if(old!=null&&Catalog.TagPattern.IsMatch(old)&&new Version(old.Substring(1))>high) high=new Version(old.Substring(1));
                    bool older=release.Number<high||(latest!=null&&release.Number<latest);
                    string profile=older?release.Tag:"primary";
                    string currentSource=previous.Length>1&&Catalog.TagPattern.IsMatch(previous[1])?Path.Combine(Root,"Profiles",previous[1],"Saved"):Saved;
                    string destinationBackup=Path.Combine(Root,"Save backups",DateTime.UtcNow.ToString("yyyyMMdd-HHmmss-fff")+"-"+Guid.NewGuid().ToString("N").Substring(0,6));
                    status("Backing up survivor slots, settings and mods…");
                    Snapshot(Saved,Path.Combine(destinationBackup,"Primary"));
                    if(currentSource!=Saved) Snapshot(currentSource,Path.Combine(destinationBackup,"Previous version"));
                    cancel.ThrowIfCancellationRequested();
                    if(older) {
                        string fork=Path.Combine(Root,"Profiles",release.Tag);
                        if(!Directory.Exists(fork)) {
                            string tmp=Path.Combine(work,"Profile");Directory.CreateDirectory(tmp);Snapshot(currentSource,Path.Combine(tmp,"Saved"));
                            cancel.ThrowIfCancellationRequested();
                            Directory.CreateDirectory(Path.GetDirectoryName(fork));Directory.Move(tmp,fork);
                        }
                    }
                    status("Finishing the installation…");
                    if(register) {
                        RememberPortable(portableOrigin);
                        InstallLauncher(release.Tag);
                    }
                    AtomicText(Pointer,new[]{release.Tag,profile,"v"+high});
                } finally {
                    string name=Path.GetFileName(work);
                    if(Path.GetDirectoryName(work)==Versions&&name.StartsWith(".stage-")&&Directory.Exists(work)) Directory.Delete(work,true);
                }
            }
        }
        static Version ToolVersion(string file) {
            var v=FileVersionInfo.GetVersionInfo(file);
            return new Version(v.FileMajorPart,v.FileMinorPart,v.FileBuildPart,v.FilePrivatePart);
        }
        void CopyNewerLauncher(string source) {
            if(source.Equals(Launcher,StringComparison.OrdinalIgnoreCase))return;
            if(File.Exists(Launcher)&&ToolVersion(Launcher)>=ToolVersion(source))return;
            string tmp=Path.Combine(Root,"launcher-"+Guid.NewGuid().ToString("N")+".exe");
            try {
                File.Copy(source,tmp);
                if(File.Exists(Launcher))File.Replace(tmp,Launcher,null);else File.Move(tmp,Launcher);
            } finally{if(File.Exists(tmp))File.Delete(tmp);}
        }
        internal void RefreshLauncher(string source) {
            if(!File.Exists(Pointer))return;
            using(var locker=new FileStream(Path.Combine(Root,"install.lock"),FileMode.OpenOrCreate,FileAccess.ReadWrite,FileShare.None)) {
                var active=Active();
                if(active.Length<2||!Catalog.TagPattern.IsMatch(active[0]))return;
                string versionDir=Path.Combine(Versions,active[0]);
                string bundled=Path.Combine(versionDir,"EARTHtotaled","Content","UpdateTools","EARTHtotaled-Setup.exe");
                if(Path.GetFullPath(source).Equals(bundled,StringComparison.OrdinalIgnoreCase)&&File.Exists(Path.Combine(versionDir,".release")))
                    CopyNewerLauncher(bundled);
            }
        }
        internal void InstallVersionLauncher(string tag,string installer) {
            string bundled=Path.Combine(Versions,tag,"EARTHtotaled","Content","UpdateTools","EARTHtotaled-Setup.exe");
            string source=File.Exists(bundled)&&ToolVersion(bundled)>ToolVersion(installer)?bundled:installer;
            try { CopyNewerLauncher(source); }
            catch(IOException ex) {
                // A running version manager can keep playing the selected game until the next install.
                int error=ex.HResult&65535;
                if(!File.Exists(Launcher)||(error!=32&&error!=33))throw;
            }
        }
        internal void RepairPlayLink(dynamic shell,string file,string[] origins) {
            if(!File.Exists(file))return;
            dynamic link=shell.CreateShortcut(file);
            string target=link.TargetPath;
            if(!String.IsNullOrEmpty((string)link.Arguments)||!origins.Contains(target,StringComparer.OrdinalIgnoreCase))return;
            link.TargetPath=Launcher;link.WorkingDirectory=Root;link.Save();
        }
        void InstallLauncher(string tag) {
            InstallVersionLauncher(tag,Assembly.GetExecutingAssembly().Location);
            string programs=Environment.GetFolderPath(Environment.SpecialFolder.Programs);
            string menu=Path.Combine(programs,"EARTHtotaled");Directory.CreateDirectory(menu);
            dynamic shell=Activator.CreateInstance(Type.GetTypeFromProgID("WScript.Shell"));
            foreach(var title in new[]{"EARTHtotaled","Manage EARTHtotaled versions"}) {
                dynamic link=shell.CreateShortcut(Path.Combine(menu,title+".lnk"));link.TargetPath=Launcher;link.WorkingDirectory=Root;
                link.Arguments=title=="EARTHtotaled"?"":"--manage";link.Save();
            }
            string recorded=Path.Combine(Root,"portable-origins.txt");
            if(File.Exists(recorded)) {
                string[] origins=File.ReadAllLines(recorded);
                string taskbar=Path.Combine(Environment.GetFolderPath(Environment.SpecialFolder.ApplicationData),"Microsoft","Internet Explorer","Quick Launch","User Pinned","TaskBar");
                foreach(string directory in new[]{taskbar,programs}) foreach(string name in new[]{"EARTHtotaled.lnk","EARTHtotaled Unreal.lnk"}) {
                    try { RepairPlayLink(shell,Path.Combine(directory,name),origins); }
                    catch(System.Runtime.InteropServices.COMException) {}
                    catch(UnauthorizedAccessException) {}
                    catch(IOException) {}
                }
            }
        }
        internal void RememberPortable(string executable) {
            if(String.IsNullOrEmpty(executable)) return;
            string origin=Path.GetFullPath(executable);
            if(!String.Equals(Path.GetFileName(origin),"EARTHtotaled-Win64-Shipping.exe",StringComparison.OrdinalIgnoreCase)||!File.Exists(origin)||origin.StartsWith(Versions+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase)) return;
            string file=Path.Combine(Root,"portable-origins.txt");
            var entries=File.Exists(file)?File.ReadAllLines(file).ToList():new List<string>();
            if(!entries.Contains(origin,StringComparer.OrdinalIgnoreCase)){entries.Add(origin);AtomicText(file,entries.ToArray());}
        }
        internal void Play() {
            var active=Active();if(active.Length<2||!Catalog.TagPattern.IsMatch(active[0])||!File.Exists(Game(active[0]))) throw new IOException("Select and install a version first.");
            string args=Catalog.TagPattern.IsMatch(active[1])?"-UserDir=\""+Path.Combine(Root,"Profiles",active[1])+"\"":"";
            Process.Start(new ProcessStartInfo(Game(active[0]),args){UseShellExecute=false,WorkingDirectory=Path.GetDirectoryName(Game(active[0]))});
        }
    }
    internal static class Runtime {
        [DllImport("msi.dll",CharSet=CharSet.Unicode)] static extern uint MsiEnumRelatedProducts(string upgrade,int reserved,int index,StringBuilder product);
        [DllImport("msi.dll",CharSet=CharSet.Unicode)] static extern uint MsiGetProductInfo(string product,string property,StringBuilder value,ref int length);
        internal static bool HasVC() {
            using(var reg=RegistryKey.OpenBaseKey(RegistryHive.LocalMachine,RegistryView.Registry64))
            using(var key=reg.OpenSubKey(@"SOFTWARE\Microsoft\VisualStudio\14.0\VC\Runtimes\x64")) {
                Version v;return key!=null&&Convert.ToInt32(key.GetValue("Installed",0))==1&&Version.TryParse(Convert.ToString(key.GetValue("Version","")).TrimStart('v'),out v)&&v>=new Version(14,50,35719,0);
            }
        }
        internal static bool HasInput() {
            for(int i=0;;i++) {
                var id=new StringBuilder(40);if(MsiEnumRelatedProducts("{EB53D3A0-F53C-50FF-BB3B-13C5B310D4DA}",0,i,id)!=0) return false;
                var text=new StringBuilder(128);int n=text.Capacity;Version v;
                if(MsiGetProductInfo(id.ToString(),"VersionString",text,ref n)==0&&Version.TryParse(text.ToString(),out v)&&v>=new Version(10,1,26100,6879)) return true;
            }
        }
        internal static bool RestartRecommended {get; private set;}
        internal static void Ensure() {
            if(!Environment.Is64BitOperatingSystem) throw new PlatformNotSupportedException("EARTHtotaled needs 64-bit Windows.");
            if(HasVC()&&HasInput()) return;
            string temp=Path.Combine(Path.GetTempPath(),"EARTHtotaled-runtime-"+Guid.NewGuid().ToString("N"));Directory.CreateDirectory(temp);
            try {
                if(!HasVC()) Run(temp,"vc_redist.x64.exe",false);
                if(!HasInput()) Run(temp,"GameInputRedist.msi",true);
            } finally {foreach(string item in new[]{"vc_redist.x64.exe","GameInputRedist.msi"}) File.Delete(Path.Combine(temp,item));Directory.Delete(temp);}
        }
        static void Run(string dir,string name,bool msi) {
            string file=Path.Combine(dir,name);
            using(var data=Assembly.GetExecutingAssembly().GetManifestResourceStream(name)) using(var outFile=File.Create(file)) data.CopyTo(outFile);
            var cmd=new ProcessStartInfo(msi?Path.Combine(Environment.SystemDirectory,"msiexec.exe"):file,msi?"/i \""+file+"\" /passive /norestart":"/install /passive /norestart");
            cmd.UseShellExecute=true;cmd.Verb="runas";using(var p=Process.Start(cmd)) {
                p.WaitForExit();if(p.ExitCode==3010||p.ExitCode==1641) RestartRecommended=true;
                if(p.ExitCode!=0&&p.ExitCode!=3010&&p.ExitCode!=1641&&!(msi?HasInput():HasVC())) throw new IOException("Microsoft prerequisite installation returned "+p.ExitCode+".");
            }
        }
    }
}
