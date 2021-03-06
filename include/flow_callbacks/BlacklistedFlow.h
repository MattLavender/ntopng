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

#ifndef _BLACKLISTED_FLOW_H_
#define _BLACKLISTED_FLOW_H_

#include "ntop_includes.h"

class BlacklistedFlow : public FlowCallback {
 private:
  
 public:
  BlacklistedFlow() : FlowCallback(ntopng_edition_community,
				   false /* All interfaces */, false /* Don't exclude for nEdge */, false /* NOT only for nEdge */,
				   true /* has_protocol_detected */, false /* has_periodic_update */, false /* has_flow_end */) {};
  ~BlacklistedFlow() {};

  void protocolDetected(Flow *f);
  bool loadConfiguration(json_object *config);
  
  std::string getName()          const { return(std::string("blacklisted")); }
  ScriptCategory getCategory()   const { return script_category_security;    }
  FlowCallbackStatus getStatus() const { return status_blacklisted;        }
};

#endif /* _BLACKLISTED_FLOW_H_ */
