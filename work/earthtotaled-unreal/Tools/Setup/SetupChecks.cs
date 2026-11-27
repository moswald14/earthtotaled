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
            File.WriteAllText(Path.Combine(Path.GetDirectoryName(root),"tested-install.txt"),Path.GetDirectoryName(m.Game(current.Tag)));
            File.WriteAllText(Path.Combine(Path.GetDirectoryName(root),"tested-profile.txt"),Path.GetDirectoryName(m.Saved));
        }
        static int Main(string[] args){
            Manager.EnableLongPaths();
            try{System.Net.ServicePointManager.SecurityProtocol=System.Net.SecurityProtocolType.Tls12;string root=Path.Combine(Path.GetFullPath(args[0]),Guid.NewGuid().ToString("N"));Directory.CreateDirectory(root);Tests(root,args.Contains("--online")).GetAwaiter().GetResult();
                if(args.Length>=5&&args[1]=="--packages")RealPackages(root,args[2],args[3],args[4],args.Contains("--download")).GetAwaiter().GetResult();return 0;}
            catch(Exception e){Console.WriteLine(e.ToString());return 1;}
        }
    }
}
