// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef COMPONENTS_CRONET_ANDROID_URL_REQUEST_ADAPTER_H_
#define COMPONENTS_CRONET_ANDROID_URL_REQUEST_ADAPTER_H_

#include <jni.h>

#include <string>

#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/scoped_ptr.h"
#include "net/base/request_priority.h"
#include "net/http/http_request_headers.h"
#include "net/url_request/url_request.h"

namespace net {
class GrowableIOBuffer;
class HttpResponseHeaders;
class UploadDataStream;
}  // namespace net

namespace cronet {

class URLRequestContextAdapter;

// An adapter from the JNI |UrlRequest| object and the Chromium |URLRequest|
// object.
class URLRequestAdapter : public net::URLRequest::Delegate {
 public:
  // The delegate which is called when the request finishes.
  class URLRequestAdapterDelegate
      : public base::RefCountedThreadSafe<URLRequestAdapterDelegate> {
   public:
    virtual void OnResponseStarted(URLRequestAdapter* request) = 0;
    virtual void OnBytesRead(URLRequestAdapter* request) = 0;
    virtual void OnRequestFinished(URLRequestAdapter* request) = 0;
    virtual int ReadFromUploadChannel(net::IOBuffer* buf, int buf_length) = 0;

   protected:
    friend class base::RefCountedThreadSafe<URLRequestAdapterDelegate>;
    virtual ~URLRequestAdapterDelegate() {}
  };

  URLRequestAdapter(URLRequestContextAdapter* context,
                    URLRequestAdapterDelegate* delegate,
                    GURL url,
                    net::RequestPriority priority);
  virtual ~URLRequestAdapter();

  // Sets the request method GET, POST etc
  void SetMethod(const std::string& method);

  // Adds a header to the request
  void AddHeader(const std::string& name, const std::string& value);

  // Sets the contents of the POST or PUT request
  void SetUploadContent(const char* bytes, int bytes_len);

  // Sets the request to streaming upload.
  void SetUploadChannel(JNIEnv* env, int64 content_length);

  // Indicates that the request body will be streamed by calling AppendChunk()
  // repeatedly. This must be called before Start().
  void EnableChunkedUpload();

  // Appends a chunk to the POST body.
  // This must be called after EnableChunkedUpload() and Start().
  void AppendChunk(const char* bytes, int bytes_len, bool is_last_chunk);

  // Starts the request.
  void Start();

  // Cancels the request.
  void Cancel();

  // Releases all resources for the request and deletes the object itself.
  void Destroy();

  // Returns the URL of the request.
  GURL url() const { return url_; }

  // Returns the error code after the request is complete.
  // Negative codes indicate system errors.
  int error_code() const { return error_code_; }

  // Returns the HTTP status code.
  int http_status_code() const {
    return http_status_code_;
  };

  // Returns the value of the content-length response header.
  int64 content_length() const { return expected_size_; }

  // Returns the value of the content-type response header.
  std::string content_type() const { return content_type_; }

  // Returns the value of the specified response header.
  std::string GetHeader(const std::string& name) const;

  // Get all response headers, as a HttpResponseHeaders object.
  net::HttpResponseHeaders* GetResponseHeaders() const;

  // Returns the overall number of bytes read.
  size_t bytes_read() const { return bytes_read_; }

  // Returns a pointer to the downloaded data.
  unsigned char* Data() const;

  virtual void OnResponseStarted(net::URLRequest* request) OVERRIDE;

  virtual void OnReadCompleted(net::URLRequest* request,
                               int bytes_read) OVERRIDE;

 private:
  static void OnDestroyRequest(URLRequestAdapter* self);

  void OnInitiateConnection();
  void OnCancelRequest();
  void OnRequestSucceeded();
  void OnRequestFailed();
  void OnRequestCompleted();
  void OnRequestCanceled();
  void OnBytesRead(int bytes_read);
  void OnAppendChunk(const scoped_ptr<char[]> bytes, int bytes_len,
                     bool is_last_chunk);

  void Read();

  URLRequestContextAdapter* context_;
  scoped_refptr<URLRequestAdapterDelegate> delegate_;
  GURL url_;
  net::RequestPriority priority_;
  std::string method_;
  net::HttpRequestHeaders headers_;
  net::URLRequest* url_request_;
  scoped_ptr<net::UploadDataStream> upload_data_stream_;
  scoped_refptr<net::GrowableIOBuffer> read_buffer_;
  int bytes_read_;
  int total_bytes_read_;
  int error_code_;
  int http_status_code_;
  std::string content_type_;
  bool canceled_;
  int64 expected_size_;
  bool chunked_upload_;

  DISALLOW_COPY_AND_ASSIGN(URLRequestAdapter);
};

}  // namespace cronet

#endif  // COMPONENTS_CRONET_ANDROID_URL_REQUEST_ADAPTER_H_
