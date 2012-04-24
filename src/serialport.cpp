
#include "serialport.h"

v8::Handle<v8::Value> Open(const v8::Arguments& args) {
  v8::HandleScope scope;

  // path
  if(!args[0]->IsString()) {
    return scope.Close(v8::ThrowException(v8::Exception::TypeError(v8::String::New("First argument must be a string"))));
  }
  v8::String::Utf8Value path(args[0]->ToString());

  // options
  if(!args[1]->IsObject()) {
    return scope.Close(v8::ThrowException(v8::Exception::TypeError(v8::String::New("Second argument must be an object"))));
  }
  v8::Local<v8::Object> options = args[1]->ToObject();

  // callback
  if(!args[2]->IsFunction()) {
    return scope.Close(v8::ThrowException(v8::Exception::TypeError(v8::String::New("Third argument must be a function"))));
  }
  v8::Local<v8::Value> callback = args[2];

  OpenBaton* baton = new OpenBaton();
  memset(baton, 0, sizeof(OpenBaton));
  strcpy(baton->path, *path);
  baton->baudRate = options->Get(v8::String::New("baudRate"))->ToInt32()->Int32Value();
  baton->dataBits = options->Get(v8::String::New("dataBits"))->ToInt32()->Int32Value();
  baton->parity = ToParityEnum(options->Get(v8::String::New("parity"))->ToString());
  baton->stopBits = ToStopBitEnum(options->Get(v8::String::New("stopBits"))->ToNumber()->NumberValue());
  baton->callback = v8::Persistent<v8::Value>::New(callback);
  baton->dataCallback = v8::Persistent<v8::Value>::New(options->Get(v8::String::New("dataCallback")));
  baton->errorCallback = v8::Persistent<v8::Value>::New(options->Get(v8::String::New("errorCallback")));

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Open, EIO_AfterOpen);

  return scope.Close(v8::Undefined());
}

void EIO_AfterOpen(uv_work_t* req) {
  OpenBaton* data = static_cast<OpenBaton*>(req->data);

  v8::Handle<v8::Value> argv[2];
  if(data->errorString[0]) {
    argv[0] = v8::Exception::Error(v8::String::New(data->errorString));
    argv[1] = v8::Undefined();
  } else {
    argv[0] = v8::Undefined();
    argv[1] = v8::Int32::New(data->result);
    AfterOpenSuccess(data->result, data->dataCallback, data->errorCallback);
  }
  v8::Function::Cast(*data->callback)->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  data->callback.Dispose();
  delete data;
  delete req;
}

v8::Handle<v8::Value> Write(const v8::Arguments& args) {
  v8::HandleScope scope;

  // file descriptor
  if(!args[0]->IsInt32()) {
    return scope.Close(v8::ThrowException(v8::Exception::TypeError(v8::String::New("First argument must be an int"))));
  }
  int fd = args[0]->ToInt32()->Int32Value();

  // buffer
  if(!args[1]->IsObject() || !node::Buffer::HasInstance(args[1])) {
    return scope.Close(v8::ThrowException(v8::Exception::TypeError(v8::String::New("Second argument must be a buffer"))));
  }
  v8::Persistent<v8::Object> buffer = v8::Persistent<v8::Object>::New(args[1]->ToObject());
  char* bufferData = node::Buffer::Data(buffer);
  size_t bufferLength = node::Buffer::Length(buffer);

  // callback
  if(!args[2]->IsFunction()) {
    return scope.Close(v8::ThrowException(v8::Exception::TypeError(v8::String::New("Third argument must be a function"))));
  }
  v8::Local<v8::Value> callback = args[2];

  WriteBaton* baton = new WriteBaton();
  memset(baton, 0, sizeof(WriteBaton));
  baton->fd = fd;
  baton->buffer = buffer;
  baton->bufferData = bufferData;
  baton->bufferLength = bufferLength;
  baton->callback = v8::Persistent<v8::Value>::New(callback);

  uv_work_t* req = new uv_work_t();
  req->data = baton;
  uv_queue_work(uv_default_loop(), req, EIO_Write, EIO_AfterWrite);

  return scope.Close(v8::Undefined());
}

void EIO_AfterWrite(uv_work_t* req) {
  WriteBaton* data = static_cast<WriteBaton*>(req->data);

  v8::Handle<v8::Value> argv[2];
  if(data->errorString[0]) {
    argv[0] = v8::Exception::Error(v8::String::New(data->errorString));
    argv[1] = v8::Undefined();
  } else {
    argv[0] = v8::Undefined();
    argv[1] = v8::Int32::New(data->result);
  }
  v8::Function::Cast(*data->callback)->Call(v8::Context::GetCurrent()->Global(), 2, argv);

  data->buffer.Dispose();
  data->callback.Dispose();
  delete data;
  delete req;
}

v8::Handle<v8::Value> Close(const v8::Arguments& args) {
  v8::HandleScope scope;

  return scope.Close(v8::Undefined());
}

SerialPortParity ToParityEnum(v8::Handle<v8::String>& v8str) {
  v8::String::AsciiValue str(v8str);
  if(!stricmp(*str, "none")) {
    return SERIALPORT_PARITY_NONE;
  }
  if(!stricmp(*str, "even")) {
    return SERIALPORT_PARITY_EVEN;
  }
  if(!stricmp(*str, "mark")) {
    return SERIALPORT_PARITY_MARK;
  }
  if(!stricmp(*str, "odd")) {
    return SERIALPORT_PARITY_ODD;
  }
  if(!stricmp(*str, "space")) {
    return SERIALPORT_PARITY_SPACE;
  }
  return SERIALPORT_PARITY_NONE;
}

SerialPortStopBits ToStopBitEnum(double stopBits) {
  if(stopBits > 1.4 && stopBits < 1.6) {
    return SERIALPORT_STOPBITS_ONE_FIVE;
  }
  if(stopBits == 2) {
    return SERIALPORT_STOPBITS_TWO;
  }
  return SERIALPORT_STOPBITS_ONE;
}

extern "C" {
  void init (v8::Handle<v8::Object> target) 
  {
    v8::HandleScope scope;
    NODE_SET_METHOD(target, "open", Open);
    NODE_SET_METHOD(target, "write", Write);
    NODE_SET_METHOD(target, "close", Close);
  }
}

NODE_MODULE(serialport2, init);
