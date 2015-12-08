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

#include <aws/s3/S3Client.h>
#include <aws/s3/model/GetObjectRequest.h>
//#include <aws/core/auth/AWSCredentialsProvider.h>
//#include <aws/core/utils/DateTime.h>
//#include <aws/core/utils/HashingUtils.h>

#include "AWSCache.h"

#define REC_CONFIG_AWS_CONFIG_FILE_FIELD "proxy.config.http.cache.cloud.provider.aws.config_filename"
#define AWS_CONFIG_FILENAME "aws_cache.config"


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
  ats_scoped_str storage_path(RecConfigReadConfigPath(REC_CONFIG_AWS_CONFIG_FILE_FIELD, AWS_CONFIG_FILENAME));

  Debug("cache_init", "AWSCache::read_config, \"%s\"", (const char *)storage_path);

  fd = ::open(storage_path, O_RDONLY);
  if (fd < 0) {
    err = "Could not open AWS configuration file";
    goto Lfail;
  }

  // TODO: finish reading of config file, look at Store.cc

  bucket_name = "flyingsquid-ats-test1-sunjay";

//  ::close(fd);

  Debug("cloud_cache_aws", "Using S3 bucket: %s", bucket_name);

Lfail:
  // write error message

  return err;
}

Action *
AWSCache::open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                    CacheLookupHttpConfig *params, time_t pin_in_cache)
{
  if (!this->s3Client) {
    Debug("cloud_cache_aws", "s3client not initialized for read");
    cont->handleEvent(CACHE_EVENT_OPEN_READ_FAILED, (void *)-ECACHE_NOT_READY);
    return ACTION_RESULT_DONE;
  }

  GetObjectRequest getObjectRequest;
  getObjectRequest.SetBucket(bucket_name);
  getObjectRequest.SetKey("sample_key");
  getObjectRequest.SetResponseStreamFactory(
    [](){
      return Aws::New(ALLOCATION_TAG, DOWNLOADED_FILENAME, std::ios_base::out | std::ios_base::in | std::ios_base::trunc);
  });
  auto getObjectOutcome = s3Client.GetObject(getObjectRequest);
  if(getObjectOutcome.IsSuccess()) {
    std::cout << "File downloaded from S3 to location " << DOWNLOADED_FILENAME;
  } else {
    std::cout << "File download failed from s3 with error " << getObjectOutcome.GetError().GetMessage();
  }

  return NULL;
}

Action *
AWSCache::open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                     CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache)
{
  if (!this.s3Client) {
    Debug("cloud_cache_aws", "s3client not initialized for write");
    cont->handleEvent(CACHE_EVENT_OPEN_WRITE_FAILED, (void *)-ECACHE_NOT_READY);
    return ACTION_RESULT_DONE;
  }


  return NULL;
}
