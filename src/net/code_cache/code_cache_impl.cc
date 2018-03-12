// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#include "net/code_cache/code_cache_impl.h"

#include "base/files/file.h"
#include "base/files/file_util.h"
#include "net/base/net_errors.h"
#include "net/disk_cache/disk_cache.h"
#include "url/gurl.h"

class IOBufffer;

namespace net {

CodeCacheImpl::CodeCacheImpl(
    const base::FilePath& path,
    int max_size,
    const scoped_refptr<base::SingleThreadTaskRunner>& thread)
    : path_(path),
      max_size_(max_size),
      thread_(thread),
      is_backend_creating_(false),
      is_backend_cleaning_data_(false),
      weak_factory_(this) {
  Initialize();
}

CodeCacheImpl::~CodeCacheImpl() {
  LOG(INFO) << "~CodeCacheImpl";

  disk_cache_.reset();
}

CodeCacheImpl::WriteData::WriteData(const GURL& url,
                                    scoped_refptr<IOBufferWithSize> buf)
    : entry_(nullptr), url_(url), buf_(buf) {
}

CodeCacheImpl::WriteData::~WriteData() {
  if (entry_)
    entry_->Close();
}

CodeCacheImpl::ReadData::ReadData(const GURL& url,
                                  const base::Time& last_modified,
                                  CodeCache::ReadCallback read_callback)
    : url_(url),
      entry_(nullptr),
      last_modified_(last_modified),
      read_callback_(read_callback),
      buf_(nullptr) {
}

CodeCacheImpl::ReadData::~ReadData() {
  if (entry_)
    entry_->Close();
}

void CodeCacheImpl::Initialize() {
  CompletionCallback callback(
      base::Bind(&CodeCacheImpl::DidBackendCreate, base::Unretained(this)));
  CreateBackend(callback);
}

void CodeCacheImpl::CreateBackend(const CompletionCallback& callback) {
  is_backend_creating_ = true;
  const base::FilePath cache_path = path_.Append("CodeCache");
  LOG(INFO) << "CreateBackend(" << cache_path.value() << ")";
  disk_cache::CreateCacheBackend(net::CODE_CACHE, net::CACHE_BACKEND_DEFAULT,
                                 cache_path, max_size_, true, thread_, nullptr,
                                 &disk_cache_, callback);

}

void CodeCacheImpl::DidBackendCreate(int result) {
  LOG(INFO) << "CodeCacheImpl::DidBackendCreate " << result;
  is_backend_creating_ = false;

  if (result != OK) {
    write_list_.clear();
    for (auto read_data : read_list_) {
      scoped_refptr<IOBufferWithSize> empty;
      read_data->read_callback().Run(ERR_FAILED, empty);
    }
    read_list_.clear();
    return;
  }
  CHECK(disk_cache_.get());

  for (auto write_data : write_list_)
    WriteMetadataInternal(write_data->url(), write_data->buf());
  write_list_.clear();

  for (auto read_data : read_list_)
    ReadMetadataInternal(read_data->url(), read_data->last_modified(),
                         read_data->read_callback());
  read_list_.clear();

  if (is_backend_cleaning_data_) {
    CompletionCallback doom_callback(base::Bind(
        &CodeCacheImpl::DidDoomAllEntries, base::Unretained(this)));

    DoomAllEntries(doom_callback);

  }
}

void CodeCacheImpl::DidEntryCreate(scoped_refptr<WriteData> write_data,
                                   int result) {
  LOG(INFO) << "DidEntryCreate " << result;
  if (result == OK) {
    CompletionCallback callback(base::Bind(&CodeCacheImpl::DidEntryWrite,
                                           base::Unretained(this), write_data));
    int rv =
        write_data->entry()->WriteData(kMetadataIndex, 0, write_data->buf().get(),
                                       write_data->buf()->size(), callback, true);
    if (rv != net::ERR_IO_PENDING)
      callback.Run(rv);
  }
}

void CodeCacheImpl::DidEntryWrite(scoped_refptr<WriteData> write_data,
                                  int bytes_written) {
  LOG(INFO) << "DidEntryWrite " << bytes_written;

  if (bytes_written != write_data->buf()->size())
    write_data->entry()->Doom();
}

void CodeCacheImpl::WriteMetadata(const GURL& url,
                                  scoped_refptr<IOBufferWithSize> buf) {
  if (!disk_cache_.get()) {
    if (!is_backend_creating_) {
      CompletionCallback callback(
          base::Bind(&CodeCacheImpl::DidBackendCreate, base::Unretained(this)));
      CreateBackend(callback);
    }

    scoped_refptr<WriteData> write_data(new WriteData(url, buf));
    write_list_.push_back(write_data);
    return;
  }
  CHECK(disk_cache_.get());
  WriteMetadataInternal(url, buf);
}

void CodeCacheImpl::WriteMetadataInternal(const GURL& url,
                                          scoped_refptr<IOBufferWithSize> buf) {
  scoped_refptr<WriteData> write_data(new WriteData(url, buf));
  CompletionCallback callback(base::Bind(&CodeCacheImpl::DidEntryCreate,
                                         base::Unretained(this), write_data));
  int rv =
      disk_cache_->CreateEntry(url.spec(), &(write_data->entry()), callback);
  if (rv != net::ERR_IO_PENDING)
    callback.Run(rv);
}

void CodeCacheImpl::ReadMetadata(const GURL& url,
                                 const base::Time& last_modified,
                                 ReadCallback& read_callback) {
  if (!disk_cache_.get()) {
    if (!is_backend_creating_) {
      CompletionCallback callback(
          base::Bind(&CodeCacheImpl::DidBackendCreate, base::Unretained(this)));
      CreateBackend(callback);
    }

    scoped_refptr<ReadData> read_data(
        new ReadData(url, last_modified, read_callback));
    read_list_.push_back(read_data);
    return;
  }
  CHECK(disk_cache_.get());
  ReadMetadataInternal(url, last_modified, read_callback);
}

void CodeCacheImpl::ReadMetadataInternal(const GURL& url,
                                         const base::Time& last_modified,
                                         ReadCallback& read_callback) {
  scoped_refptr<ReadData> read_data(
      new ReadData(url, last_modified, read_callback));
  CompletionCallback open_entry_callback(base::Bind(
      &CodeCacheImpl::DidEntryOpen, base::Unretained(this), read_data));
  int rv = disk_cache_->OpenEntry(url.spec(), &(read_data->entry()),
                                  open_entry_callback);
  if (rv != ERR_IO_PENDING)
    open_entry_callback.Run(rv);
}

void CodeCacheImpl::DidEntryOpen(scoped_refptr<ReadData> read_data,
                                 int result) {
  LOG(INFO) << "DidEntryOpen " << result;
  if (result == OK) {
    bool valid =
        read_data->last_modified() <= read_data->entry()->GetLastModified();
    if (!valid) {
      read_data->entry()->Doom();
      scoped_refptr<IOBufferWithSize> empty;
      read_data->read_callback().Run(ERR_FAILED, empty);
      return;
    }

    const int data_size = read_data->entry()->GetDataSize(0);
    read_data->set_buf(new IOBufferWithSize(data_size));
    CompletionCallback readdata_callback(base::Bind(
        &CodeCacheImpl::DidEntryRead, base::Unretained(this), read_data));
    int rv =
        read_data->entry()->ReadData(kMetadataIndex, 0, read_data->buf().get(),
                                     data_size, readdata_callback);
    if (rv != ERR_IO_PENDING)
      readdata_callback.Run(rv);
  } else {
    scoped_refptr<IOBufferWithSize> empty;
    read_data->read_callback().Run(result, empty);
  }
}

void CodeCacheImpl::DidEntryRead(scoped_refptr<ReadData> read_data,
                                 int bytes_read) {
  LOG(INFO) << "DidEntryRead " << bytes_read;
  const int data_size = read_data->entry()->GetDataSize(0);
  ReadCallback& read_callback = read_data->read_callback();
  int result = (data_size == bytes_read) ? OK : ERR_FAILED;
  read_callback.Run(result, read_data->buf());
}

void CodeCacheImpl::ClearData(const CompletionCallback& callback) {
  DCHECK(!callback.is_null());
  clear_data_done_callback_ = callback;

  is_backend_cleaning_data_ = true;

  if (!disk_cache_.get()) {
    if (!is_backend_creating_) {
      CompletionCallback create_callback(
          base::Bind(&CodeCacheImpl::DidBackendCreate, base::Unretained(this)));
      CreateBackend(create_callback);
    }
  }

  CompletionCallback doom_callback(base::Bind(
        &CodeCacheImpl::DidDoomAllEntries, base::Unretained(this)));
  DoomAllEntries(doom_callback);
}

void CodeCacheImpl::DoomAllEntries(const CompletionCallback& callback) {

  disk_cache_->DoomAllEntries(callback);
}

void CodeCacheImpl::DidDoomAllEntries(int result)
{
  LOG(INFO) << "DidDoomAllEntries " << result;

  is_backend_cleaning_data_ = false;
  if (result != ERR_IO_PENDING)
    clear_data_done_callback_.Run(result);
}

}  // namespace content
