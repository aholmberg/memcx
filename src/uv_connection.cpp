#include "uv_connection.h"

#include <sstream>
using std::stringstream;
#include <string>
using std::string;

using std::unique_ptr;

using namespace memcx::memcuv;

template<typename T>
void OnCloseHandle(uv_handle_t* handle) {
  T* t = reinterpret_cast<T*>(handle);
  delete t;
}

UvConnection::UvConnection(uv_loop_t *loop,
                           const struct sockaddr_in &endpoint,
                           const ConnectionCallback& connect_callback):
loop_(loop),
endpoint_(endpoint),
connected_(false),
connect_callback_(connect_callback) {
  Connect();
}

UvConnection::~UvConnection() {
  Close("object being destructed");
}

void UvConnection::Close(const string& message) {
  if (socket_ != nullptr) {
    uv_read_stop(socket_stream());
    uv_close(socket_handle(), OnCloseHandle<uv_tcp_t>);
  }
  connected_ = false;
  socket_ = nullptr;

  while (requests_.size()) {
    Request* req = requests_.front();
    req->error_msg(message);
    req->Notify();
    requests_.pop();
  }
}

void UvConnection::Connect() {
    
    uv_tcp_t* socket = new uv_tcp_t();
    uv_tcp_init(loop_, socket);
    socket->data = this;
    uv_connect_t* req = new uv_connect_t();
    req->data = this;
    int r = uv_tcp_connect(req, socket, endpoint_, UvConnection::OnConnect);
    if (r != 0) {
      delete req;
      delete socket;
      Reset("connect initiation error");
    }
}

void UvConnection::OnConnect(uv_connect_t *req, int status) {
  unique_ptr<uv_connect_t> connect_req(req);
  UvConnection* ths = (UvConnection*)req->data;
  ths->socket_ = reinterpret_cast<uv_tcp_t*>(req->handle);
  if (status == -1) {
    ths->Reset("tcp connect error");
    return;
  }

  ths->connected_ = true;
  ths->NotifyConnect();

  int r = uv_read_start(ths->socket_stream(), UvConnection::GetBuffer, UvConnection::OnRead);
  if (r != 0) {
    ths->Reset("tcp read initiation error");
  }
}

void UvConnection::NotifyConnect() {
  connect_callback_(this);
}

void UvConnection::Reset(const char* context) {
  Close(GetErrorMessage(context));

  uv_timer_t* timer = new uv_timer_t();
  uv_timer_init(loop_, timer);
  timer->data = this;
  uv_timer_start(timer, UvConnection::Reconnect, 500, 0);
}

void UvConnection::Reconnect(uv_timer_t* handle, int status) {
  UvConnection* ths = static_cast<UvConnection*>(handle->data);
  uv_timer_stop(handle);
  //uv_close(reinterpret_cast<uv_handle_t*>(handle), OnCloseHandle<uv_timer_t>);
  ths->Connect();
}

void UvConnection::SubmitRequest(unique_ptr<Request> request) {
  uv_async_t* async = new uv_async_t();
  uv_async_init(loop_, async, UvConnection::WriteRequest);
  request->set_data(this);
  async->data = request.release();
  uv_async_send(async);
}

void UvConnection::WriteRequest(uv_async_t* handle, int status) {
  //TODO: guard not connected
  unique_ptr<Request> request(static_cast<Request*>(handle->data));
  uv_close(reinterpret_cast<uv_handle_t*>(handle), OnCloseHandle<uv_async_t>);

  const string& cmd = request->command();
  uv_buf_t write_buffer = uv_buf_init(const_cast<char*>(cmd.data()), cmd.size());
  uv_write_t* write_req = new uv_write_t();
  write_req->data = request.get();

  UvConnection* ths = static_cast<UvConnection*>(request->data());
  int r = uv_write(write_req, ths->socket_stream(), &write_buffer, 1, UvConnection::OnWrite);
  if (r != 0) {
    ths->Reset("tcp write initiation error");
  }
  request.release();
}

void UvConnection::OnWrite(uv_write_t *req, int status) {
  unique_ptr<uv_write_t> write_req(req);
  unique_ptr<Request> request(static_cast<Request*>(write_req->data));
  if (status != 0) {
    //TODO:
    request->Notify(/*err*/);
    return;
  }
  UvConnection* ths = static_cast<UvConnection*>(request->data());
  ths->requests_.push(request.release());
}

uv_buf_t UvConnection::GetBuffer(uv_handle_t* handle, size_t suggested_size) {
  UvConnection* ths = static_cast<UvConnection*>(handle->data);
  return ths->buffer_.InitRead();
}

void UvConnection::OnRead(uv_stream_t* req, ssize_t nread, uv_buf_t buf) {
  UvConnection* ths = static_cast<UvConnection*>(req->data);

  if (nread > 0) {
    ths->buffer_.MarkNewBytesRead(nread);
    ths->ProcessData();
  } else {
    ths->Reset("read return error");
  }
}

void UvConnection::ProcessData() {
  while (!requests_.empty()) {
    Request* req = requests_.front();
//TODO: possibly switch this conditional order
//TODO: maybe only process if new data since last Ingest/tryreadline
    if (req->Ingest(buffer_) > 0) {
      if (req->complete()) {
        req->Notify();
        requests_.pop();
        delete req;
      }
    } else {// out of data
      break;
    }
  } 
}

string UvConnection::GetErrorMessage(const char* context) {
  uv_err_t e = uv_last_error(this->loop_);
  stringstream ss;
  ss << context << ": " << uv_err_name(e) << " (" << uv_strerror(e) << ")";
  return ss.str();
}
