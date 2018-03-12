// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "content/browser/codecache/codecache_request_handler.h"

#include "base/files/file_path.h"
#include "content/browser/codecache/codecache_url_request_job.h"
#include "content/public/browser/browser_thread.h"
#include "net/base/filename_util.h"
#include "net/url_request/url_request.h"
#include "net/url_request/url_request_job.h"

namespace content {

CodeCacheRequestHandler::CodeCacheRequestHandler()
    : request_job_delivered_(false) {}

CodeCacheURLRequestJob* CodeCacheRequestHandler::MaybeLoadResource(
      net::URLRequest* request,
      net::NetworkDelegate* network_delegate) {
  if (request_job_delivered_)
    return nullptr;

  base::FilePath file_path;
  const bool is_file = net::FileURLToFilePath(request->url(), &file_path);
  if (!is_file)
    return nullptr;

  // It will be destroied after the job finished.
  CodeCacheURLRequestJob* job = new CodeCacheURLRequestJob(
      request, network_delegate, file_path,
      content::BrowserThread::GetBlockingPool()
          ->GetTaskRunnerWithShutdownBehavior(
              base::SequencedWorkerPool::SKIP_ON_SHUTDOWN));
  request_job_delivered_ = true;
  return job;
}

}  // namespace content
