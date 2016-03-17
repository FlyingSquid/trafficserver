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
#include "HttpSM.h"
#include "HttpCacheSM.h"

#define AWSCACHE_ALLOCATION_TAG "AWSCache"
#define REC_CONFIG_AWS_CONFIG_FILE_FIELD "proxy.config.http.cache.cloud.provider.aws.config_filename"
#define AWSCACHE_CONFIG_FILENAME "aws_cache.config"


using namespace Aws::Client;
using namespace Aws::S3;
using namespace Aws::S3::Model;
using namespace Aws::Auth;
using namespace Aws::Utils;


DefaultAWSCache::DefaultAWSCache() : CloudProvider() {
  // Initialize Redis client
  if (!redisCli.Initialize("127.0.0.1", 30001, 2, 10)) { // TODO: get values from config file
    // TODO: handle error
  }

  // Initialize Redis lock manager
  redisLockManager = new CRedLock();
  redisLockManager->AddServerUrl("127.0.0.1", 30001); // TODO: get these values from config file
  redisLockManager->SetRetry(REDLOCK_MAX_NUM_RETRIES, REDLOCK_RETRY_DELAY_MS);

  // TODO: Set S3 bucket name here?
}


DefaultAWSCache::~DefaultAWSCache() {}


bool DefaultAWSCache::getRedisObjectLock(string key, CLock &lock)
{
  return redisLockManager->ContinueLock(key.c_str(), REDLOCK_TTL, lock);
}


bool DefaultAWSCache::releaseRedisObjectLock(string key, Clock &lock)
{
  // Redlock library has this always return true
  return redisLockManager->Unlock(lock);
}


bool DefaultAWSCache::objectInCache(string key)
{
  long exists;
  if (redisClient.Exists(key, exists) == RC_SUCCESS) {
    return (exists == 1);
  }
  // TODO: handle error
}


ObjectCacheMeta DefaultAWSCache::getObjectCacheMeta(string key)
{
  string value;
  if (redisClient.Get(key, &value) == RC_SUCCESS) {
    return (ObjectCacheMeta) value.c_str();
  }
  // TODO: handle error
}


inline long getNowTimestamp() {
  return (long) duration_cast<milliseconds>(system_clock::now().time_since_epoch()).count();
}


bool DefaultAWSCache::objectInCloudFront(ObjectCacheMeta m)
{
  // TODO: add fuzziness in this check (if within certain threshold, return true)?
  return m.cloudFrontExpiry > getNowTimestamp();
}


bool DefaultAWSCache::putObjectCacheMeta(string key, ObjectCacheMeta m)
{
  // Set expiry time in Redis metadata store
  if (redisClient.Set(key, string((char *) m)) == RC_SUCCESS) {
    // Set expiry time on S3 object
    CopyObjectRequest copyObjectRequest;
    copyObjectRequest.SetExpires((double) m.cloudFrontExpiry);
    CopyObjectOutcome copyObjectOutcome = s3Client->CopyObject(copyObjectRequest);
    if (CopyObjectOutcome.IsSuccess()) {
      // TODO: handle success, return true?
    } else {
      // TODO: handle error, return false?
    }

  }
  // TODO: handle error
}


string DefaultAWSCache::getObjectCloudFrontLocation(string key)
{
  return cloudFrontDistrBase + key;
}

getObjectS3(string key, const char *rangeValue = NULL)
{
  GetObjectRequest getObjectRequest;
  getObjectRequest.SetBucket(s3BucketName);
  getObjectRequest.SetKey(key);
  if (rangeValue) {
    getObjectRequest.SetRange(rangeValue);
  }

  std::stringbuf buf;
  getObjectRequest.SetResponseStreamFactory(
    [&buf](){
      return Aws::New<Aws::IOStream>(AWSCACHE_ALLOCATION_TAG, &buf);
    }
  );
  auto getObjectOutcome = s3Client.GetObject(getObjectRequest);
  if(getObjectOutcome.IsSuccess()) {
    Debug("cloud_cache_aws", "S3 bucket (%s) hit and download success for key: %u", s3BucketName, key);
    return buf.str().c_str();
  } else { // TODO: change to list bucket objects, inspect outcome more to set the event handling properly
    Debug("cloud_cache_aws", "S3 bucket (%s) cache miss for key: %u", s3BucketName, key);
    return NULL;
  }
}


const char *
DefaultAWSCache::read_config()
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

  // TODO: read CloudFront distribution name
  s3bucket_name = "flyingsquid-ats-test2-sunjay";

//  ::close(fd);

  Debug("cloud_cache_aws", "using S3 bucket: %s", s3bucket_name);

Lfail:
  // write error message

  return err;
}

Action *
DefaultAWSCache::open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                    CacheLookupHttpConfig *params, time_t pin_in_cache)
{
  string keyStr = to_string(cache_hash(key->hash));

  if (objectInCache(keyStr)) {
    Clock redisLock;
    if (!getRedisObjectLock(keyStr, &redisLock)) {
      // TODO: handle error
    }

    ObjectCacheMeta m = getObjectCacheMeta(keyStr);

    if (objectInCloudFront(m)) {
      // TODO: if accessed very recently, move to local RAM cache

      // TODO: increment CF life if close to expiration

      // TODO: return CF redirect
    } else {
      // TODO: if object accessed last before some threshold, serve from S3
      if (1/*the above*/) {
        char *doc;
        if (doc = getObjectS3(keyStr)) {
          CacheVC *cacheVC = (CacheVC *((HttpCacheSM *)cont)->cache_read_vc);
          char *cacheVCBuf = cacheVC->buf;
          (*cacheVCBuf) = doc;

//        return whatever continuation means that we have finished cache read and we can start returning to the client
        } else {
          // Should never happen, just in case
          //    CACHE_INCREMENT_DYN_STAT(cache_read_failure_stat);
          cont->handleEvent(CACHE_EVENT_OPEN_READ_FAILED, (void *)-ECACHE_NO_DOC);
          return ACTION_RESULT_DONE;
        }
      } else {
        // TODO: check if object last accessed very recently
        if (1 /* above */) {
          // m.cloudFrontExpiry = SOME_GENERATED_VALUE;
          // putObjectCacheMeta(keyStr, m);
        }

        // TODO: return CF redirect
      }
    }

    if (!releaseRedisObjectLock(keyStr, &redisLock)) {
      // TODO: handle error
    }
  } else {
    CACHE_INCREMENT_DYN_STAT(cache_read_failure_stat);
    cont->handleEvent(CACHE_EVENT_OPEN_READ_FAILED, (void *)-ECACHE_NO_DOC);
    return ACTION_RESULT_DONE;
  }
}


Action *
DefaultAWSCache::open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                            CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache)
{
  string keyStr = to_string(cache_hash(key->hash));

  // Get pointer to VIO/IOBufferReader thing

  // Pass key and VIO/IOBufferReader thing to putObjectS3

  // Make Object meta object to put in Redis
  ObjectCacheMeta m;
  m.size = (size_t) expected_size;
  m.lastAccessed = getNowTimestamp();
  m.cloudFrontExpiry = 0;
  putObjectCacheMeta(keyStr, m);








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
