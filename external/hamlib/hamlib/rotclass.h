/*
 *  Hamlib C++ bindings - rotator API header
 *  Copyright (c) 2002 by Stephane Fillod
 *
 *
 *   This library is free software; you can redistribute it and/or
 *   modify it under the terms of the GNU Lesser General Public
 *   License as published by the Free Software Foundation; either
 *   version 2.1 of the License, or (at your option) any later version.
 *
 *   This library is distributed in the hope that it will be useful,
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 *   Lesser General Public License for more details.
 *
 *   You should have received a copy of the GNU Lesser General Public
 *   License along with this library; if not, write to the Free Software
 *   Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA
 *
 */

#ifndef _ROTCLASS_H
#define _ROTCLASS_H 1

#include <hamlib/rotator.h>



class BACKEND_IMPEXP Rotator {
private:
  ROT* theRot;  // Global ref. to the rot

protected:
public:
  Rotator(rot_model_t rot_model);

  virtual ~Rotator();

  const struct rot_caps *caps;

  // This method open the communication port to the rot
  void open(void);

  // This method close the communication port to the rot
  void close(void);

  void setConf(token_t token, const char *val);
  void setConf(const char *name, const char *val);
  void getConf(token_t token, char *val);
  void getConf(const char *name, char *val);
  token_t tokenLookup(const char *name);

  void setPosition(azimuth_t az, elevation_t el);
  void getPosition(azimuth_t& az, elevation_t& el);
  void stop();
  void park();
  void reset (rot_reset_t reset);

  void move(int direction, int speed);
};



#endif	// _ROTCLASS_H
