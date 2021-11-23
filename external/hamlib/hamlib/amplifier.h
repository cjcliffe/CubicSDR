/*
 *  Hamlib Interface - Amplifier API header
 *  Copyright (c) 2000-2005 by Stephane Fillod
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

#ifndef _AMPLIFIER_H
#define _AMPLIFIER_H 1

#include <hamlib/rig.h>
#include <hamlib/amplist.h>

/**
 * \addtogroup amplifier
 * @{
 */

/**
 * \brief Hamlib amplifier data structures.
 *
 * \file amplifier.h
 *
 * This file contains the data structures and declarations for the Hamlib
 * amplifier Application Programming Interface (API).
 *
 * See the amplifier.c file for details on the amplifier API functions.
 */



__BEGIN_DECLS

/* Forward struct references */

struct amp;
struct amp_state;


/**
 * \brief Main amplifier handle type definition.
 *
 * \typedef typedef struct amp AMP
 *
 * The #AMP handle is returned by amp_init() and is passed as a parameter to
 * every amplifier specific API call.
 *
 * amp_cleanup() must be called when this handle is no longer needed.
 */
typedef struct amp AMP;


/**
 * \brief Type definition for
 * <a href="https://en.wikipedia.org/wiki/Standing_wave_ratio" >SWR (Standing Wave Ratio)</a>.
 *
 * \typedef typedef float swr_t
 *
 * The \a swr_t type is used as a parameter for the amp_get_swr() function.
 *
 * The unit of \a swr_t is 1.0 to the maximum value reported by the amplifier's
 * internal antenna system tuner, i.e.
 * <a href="http://www.arrl.org/transmatch-antenna-tuner" >transmatch</a>,
 * representing the ratio of 1.0:1 to Maximum:1.
 */
typedef float swr_t;


/**
 * \brief Type definition for the
 * <a href="http://www.arrl.org/transmatch-antenna-tuner" >transmatch</a>
 * tuning values of
 * <a href="https://en.wikipedia.org/wiki/Capacitance" >capacitance</a>
 * and
 * <a href="https://en.wikipedia.org/wiki/Inductance" >inductance</a>.
 *
 * \typedef typedef float tune_value_t
 *
 * The \a tune_value_t type is used as a parameter for amp_get_level().
 *
 * The unit of \a tune_value_t is
 * <a href="https://en.wikipedia.org/wiki/Farad" >picoFarads (pF)</a>
 * or
 * <a href="https://en.wikipedia.org/wiki/Henry_(unit)" >nanoHenrys (nH)</a>.
 */
typedef int tune_value_t;


/**
 * \brief The token in the netampctl protocol for returning an error condition code.
 */
#define NETAMPCTL_RET "RPRT "


//! @cond Doxygen_Suppress
typedef enum
{
  AMP_RESET_MEM,    // erase tuner memory
  AMP_RESET_FAULT,  // reset any fault
  AMP_RESET_AMP     // for kpa1500
} amp_reset_t;
//! @endcond

/**
 * \brief Amplifier type flags
 */
typedef enum
{
  AMP_FLAG_1 = (1 << 1),      /*!< TBD */
  AMP_FLAG_2 = (1 << 2)       /*!< TBD */
} amp_type_t;

//! @cond Doxygen_Suppress
// TBD AMP_TYPE
#define AMP_TYPE_MASK  (AMP_FLAG_1|AMP_FLAG_2)

#define AMP_TYPE_OTHER 0
#define AMP_TYPE_1     AMP_FLAG_1
#define AMP_TYPE_2     AMP_FLAG_2
#define AMP_TYPE_ALL   (AMP_FLAG_1|AMP_FLAG_2)
//! @endcond


//! @cond Doxygen_Suppress
enum amp_level_e
{
  AMP_LEVEL_NONE          = 0,        /*!< '' -- No Level. */
  AMP_LEVEL_SWR           = (1 << 0), /*!< \c SWR 1.0 or greater. */
  AMP_LEVEL_NH            = (1 << 1), /*!< \c Tune setting in nanohenries. */
  AMP_LEVEL_PF            = (1 << 2), /*!< \c Tune setting in picofarads. */
  AMP_LEVEL_PWR_INPUT     = (1 << 3), /*!< \c Power reading from amplifier. */
  AMP_LEVEL_PWR_FWD       = (1 << 4), /*!< \c Power reading forward. */
  AMP_LEVEL_PWR_REFLECTED = (1 << 5), /*!< \c Power reading reverse. */
  AMP_LEVEL_PWR_PEAK      = (1 << 6), /*!< \c Power reading peak. */
  AMP_LEVEL_FAULT         = (1 << 7)  /*!< \c Fault code. */
};
//! @endcond

//! @cond Doxygen_Suppress
#define AMP_LEVEL_FLOAT_LIST  (AMP_LEVEL_SWR)
#define AMP_LEVEL_STRING_LIST  (AMP_LEVEL_FAULT)
#define AMP_LEVEL_IS_FLOAT(l) ((l)&AMP_LEVEL_FLOAT_LIST)
#define AMP_LEVEL_IS_STRING(l) ((l)&AMP_LEVEL_STRING_LIST)
//! @endcond

/* Basic amp type, can store some useful info about different amplifiers. Each
 * lib must be able to populate this structure, so we can make useful
 * enquiries about capabilities.
 */

//! @cond Doxygen_Suppress
#define AMP_MODEL(arg) .amp_model=arg,.macro_name=#arg
//! @endcond

/**
 * \brief Amplifier capabilities.
 *
 * \struct amp_caps
 *
 * The main idea of this struct is that it will be defined by the backend
 * amplifier driver and will remain read-only for the application.  Fields
 * that need to be modifiable by the application are copied into the
 * amp_state structure, which is the private memory area of the #AMP instance.
 *
 * This way you can have several amplifiers running within the same
 * application, sharing the amp_caps structure of the backend, while keeping
 * their own customized data.
 *
 * \b Note: Don't move fields around and only add new fields at the end of the
 * amp_caps structure.  Shared libraries and DLLs depend on a constant
 * structure to maintain compatibility.
 */
struct amp_caps
{
  amp_model_t amp_model;                      /*!< Amplifier model as defined in amplist.h. */
  const char *model_name;                     /*!< Model name, e.g. MM-5k. */
  const char *mfg_name;                       /*!< Manufacturer, e.g. Moonbeam. */
  const char *version;                        /*!< Driver version, typically in YYYYMMDD.x format. */
  const char *copyright;                      /*!< Copyright info (should be LGPL). */
  enum rig_status_e status;                   /*!< Driver status. */

  int amp_type;                               /*!< Amplifier type. */
  enum rig_port_e port_type;                  /*!< Type of communication port (serial, ethernet, etc.). */

  int serial_rate_min;                        /*!< Minimal serial speed. */
  int serial_rate_max;                        /*!< Maximal serial speed. */
  int serial_data_bits;                       /*!< Number of data bits. */
  int serial_stop_bits;                       /*!< Number of stop bits. */
  enum serial_parity_e serial_parity;         /*!< Parity. */
  enum serial_handshake_e serial_handshake;   /*!< Handshake. */

  int write_delay;                            /*!< Write delay. */
  int post_write_delay;                       /*!< Post-write delay. */
  int timeout;                                /*!< Timeout. */
  int retry;                                  /*!< Number of retries if a command fails. */

  const struct confparams *cfgparams;         /*!< Configuration parameters. */
  const rig_ptr_t priv;                       /*!< Private data. */
  const char *amp_model_macro_name;           /*!< Model macro name. */

  setting_t has_get_level;                    /*!< List of get levels. */
  setting_t has_set_level;                    /*!< List of set levels.  */

  gran_t level_gran[RIG_SETTING_MAX];         /*!< Level granularity. */
  gran_t parm_gran[RIG_SETTING_MAX];          /*!< Parameter granularity. */

  /*
   * Amp Admin API
   *
   */

  int (*amp_init)(AMP *amp);    /*!< Pointer to backend implementation of ::amp_init(). */
  int (*amp_cleanup)(AMP *amp); /*!< Pointer to backend implementation of ::amp_cleanup(). */
  int (*amp_open)(AMP *amp);    /*!< Pointer to backend implementation of ::amp_open(). */
  int (*amp_close)(AMP *amp);   /*!< Pointer to backend implementation of ::amp_close(). */

  int (*set_freq)(AMP *amp, freq_t val);  /*!< Pointer to backend implementation of ::amp_set_freq(). */
  int (*get_freq)(AMP *amp, freq_t *val); /*!< Pointer to backend implementation of ::amp_get_freq(). */

  int (*set_conf)(AMP *amp, token_t token, const char *val); /*!< Pointer to backend implementation of ::amp_set_conf(). */
  int (*get_conf)(AMP *amp, token_t token, char *val);       /*!< Pointer to backend implementation of ::amp_get_conf(). */

  /*
   *  General API commands, from most primitive to least.. :()
   *  List Set/Get functions pairs
   */

  int (*reset)(AMP *amp, amp_reset_t reset);                   /*!< Pointer to backend implementation of ::amp_reset(). */
  int (*get_level)(AMP *amp, setting_t level, value_t *val);   /*!< Pointer to backend implementation of ::amp_get_level(). */
  int (*get_ext_level)(AMP *amp, token_t level, value_t *val); /*!< Pointer to backend implementation of ::amp_get_ext_level(). */
  int (*set_powerstat)(AMP *amp, powerstat_t status);          /*!< Pointer to backend implementation of ::amp_set_powerstat(). */
  int (*get_powerstat)(AMP *amp, powerstat_t *status);         /*!< Pointer to backend implementation of ::amp_get_powerstat(). */


  /* get firmware info, etc. */
  const char *(*get_info)(AMP *amp); /*!< Pointer to backend implementation of ::amp_get_info(). */

//! @cond Doxygen_Suppress
  setting_t levels;
  unsigned ext_levels;
//! @endcond
  const struct confparams *extlevels;         /*!< Extension levels list.  \sa extamp.c */
  const struct confparams *extparms;          /*!< Extension parameters list.  \sa extamp.c */

  const char *macro_name;                     /*!< Amplifier model macro name. */
};


/**
 * \brief Amplifier state structure.
 *
 * \struct amp_state
 *
 * This structure contains live data, as well as a copy of capability fields
 * that may be updated, i.e. customized while the #AMP handle is instantiated.
 *
 * It is fine to move fields around, as this kind of struct should not be
 * initialized like amp_caps are.
 */
struct amp_state
{
  /*
   * overridable fields
   */

  /*
   * non overridable fields, internal use
   */
  hamlib_port_t ampport;  /*!< Amplifier port (internal use). */

  int comm_state;         /*!< Comm port state, opened/closed. */
  rig_ptr_t priv;         /*!< Pointer to private amplifier state data. */
  rig_ptr_t obj;          /*!< Internal use by hamlib++ for event handling. */

  setting_t has_get_level; /*!< List of get levels. */

  gran_t level_gran[RIG_SETTING_MAX]; /*!< Level granularity. */
  gran_t parm_gran[RIG_SETTING_MAX];  /*!< Parameter granularity. */
};


/**
 * \brief Master amplifier structure.
 *
 * \struct amp
 *
 * Master amplifier data structure acting as the #AMP handle for the
 * controlled amplifier.  A pointer to this structure is returned by the
 * amp_init() API function and is passed as a parameter to every amplifier
 * specific API call.
 *
 * \sa amp_init(), amp_caps, amp_state
 */
struct amp
{
  struct amp_caps *caps;      /*!< Amplifier caps. */
  struct amp_state state;     /*!< Amplifier state. */
};


//! @cond Doxygen_Suppress
/* --------------- API function prototypes -----------------*/

extern HAMLIB_EXPORT(AMP *)
amp_init HAMLIB_PARAMS((amp_model_t amp_model));

extern HAMLIB_EXPORT(int)
amp_open HAMLIB_PARAMS((AMP *amp));

extern HAMLIB_EXPORT(int)
amp_close HAMLIB_PARAMS((AMP *amp));

extern HAMLIB_EXPORT(int)
amp_cleanup HAMLIB_PARAMS((AMP *amp));

extern HAMLIB_EXPORT(int)
amp_set_conf HAMLIB_PARAMS((AMP *amp,
                            token_t token,
                            const char *val));
extern HAMLIB_EXPORT(int)
amp_get_conf HAMLIB_PARAMS((AMP *amp,
                            token_t token,
                            char *val));
extern HAMLIB_EXPORT(int)
amp_set_powerstat HAMLIB_PARAMS((AMP *amp,
                                 powerstat_t status));
extern HAMLIB_EXPORT(int)
amp_get_powerstat HAMLIB_PARAMS((AMP *amp,
                                 powerstat_t *status));


/*
 *  General API commands, from most primitive to least.. )
 *  List Set/Get functions pairs
 */
extern HAMLIB_EXPORT(int)
amp_get_freq HAMLIB_PARAMS((AMP *amp,
                            freq_t *freq));
extern HAMLIB_EXPORT(int)
amp_set_freq HAMLIB_PARAMS((AMP *amp,
                            freq_t freq));

extern HAMLIB_EXPORT(int)
amp_reset HAMLIB_PARAMS((AMP *amp,
                         amp_reset_t reset));

extern HAMLIB_EXPORT(const char *)
amp_get_info HAMLIB_PARAMS((AMP *amp));

extern HAMLIB_EXPORT(int)
amp_get_level HAMLIB_PARAMS((AMP *amp, setting_t level, value_t *val));

extern HAMLIB_EXPORT(int)
amp_register HAMLIB_PARAMS((const struct amp_caps *caps));

extern HAMLIB_EXPORT(int)
amp_unregister HAMLIB_PARAMS((amp_model_t amp_model));

extern HAMLIB_EXPORT(int)
amp_list_foreach HAMLIB_PARAMS((int (*cfunc)(const struct amp_caps *,
                                rig_ptr_t),
                                rig_ptr_t data));

extern HAMLIB_EXPORT(int)
amp_load_backend HAMLIB_PARAMS((const char *be_name));

extern HAMLIB_EXPORT(int)
amp_check_backend HAMLIB_PARAMS((amp_model_t amp_model));

extern HAMLIB_EXPORT(int)
amp_load_all_backends HAMLIB_PARAMS((void));

extern HAMLIB_EXPORT(amp_model_t)
amp_probe_all HAMLIB_PARAMS((hamlib_port_t *p));

extern HAMLIB_EXPORT(int)
amp_token_foreach HAMLIB_PARAMS((AMP *amp,
                                 int (*cfunc)(const struct confparams *,
                                     rig_ptr_t),
                                 rig_ptr_t data));

extern HAMLIB_EXPORT(const struct confparams *)
amp_confparam_lookup HAMLIB_PARAMS((AMP *amp,
                                    const char *name));

extern HAMLIB_EXPORT(token_t)
amp_token_lookup HAMLIB_PARAMS((AMP *amp,
                                const char *name));

extern HAMLIB_EXPORT(const struct amp_caps *)
amp_get_caps HAMLIB_PARAMS((amp_model_t amp_model));

extern HAMLIB_EXPORT(setting_t)
amp_has_get_level HAMLIB_PARAMS((AMP *amp,
                                 setting_t level));

extern HAMLIB_EXPORT(const struct confparams *)
amp_ext_lookup HAMLIB_PARAMS((AMP *amp,
                              const char *name));

extern HAMLIB_EXPORT(int)
amp_get_ext_level HAMLIB_PARAMS((AMP *amp,
                                 token_t token,
                                 value_t *val));

extern HAMLIB_EXPORT(const char *) amp_strlevel(setting_t);

extern HAMLIB_EXPORT(const struct confparams *)
rig_ext_lookup HAMLIB_PARAMS((RIG *rig,
                              const char *name));

extern HAMLIB_EXPORT(setting_t) amp_parse_level(const char *s);
extern HAMLIB_EXPORT(const char *) amp_strlevel(setting_t);

//! @endcond


/**
 * \brief Convenience macro for generating debugging messages.
 *
 * \def amp_debug
 *
 * This is an alias of the rig_debug() function call and is used in the same
 * manner.
 */
#define amp_debug rig_debug

__END_DECLS

#endif /* _AMPLIFIER_H */

/** @} */
