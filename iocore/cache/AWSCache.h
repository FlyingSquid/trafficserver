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

#include "HttpServerSession.h"
#include "HttpSM.h"
#include "CloudCache.h"
#include "redlock.h"
#include "RedisClient.h"

#define PROVIDER_NAME_AWS "AWS"

// Redis lock manager constants
#define REDLOCK_MAX_NUM_RETRIES 10
#define REDLOCK_RETRY_DELAY_MS 200
#define REDLOCK_TTL 1000

#define AWS_REDIS_STORE_MAX_HEADER_SIZE 8000

#define AWS_CF_URL_FORMAT "http://%s/"


class DefaultAWSCache : public CloudCacheProviderImpl
{
public:
#pragma pack(1)
  struct ObjectCacheMeta {
    int64_t size; // Object size in bytes
    int64_t headerLength;
    char responseHeader[AWS_REDIS_STORE_MAX_HEADER_SIZE];
    long lastAccessed;
    long cloudFrontExpiry;
  };
#pragma pack(0)

  DefaultAWSCache();
  ~DefaultAWSCache();

  virtual const char *read_config();

  // TODO: this interface will change, will move event/continuation handling to CloudCache::open_write etc.

  virtual Action *open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                            CacheLookupHttpConfig *params);

  virtual Action *open_write(Continuation *cont, CacheVC *cacheVC, const HttpCacheKey *key,
                             CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache);

  virtual string getProviderName()
  {
    return PROVIDER_NAME_AWS;
  }

private:
  // Lock manager used to get and release locks on Redis resources
  // Locks are required in order to prevent multiple requests modifying metadata/moving objects in the cache
  CRedLock *redisLockManager;
  CRedisClient redisClient;

  string cloudFrontDistrBase;
  string s3BucketName;
  Aws::S3::S3Client s3Client;

  // Try to obtain the Redis lock for an object and return success or failure
  // lock passed in is updated with the lock meta if successful
  bool getRedisObjectLock(string key, CLock &lock);

  // Release Redis lock for an object, return success or failure (always returns true for now)
  bool releaseRedisObjectLock(string key, CLock &lock);

  // Check if object in distributed cache by checking Redis cluster for existence of object metadata
  bool objectInCache(string key);

  ObjectCacheMeta *getObjectCacheMeta(string key);

  // Check if object's CloudFront expiry is in the future
  bool objectInCloudFront(ObjectCacheMeta m);

  // Save object metadata in Redis datastore and S3 (expiry field)
  bool putObjectCacheMeta(string key, ObjectCacheMeta m);

  // Get CloudFront resource URL (for redirecting client)
  string getObjectCloudFrontLocation(string key);

  void setCloudFrontRedirect(HttpSM *httpSM, string url);

  bool putObjectS3(string key, IOBufferReader *buf, int64_t *size);

  char *getObjectS3(string key, long *sizeofObject, const char *rangeValue);

//  moveObjectToRAM(string key);

};

#endif /* __AWS_CACHE_H__ */
