using System;
using System.IO;
using System.IO.Compression;
using System.Linq;
using System.Security.Cryptography;
using System.Threading.Tasks;

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
        static async Task Tests(string root,bool network) {
            var m=new Manager(Path.Combine(root,"Programs"),Path.Combine(root,"Personal","Saved"));
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
            Console.WriteLine("Installed Microsoft Visual C++ detected: "+Runtime.HasVC()+" / GameInput detected: "+Runtime.HasInput());
            if(network){var list=await Catalog.Read();Check(list.Any(x=>x.Tag=="v1.12.0")&&list.All(x=>x.Hash.Length==64),"Live public GitHub version list has verified Windows downloads");}
            Console.WriteLine(count+" installer checks passed.");
        }
        static async Task RealPackages(string root,string previous,string candidate,string donor) {
            var m=new Manager(Path.Combine(root,"Installed"),Path.Combine(root,"Actual","Saved"));
            string dir=Path.Combine(m.Saved,"SaveGames");Directory.CreateDirectory(dir);string slot=Path.Combine(dir,"EARTHtotaled-Unreal-01.sav");
            File.Copy(donor,slot);
            var list=await Catalog.Read();var old=list.First(r=>r.Tag=="v1.12.0");var current=list.FirstOrDefault(r=>r.Tag=="v1.13.0");
            if(current==null)using(var s=File.OpenRead(candidate))using(var sha=SHA256.Create())current=new Release{Tag="v1.13.0",Size=s.Length,Hash=BitConverter.ToString(sha.ComputeHash(s)).Replace("-","").ToLowerInvariant()};
            await m.Install(old,null,old.Number,previous,Console.WriteLine,Runtime.Ensure,false);
            Check(File.Exists(m.Game(old.Tag)),"The real previous GitHub ZIP installs");
            await m.Install(current,old.Tag,current.Number,candidate,Console.WriteLine,Runtime.Ensure,false);
            Check(File.Exists(m.Game(current.Tag))&&File.Exists(Path.Combine(m.Versions,current.Tag,"EARTHtotaled","Content","UpdateTools","EARTHtotaled-Setup.exe")),"The new Windows package installs with its in-game updater");
            await m.Install(old,current.Tag,current.Number,previous,Console.WriteLine,Runtime.Ensure,false);
            var fork=Path.Combine(m.Root,"Profiles",old.Tag,"Saved","SaveGames","EARTHtotaled-Unreal-01.sav");
            var bytes=File.ReadAllBytes(donor);
            Check(bytes.SequenceEqual(File.ReadAllBytes(fork))&&bytes.SequenceEqual(File.ReadAllBytes(slot)),"Choosing the real previous release forks a byte-identical copy of the existing survivor");
            await m.Install(current,old.Tag,current.Number,candidate,Console.WriteLine,Runtime.Ensure,false);
            Check(bytes.SequenceEqual(File.ReadAllBytes(slot))&&m.Active()[0]==current.Tag,"Upgrading again selects Landfall and keeps the survivor intact");
            File.WriteAllText(Path.Combine(Path.GetDirectoryName(root),"tested-install.txt"),Path.GetDirectoryName(m.Game(current.Tag)));
        }
        static int Main(string[] args){
            Manager.EnableLongPaths();
            try{System.Net.ServicePointManager.SecurityProtocol=System.Net.SecurityProtocolType.Tls12;string root=Path.Combine(Path.GetFullPath(args[0]),Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);Tests(root,args.Contains("--online")).GetAwaiter().GetResult();
                if(args.Length==5&&args[1]=="--packages")RealPackages(root,args[2],args[3],args[4]).GetAwaiter().GetResult();return 0;}
            catch(Exception e){Console.WriteLine(e.ToString());return 1;}
        }
    }
}
