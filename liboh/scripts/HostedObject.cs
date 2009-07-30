using System;
using System.Runtime.CompilerServices;
namespace Meru.Runtime {


public delegate void FunctionReturnCallback(byte[] header,byte[] body);

class HostedObject{
    
    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern Guid GetUUID();

    
    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool SendMessage(byte[]message);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool CallFunction(byte[]message,FunctionReturnCallback callback);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool CallFunctionWithTimeout(byte[]message,FunctionReturnCallback callback, System.DateTime t);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void TickPeriod(System.DateTime t);

    internal static long TimeTicks(System.DateTime t) {
        return t.Ticks;
    }    
}
}