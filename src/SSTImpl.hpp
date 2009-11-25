#ifndef SST_IMPL_HPP
#define SST_IMPL_HPP

#include <boost/shared_ptr.hpp>

#include "oh/ObjectHostContext.hpp"
#include "oh/ObjectHost.hpp"
#include "Utility.hpp"

#include <sirikata/util/SerializationCheck.hpp>



namespace CBR {
class ServerIDMap;

typedef ObjectHost::SpaceNodeConnection SpaceNodeConnection;

template <typename EndObjectType>
class EndPoint {
public:
  EndObjectType endPoint;
  uint16 port;
};

template <typename EndPointType>
class BaseDatagramLayer
{
/* This function will have to be re-implemented to support sending using
     other kinds of packets. For now, I'll implement only for ODP. */
  void send(EndPoint<EndPointType> endPoint, void* data, int len);

  /* This function will also have to be re-implemented to support receiving
     using other kinds of packets. For now, I'll implement only for ODP. */
  void handleReceive(EndPoint<EndPointType> endPoint, void* data, int len);
};



template <class EndPointType>
class Stream;


typedef std::tr1::function< void(boost::shared_ptr< Stream<UUID> >) >  StreamReturnCallbackFunction;
typedef std::tr1::function< void (int, void*) >  DatagramSendDoneCallback;

typedef std::tr1::function<void (uint8*, int) >  ReadDatagramCallback;

typedef std::tr1::function<void (uint8*, int) > ReadCallback;

template <class EndPointType>
class Connection {
private:
    
    const ObjectHostContext* mContext;

public:

  typedef std::tr1::function< void(boost::shared_ptr< Connection<UUID> > )> ConnectionReturnCallbackFunction;

  Connection(const ObjectHostContext* ctx) : mContext(ctx) {

  }

  ~Connection() {

  }

  void setContext(const ObjectHostContext* ctx) {
    ctx = mContext;
  }
  
  enum ConnectionStates {
       CONNECTION_DISCONNECTED = 1,      // no network connectivity for this connection. 
                              // It has either never been connected or has 
                              // been fully disconnected.
       CONNECTION_PENDING_CONNECT = 2,   // this connection is in the process of setting
                              // up a connection. The connection setup will be
                              // complete (or fail with an error) when the 
                              // application-specified callback is invoked.
       CONNECTION_CONNECTED=3,           // The connection is connected to a remote end 
                              // point.
       CONNECTION_PENDING_DISCONNECT=4,  // The connection is in the process of 
                              // disconnecting from the remote end point.
  };

public:
  /* Creates a connection for the application to a remote 
     endpoint. The EndPoint argument specifies the location of the remote
     endpoint. It is templatized to enable it to refer to either IP 
     addresses and ports, or object identifiers. The 
     ConnectionReturnCallbackFunction returns a reference-counted, shared-
     pointer of the Connection that was created. The constructor may or 
     may not actually synchronize with the remote endpoint. Instead the 
     synchronization may be done when the first stream is created.

     @EndPoint A templatized argument specifying the remote end-point to 
               which this connection is connected.

     @ConnectionReturnCallbackFunction A callback function which will be
               called once the connection is created and will provide  a
               reference-counted, shared-pointer to the  connection. 
               ConnectionReturnCallbackFunction should have the signature
               void (boost::shared_ptr<Connection>). If the boost::shared_ptr argument
               is NULL, the connection setup failed.
  */
  static  void createConnection(EndPoint <EndPointType>, ConnectionReturnCallbackFunction);

  /* Creates a stream on top of this connection. The function also queues 
     up any initial data that needs to be sent on the stream. The function
     does not return a stream immediately since stream  creation might
     take some time and yet fail in the end. So the function returns without
     synchronizing with the remote host. Instead the callback function 
     provides a reference-counted,  shared-pointer to the stream.
     If this connection hasn't synchronized with the remote endpoint yet, 
     this function will also take care of doing that.
 
     @data A pointer to the initial data buffer that needs to be sent on
           this stream. Having this pointer removes the need for the 
           application to enqueue data until the stream is actually
           created.
    @port The length of the data buffer.
    @StreamReturnCallbackFunction A callback function which will be 
                                 called once the stream is created and
                                 the initial data queued up (or actually 
                                 sent?). The function will provide a 
                                 reference counted, shared pointer to the
                                 connection. StreamReturnCallbackFunction
                                 should have the signature void (boost::shared_ptr<Stream>).
                                 
  */
  virtual void stream(StreamReturnCallbackFunction, void* data, int length) {

  }


  /* Sends the specified data buffer using best-effort datagrams on the 
     underlying connection. This may be done using an ephemeral stream 
     on top of the underlying connection or some other mechanism (e.g.
     datagram packets sent directly on the underlying  connection). 

     @param data the buffer to send
     @param length the length of the buffer
     @param DatagramSendDoneCallback a callback of type 
                                     void (int errCode, void*)
                                     which is called when sending
                                     the datagram failed or succeeded.
                                     'errCode' contains SUCCESS or FAILURE
                                     while the 'void*' argument is a pointer
                                     to the buffer that couldn't be sent.
  */
  virtual void datagram(void* data, int length, DatagramSendDoneCallback cb) {

  }

  /*
    Register a callback which will be called when there is a datagram
    available to be read.

    @param ReadDatagramCallback a function of type "void (uint8*, int)"  
           which will be called when a datagram is available. The 
           "uint8*" field will be filled up with the received datagram,
           while the 'int' field will contain its size.
    @return true if the callback was successfully registered.
  */
  virtual bool registerReadDatagramCallback( ReadDatagramCallback cb) {

  }
 

  /*
    Register a callback which will be called when there is a new 
    datagram available to be read. In other words, datagrams we have
    seen previously will not trigger this callback.

    @param ReadDatagramCallback a function of type "void (uint8*, int)"  
           which will be called when a datagram is available. The 
           "uint8*" field will be filled up with the received datagram,
           while the 'int' field will contain its size.
    @return true if the callback was successfully registered.
  */
  virtual bool registerReadOrderedDatagramCallback( ReadDatagramCallback cb )  {

  }

  /*  Closes the connection.
 
      @param force if true, the connection is closed forcibly and 
             immediately. Otherwise, the connection is closed 
             gracefully and all outstanding packets are sent and
             acknowledged. Note that even in the latter case,
             the function returns without synchronizing with the 
             remote end point.
  */
  virtual void close(bool force) {

  }

  

  

  /*
    Returns the local endpoint to which this connection is bound.

    @return the local endpoint.
  */
  virtual EndPoint <EndPointType> localEndPoint()  {

  }

  /*
    Returns the remote endpoint to which this connection is connected.

    @return the remote endpoint.
  */
  virtual EndPoint <EndPointType> remoteEndPoint() {

  }
};

template <class EndPointType>
class Stream {
   enum StreamStates {
       DISCONNECTED = 1,       
       CONNECTED=2,
       PENDING_DISCONNECT=3,
     };
  
  /* Writes data bytes to the stream. If not all bytes can be transmitted
     immediately, they are queued locally until ready to transmit. 
     @param data the buffer containing the bytes to be written
     @param len the length of the buffer
     @return the number of bytes written or enqueued, or -1 if an error  
             occurred
  */
  virtual int write(const uint8* data, int len) {

  }

  /* Reads data bytes from a stream. If no bytes are available to be
     read immediately, it returns without reading anything. The function
     does not block if no bytes are available to read.
     @param data the buffer into which to read. If NULL, the received data
                 is discarded.
     @param len the length of the buffer
     @return the number of bytes read into the buffer.
  */
  virtual int read(uint8* data, int len) {

  }

  /* Gathers data from the buffers described in 'vec',
     which is taken to be 'count' structures long, and
     writes them to the stream. As each buffer is 
     written, it moves on to the next. If not all bytes
     can be transmitted immediately, they are queued 
     locally until ready to transmit.

     The return value is a count of bytes written.

     @param vec the array containing the iovec buffers to be written
     @param count the number of iovec buffers in the array
     @return the number of bytes written or enqueued, or -1 if an error  
             occurred
  */
  virtual int writev(const struct iovec* vec, int count) {

  }

  /* Reads data from stream and scatters it into the buffers
     described in 'vec', which is taken to be 'count' structures
     long. As each buffer is filled, data is sent to the next.

      Note that readv is not guaranteed to fill all the buffers.
      It may stop at any point, for the same reasons read would.

     @param vec the array containing the iovec buffers to be read into
     @param count the number of iovec buffers in the array
     @return the number of bytes read, or -1 if an error  
             occurred
  */
  virtual int readv(const struct iovec* vec, int count) {

  }

  /*
    Returns true if there are bytes available to read from the stream.

    @return true, if there are bytes available to read from the stream;
            false otherwise.
  */
  virtual bool pollIfReadable() {

  }
  
  /*
    Register a callback which will be called when there are bytes to be
    read from the stream. 

    @param ReadCallback a function of type "void (uint8*, int)" which will 
           be called when data is available. The "uint8*" field will be filled
           up with the received data, while the 'int' field will contain 
           the size of the data.
    @return true if the callback was successfully registered.
  */
  virtual bool registerReadCallback( ReadCallback ) {

  }

  /* Close this stream. If the 'force' parameter is 'false',
     all outstanding data is sent and acknowledged before the stream is closed.
     Otherwise, the stream is closed immediately and outstanding data may be lost. 
     Note that in the former case, the function will still return immediately, changing
     the state of the connection PENDING_DISCONNECT without necessarily talking to the
     remote endpoint.
     @param force use false if the stream should be gracefully closed, true otherwise.
     @return  true if the stream was successfully closed. 
     
  */
  virtual bool close(bool force) {

  }

  /* 
    Sets the priority of this stream.
    As in the original SST interface, this implementation gives strict preference to 
    streams with higher priority over streams with lower priority, but it divides
    available transmit bandwidth evenly among streams with the same priority level.
    All streams have a default priority level of zero.
    @param the new priority level of the stream.
  */
  virtual void setPriority(int pri) {

  }

  /*Returns the stream's current priority level.
    @return the stream's current priority level
  */
  virtual int priority() {

  }

  /* Returns the top-level connection that created this stream.
     @return a pointer to the connection that created this stream.
  */
  virtual Connection<EndPointType>* connection() {

  }
    
  /* Creates a child stream. The function also queues up
     any initial data that needs to be sent on the child stream. The function does not
     return a stream immediately since stream creation might take some time and
     yet fail in the end. So the function returns without synchronizing with the
     remote host. Instead the callback function provides a reference-counted,
     shared-pointer to the stream. If this connection hasn't synchronized with 
     the remote endpoint yet, this function will also take care of doing that.

     All the callbacks registered on the child stream will also be registered on the
     parent stream, but not vice versa. 
 
     @data A pointer to the initial data buffer that needs to be sent on this stream.
         Having this pointer removes the need for the application to enqueue data
         until the stream is actually created.
     @port The length of the data buffer.
     @StreamReturnCallbackFunction A callback function which will be called once the 
                                  stream is created and the initial data queued up
                                  (or actually sent?). The function will provide  a 
                                  reference counted, shared pointer to the  connection.
  */
  virtual void createChildStream(StreamReturnCallbackFunction, void* data, int length) {

  }


};

}

#endif
