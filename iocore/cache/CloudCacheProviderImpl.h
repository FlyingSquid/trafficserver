/** @file
 *
 *  CloudProvider.h - interface for cloud cache operations, implemented
 *  by specific cloud provider classes
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

#ifndef __CLOUD_CACHE_PROVIDER_IMPL_H__
#define __CLOUD_CACHE_PROVIDER_IMPL_H__


class CloudCacheProviderImpl
{
public:
//  CloudProvider() {}
  virtual ~CloudCacheProviderImpl() {}

  virtual const char *read_config() = 0;

  // TODO: simply pass an iostream, key, metadata, cleaned up by CloudCache::open_read etc.

  virtual Action *open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                             CacheLookupHttpConfig *params) = 0;

  virtual Action *open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                             CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache) = 0;

  virtual std::string getProviderName() = 0;
};

#endif /* __CLOUD_CACHE_PROVIDER_IMPL_H__ */
