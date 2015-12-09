/** @file
 *
 *  AWSCache.h - AWS specific cache operations, implements
 *  CloudProvider interface
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

#ifndef __AWS_CACHE_H__
#define __AWS_CACHE_H__

#include <aws/s3/S3Client.h>

#include "CloudCache.h"

#define PROVIDER_NAME_AWS "AWS"


class AWSCache : public CloudProvider
{
public:
  AWSCache();
  ~AWSCache();

  virtual const char *read_config();

  // TODO: this interface will change, will move event/continuation handling to CloudCache::open_write etc.

  virtual Action *open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                            CacheLookupHttpConfig *params, time_t pin_in_cache);

  virtual Action *open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                             CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache);

private:
  const char *s3bucket_name;

  Aws::S3::S3Client s3Client;

  // TODO: implement these for cleanliness, should just take a key, metadata, iostream etc.
//  s3_put()
//  s3_get()
//  s3_delete()
};

#endif /* __AWS_CACHE_H__ */
