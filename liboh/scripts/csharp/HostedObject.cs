using System;
using System.Runtime.CompilerServices;
namespace Sirikata.Runtime {


public delegate bool FunctionReturnCallback(byte[] header,byte[] body);

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

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern Sirikata.Runtime.Time iGetTime(Guid spaceid);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern Sirikata.Runtime.Time iGetTimeByteArray(byte[] spaceid);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern Sirikata.Runtime.Time iGetTimeString(string spaceid);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern Sirikata.Runtime.Time iGetLocalTime( );


    internal static long TimeTicks(Sirikata.Runtime.Time t) {
        return t.microseconds();
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
    public Sirikata.Runtime.Time GetTime(Guid spaceid){
        return iGetTime(spaceid);
    }
    public static Sirikata.Runtime.Time GetLocalTime() {
        return iGetLocalTime();
    }
    public static Sirikata.Runtime.Time GetTimeFromByteArraySpace(byte[] spaceid){
        if (spaceid==null) {
            return GetLocalTime();
        }
/*
        byte[] newArray=new byte[16];
        for (int i=0;i<16;++i) {
            newArray[i]=(System.Byte)spaceid[i];
        }
*/

        return iGetTimeByteArray(spaceid);

        //return GetLocalTime();
    }
    public static Sirikata.Runtime.Time GetTimeFromStringSpace(string spaceid){
        if (spaceid==null) {
            return GetLocalTime();
        }
        return iGetTimeString(spaceid);
    }
}
}