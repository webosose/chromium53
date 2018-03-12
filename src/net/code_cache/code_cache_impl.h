// Copyright (c) 2016 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef NET_CODE_CACHE_CODE_CACHE_IMPL_H_
#define NET_CODE_CACHE_CODE_CACHE_IMPL_H_

#include "net/code_cache/code_cache.h"

#include "base/files/file_path.h"
#include "base/threading/non_thread_safe.h"
#include "net/base/io_buffer.h"
#include "net/base/net_export.h"
#include "net/base/request_priority.h"
#include "url/gurl.h"

#include <memory>
#include <vector>

class GURL;

namespace base {
class SingleThreadTaskRunner;
}

namespace disk_cache {
class Backend;
class Entry;
}

namespace net {

class WriteData;
class ReadData;

class NET_EXPORT CodeCacheImpl : public CodeCache,
                                 NON_EXPORTED_BASE(public base::NonThreadSafe) {
 public:
  const int kMetadataIndex = 0;

  CodeCacheImpl(const base::FilePath& path,
                int max_size,
                const scoped_refptr<base::SingleThreadTaskRunner>& thread);

  ~CodeCacheImpl() override;
  void WriteMetadata(const GURL& url,
                     scoped_refptr<IOBufferWithSize> buf) override;

  void ReadMetadata(const GURL& url,
                    const base::Time& last_modified,
                    ReadCallback& callback) override;
  void ClearData(const CompletionCallback& callback) override;
private:
  class WriteData : public base::RefCountedThreadSafe<WriteData> {
   public:
    explicit WriteData(const GURL& url, scoped_refptr<IOBufferWithSize> buf);

    disk_cache::Entry*& entry() { return entry_; }
    GURL url() const { return url_; }
    scoped_refptr<IOBufferWithSize> buf() { return buf_; }

   protected:
    virtual ~WriteData();
    friend class base::RefCountedThreadSafe<WriteData>;
    DISALLOW_COPY_AND_ASSIGN(WriteData);

   private:
    disk_cache::Entry* entry_;
    GURL url_;
    scoped_refptr<IOBufferWithSize> buf_;
  };

  class ReadData : public base::RefCountedThreadSafe<ReadData> {
   public:
    explicit ReadData(const GURL& url,
                      const base::Time& last_modified,
                      CodeCache::ReadCallback read_callback);

    disk_cache::Entry*& entry() { return entry_; }
    GURL url() const { return url_; }
    base::Time last_modified() const { return last_modified_; }
    CodeCache::ReadCallback& read_callback() { return read_callback_; }
    void set_buf(scoped_refptr<IOBufferWithSize> buf) { buf_ = buf; }
    scoped_refptr<IOBufferWithSize> buf() { return buf_; }

   protected:
    virtual ~ReadData();
    friend class base::RefCountedThreadSafe<ReadData>;
    DISALLOW_COPY_AND_ASSIGN(ReadData);

   private:
    GURL url_;
    disk_cache::Entry* entry_;
    base::Time last_modified_;
    CodeCache::ReadCallback read_callback_;
    scoped_refptr<IOBufferWithSize> buf_;
  };

  void Initialize();
  void CreateBackend(const CompletionCallback& callback);
  void DoomAllEntries(const CompletionCallback& callback);
  void DidBackendCreate(int result);
  void DidDoomAllEntries(int result);
  void DidEntryCreate(scoped_refptr<WriteData> write_data, int result);
  void DidEntryWrite(scoped_refptr<WriteData> write_data, int bytes_written);
  void DidEntryOpen(scoped_refptr<ReadData> read_data, int result);
  void DidEntryRead(scoped_refptr<ReadData> read_data, int bytes_read);

  void WriteMetadataInternal(const GURL& url,
                             scoped_refptr<IOBufferWithSize> buf);

  void ReadMetadataInternal(const GURL& url,
                            const base::Time& last_modified,
                            ReadCallback& read_callback);

  base::FilePath path_;
  int max_size_;
  scoped_refptr<base::SingleThreadTaskRunner> thread_;
  std::unique_ptr<disk_cache::Backend> disk_cache_;
  bool is_backend_creating_;
  bool is_backend_cleaning_data_;
  std::vector<scoped_refptr<WriteData>> write_list_;
  std::vector<scoped_refptr<ReadData>> read_list_;

  CompletionCallback clear_data_done_callback_;

  base::WeakPtrFactory<CodeCacheImpl> weak_factory_;

  DISALLOW_COPY_AND_ASSIGN(CodeCacheImpl);
};

}  // namespace net

#endif  // NET_CODE_CACHE_CODE_CACHE_IMPL_H_
