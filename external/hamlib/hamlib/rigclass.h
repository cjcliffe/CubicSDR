/*
 *  Hamlib C++ bindings - API header
 *  Copyright (c) 2001-2002 by Stephane Fillod
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

#ifndef _RIGCLASS_H
#define _RIGCLASS_H 1

#include <hamlib/rig.h>
#include <iostream>


class BACKEND_IMPEXP Rig {
private:
  RIG* theRig;  // Global ref. to the rig

protected:
public:
  Rig(rig_model_t rig_model);

  virtual ~Rig();

  const struct rig_caps *caps;

  // This method open the communication port to the rig
  void open(void);

  // This method close the communication port to the rig
  void close(void);

  void setConf(token_t token, const char *val);
  void setConf(const char *name, const char *val);
  void getConf(token_t token, char *val);
  void getConf(const char *name, char *val);
  token_t tokenLookup(const char *name);

  void setFreq(freq_t freq, vfo_t vfo = RIG_VFO_CURR);
  freq_t getFreq(vfo_t vfo = RIG_VFO_CURR);
  void setMode(rmode_t, pbwidth_t width = RIG_PASSBAND_NORMAL, vfo_t vfo = RIG_VFO_CURR);
  rmode_t getMode(pbwidth_t&, vfo_t vfo = RIG_VFO_CURR);
  void setVFO(vfo_t);
  vfo_t getVFO();

  void setPTT (ptt_t ptt, vfo_t vfo = RIG_VFO_CURR);
  ptt_t getPTT (vfo_t vfo = RIG_VFO_CURR);
  dcd_t getDCD (vfo_t vfo = RIG_VFO_CURR);

  void setLevel(setting_t level, int vali, vfo_t vfo = RIG_VFO_CURR);
  void setLevel(setting_t level, float valf, vfo_t vfo = RIG_VFO_CURR);
  void getLevel(setting_t level, int& vali, vfo_t vfo = RIG_VFO_CURR);
  void getLevel(setting_t level, float& valf, vfo_t vfo = RIG_VFO_CURR);
  int getLevelI(setting_t level, vfo_t vfo = RIG_VFO_CURR);
  float getLevelF(setting_t level, vfo_t vfo = RIG_VFO_CURR);
  bool hasGetLevel (setting_t level);
  bool hasSetLevel (setting_t level);

  void setParm(setting_t parm, int vali);
  void setParm(setting_t parm, float valf);
  void getParm(setting_t parm, int& vali);
  void getParm(setting_t parm, float& valf);
  int getParmI(setting_t parm);
  float getParmF(setting_t parm);
  bool hasGetParm (setting_t parm);
  bool hasSetParm (setting_t parm);

  void setFunc (setting_t func, bool status, vfo_t vfo = RIG_VFO_CURR);
  bool getFunc (setting_t func, vfo_t vfo = RIG_VFO_CURR);
  bool hasGetFunc (setting_t func);
  bool hasSetFunc (setting_t func);

  void VFOop(vfo_op_t op, vfo_t vfo = RIG_VFO_CURR);
  bool hasVFOop (vfo_op_t op);

  void scan(scan_t scan, int ch, vfo_t vfo = RIG_VFO_CURR);
  bool hasScan (scan_t scan);

  const char *getInfo (void);
  pbwidth_t passbandNormal (rmode_t);
  pbwidth_t passbandNarrow (rmode_t);
  pbwidth_t passbandWide (rmode_t);

  void setRptrShift (rptr_shift_t rptr_shift, vfo_t vfo = RIG_VFO_CURR);
  rptr_shift_t getRptrShift (vfo_t vfo = RIG_VFO_CURR);
  void setRptrOffs (shortfreq_t rptr_offs, vfo_t vfo = RIG_VFO_CURR);
  shortfreq_t getRptrOffs (vfo_t vfo = RIG_VFO_CURR);
  void setTs (shortfreq_t ts, vfo_t vfo = RIG_VFO_CURR);
  shortfreq_t getTs (vfo_t vfo = RIG_VFO_CURR);

  void setCTCSS (tone_t tone, vfo_t vfo = RIG_VFO_CURR);
  tone_t getCTCSS (vfo_t vfo = RIG_VFO_CURR);
  void setDCS (tone_t code, vfo_t vfo = RIG_VFO_CURR);
  tone_t getDCS (vfo_t vfo = RIG_VFO_CURR);

  void setCTCSSsql (tone_t tone, vfo_t vfo = RIG_VFO_CURR);
  tone_t getCTCSSsql (vfo_t vfo = RIG_VFO_CURR);
  void setDCSsql (tone_t tone, vfo_t vfo = RIG_VFO_CURR);
  tone_t getDCSsql (vfo_t vfo = RIG_VFO_CURR);


  unsigned int power2mW (float power, freq_t freq, rmode_t mode);
  float mW2power (unsigned int mwpower, freq_t freq, rmode_t mode);
  void setTrn (int trn);
  int getTrn (void);
  void setBank (int bank, vfo_t vfo = RIG_VFO_CURR);
  void setMem (int ch, vfo_t vfo = RIG_VFO_CURR);
  int getMem (vfo_t vfo = RIG_VFO_CURR);

  void setChannel (const channel_t *chan);
  void getChannel (channel_t *chan);

  void setPowerStat (powerstat_t status);
  powerstat_t getPowerStat (void);
  rmode_t RngRxModes (freq_t freq);
  rmode_t RngTxModes (freq_t freq);

  void setSplitFreq (freq_t tx_freq, vfo_t vfo = RIG_VFO_CURR);
  freq_t getSplitFreq (vfo_t vfo = RIG_VFO_CURR);
  void setSplitMode(rmode_t, pbwidth_t width = RIG_PASSBAND_NORMAL, vfo_t vfo = RIG_VFO_CURR);
  rmode_t getSplitMode(pbwidth_t&, vfo_t vfo = RIG_VFO_CURR);
  void setSplitFreqMode(freq_t, rmode_t, pbwidth_t width = RIG_PASSBAND_NORMAL, vfo_t vfo = RIG_VFO_CURR);
  freq_t getSplitFreqMode(rmode_t&, pbwidth_t&, vfo_t vfo = RIG_VFO_CURR);
  void setSplitVFO(split_t split, vfo_t vfo = RIG_VFO_CURR, vfo_t tx_vfo = RIG_VFO_CURR);
  split_t getSplitVFO(vfo_t &tx_vfo, vfo_t vfo = RIG_VFO_CURR);

  void setRit  (shortfreq_t rit, vfo_t vfo = RIG_VFO_CURR);
  shortfreq_t getRit  (vfo_t vfo = RIG_VFO_CURR);
  void setXit  (shortfreq_t xit, vfo_t vfo = RIG_VFO_CURR);
  shortfreq_t getXit  (vfo_t vfo = RIG_VFO_CURR);

  void setAnt  (ant_t ant, vfo_t vfo = RIG_VFO_CURR);
  ant_t getAnt  (vfo_t vfo = RIG_VFO_CURR);

  void sendDtmf  (const char *digits, vfo_t vfo = RIG_VFO_CURR);
  int recvDtmf  (char *digits, vfo_t vfo = RIG_VFO_CURR);
  void sendMorse  (const char *msg, vfo_t vfo = RIG_VFO_CURR);


  shortfreq_t getResolution (rmode_t mode);
  void reset (reset_t reset);

  // callbacks available in your derived object
  virtual int FreqEvent(vfo_t, freq_t, rig_ptr_t) const {
		  return RIG_OK;
  }
  virtual int ModeEvent(vfo_t, rmode_t, pbwidth_t, rig_ptr_t) const {
		  return RIG_OK;
  }
  virtual int VFOEvent(vfo_t, rig_ptr_t) const {
		  return RIG_OK;
  }
  virtual int PTTEvent(vfo_t, ptt_t, rig_ptr_t) const {
		  return RIG_OK;
  }
  virtual int DCDEvent(vfo_t, dcd_t, rig_ptr_t) const {
		  return RIG_OK;
  }


};



#ifdef __GNUG__
#  if ((__GNUG__ <= 2) && (__GNUC_MINOR__ < 8))
#    if HAVE_TYPEINFO
#      include <typeinfo>
#    endif
#  endif
#endif

#if defined(__GNUG__)
#  if HAVE_BUILTIN_H || HAVE_GXX_BUILTIN_H || HAVE_GPP_BUILTIN_H
#    if ETIP_NEEDS_MATH_H
#      if ETIP_NEEDS_MATH_EXCEPTION
#        undef exception
#        define exception math_exception
#      endif
#      include <math.h>
#    endif
#    undef exception
#    define exception builtin_exception
#    if HAVE_GPP_BUILTIN_H
#     include <gpp/builtin.h>
#    elif HAVE_GXX_BUILTIN_H
#     include <g++/builtin.h>
#    else
#     include <builtin.h>
#    endif
#    undef exception
#  endif
#elif defined (__SUNPRO_CC)
#  include <generic.h>
#  include <string.h>
#else
#  include <string.h>
#endif


extern "C" {
#if HAVE_VALUES_H
#  include <values.h>
#endif

#include <assert.h>
#include <errno.h>
}

#include <iostream>
#if !(defined(__GNUG__)||defined(__SUNPRO_CC))
   extern "C" void exit(int);
#endif

// Forward Declarations

class BACKEND_IMPEXP RigException
{
public:
  const char *message;
  int errorno;

  RigException (const char* msg, int err)
    : message(msg), errorno (err)
    {};

  RigException (int err)
    : message(rigerror(err)), errorno (err)
    {};

  RigException (const char* msg)
    : message(msg), errorno (-RIG_EINTERNAL)
    {};

  virtual ~RigException()
    {};

  void print() const {
	  std::cerr << "Rig exception: " << message << std::endl;
  }
  virtual const char *classname() const {
    return "Rig";
  }
};

inline void THROW(const RigException *e) {
#if defined(__GNUG__)
#  if ((__GNUG__ <= 2) && (__GNUC_MINOR__ < 8))
      (*lib_error_handler)(e?e->classname():"",e?e->message:"");
#else
      throw *e;
#endif
#elif defined(__SUNPRO_CC)
  genericerror(1, ((e != 0) ? (char *)(e->message) : ""));
#else
  if (e)
    std::cerr << e->message << endl;
  exit(0);
#endif
}

#define THROWS(s)


#endif	// _RIGCLASS_H
