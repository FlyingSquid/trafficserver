/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

/******************************************************************************
 * flying_squid.cc
 *
 *  
 *****************************************************************************/

#include <stdlib.h>
#include <string.h>

#include "ts/ts.h"

#define PLUGIN_NAME "flying_squid"

static char *
compute_cache_key(TSHttpTxn txnp)
{
  TSMBuffer bufp;
  TSMLoc hdr_loc;
  TSMLoc url_loc;
  int url_length;
  char *url_str;

  TSDebug(PLUGIN_NAME, "computing cache key");

  if (TSHttpTxnClientReqGet(txnp, &bufp, &hdr_loc) != TS_SUCCESS) {
    TSError("[%s] Couldn't retrieve client request header", PLUGIN_NAME);
    // handle error
  }

  if (TSHttpHdrUrlGet(bufp, hdr_loc, &url_loc) != TS_SUCCESS) {
    TSError("[blacklist] Couldn't retrieve request url");
    TSHandleMLocRelease(bufp, TS_NULL_MLOC, hdr_loc);
    // handle error
  }

  url_str = TSUrlStringGet(bufp, url_loc, &url_length);

  TSDebug(PLUGIN_NAME, (char *)url_str);

  // release header and url strings
  TSHandleMLocRelease (bufp, hdr_loc, url_loc);
  TSHandleMLocRelease (bufp, TS_NULL_MLOC, hdr_loc);

  return (char *)"thisisakey";
}

static bool
cache_lookup(char *key)
{
  TSDebug(PLUGIN_NAME, "cache lookup");

  // return s3 bucket query for key

  if (true) {
    TSDebug(PLUGIN_NAME, "cache hit");
//    return true;
  } else {
    TSDebug(PLUGIN_NAME, "cache miss");
//    return false;
  }

  return false;
}

static int
cache_write(TSCont contp, TSEvent event, void *edata)
{

  return 0;
}

static int
flying_squid_plugin(TSCont contp, TSEvent event, void *edata)
{
  TSDebug(PLUGIN_NAME, "Plugin callback after hook called");

  TSHttpTxn txnp = (TSHttpTxn)edata;

  switch (event) {
    case TS_EVENT_HTTP_READ_REQUEST_HDR:
      TSDebug(PLUGIN_NAME, "Request header hook callback");
      if (cache_lookup(compute_cache_key(txnp))) {
        TSDebug(PLUGIN_NAME, "register handler for returning cache object");
      } else {
        TSDebug(PLUGIN_NAME, "register handler for transform hook");
//        TSHttpTxnHookAdd(txnp, TS_HTTP_RESPONSE_TRANSFORM_HOOK, TSTransformCreate(cache_write, txnp));
      }
      TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
      break;
    // case transform
    case TS_EVENT_HTTP_READ_RESPONSE_HDR:
      TSDebug(PLUGIN_NAME, "read response header callback");
      // Check if response was from cache or from OS
      // If not from cache,
      //     Write response to cache
      // Go on to do whatever is done normally
      TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
      break;
    default:
      break;
  }

  return 0;
}

void
TSPluginInit(int argc, const char *argv[])
{
  TSPluginRegistrationInfo info;
  TSCont contp;

  info.plugin_name = (char *)PLUGIN_NAME;
  info.vendor_name = (char *)"VENDOR_NAME";
  info.support_email = (char *)"VENDOR_EMAIL@example.com";

  if (TSPluginRegister(&info) != TS_SUCCESS) {
    TSError("[%s] %s", PLUGIN_NAME, "Plugin registration failed");
    TSError("[%s] Unable to initialize plugin (disabled)", PLUGIN_NAME);
    return;
  }
  TSDebug(PLUGIN_NAME, "Plugin registration succeeded");

  contp = TSContCreate((TSEventFunc)flying_squid_plugin, NULL);
  TSHttpHookAdd(TS_HTTP_READ_REQUEST_HDR_HOOK, contp);
}
