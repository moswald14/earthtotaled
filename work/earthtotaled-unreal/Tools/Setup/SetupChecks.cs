using System;
using System.Collections.Generic;
using System.Diagnostics;
using System.IO;
using System.Net;
using System.Net.Http;
using System.Reflection;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Threading;
using System.Threading.Tasks;
using System.Web.Script.Serialization;

namespace Earthtotaled.Setup {
    internal static class SetupChecks {
        static int count;
        static void Check(bool good,string title) {if(!good)throw new Exception(title);Console.WriteLine("PASS "+title);count++;}
        static Release Fixture(string folder,string tag,bool bad) {
            string path=Zip(folder,tag);using(var z=ZipFile.Open(path,ZipArchiveMode.Create)) {
                foreach(var part in new[]{"EARTHtotaled.exe","EARTHtotaled/Binaries/Win64/EARTHtotaled-Win64-Shipping.exe","EARTHtotaled/Content/Paks/world.pak"})
                    using(var text=new StreamWriter(z.CreateEntry("EARTHtotaled-Unreal/"+part).Open()))text.Write(part);
                if(bad) using(var writer=new StreamWriter(z.CreateEntry("EARTHtotaled-Unreal/../outside.exe").Open()))writer.Write("bad path");
            }
            using(var s=File.OpenRead(path))using(var sha=SHA256.Create())return new Release{Tag=tag,Size=s.Length,Hash=BitConverter.ToString(sha.ComputeHash(s)).Replace("-","").ToLowerInvariant()};
        }
        static string Zip(string root,string tag){return Path.Combine(root,tag+".zip");}
        sealed class TestHttp : HttpMessageHandler {
            internal readonly List<string> Requests=new List<string>();
            internal Func<HttpRequestMessage,CancellationToken,Task<HttpResponseMessage>> Respond;
            protected override Task<HttpResponseMessage> SendAsync(HttpRequestMessage request,CancellationToken cancel) {
                Requests.Add(request.RequestUri.AbsoluteUri);return Respond(request,cancel);
            }
        }
        sealed class StalledBody : MemoryStream {
            public override async Task<int> ReadAsync(byte[] buffer,int offset,int count,CancellationToken cancel) {
                await Task.Delay(Timeout.Infinite,cancel);return 0;
            }
        }
        static async Task UpdateRecovery(string root,Manager manager) {
            string separate=Path.Combine(root,"separate profile");Directory.CreateDirectory(separate);
            string child=Path.Combine(root,"EARTHtotaled-Win64-Shipping.exe");
            File.Copy(Assembly.GetExecutingAssembly().Location,child);
            string args="--fixture-game -UserDir=\""+separate+"\"";
            using(var game=Process.Start(new ProcessStartInfo(child,args){UseShellExecute=false,CreateNoWindow=true})) {
                try {
                    Check(GameSessions.BlockingGame(manager,0)!=game.Id,"A running isolated game with a quoted save path does not block installation");
                    Check(GameSessions.BlockingGame(manager,game.Id)==game.Id,"The game that requested the update must finish saving even with a separate profile");
                    using(var cancel=new CancellationTokenSource()) {
                        bool waiting=false,cancelled=false;
                        try{await GameSessions.WaitForSaves(manager,game.Id,s=>{if(s.StartsWith("Waiting for your game")){waiting=true;cancel.Cancel();}},cancel.Token);}
                        catch(OperationCanceledException){cancelled=true;}
                        Check(waiting&&cancelled&&!game.HasExited,"Waiting names the saving step and can be cancelled without stopping the game");
                    }
                } finally{if(!game.HasExited)game.Kill();game.WaitForExit();}
            }
            Directory.CreateDirectory(manager.Saved);Directory.CreateDirectory(manager.Root);
            string primary=Path.GetDirectoryName(manager.Saved);
            var protectedArgs=new[]{"","-UserDir=relative","-UserDir=\""+primary+"\"","-UserDir=\""+manager.Root+"\"",
                "-UserDir=\""+separate+"\" -UserDir=\""+primary+"\""};
            Check(protectedArgs.All(arg=>!GameSessions.UsesSeparateSaves("game.exe "+arg,manager)),
                "Default, managed, relative and ambiguous save profiles remain protected");
            string tag="v1.19.2",name="EARTHtotaled-Unreal-Windows-"+tag+".zip",hash=new string('a',64);
            string json=new JavaScriptSerializer().Serialize(new[]{new{tag_name=tag,draft=false,prerelease=false,assets=new[]{new{name=name,
                browser_download_url=Catalog.Assets+tag+"/"+name,size=12,digest="sha256:"+hash}}}});
            var api=new TestHttp();
            api.Respond=(req,token)=>Task.FromResult(req.RequestUri.AbsoluteUri==Catalog.LatestFile?
                new HttpResponseMessage(HttpStatusCode.OK){Content=new StringContent(json)}:new HttpResponseMessage(HttpStatusCode.Forbidden));
            using(var http=new HttpClient(api)) {
                var list=await Catalog.Read(http,CancellationToken.None,null);
                Check(api.Requests.Count==2&&list.Count==1&&list[0].Hash==hash&&list[0].Size==12,"A rate-limited API recovers through the verified latest-release file");
                api.Respond=(req,token)=>Task.FromResult(new HttpResponseMessage(HttpStatusCode.ServiceUnavailable));
                bool retry=false;try{await Catalog.Read(http,CancellationToken.None,null);}catch(IOException ex){retry=ex.Message.Contains("Retry releases");}
                Check(retry,"When both GitHub routes fail Setup offers a release retry");
                api.Respond=(req,token)=>Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK){Content=new StringContent(json)});
                Check((await Catalog.Read(http,CancellationToken.None,null)).Count==1,"Retry recovers when the release list returns");
            }
            var stalled=new TestHttp();
            stalled.Respond=async(req,token)=>{await Task.Delay(Timeout.Infinite,token);return new HttpResponseMessage();};
            using(var http=new HttpClient(stalled))using(var cancel=new CancellationTokenSource()) {
                cancel.CancelAfter(70);bool stopped=false;
                try{await Catalog.Read(http,cancel.Token,null);}catch(OperationCanceledException){stopped=true;}
                Check(stopped&&stalled.Requests.Count==1,"Cancelling a slow release lookup does not start another lookup");
                bool timedOut=false;
                try{await Manager.Download(http,new Release{Tag=tag,Url=Catalog.Assets+tag+"/"+name,Size=12},Path.Combine(root,"stall.zip"),s=>{},CancellationToken.None,TimeSpan.FromMilliseconds(80));}
                catch(IOException ex){timedOut=ex.Message.Contains("Retry install");}
                Check(timedOut,"An unresponsive download stops and supplies a retry");
                stalled.Respond=(req,token)=>Task.FromResult(new HttpResponseMessage(HttpStatusCode.OK){Content=new StreamContent(new StalledBody())});
                timedOut=false;
                try{await Manager.Download(http,new Release{Tag=tag,Url=Catalog.Assets+tag+"/"+name,Size=12},Path.Combine(root,"body.zip"),s=>{},CancellationToken.None,TimeSpan.FromMilliseconds(90));}
                catch(IOException ex){timedOut=ex.Message.Contains("Retry install");}
                Check(timedOut,"A stalled transfer after headers also stops and supplies a retry");
            }
        }
        static async Task Tests(string root,bool network) {
            var m=new Manager(Path.Combine(root,"Programs"),Path.Combine(root,"Personal","Saved"));
            await UpdateRecovery(root,m);
            string slots=Path.Combine(m.Saved,"SaveGames"),mods=Path.Combine(m.Saved,"Mods");Directory.CreateDirectory(slots);Directory.CreateDirectory(mods);
            for(int i=1;i<=10;i++){File.WriteAllText(Path.Combine(slots,i+".sav"),"slot "+i);File.WriteAllText(Path.Combine(slots,i+"-previous.sav"),"previous "+i);}
            File.WriteAllText(Path.Combine(mods,"existing.earthmod.json"),"my mod");
            var r=Fixture(root,"v1.13.0",false);var old=Fixture(root,"v1.12.0",false);var future=Fixture(root,"v1.14.0",false);var bad=Fixture(root,"v1.15.0",true);
            Action<string> quiet=s=>{};int calls=0;Action req=()=>calls++;
            await m.Install(r,null,r.Number,Zip(root,r.Tag),quiet,req,false);
            Check(File.Exists(m.Game(r.Tag))&&m.Active()[1]=="primary"&&calls==1,"Fresh installation runs prerequisites and activates the game");
            await m.Install(old,r.Tag,r.Number,Zip(root,old.Tag),quiet,req,false);
            string fork=Path.Combine(m.Root,"Profiles",old.Tag,"Saved","SaveGames");
            Check(m.Active()[1]==old.Tag&&Directory.GetFiles(fork).Length==20,"Downgrade forks every slot and previous checkpoint");
            File.WriteAllText(Path.Combine(fork,"1.sav"),"older branch");
            await m.Install(r,old.Tag,r.Number,Zip(root,r.Tag),quiet,req,false);
            Check(File.ReadAllText(Path.Combine(slots,"1.sav"))=="slot 1"&&m.Active()[1]=="primary","Returning to current version preserves the original survivor");
            Check(File.ReadAllText(Path.Combine(mods,"existing.earthmod.json"))=="my mod"&&Directory.GetFiles(Path.Combine(m.Root,"Save backups"),"*.sav",SearchOption.AllDirectories).Length>=60,"Mods and backup copies survive repeated switches");
            await m.Install(old,r.Tag,r.Number,Zip(root,old.Tag),quiet,req,false);
            Check(File.ReadAllText(Path.Combine(fork,"1.sav"))=="older branch","Reopening an older version resumes its separate save");
            bool failed=false;try{await m.Install(future,r.Tag,future.Number,Zip(root,future.Tag),quiet,()=>{throw new IOException("cancelled prerequisite");},false);}catch(IOException){failed=true;}
            Check(failed&&m.Active()[0]==old.Tag,"Cancelled prerequisite leaves the previous version playable");
            await m.Install(future,r.Tag,future.Number,Zip(root,future.Tag),quiet,req,false);
            Check(m.Active()[0]==future.Tag,"A failed activation can be retried");
            failed=false;try{await m.Install(bad,null,bad.Number,Zip(root,bad.Tag),quiet,req,false);}catch(InvalidDataException){failed=true;}
            Check(failed&&m.Active()[0]==future.Tag&&!File.Exists(Path.Combine(m.Versions,"outside.exe")),"An invalid archive cannot replace the active installation");
            bad.Hash=new string('0',64);failed=false;try{Manager.VerifyArchive(Zip(root,bad.Tag),bad);}catch(IOException){failed=true;}
            Check(failed,"A mismatched download checksum is rejected");
            using(var cancelled=new CancellationTokenSource()) {
                failed=false;try{await m.Install(r,future.Tag,future.Number,Zip(root,r.Tag),quiet,()=>cancelled.Cancel(),false,cancelled.Token);}catch(OperationCanceledException){failed=true;}
                Check(failed&&m.Active()[0]==future.Tag&&File.ReadAllText(Path.Combine(slots,"1.sav"))=="slot 1","Cancelling before activation keeps the playable version and original saves");
            }
            Console.WriteLine("Installed Microsoft Visual C++ detected: "+Runtime.HasVC()+" / GameInput detected: "+Runtime.HasInput());
            if(network){var list=await Catalog.Read();Check(list.Any(x=>x.Tag=="v1.12.0")&&list.All(x=>x.Hash.Length==64),"Live public GitHub version list has verified Windows downloads");}
            Console.WriteLine(count+" installer checks passed.");
        }
        static Release PackageRecord(string file) {
            const string prefix="EARTHtotaled-Unreal-Windows-";
            var name=Path.GetFileNameWithoutExtension(file);
            if(!name.StartsWith(prefix,StringComparison.Ordinal)||!Catalog.TagPattern.IsMatch(name.Substring(prefix.Length)))
                throw new ArgumentException("Pass a versioned EARTHtotaled Windows release ZIP.");
            using(var input=File.OpenRead(file))using(var sha=SHA256.Create())
                return new Release{Tag=name.Substring(prefix.Length),Size=input.Length,Hash=BitConverter.ToString(sha.ComputeHash(input)).Replace("-","").ToLowerInvariant()};
        }
        static async Task RealPackages(string root,string previous,string candidate,string donor,bool download) {
            var m=new Manager(Path.Combine(root,"Installed"),Path.Combine(root,"Actual","Saved"));
            string dir=Path.Combine(m.Saved,"SaveGames");Directory.CreateDirectory(dir);
            // Use distinct real checkpoints when the test profile has them. A slot swap must fail.
            var donors=new[]{donor}.Concat(Directory.GetFiles(Path.GetDirectoryName(donor),"EARTHtotaled-Unreal-*.sav")
                .OrderBy(p=>p,StringComparer.OrdinalIgnoreCase)).Distinct(StringComparer.OrdinalIgnoreCase).ToArray();
            int next=0;
            for(int i=1;i<=10;i++)foreach(string suffix in new[]{"","-previous"})
                File.Copy(donors[next++%donors.Length],Path.Combine(dir,"EARTHtotaled-Unreal-"+i.ToString("00")+suffix+".sav"));
            var originals=Directory.GetFiles(dir).ToDictionary(p=>Path.GetFileName(p),p=>File.ReadAllBytes(p));
            Func<string,bool> intact=folder=>originals.All(pair=>File.Exists(Path.Combine(folder,pair.Key))&&
                pair.Value.SequenceEqual(File.ReadAllBytes(Path.Combine(folder,pair.Key))));
            var oldFile=PackageRecord(previous);var newFile=PackageRecord(candidate);
            var old=oldFile;var current=newFile;
            if(download){
                var list=await Catalog.Read();
                old=list.First(r=>r.Tag==oldFile.Tag);current=list.First(r=>r.Tag==newFile.Tag);
                Check(list[0].Tag==current.Tag,"The public updater offers "+newFile.Tag+" first");
                Check(old.Hash==oldFile.Hash&&old.Size==oldFile.Size&&current.Hash==newFile.Hash&&current.Size==newFile.Size,
                    "Both public releases match the verified local archives");
            }
            Check(old.Number<current.Number,"The previous and new packages are in upgrade order");
            await m.Install(old,null,old.Number,previous,Console.WriteLine,Runtime.Ensure,false);
            Check(File.Exists(m.Game(old.Tag)),"The real previous GitHub ZIP installs");
            await m.Install(current,old.Tag,current.Number,download?null:candidate,Console.WriteLine,Runtime.Ensure,false);
            Check(File.Exists(m.Game(current.Tag))&&File.Exists(Path.Combine(m.Versions,current.Tag,"EARTHtotaled","Content","UpdateTools","EARTHtotaled-Setup.exe")),"The new Windows package installs with its in-game updater");
            Check(m.Active()[0]==current.Tag&&m.Active()[1]=="primary"&&intact(dir),"All ten survivor slots and previous checkpoints survive the upgrade byte for byte");
            var backups=Directory.GetDirectories(Path.Combine(m.Root,"Save backups"));
            Check(backups.Length>=2&&backups.All(b=>intact(Path.Combine(b,"Primary","SaveGames"))),"The original survivor files are present in every upgrade backup");
            await m.Install(old,current.Tag,current.Number,previous,Console.WriteLine,Runtime.Ensure,false);
            var fork=Path.Combine(m.Root,"Profiles",old.Tag,"Saved","SaveGames");
            Check(m.Active()[1]==old.Tag&&intact(fork)&&intact(dir),"Returning to the previous release forks all twenty files");
            await m.Install(current,old.Tag,current.Number,candidate,Console.WriteLine,Runtime.Ensure,false);
            Check(m.Active()[0]==current.Tag&&m.Active()[1]=="primary"&&intact(dir)&&intact(fork),"Switching forward again resumes the original survivor files");
            string oldSetup=Path.Combine(m.Versions,old.Tag,"EARTHtotaled","Content","UpdateTools","EARTHtotaled-Setup.exe");
            string currentSetup=Path.Combine(m.Versions,current.Tag,"EARTHtotaled","Content","UpdateTools","EARTHtotaled-Setup.exe");
            File.Copy(oldSetup,m.Launcher);
            m.InstallVersionLauncher(current.Tag,oldSetup);
            Check(File.ReadAllBytes(currentSetup).SequenceEqual(File.ReadAllBytes(m.Launcher))&&intact(dir),
                "Installing a new package updates the stable launcher before the game starts");
            dynamic shell=Activator.CreateInstance(Type.GetTypeFromProgID("WScript.Shell"));
            string pin=Path.Combine(root,"test-play.lnk"),legacy=Path.Combine(root,"old-portable","EARTHtotaled-Win64-Shipping.exe");
            dynamic link=shell.CreateShortcut(pin);link.TargetPath=legacy;link.Save();
            m.RepairPlayLink(shell,pin,new[]{legacy});
            Check(String.Equals((string)shell.CreateShortcut(pin).TargetPath,m.Launcher,StringComparison.OrdinalIgnoreCase),
                "An existing shortcut to a recorded portable game follows the installed version");
            link=shell.CreateShortcut(pin);link.TargetPath=legacy;link.Arguments="-windowed";link.Save();
            m.RepairPlayLink(shell,pin,new[]{legacy});
            Check(String.Equals((string)shell.CreateShortcut(pin).TargetPath,legacy,StringComparison.OrdinalIgnoreCase),
                "A customized shortcut keeps its target and options");
            File.Copy(oldSetup,m.Launcher,true);
            m.RefreshLauncher(currentSetup);
            Check(File.ReadAllBytes(currentSetup).SequenceEqual(File.ReadAllBytes(m.Launcher))&&intact(dir),
                "Launching a legacy upgrade refreshes the stable Start menu launcher without touching saves");
            m.RefreshLauncher(oldSetup);
            Check(File.ReadAllBytes(currentSetup).SequenceEqual(File.ReadAllBytes(m.Launcher)),
                "An inactive older game cannot downgrade the selected version manager");
            File.WriteAllText(Path.Combine(Path.GetDirectoryName(root),"tested-install.txt"),Path.GetDirectoryName(m.Game(current.Tag)));
            File.WriteAllText(Path.Combine(Path.GetDirectoryName(root),"tested-profile.txt"),Path.GetDirectoryName(m.Saved));
        }
        static int Main(string[] args){
            if(args.Contains("--fixture-game")){Thread.Sleep(18000);return 0;}
            Manager.EnableLongPaths();
            try{System.Net.ServicePointManager.SecurityProtocol=System.Net.SecurityProtocolType.Tls12;string root=Path.Combine(Path.GetFullPath(args[0]),Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);Tests(root,args.Contains("--online")).GetAwaiter().GetResult();
                if(args.Length>=5&&args[1]=="--packages")RealPackages(root,args[2],args[3],args[4],args.Contains("--download")).GetAwaiter().GetResult();return 0;}
            catch(Exception e){Console.WriteLine(e.ToString());return 1;}
        }
    }
}
