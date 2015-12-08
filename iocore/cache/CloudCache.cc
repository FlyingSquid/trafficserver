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
  const char *err = NULL;

  if (!cloud_provider)
    err = "No cloud cache provider specified in records.config";

  // normalize cloud provider name (to upper case)
  int i = 0;
  while (cloud_provider[i]) {
    cloud_provider[i] = toupper(cloud_provider[i]);
    ++i;
  }

  Debug("cache_init", "Cloud cache provider: \"%s\"", (const char *)cloud_provider);

  // create provider object based on name
  if (strcmp(cloud_provider, PROVIDER_NAME_AWS) == 0)
    cCache = new AWSCache();
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
                      CacheLookupHttpConfig *params, time_t pin_in_cache)
{
//  cCache->open_read();
  return NULL;
}

Action *
CloudCache::open_write(Continuation *cont, int expected_size, const HttpCacheKey *key,
                       CacheHTTPHdr *request, CacheHTTPInfo *old_info, time_t pin_in_cache)
{
  return cCache->open_write(cont, expected_size, key, request, old_info, pin_in_cache);
}
