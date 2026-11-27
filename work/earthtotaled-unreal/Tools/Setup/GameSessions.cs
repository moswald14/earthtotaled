using System;
using System.Diagnostics;
using System.IO;
using System.Management;
using System.Runtime.InteropServices;
using System.Threading;
using System.Threading.Tasks;

namespace Earthtotaled.Setup {
    internal static class GameSessions {
        const string ShippingName="EARTHtotaled-Win64-Shipping";
        [DllImport("shell32.dll",CharSet=CharSet.Unicode)]
        static extern IntPtr CommandLineToArgvW(string line,out int count);
        [DllImport("kernel32.dll")] static extern IntPtr LocalFree(IntPtr ptr);

        static bool ContainsPath(string parent,string child) {
            parent=parent.TrimEnd(Path.DirectorySeparatorChar);
            child=child.TrimEnd(Path.DirectorySeparatorChar);
            return child.Equals(parent,StringComparison.OrdinalIgnoreCase)||
                child.StartsWith(parent+Path.DirectorySeparatorChar,StringComparison.OrdinalIgnoreCase);
        }
        internal static bool UsesSeparateSaves(string commandLine,Manager manager) {
            if(String.IsNullOrWhiteSpace(commandLine))return false;
            int count;
            var args=CommandLineToArgvW(commandLine,out count);
            if(args==IntPtr.Zero)return false;
            try {
                string profile=null;
                for(int i=1;i<count;i++) {
                    var arg=Marshal.PtrToStringUni(Marshal.ReadIntPtr(args,i*IntPtr.Size));
                    if(!arg.StartsWith("-UserDir=",StringComparison.OrdinalIgnoreCase))continue;
                    if(profile!=null)return false;
                    profile=arg.Substring("-UserDir=".Length);
                }
                // Only skip local, absolute, existing profiles with an unambiguous save location.
                if(String.IsNullOrEmpty(profile)||profile.Length<3||!Char.IsLetter(profile[0])||
                    profile[1]!=':'||(profile[2]!='\\'&&profile[2]!='/'))return false;
                var dir=new DirectoryInfo(profile);
                if(!dir.Exists)return false;
                string primary=Path.GetDirectoryName(manager.Saved);
                foreach(var protectedPath in new[]{primary,manager.Root}) {
                    if(ContainsPath(protectedPath,dir.FullName)||ContainsPath(dir.FullName,protectedPath))return false;
                }
                for(var parent=dir;parent!=null;parent=parent.Parent)
                    if((parent.Attributes&FileAttributes.ReparsePoint)!=0)return false;
                // A profile can redirect just its Saved folder or individual backed-up sections.
                foreach(string suffix in new[]{"Saved","Saved/SaveGames","Saved/Config","Saved/Mods","Saved/SaveExports"}) {
                    string path=Path.Combine(dir.FullName,suffix);
                    if(Directory.Exists(path)&&(File.GetAttributes(path)&FileAttributes.ReparsePoint)!=0)return false;
                }
                return true;
            } catch(ArgumentException){return false;}
              catch(IOException){return false;}
              catch(UnauthorizedAccessException){return false;}
              catch(NotSupportedException){return false;}
            finally{LocalFree(args);}
        }
        static string CommandLine(int pid) {
            try {
                using(var process=new ManagementObject("Win32_Process.Handle='"+pid+"'")) {
                    process.Get(); return Convert.ToString(process["CommandLine"]);
                }
            } catch(ManagementException){return null;} catch(UnauthorizedAccessException){return null;}
              catch(COMException){return null;}
        }
        internal static int BlockingGame(Manager manager,int caller) {
            int blocked=0;
            foreach(var game in Process.GetProcessesByName(ShippingName))using(game) {
                try {
                    if(game.HasExited)continue;
                    if(game.Id!=caller&&UsesSeparateSaves(CommandLine(game.Id),manager))continue;
                    if(!game.HasExited&&(blocked==0||game.Id==caller))blocked=game.Id;
                } catch(InvalidOperationException){} // The game may finish while reading its command line.
            }
            return blocked;
        }
        internal static async Task WaitForSaves(Manager manager,int caller,Action<string> status,CancellationToken cancel) {
            status("Checking whether EARTHtotaled is still saving…");
            var elapsed=Stopwatch.StartNew();
            while(true) {
                cancel.ThrowIfCancellationRequested();
                int pid=await Task.Run(()=>BlockingGame(manager,caller),cancel).ConfigureAwait(false);
                if(pid==0)return;
                if(elapsed.Elapsed.TotalSeconds>=25)
                    throw new IOException("EARTHtotaled is still open (process "+pid+"). Save and close the game, then choose Retry install.");
                status(pid==caller?"Waiting for your game to finish saving and close…":
                    "Another EARTHtotaled session is open (process "+pid+"). Save and close that game to continue.");
                await Task.Delay(800,cancel).ConfigureAwait(false);
            }
        }
    }
}
