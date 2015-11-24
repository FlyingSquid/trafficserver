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

#include "ts/ts.h"

#define PLUGIN_NAME "flying_squid"


static int
flying_squid_plugin(TSCont contp, TSEvent event, void *edata)
{
  TSDebug(PLUGIN_NAME, "Plugin callback after hook called");

  TSHttpTxn txnp = (TSHttpTxn)edata;

  switch (event) {
    case TS_EVENT_HTTP_READ_REQUEST_HDR:
      TSDebug(PLUGIN_NAME, "Request header hook callback");
      // compute cache key from request header
      // do cache lookup
      //   if in cache
      //     return appropriate response (have to skip over some states)
      //   if not in cache
      //     register handler for transform/send response header
      TSHttpTxnReenable(txnp, TS_EVENT_HTTP_CONTINUE);
      break;
    // case transform
    // case response header
    // case return response body
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
