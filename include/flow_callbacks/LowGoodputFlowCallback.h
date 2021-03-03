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

#ifndef _LOWGOODPUT_FLOW_CALLBACK_H_
#define _LOWGOODPUT_FLOW_CALLBACK_H_

#include "ntop_includes.h"

class LowGoodputFlowCallback : public FlowCallback {
 private:
  void periodicUpdate(Flow *f) { /* f->setStatus(); */ };
  void flowEnd(Flow *f)        { /* f->setStatus(); */ };

 public:
  LowGoodputFlowCallback(json_object *json_config) : FlowCallback(json_config) {};
  virtual ~LowGoodputFlowCallback() {};

  bool hasCallback(FlowLuaCall flow_lua_call) const { return flow_lua_call == flow_lua_call_periodic_update || flow_lua_call == flow_lua_call_idle; };
};

#endif /* _LOWGOODPUT_FLOW_CALLBACK_H_ */
