using System;
using System.Runtime.CompilerServices;
namespace Sirikata.Runtime {


public delegate bool FunctionReturnCallback(byte[] header,byte[] body);
public delegate void TimeoutCallback();

/** HostedObject bridges the C++/.NET gap. The public interface should be
 *  stable, but is a relatively thin wrapper around the internal interface.  The
 *  internal methods which actually cross the barrier *MUST* be marked as
 *  internal so the C++ implementation can change as needed.
 */
public class HostedObject {

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iInternalID(out Guid result);

    /** Get this object's internal UUID. */
    public static Guid InternalID() {
        Guid result = new Guid();
        iInternalID(out result);
        return result;
    }

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iObjectReference(ref Guid space, out Guid result);

    /** Get this object's reference within the given space.  If the object isn't
     *  connected to the space, returns a null Guid.
     */
    public static Guid ObjectReference(Guid space) {
        Guid result = new Guid();
        iObjectReference(ref space, out result);
        return result;
    }


    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool iSendMessage(ref Guid dest_space, ref Guid dest_obj, uint dest_port, byte[] payload);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool iCallFunction(byte[]message,FunctionReturnCallback callback);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern bool iCallFunctionWithTimeout(byte[]message,FunctionReturnCallback callback, Sirikata.Runtime.TimeClass t);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iTickPeriod(Sirikata.Runtime.TimeClass t);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iGetTime(Guid spaceid,  Sirikata.Runtime.TimeClass retval);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iGetTimeByteArray(byte[] spaceid,  Sirikata.Runtime.TimeClass retval);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iGetTimeString(string spaceid,  Sirikata.Runtime.TimeClass retval);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iGetLocalTime( Sirikata.Runtime.TimeClass retval);

    [MethodImplAttribute(MethodImplOptions.InternalCall)]
    internal static extern void iAsyncWait(TimeoutCallback callback, Sirikata.Runtime.TimeClass t);

    internal static long TimeTicks(Sirikata.Runtime.Time t) {
        return t.microseconds();
    }
    public static  void SetupTickFunction(TimeoutCallback tc, Sirikata.Runtime.Time t){

        long us=t.microseconds();
        TimeoutCallback wrapped_tc=null;
        long estLocalTime=GetLocalTime().microseconds()+us*2;
        wrapped_tc=new TimeoutCallback(delegate(){
                try {
                    tc();
                }finally {
                    long delta=estLocalTime-GetLocalTime().microseconds();
                    estLocalTime+=us;
                    if (delta>0) {
                        iAsyncWait(wrapped_tc,new TimeClass((ulong)delta));
                    }else {
                        iAsyncWait(wrapped_tc,new TimeClass());
                    }
                }
            });
        iAsyncWait(wrapped_tc,t.toClass());
    }
    public static bool AsyncWait(TimeoutCallback callback, Sirikata.Runtime.Time t) {
        if (callback==null) return false;
        iAsyncWait(callback,t.toClass());
        return true;
    }
    public static bool CallFunctionWithTimeout(byte[]message,FunctionReturnCallback callback, Sirikata.Runtime.Time t) {
        if (message==null||callback==null||message.Length==0)
            return false;
        return iCallFunctionWithTimeout(message,callback,t.toClass());
    }
    public static bool CallFunction(byte[]message,FunctionReturnCallback callback) {
        if (message==null||callback==null||message.Length==0)
            return false;
        return iCallFunction(message,callback);
    }
    public static bool SendMessage(Guid dest_space, Guid dest_obj, uint dest_port, byte[] payload) {
        if (payload == null || payload.Length == 0)
            return false;
        return iSendMessage(ref dest_space, ref dest_obj, dest_port, payload);
    }
    public static Sirikata.Runtime.Time GetTime(Guid spaceid){
        Sirikata.Runtime.TimeClass retval=new Sirikata.Runtime.TimeClass();
        iGetTime(spaceid,retval);
        return new Sirikata.Runtime.Time(retval);
    }
    public static Sirikata.Runtime.Time GetLocalTime() {
        Sirikata.Runtime.TimeClass retval=new Sirikata.Runtime.TimeClass();
        iGetLocalTime(retval);
        return new Sirikata.Runtime.Time(retval);
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
        Sirikata.Runtime.TimeClass retval=new Sirikata.Runtime.TimeClass();
        iGetTimeByteArray(spaceid,retval);
        return new Sirikata.Runtime.Time(retval);
        //return GetLocalTime();
    }
    public static Sirikata.Runtime.Time GetTimeFromStringSpace(string spaceid){
        if (spaceid==null) {
            return GetLocalTime();
        }
        Sirikata.Runtime.TimeClass retval=new Sirikata.Runtime.TimeClass();
        iGetTimeString(spaceid,retval);
        return new Sirikata.Runtime.Time(retval);
    }
}
}