// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef CONTENT_BROWSER_CODECACHE_CODECACHE_URL_REQUEST_JOB_H_
#define CONTENT_BROWSER_CODECACHE_CODECACHE_URL_REQUEST_JOB_H_

#include <memory>
#include <string>
#include <vector>

#include "base/files/file_path.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "content/common/content_export.h"
#include "net/base/net_errors.h"
#include "net/base/net_export.h"
#include "net/http/http_byte_range.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

namespace base {
class TaskRunner;
}
namespace file_util {
struct FileInfo;
}

namespace net {
class URLRequest;
class FileStream;
class NetworkDelegate;
class IOBufferWithSize;
}

namespace content {
using net::URLRequestJob;
using net::NetworkDelegate;
using net::URLRequest;
using net::IOBuffer;
using net::IOBufferWithSize;
using net::Filter;
using net::HttpRequestHeaders;
using net::FileStream;
using net::HttpByteRange;

// A request job that handles reading file URLs
class CONTENT_EXPORT CodeCacheURLRequestJob : public net::URLRequestJob,
    public base::RefCounted<CodeCacheURLRequestJob> {
 public:
  CodeCacheURLRequestJob(
      URLRequest* request,
      NetworkDelegate* network_delegate,
      const base::FilePath& file_path,
      const scoped_refptr<base::TaskRunner>& file_task_runner);
  virtual ~CodeCacheURLRequestJob();

  // URLRequestJob:
  void Start() override;
  void Kill() override;
  int ReadRawData(IOBuffer* buf, int buf_size) override;
  bool IsRedirectResponse(GURL* location, int* http_status_code) override;
  std::unique_ptr<Filter> SetupFilter() const override;
  bool GetMimeType(std::string* mime_type) const override;
  void SetExtraRequestHeaders(const HttpRequestHeaders& headers) override;

  // An interface for subclasses who wish to monitor read operations.
  virtual void OnSeekComplete(int64_t result);
  virtual void OnReadComplete(IOBuffer* buf, int result);

 protected:
  int64_t remaining_bytes() const { return remaining_bytes_; }

  // The OS-specific full path name of the file
  base::FilePath file_path_;

 private:
  // Meta information about the file. It's used as a member in the
  // URLRequestFileJob and also passed between threads because disk access is
  // necessary to obtain it.
  struct FileMetaInfo {
    FileMetaInfo();

    // Size of the file.
    int64_t file_size;
    // Mime type associated with the file.
    std::string mime_type;
    // Result returned from GetMimeTypeFromFile(), i.e. flag showing whether
    // obtaining of the mime type was successful.
    bool mime_type_result;
    // Flag showing whether the file exists.
    bool file_exists;
    // Flag showing whether the file name actually refers to a directory.
    bool is_directory;
    // The last modified time of a file.
    base::Time last_modified;
  };

  // Fetches file info on a background thread.
  static void FetchMetaInfo(const base::FilePath& file_path,
                            FileMetaInfo* meta_info);

  // Callback after fetching file info on a background thread.
  void DidFetchMetaInfo(const FileMetaInfo* meta_info);

  // Callback after opening file on a background thread.
  void DidOpen(int result);

  // Callback after seeking to the beginning of |byte_range_| in the file
  // on a background thread.
  void DidSeek(int64_t result);

  // Callback after data is asynchronously read from the file into |buf|.
  void DidRead(scoped_refptr<IOBuffer> buf, int result);

  void OpenEntry();
  void DidReadData(int result, scoped_refptr<IOBufferWithSize> buf);

  std::unique_ptr<FileStream> stream_;
  FileMetaInfo meta_info_;
  const scoped_refptr<base::TaskRunner> file_task_runner_;

  std::vector<HttpByteRange> byte_ranges_;
  HttpByteRange byte_range_;
  int64_t remaining_bytes_;

  bool waiting_open_entry_;
  int64_t pending_first_byte_position_;

  net::Error range_parse_result_;

  base::WeakPtrFactory<CodeCacheURLRequestJob> weak_ptr_factory_;

  DISALLOW_COPY_AND_ASSIGN(CodeCacheURLRequestJob);
};

}  // namespace content

#endif  // CONTENT_BROWSER_CODECACHE_CODECACHE_URL_REQUEST_JOB_H_
