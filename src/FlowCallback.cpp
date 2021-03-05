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

/* **************************************************** */

FlowCallback::FlowCallback(bool _packet_interface_only, bool _nedge_exclude, bool _nedge_only,
			   bool _has_protocol_detected, bool _has_periodic_update, bool _has_flow_end) {

  if(_packet_interface_only)  packet_interface_only = 1;
  if(_nedge_exclude)          nedge_exclude = 1;
  if(_nedge_only)             nedge_only = 1;
  if(_has_protocol_detected)  has_protocol_detected = 1;
  if(_has_periodic_update)    has_periodic_update = 1;
  if(_has_flow_end)           has_flow_end = 1;
  
  enabled = 0, severity_id = alert_level_warning;
};

/* **************************************************** */

FlowCallback::~FlowCallback() {
};

/* **************************************************** */

bool FlowCallback::triggerAlert(Flow *f, FlowCallbackStatus status, AlertLevel severity, u_int16_t alert_score, const char *alert_json) {
  bool first_alert = !f->isFlowAlerted();
  bool rv = false;
  u_int32_t buflen;
  AlertFifoItem notification;

  /* Logic similar to ntop_flow_trigger_alert */
  if(f->triggerAlert(status, severity, alert_score, alert_json)) {
    /* The alert was successfully triggered */
    ndpi_serializer flow_json;
    const char *flow_str;

    ndpi_init_serializer(&flow_json, ndpi_serialization_format_json);

    /* Only proceed if there is some space in the queues */
    f->flow2alertJson(&flow_json, time(NULL));

    if(!first_alert)
      ndpi_serialize_string_boolean(&flow_json, "replace_alert", true);

    if(false /* status_always_notify */)
      ndpi_serialize_string_boolean(&flow_json, "status_always_notify", true);

    flow_str = ndpi_serializer_get_buffer(&flow_json, &buflen);

    /* TODO: read all the recipients responsible for flows, and enqueue only to them */
    /* Currenty, we forcefully enqueue only to the builtin sqlite */
    
    if((notification.alert = strdup(flow_str))) {
      notification.alert_severity = severity;

      rv = ntop->recipient_enqueue(0/* SQLite builtin*/,
				   severity >= alert_level_error ? recipient_notification_priority_high : recipient_notification_priority_low,
				   &notification);
    }

    if(!rv) {
      NetworkInterface *iface = f->getInterface();
      if(iface)
	iface->incNumDroppedAlerts(1);

      if(notification.alert)
	free(notification.alert);
    }

    ndpi_term_serializer(&flow_json);
  }

  return true;
}

/* **************************************************** */

bool FlowCallback::isCallbackCompatibleWithInterface(NetworkInterface *iface) {
  if(packet_interface_only && (!iface->isPacketInterface())) return(false);
  if(nedge_only && (!ntop->getPrefs()->is_nedge_edition()))  return(false);
  if(nedge_exclude && ntop->getPrefs()->is_nedge_edition())  return(false);

  return(true);
}

/* **************************************************** */

void FlowCallback::addCallback(std::list<FlowCallback*> *l, NetworkInterface *iface, FlowCallbacks callback) {
  if(!isCallbackCompatibleWithInterface(iface)) return;

  switch(callback) {
  case flow_callback_protocol_detected:
    if(has_protocol_detected) l->push_back(this);
    break;
    
  case flow_callback_periodic_update:
    if(has_periodic_update) l->push_back(this);
    break;
    
  case flow_callback_flow_end:
    if(has_flow_end) l->push_back(this);
    break;
  }
}

/* **************************************************** */

bool FlowCallback::loadConfiguration(json_object *config) {
  json_object *obj;
  bool rc = true;
  
  ntop->getTrace()->traceEvent(TRACE_NORMAL, "%s()", __FUNCTION__);
  
  if(json_object_object_get_ex(config, "severity_id", &obj)) {
    if((severity_id = (AlertLevel)json_object_get_int(obj)) >= ALERT_LEVEL_MAX_LEVEL)
      severity_id = alert_level_emergency;
  } else
    rc = false;
  
  return(rc);
}