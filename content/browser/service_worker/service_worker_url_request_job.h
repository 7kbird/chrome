// Copyright 2014 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_URL_REQUEST_JOB_H_
#define CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_URL_REQUEST_JOB_H_

#include <map>
#include <string>

#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "content/common/service_worker/service_worker_status_code.h"
#include "content/common/service_worker/service_worker_types.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

namespace storage {
class BlobDataHandle;
class BlobStorageContext;
}

namespace content {

class ResourceRequestBody;
class ServiceWorkerContextCore;
class ServiceWorkerFetchDispatcher;
class ServiceWorkerProviderHost;

class CONTENT_EXPORT ServiceWorkerURLRequestJob
    : public net::URLRequestJob,
      public net::URLRequest::Delegate {
 public:
  ServiceWorkerURLRequestJob(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate,
      base::WeakPtr<ServiceWorkerProviderHost> provider_host,
      base::WeakPtr<storage::BlobStorageContext> blob_storage_context,
      scoped_refptr<ResourceRequestBody> body);

  // Sets the response type.
  void FallbackToNetwork();
  void ForwardToServiceWorker();

  bool ShouldFallbackToNetwork() const {
    return response_type_ == FALLBACK_TO_NETWORK;
  }
  bool ShouldForwardToServiceWorker() const {
    return response_type_ == FORWARD_TO_SERVICE_WORKER;
  }

  // net::URLRequestJob overrides:
  virtual void Start() OVERRIDE;
  virtual void Kill() OVERRIDE;
  virtual net::LoadState GetLoadState() const OVERRIDE;
  virtual bool GetCharset(std::string* charset) OVERRIDE;
  virtual bool GetMimeType(std::string* mime_type) const OVERRIDE;
  virtual void GetResponseInfo(net::HttpResponseInfo* info) OVERRIDE;
  virtual int GetResponseCode() const OVERRIDE;
  virtual void SetExtraRequestHeaders(
      const net::HttpRequestHeaders& headers) OVERRIDE;
  virtual bool ReadRawData(net::IOBuffer* buf,
                           int buf_size,
                           int *bytes_read) OVERRIDE;

  // net::URLRequest::Delegate overrides that read the blob from the
  // ServiceWorkerFetchResponse.
  virtual void OnReceivedRedirect(net::URLRequest* request,
                                  const net::RedirectInfo& redirect_info,
                                  bool* defer_redirect) OVERRIDE;
  virtual void OnAuthRequired(net::URLRequest* request,
                              net::AuthChallengeInfo* auth_info) OVERRIDE;
  virtual void OnCertificateRequested(
      net::URLRequest* request,
      net::SSLCertRequestInfo* cert_request_info) OVERRIDE;
  virtual void OnSSLCertificateError(net::URLRequest* request,
                                     const net::SSLInfo& ssl_info,
                                     bool fatal) OVERRIDE;
  virtual void OnBeforeNetworkStart(net::URLRequest* request,
                                    bool* defer) OVERRIDE;
  virtual void OnResponseStarted(net::URLRequest* request) OVERRIDE;
  virtual void OnReadCompleted(net::URLRequest* request,
                               int bytes_read) OVERRIDE;

  const net::HttpResponseInfo* http_info() const;

  void GetExtraResponseInfo(bool* was_fetched_via_service_worker,
                            GURL* original_url_via_service_worker) const;

 protected:
  virtual ~ServiceWorkerURLRequestJob();

 private:
  enum ResponseType {
    NOT_DETERMINED,
    FALLBACK_TO_NETWORK,
    FORWARD_TO_SERVICE_WORKER,
  };

  // We start processing the request if Start() is called AND response_type_
  // is determined.
  void MaybeStartRequest();
  void StartRequest();

  // Creates ServiceWorkerFetchRequest from |request_| and |body_|.
  scoped_ptr<ServiceWorkerFetchRequest> CreateFetchRequest();

  // Creates BlobDataHandle of the request body from |body_|. This handle
  // |request_body_blob_data_handle_| will be deleted when
  // ServiceWorkerURLRequestJob is deleted.
  bool CreateRequestBodyBlob(std::string* blob_uuid, uint64* blob_size);

  // For FORWARD_TO_SERVICE_WORKER case.
  void DidDispatchFetchEvent(ServiceWorkerStatusCode status,
                             ServiceWorkerFetchEventResult fetch_result,
                             const ServiceWorkerResponse& response);

  // Populates |http_response_headers_|.
  void CreateResponseHeader(int status_code,
                            const std::string& status_text,
                            const std::map<std::string, std::string>& headers);

  // Creates |http_response_info_| using |http_response_headers_| and calls
  // NotifyHeadersComplete.
  void CommitResponseHeader();

  // Creates and commits a response header indicating error.
  void DeliverErrorResponse();

  base::WeakPtr<ServiceWorkerProviderHost> provider_host_;

  ResponseType response_type_;
  bool is_started_;

  net::HttpByteRange byte_range_;
  scoped_ptr<net::HttpResponseInfo> range_response_info_;
  scoped_ptr<net::HttpResponseInfo> http_response_info_;
  // Headers that have not yet been committed to |http_response_info_|.
  scoped_refptr<net::HttpResponseHeaders> http_response_headers_;
  GURL response_url_;

  // Used when response type is FORWARD_TO_SERVICE_WORKER.
  scoped_ptr<ServiceWorkerFetchDispatcher> fetch_dispatcher_;
  base::WeakPtr<storage::BlobStorageContext> blob_storage_context_;
  scoped_ptr<net::URLRequest> blob_request_;
  // ResourceRequestBody has a collection of BlobDataHandles attached to it
  // using the userdata mechanism. So we have to keep it not to free the blobs.
  scoped_refptr<ResourceRequestBody> body_;
  scoped_ptr<storage::BlobDataHandle> request_body_blob_data_handle_;

  base::WeakPtrFactory<ServiceWorkerURLRequestJob> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(ServiceWorkerURLRequestJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_SERVICE_WORKER_SERVICE_WORKER_URL_REQUEST_JOB_H_
