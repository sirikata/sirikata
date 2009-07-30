using System;
using System.Runtime.CompilerServices;
namespace Sirikata.Runtime {


public delegate void FunctionReturnCallback(byte[] header,byte[] body);

public class HostedObject{
    
    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    public static extern Guid GetUUID();

    
    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool iSendMessage(byte[]message);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool iCallFunction(byte[]message,FunctionReturnCallback callback);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool iCallFunctionWithTimeout(byte[]message,FunctionReturnCallback callback, System.DateTime t);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iTickPeriod(System.DateTime t);

    internal static long TimeTicks(System.DateTime t) {
        return t.Ticks;
    }
    public static  void TickPeriod(System.DateTime t){
        iTickPeriod(t);
    }
    public static bool CallFunctionWithTimeout(byte[]message,FunctionReturnCallback callback, System.DateTime t) {
        if (message==null||callback==null||message.Length==0)
            return false;
        return iCallFunctionWithTimeout(message,callback,t);
    }
    public static bool CallFunction(byte[]message,FunctionReturnCallback callback) {
        if (message==null||callback==null||message.Length==0)
            return false;
        return iCallFunction(message,callback);
    }
    public static bool SendMessage(byte[]message) {
        if (message==null||message.Length==0)
            return false;
        return iSendMessage(message);
    }
}
}