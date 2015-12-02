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

#include "CloudCache.h"

CloudCache::~CloudCache()
{
}

const char *
CloudCache::read_config(char *cloud_provider)
{
  // normalize cloud provider name (to upper case)
  int i = 0;
  while (cloud_provider[i]) {
    cloud_provider[i] = toupper(cloud_provider[i]);
    ++i;
  }

  Debug("cache_init", "cloud_provider normalized %s", cloud_provider);

  // create provider object based on name

//  ats_scoped_str storage_path(RecConfigReadConfigPath("proxy.config.cache.storage_filename", "aws_cache.config"));
//  Debug("http_flying_squid", (const char *)storage_path);

  return "as";
}
