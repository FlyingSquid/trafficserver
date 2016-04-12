/** @file
 *
 *  CloudCache.cc
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


#include "AWSCache.h"
#include "CloudCache.h"


const char *
CloudCache::read_config(char *cloud_provider)
{
  Debug("FLYING_SQUID", "in CloudCache::read_config");

  const char *err = NULL;

  if (!cloud_provider)
    err = "No cloud cache provider specified in records.config";

  // normalize cloud provider name (to upper case)
  int i = 0;
  while (cloud_provider[i]) {
    cloud_provider[i] = toupper(cloud_provider[i]);
    ++i;
  }

  Debug("FLYING_SQUID", "Cloud cache provider: \"%s\"", (const char *)cloud_provider);

  // create provider object based on name
  if (strcmp(cloud_provider, PROVIDER_NAME_AWS) == 0)
    cCache = new DefaultAWSCache();
  else {
    err = "Invalid cloud cache provider name specified in records.config";
    goto Lfail;
  }

  err = cCache->read_config();

Lfail:
  return err;
}

Action *
CloudCache::open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                      CacheLookupHttpConfig *params)
{
  Debug("FLYING_SQUID", "in CloudCache::open_read");
  return cCache->open_read(cont, key, request, params);
}

Action *
CloudCache::open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                       CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache)
{
Debug("FLYING_SQUID", "in CloudCache::open_write");
  return cCache->open_write(cont, expected_size, key, request, old_info, pin_in_cache);
}


IOBufferReader *
CloudCache::getHTTPSMIOBufferReader(Continuation *cont)
{
  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader");
  HttpCacheSM *httpCacheSM = (HttpCacheSM *) cont;
  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 2");
//  HttpSM *httpSM = httpCacheSM->master_sm;
//  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 3");
  CacheVC *cacheVC = (CacheVC *) httpCacheSM->cache_write_vc;
  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 3");
  VIO *vio = &(cacheVC->vio);
  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 4");
  MIOBufferAccessor *mioBufferAccessor = &(vio->buffer);
  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 5");
  IOBufferReader *ioBufferReader = mioBufferAccessor->reader();
  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 6");
  int64_t read_avail = ioBufferReader->read_avail();
  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader %ld bytes available", read_avail);

//  HttpServerSession *httpServerSession = httpSM->get_server_session();
//  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 4");
//  if (!httpServerSession)
//    Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader httpServerSession == NULL");
//  if (!(httpSM->server_buffer_reader))
//    Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader server_buffer_reader == NULL");
//  UnixNetVConnection *netVC = (UnixNetVConnection *) httpServerSession->get_netvc();
//  Debug("FLYING_SQUID", "in CloudCache::getHTTPSMIOBufferReader 5");
//  UnixNetVConnection *netVC = (UnixNetVConnection *) (((HttpCacheSM *) cont)->master_sm->get_server_session()->get_netvc());
//  MIOBufferAccessor *buf = &(netVC->read.vio.buffer);
//  IOBufferReader *reader = buf->reader();
//  return reader;
  return NULL;
}

int64_t
CloudCache::getObjectSize(void *cloudCacheInfo)
{
  Debug("FLYING_SQUID", "in CloudCache::getObjectSize");
  string provider = cCache->getProviderName();

  if (provider == PROVIDER_NAME_AWS) {
    DefaultAWSCache::ObjectCacheMeta *m = (DefaultAWSCache::ObjectCacheMeta *) cloudCacheInfo;
    return m->size;
  }

  // TODO: handle error (invalid provider name etc.)

  return -1;
}


char *
CloudCache::getResponseHeader(void *cloudCacheInfo, int64_t *hdr_size)
{
  Debug("FLYING_SQUID", "in CloudCache::getResponseHeader");
  string provider = cCache->getProviderName();

  if (provider == PROVIDER_NAME_AWS) {
    DefaultAWSCache::ObjectCacheMeta *m = (DefaultAWSCache::ObjectCacheMeta *) cloudCacheInfo;
    char *responseHeader = (char *) malloc(m->headerLength);
    memcpy(responseHeader, m->responseHeader, m->headerLength);
    *hdr_size = m->headerLength;
    return responseHeader;
  }

  // TODO: handle error (invalid provider name etc.)

  return NULL;
}
