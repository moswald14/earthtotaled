using System;
using System.Drawing;
using System.Runtime.InteropServices;
using System.Windows.Forms;

// Foreground-only raw input. Legacy messages remain enabled for normal clicks.
sealed class DesktopMouse : IDisposable {
    [StructLayout(LayoutKind.Sequential)] struct Device { public ushort Page,Usage; public uint Flags; public IntPtr Target; }
    [StructLayout(LayoutKind.Sequential)] struct Header { public uint Type,Size; public IntPtr Device,Param; }
    [DllImport("user32.dll")] static extern bool RegisterRawInputDevices(Device[] devices,uint count,uint size);
    [DllImport("user32.dll")] static extern uint GetRawInputData(IntPtr input,uint command,IntPtr data,ref uint size,uint headerSize);
    [DllImport("user32.dll")] static extern IntPtr GetForegroundWindow();
    readonly Form owner; readonly Control view;
    readonly Action<bool> status; readonly Action<int,int> movement;
    readonly Timer timer = new Timer();
    readonly IntPtr buffer = Marshal.AllocHGlobal(256);
    bool captured,disposed,absoluteReady; int dx,dy,lastAbsoluteX,lastAbsoluteY;
    public DesktopMouse(Form owner,Control view,Action<bool> status,Action<int,int> movement) {
        this.owner=owner;this.view=view;this.status=status;this.movement=movement;
        timer.Interval=8; timer.Tick+=(s,e)=>Flush();timer.Start();
    }
    public bool Capture() {
        if(disposed||GetForegroundWindow()!=owner.Handle){status(false);return false;}
        if(captured){status(true);return true;}
        var device=new Device {Page=1,Usage=2,Flags=0,Target=owner.Handle};
        if(!RegisterRawInputDevices(new[]{device},1,(uint)Marshal.SizeOf(typeof(Device)))){status(false);return false;}
        dx=dy=0;absoluteReady=false;view.Focus();captured=true;
        UpdateBounds();Cursor.Position=view.PointToScreen(new Point(view.Width/2,view.Height/2));
        Cursor.Hide();status(true);return true;
    }
    public void UpdateBounds(){if(captured)Cursor.Clip=view.RectangleToScreen(view.ClientRectangle);}
    public void Release() {
        dx=dy=0;absoluteReady=false;
        if(captured){captured=false;Cursor.Clip=Rectangle.Empty;Cursor.Show();
            RegisterRawInputDevices(new[]{new Device{Page=1,Usage=2,Flags=1,Target=IntPtr.Zero}},1,(uint)Marshal.SizeOf(typeof(Device)));
        }
        if(!disposed)status(false);
    }
    public void Read(IntPtr input) {
        if(!captured||disposed)return;
        uint size=256,header=(uint)Marshal.SizeOf(typeof(Header));
        uint received=GetRawInputData(input,0x10000003,buffer,ref size,header);
        if(received==uint.MaxValue||received<header+24||Marshal.ReadInt32(buffer)!=0)return;
        int at=(int)header;ushort flags=(ushort)Marshal.ReadInt16(buffer,at);
        int x=Marshal.ReadInt32(buffer,at+12),y=Marshal.ReadInt32(buffer,at+16);
        if((flags&1)!=0){
            var screen=(flags&2)!=0?SystemInformation.VirtualScreen:Screen.PrimaryScreen.Bounds;
            int px=screen.Left+(int)(x*(double)screen.Width/65535),py=screen.Top+(int)(y*(double)screen.Height/65535);
            if(absoluteReady){dx+=px-lastAbsoluteX;dy+=py-lastAbsoluteY;}
            absoluteReady=true;lastAbsoluteX=px;lastAbsoluteY=py;
        }else{dx+=x;dy+=y;}
    }
    void Flush() {
        if(!captured)return;
        if(GetForegroundWindow()!=owner.Handle){Release();return;}
        int x=dx,y=dy;dx=dy=0;if(x!=0||y!=0)movement(x,y);
    }
    public void Dispose(){if(disposed)return;Release();disposed=true;timer.Stop();timer.Dispose();Marshal.FreeHGlobal(buffer);}
}
