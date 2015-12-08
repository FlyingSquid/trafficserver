/** @file
 *
 *  CloudCache.h - general class as a wrapper API to cache operations
 *  implemented by CloudProvider subclasses
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

#ifndef __CLOUD_CACHE_H__
#define __CLOUD_CACHE_H__

#include "P_Cache.h"
#include "CloudProvider.h"


class CloudCache
{
public:
  CloudCache() {}
  ~CloudCache() {}

  /**
   * Called to read the provider specific configuration file and initialize
   * the cache object. Returns NULL if no error and a descriptive error if
   * invalid provider, unable to read config, or if config is malformed
   */
  const char *read_config(char *cloud_provider);

  /**
   * TODO: document
   */
  Action *open_read(Continuation *cont, const HttpCacheKey *key, CacheHTTPHdr *request,
                    CacheLookupHttpConfig *params, time_t pin_in_cache);

  /**
   * TODO: document
   */
  Action *open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                     CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache);

  // TODO: find the other cache actions and implement
//  Action *remove();


private:
  CloudProvider *cCache;
};

#endif /* __CLOUD_CACHE_H__ */
