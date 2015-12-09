/** @file
 *
 *  AWSCache.cc
 *
 *  @section license License
 *
 *  Licensed to the Apache Software Foundation (ASF) under one
 *  or more contributor license agreements.  See the NOTICE file
 *  distributed with this work for additional information
 *  regarding copyright ownership.  The ASF licenses this file
 *  to you under the Apache License, Version 2.0 (the
 *  "License"); you may not use this file except in compliance
 *  with the License.  You may obtain a copy of the License at
 *
 *      http://www.apache.org/licenses/LICENSE-2.0
 *
 *  Unless required by applicable law or agreed to in writing, software
 *  distributed under the License is distributed on an "AS IS" BASIS,
 *  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *  See the License for the specific language governing permissions and
 *  limitations under the License.
 */

#include <sstream>

#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
#include <aws/s3/model/PutObjectRequest.h>
#include <aws/core/utils/HashingUtils.h>

#include "AWSCache.h"

#define AWSCACHE_ALLOCATION_TAG "AWSCache"
#define REC_CONFIG_AWS_CONFIG_FILE_FIELD "proxy.config.http.cache.cloud.provider.aws.config_filename"
#define AWSCACHE_CONFIG_FILENAME "aws_cache.config"


using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::Auth;
using namespace Aws::Utils;

AWSCache::AWSCache() : CloudProvider() {}

AWSCache::~AWSCache() {}

const char *
AWSCache::read_config()
{
  const char *err = NULL;
  ats_scoped_fd fd;
  ats_scoped_str storage_path(RecConfigReadConfigPath(REC_CONFIG_AWS_CONFIG_FILE_FIELD, AWSCACHE_CONFIG_FILENAME));

  Debug("cache_init", "AWSCache::read_config, \"%s\"", (const char *)storage_path);

  fd = ::open(storage_path, O_RDONLY);
  if (fd < 0) {
    err = "Could not open AWS configuration file";
    goto Lfail;
  }

  // TODO: finish reading of config file, look at Store.cc

  s3bucket_name = "flyingsquid-ats-test2-sunjay";

//  ::close(fd);

  Debug("cloud_cache_aws", "using S3 bucket: %s", s3bucket_name);

Lfail:
  // write error message

  return err;
}

Action *
AWSCache::open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                    CacheLookupHttpConfig *params, time_t pin_in_cache)
{
  Debug("cloud_cache_aws", "starting cache read");

  // TODO: change to list object request

  // s3 get object request
  GetObjectRequest getObjectRequest;
  getObjectRequest.SetBucket(s3bucket_name);
  // TODO: get hash string a better way?
  unsigned int key_hash = cache_hash(key->hash);
  Debug("cloud_cache_aws", "cache read hash key: %u", key_hash);
  getObjectRequest.SetKey(std::to_string(key_hash));
  std::stringbuf buf;
  getObjectRequest.SetResponseStreamFactory(
    [&buf](){
      return Aws::New<Aws::IOStream>(AWSCACHE_ALLOCATION_TAG, &buf);
    }
  );
  auto getObjectOutcome = s3Client.GetObject(getObjectRequest);
  if(getObjectOutcome.IsSuccess()) {
    Debug("cloud_cache_aws", "S3 bucket (%s) hit and download success for key: %u", s3bucket_name, key_hash);
    std::cout << buf.str();
  } else { // TODO: change to list bucket objects, inspect outcome more to set the event handling properly
    Debug("cloud_cache_aws", "S3 bucket (%s) cache miss for key: %u", s3bucket_name, key_hash);
//    CACHE_INCREMENT_DYN_STAT(cache_read_failure_stat);
    cont->handleEvent(CACHE_EVENT_OPEN_READ_FAILED, (void *)-ECACHE_NO_DOC);
    return ACTION_RESULT_DONE;
  }

  return NULL;
}

Action *
AWSCache::open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                     CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache)
{
  Debug("cloud_cache_aws", "starting cache write");

  // s3 put object request
  PutObjectRequest putObjectRequest;
  putObjectRequest.SetBucket(s3bucket_name);
  // TODO: get hash string a better way?
  unsigned int key_hash = cache_hash(key->hash);
  Debug("cloud_cache_aws", "cache write hash key: %u", key_hash);
  putObjectRequest.SetKey(std::to_string(key_hash));
  // TODO: transitioning from all on disk, still figuring out how to get data from OS into the buffer and vice versa
  std::stringbuf buf;
  std::shared_ptr<Aws::IOStream> writestream = Aws::MakeShared<Aws::IOStream>(AWSCACHE_ALLOCATION_TAG, &buf);
  putObjectRequest.SetBody(writestream);
  putObjectRequest.SetContentLength(static_cast<long>(putObjectRequest.GetBody()->tellp()));
  putObjectRequest.SetContentMD5(HashingUtils::Base64Encode(HashingUtils::CalculateMD5(*putObjectRequest.GetBody())));
  PutObjectOutcome putObjectOutcome = s3Client.PutObject(putObjectRequest);
  if (putObjectOutcome.IsSuccess()) {
    Debug("cloud_cache_aws", "S3 bucket (%s) write succeeded for key: %u", s3bucket_name, key_hash);
  } else { // TODO: inspect outcome more to set event handling properly
    Debug("cloud_cache_aws", "S3 bucket (%s) write failed for key: %u", s3bucket_name, key_hash);
//    CACHE_INCREMENT_DYN_STAT()
    cont->handleEvent(CACHE_EVENT_OPEN_WRITE_FAILED, (void *)-ECACHE_NOT_READY);
    return ACTION_RESULT_DONE;
  }

  return NULL;
}
