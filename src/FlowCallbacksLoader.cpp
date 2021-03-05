/*
 *
 * (C) 2013-21 - ntop.org
 *
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place - Suite 330, Boston, MA 02111-1307, USA.
 *
 */

#include "ntop_includes.h"

FlowCallbacksLoader::FlowCallbacksLoader(){
  registerFlowCallbacks();
  loadConfiguration();
}

/* **************************************************** */

FlowCallbacksLoader::~FlowCallbacksLoader() {
  for(std::map<std::string, FlowCallback*>::const_iterator it = cb_all.begin(); it != cb_all.end(); ++it)
    delete it->second;
}

/* **************************************************** */

void FlowCallbacksLoader::registerFlowCallbacks() {
  /* TODO: implement dynamic loading */
  FlowCallback *fcb;

  if((fcb = new BlacklistedFlow()))             cb_all[fcb->getName()] = fcb;
  if((fcb = new ExternalAlertCheck()))          cb_all[fcb->getName()] = fcb;
  if((fcb = new LowGoodputFlow()))              cb_all[fcb->getName()] = fcb;
  if((fcb = new TlsCertificateMismatch()))      cb_all[fcb->getName()] = fcb;
  if((fcb = new DNSDataExfiltration()))         cb_all[fcb->getName()] = fcb;
  if((fcb = new DataExfiltration()))            cb_all[fcb->getName()] = fcb;
  if((fcb = new DeviceProtocolNotAllowed()))    cb_all[fcb->getName()] = fcb;
  if((fcb = new FlowRisk()))                    cb_all[fcb->getName()] = fcb;
  if((fcb = new RemoteToRemote()))              cb_all[fcb->getName()] = fcb;
  if((fcb = new RemoteToLocalInsecureProto()))  cb_all[fcb->getName()] = fcb;
  if((fcb = new TLSCertificateExpired()))       cb_all[fcb->getName()] = fcb;
  if((fcb = new TLSOldProtocolVersion()))       cb_all[fcb->getName()] = fcb;
  if((fcb = new TLSUnsafeCiphers()))            cb_all[fcb->getName()] = fcb;
  if((fcb = new TcpZeroWindow()))               cb_all[fcb->getName()] = fcb;
  if((fcb = new TCPIssues()))                   cb_all[fcb->getName()] = fcb;
  if((fcb = new UDPUnidirectional()))           cb_all[fcb->getName()] = fcb;
}

/* **************************************************** */

void FlowCallbacksLoader::loadConfiguration() {
  json_object *json = NULL, *json_config, *json_config_flow;
  struct json_object_iterator it;
  struct json_object_iterator itEnd;
  enum json_tokener_error jerr = json_tokener_success;
  char *value = NULL;
  u_int actual_len = ntop->getRedis()->len(FLOW_CALLBACKS_CONFIG);

  if((value = (char *) malloc(actual_len + 1)) == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Unable to allocate memory to deserialize %s", FLOW_CALLBACKS_CONFIG);
    goto out;
  }

  if(ntop->getRedis()->get((char*)FLOW_CALLBACKS_CONFIG, value, actual_len + 1) != 0) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "Unable to find configuration %s", FLOW_CALLBACKS_CONFIG);
    goto out;
  }

  if((json = json_tokener_parse_verbose(value, &jerr)) == NULL) {
    ntop->getTrace()->traceEvent(TRACE_ERROR, "JSON Parse error [%s] %s [len: %u][strlen: %u]",
				 json_tokener_error_desc(jerr), value, actual_len, strlen(value));
    goto out;
  }

  if(!json_object_object_get_ex(json, "config", &json_config)) {
    /* 'config' section inside the JSON */
    ntop->getTrace()->traceEvent(TRACE_ERROR, "'config' not found in JSON");
    goto out;
  }

  if(!json_object_object_get_ex(json_config, "flow", &json_config_flow)) {
    /* 'flow' section inside 'config' JSON */
    ntop->getTrace()->traceEvent(TRACE_ERROR, "'flow' not found in 'config' JSON");
    goto out;
  }

  /*
    Iterate over all script configurations
  */
  it = json_object_iter_begin(json_config_flow);
  itEnd = json_object_iter_end(json_config_flow);

  while(!json_object_iter_equal(&it, &itEnd)) {
    const char *callback_key   = json_object_iter_peek_name(&it);
    json_object *callback_config = json_object_iter_peek_value(&it);
    json_object *json_script_conf, *json_hook_all;

    if(json_object_object_get_ex(callback_config, "all", &json_hook_all)) {
      json_object *json_enabled;
      bool enabled;

      if(json_object_object_get_ex(json_hook_all, "enabled", &json_enabled))
	enabled = json_object_get_boolean(json_enabled);
      else
	enabled = false;

      if(enabled) {
	if(json_object_object_get_ex(json_hook_all, "script_conf", &json_script_conf)) {
	  if(cb_all.find(callback_key) != cb_all.end()) {
	    FlowCallback *cb = cb_all[callback_key];


	    if(cb->loadConfiguration(json_script_conf)) {
	      ntop->getTrace()->traceEvent(TRACE_NORMAL, "Successfully loadeded configuration for callback %s", callback_key);
	    } else {
	      ntop->getTrace()->traceEvent(TRACE_WARNING, "Error while loading callback %s configuration", callback_key);
	    }

	    cb->enable();
	  } else
	    ntop->getTrace()->traceEvent(TRACE_WARNING, "Unable to find flow callback  %s", callback_key);
	}
      } /* enabled */
    }

    /* Move to the next element */
    json_object_iter_next(&it);
  } /* while */

 out:
  /* Free the json */
  if(json)  json_object_put(json);
  if(value) free(value);
}

/* **************************************************** */

void FlowCallbacksLoader::printCallbacks() {
  ntop->getTrace()->traceEvent(TRACE_NORMAL, "Available Callbacks:");

  for(std::map<std::string, FlowCallback*>::const_iterator it = cb_all.begin(); it != cb_all.end(); ++it)
    ntop->getTrace()->traceEvent(TRACE_NORMAL, "\t%s", it->first.c_str());
}

/* **************************************************** */

std::list<FlowCallback*>* FlowCallbacksLoader::getCallbacks(NetworkInterface *iface, FlowCallbacks callback) {
  std::list<FlowCallback*> *l = new std::list<FlowCallback*>;

  for(std::map<std::string, FlowCallback*>::const_iterator it = cb_all.begin(); it != cb_all.end(); ++it) {
    FlowCallback *cb = it->second;

    if(cb->isEnabled())
      cb->addCallback(l, iface, callback);
  }

  return(l);
}