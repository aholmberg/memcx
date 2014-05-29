#include "uv_connection.h"

#include <sstream>
using std::stringstream;
#include <string>
using std::string;

using std::unique_ptr;

using namespace memcx::memcuv;

UvConnection::UvConnection(uv_loop_t *loop,
                           const struct sockaddr_in &endpoint,
                           const ConnectionCallback& connect_callback):
connect_callback_(connect_callback),
connected_(false),
endpoint_(endpoint),
loop_(loop) {
  Connect();
}

UvConnection::~UvConnection() {
  Close("object being destructed");
}

void UvConnection::Close(const string& message) {
  if (socket_ != nullptr) {
    uv_read_stop(socket_stream());
    uv_close(socket_handle(), OnCloseHandle<uv_tcp_t>);
    socket_ = nullptr;
  }
  connected_ = false;

  while (requests_.size()) {
    UvRequest* req = requests_.front();
    req->set_error_msg(message);
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
  uv_close(reinterpret_cast<uv_handle_t*>(handle), OnCloseHandle<uv_timer_t>);
  ths->Connect();
}

void UvConnection::WriteRequest(unique_ptr<UvRequest> request) {

  const string& cmd = request->command();
  uv_buf_t write_buffer = uv_buf_init(const_cast<char*>(cmd.data()), cmd.size());
  uv_write_t* write_req = new uv_write_t();
  request->set_connection(this);
  write_req->data = request.get();

  int r = uv_write(write_req, socket_stream(), &write_buffer, 1, UvConnection::OnWrite);
  if (r != 0) {
    request->set_error_msg("write init failed");
    request->Notify();
    Reset("tcp write initiation error");
    return;
  }
  request.release();
}

void UvConnection::OnWrite(uv_write_t *req, int status) {
  unique_ptr<uv_write_t> write_req(req);
  unique_ptr<UvRequest> request(static_cast<UvRequest*>(write_req->data));
  if (status != 0) {
    request->set_error_msg("write failed");
    request->Notify();
    return;
  }
  UvConnection* ths = static_cast<UvConnection*>(request->connection());
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
    UvRequest* req = requests_.front();
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
