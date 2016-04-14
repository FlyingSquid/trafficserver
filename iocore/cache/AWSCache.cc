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
#include <aws/s3/model/CopyObjectRequest.h>
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


DefaultAWSCache::DefaultAWSCache() : CloudCacheProviderImpl() {
  Debug("FLYING_SQUID", "in DefaultAWSCache::DefaultAWSCache");

  // Initialize Redis client
  if (!redisClient.Initialize("127.0.0.1", 30001, 2, 10)) { // TODO: get values from config file
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
  Debug("FLYING_SQUID", "in DefaultAWSCache::getRedisObjectLock");
  return redisLockManager->ContinueLock(key.c_str(), REDLOCK_TTL, lock);
}


bool DefaultAWSCache::releaseRedisObjectLock(string key, CLock &lock)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::releaseRedisObjectLock");
  // Redlock library has this always return true
  return redisLockManager->Unlock(lock);
}


bool DefaultAWSCache::objectInCache(string key)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::objectInCache");
  long exists;
  if (redisClient.Exists(key, &exists) == RC_SUCCESS) {
    return (exists == 1);
  }
  // TODO: handle error
  return false;
}


DefaultAWSCache::ObjectCacheMeta *DefaultAWSCache::getObjectCacheMeta(string key)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::getObjectCacheMeta");
  string *value = new string;
  if (redisClient.Get(key, value) == RC_SUCCESS) {
    return (ObjectCacheMeta *) value->c_str();
  }
  // TODO: handle error
  return NULL;
}


inline long getNowTimestamp() {
  return (long) std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::system_clock::now().time_since_epoch()).count();
}


bool DefaultAWSCache::objectInCloudFront(ObjectCacheMeta m)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::objectInCloudFront");
  // TODO: add fuzziness in this check (if within certain threshold, return true)?
  return m.cloudFrontExpiry > getNowTimestamp();
}


bool DefaultAWSCache::putObjectCacheMeta(string key, ObjectCacheMeta m)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::putObjectCacheMeta");
  // Set expiry time in Redis metadata store
  if (redisClient.Set(key, string((char *) &m)) == RC_SUCCESS) {
    // Set expiry time on S3 object
    CopyObjectRequest copyObjectRequest;
    copyObjectRequest.SetExpires((double) m.cloudFrontExpiry);
    CopyObjectOutcome copyObjectOutcome = s3Client.CopyObject(copyObjectRequest);
    if (copyObjectOutcome.IsSuccess()) {
      // TODO: handle success, return true?
      return true;
    } else {
      // TODO: handle error, return false?
      return false;
    }

  }
  // TODO: handle error
  return false;
}


string DefaultAWSCache::getObjectCloudFrontLocation(string key)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::getObjectCloudFrontLocation");
  return cloudFrontDistrBase + key;
}

void DefaultAWSCache::setCloudFrontRedirect(HttpSM *httpSM, string url)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::setCloudFrontRedirect");
  httpSM->enable_redirection = true;
  httpSM->redirect_url = (char *) url.c_str();
  httpSM->redirect_url_len = url.length();
}

bool DefaultAWSCache::putObjectS3(string key, IOBufferReader *buf) {
  Debug("FLYING_SQUID", "in DefaultAWSCache::putObjectS3");
  PutObjectRequest putObjectRequest;
  putObjectRequest.SetBucket(s3BucketName);
  putObjectRequest.SetKey(key);

  char wb[buf->read_avail()];
  // TODO: error checking to make sure we actually read the number of bytes we wanted
  // TODO: make more robust for large objects and multipart upload?
  buf->read(wb, buf->read_avail());
  std::stringbuf writeBuf;
  writeBuf.pubsetbuf(wb, buf->read_avail());
  std::shared_ptr<Aws::IOStream> writestream = Aws::MakeShared<Aws::IOStream>(AWSCACHE_ALLOCATION_TAG, &writeBuf);
  putObjectRequest.SetBody(writestream);
  putObjectRequest.SetContentLength(static_cast<long>(putObjectRequest.GetBody()->tellp()));
  putObjectRequest.SetContentMD5(HashingUtils::Base64Encode(HashingUtils::CalculateMD5(*putObjectRequest.GetBody())));
  PutObjectOutcome putObjectOutcome = s3Client.PutObject(putObjectRequest);

  return putObjectOutcome.IsSuccess();
}

char *DefaultAWSCache::getObjectS3(string key, long *sizeOfObject, const char *rangeValue = NULL)
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::getObjectS3");
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
    GetObjectResult *getObjectResult = &(getObjectOutcome.GetResult());
    *sizeOfObject = getObjectResult->GetContentLength();
    return (char *) buf.str().c_str();
  } else {
    return NULL;
  }
}


const char *
DefaultAWSCache::read_config()
{
  Debug("FLYING_SQUID", "in DefaultAWSCache::read_config");
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
  s3BucketName = "flyingsquid-ats-test2-sunjay";

//  ::close(fd);

  Debug("FLYING_SQUID", "using S3 bucket: %s", s3BucketName.c_str());

Lfail:
  // write error message

  return err;
}

Action *
DefaultAWSCache::open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                    CacheLookupHttpConfig *params)
{
Debug("FLYING_SQUID", "in DefaultAWSCache::open_read");
  string keyStr = to_string(cache_hash(key->hash));

  if (objectInCache(keyStr)) {
Debug("FLYING_SQUID", "object in cache");
    CLock redisLock;
    if (!getRedisObjectLock(keyStr, redisLock)) {
      // TODO: handle error
    }

    ObjectCacheMeta *m = getObjectCacheMeta(keyStr);

    if (objectInCloudFront(*m)) {
Debug("FLYING_SQUID", "object in CloudFront");
      // TODO: if accessed very recently, move to local RAM cache

      // TODO: increment CF life if close to expiration

      setCloudFrontRedirect((HttpSM *) cont, getObjectCloudFrontLocation(keyStr));
    } else {
Debug("FLYING_SQUID", "object not in CloudFront");

      // TODO: if object accessed last before some threshold, serve from S3
      if (1/*the above*/) {
Debug("FLYING_SQUID", "object stale, serving from S3");
        char *doc;
        long objectSize;
        if ((doc = getObjectS3(keyStr, &objectSize))) {
          CacheVC *cacheVC = (CacheVC *) (((HttpCacheSM *)cont)->cache_read_vc);
          memcpy(cacheVC->buf->_data, doc, objectSize);

          // TODO: may be the wrong action below, also may change for chunked downloads from S3

          return ACTION_RESULT_DONE;
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

        setCloudFrontRedirect((HttpSM *) cont, getObjectCloudFrontLocation(keyStr));
      }
    }

    // TODO: if m was modified, update in Redis

    ((HttpSM *) cont)->t_state.cloud_cache_info = (void *) m;

    if (!releaseRedisObjectLock(keyStr, redisLock)) {
      // TODO: handle error
    }
  } else {
Debug("FLYING_SQUID", "object not in cache");
    // TODO: add in statistics -> CACHE_INCREMENT_DYN_STAT(cache_read_failure_stat);
    cont->handleEvent(CACHE_EVENT_OPEN_READ_FAILED, (void *)-ECACHE_NO_DOC);
    return ACTION_RESULT_DONE;
  }


  // TODO: fix above to return the correct action, etc.
  return ACTION_RESULT_DONE;
}


Action *
DefaultAWSCache::open_write(Continuation *cont, CacheVC *cacheVC, const HttpCacheKey *key,
                            CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache)
{
Debug("FLYING_SQUID", "in DefaultAWSCache::open_write");

  if (!key)
    Debug("FLYING_SQUID", "key is NULL");

  string keyStr = to_string(cache_hash(key->hash));

Debug("FLYING_SQUID", "got keyStr");

  // Get pointer to IOBufferReader with data from origin server
  IOBufferReader *reader = cacheVC->vio.buffer.reader();

  if (putObjectS3(keyStr, reader)) {
    // Make Object meta object to put in Redis
    ObjectCacheMeta m;
    m.size = (size_t) reader->read_avail();
    m.lastAccessed = getNowTimestamp();
    m.cloudFrontExpiry = 0;
    HttpTransact::HeaderInfo *hdr_info = &(((HttpSM *) cont)->t_state.hdr_info);
    m.headerLength = hdr_info->response_content_length;
    char *buf = (char *) malloc(hdr_info->response_content_length);
    int bufIndex = 0;
    hdr_info->client_response.print(buf, hdr_info->response_content_length, &bufIndex, &bufIndex);
    memcpy(m.responseHeader, buf, hdr_info->response_content_length);
    free(buf);
    putObjectCacheMeta(keyStr, m);

    // TODO: update stats, maybe have different handler etc.
    return ACTION_RESULT_DONE;
  } else {
    // TODO: handle error properly
    cont->handleEvent(CACHE_EVENT_OPEN_WRITE_FAILED, (void *)-ECACHE_NOT_READY);
    return ACTION_RESULT_DONE;
  }
}
