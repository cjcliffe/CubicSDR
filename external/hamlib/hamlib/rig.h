/*
 *  Hamlib Interface - API header
 *  Copyright (c) 2000-2003 by Frank Singleton
 *  Copyright (c) 2000-2012 by Stephane Fillod
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


#ifndef _RIG_H
#define _RIG_H 1

#include <stdio.h>
#include <stdarg.h>

#include <hamlib/riglist.h>	/* list in another file to not mess up w/ this one */

/**
 * \addtogroup rig
 * @{
 */

/*! \file rig.h
 *  \brief Hamlib rig data structures.
 *
 *  This file contains the data structures and definitions for the Hamlib rig API.
 *  see the rig.c file for more details on the rig API.
 */


/* __BEGIN_DECLS should be used at the beginning of your declarations,
 * so that C++ compilers don't mangle their names.  Use __END_DECLS at
 * the end of C declarations. */
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
# define __BEGIN_DECLS extern "C" {
# define __END_DECLS }
#else
# define __BEGIN_DECLS		/* empty */
# define __END_DECLS		/* empty */
#endif

/* HAMLIB_PARAMS is a macro used to wrap function prototypes, so that compilers
 * that don't understand ANSI C prototypes still work, and ANSI C
 * compilers can issue warnings about type mismatches. */
#undef HAMLIB_PARAMS
#if defined (__STDC__) || defined (_AIX) || (defined (__mips) && defined (_SYSTYPE_SVR4)) || defined(__CYGWIN__) || defined(_WIN32) || defined(__cplusplus)
# define HAMLIB_PARAMS(protos) protos
# define rig_ptr_t     void*
#else
# define HAMLIB_PARAMS(protos) ()
# define rig_ptr_t     char*
#endif

#include <hamlib/rig_dll.h>


__BEGIN_DECLS

extern HAMLIB_EXPORT_VAR(const char) hamlib_version[];
extern HAMLIB_EXPORT_VAR(const char) hamlib_copyright[];
extern HAMLIB_EXPORT_VAR(const char *) hamlib_version2;
extern HAMLIB_EXPORT_VAR(const char *) hamlib_copyright2;

/**
 * \brief Hamlib error codes
 * Error code definition that can be returned by the Hamlib functions.
 * Unless stated otherwise, Hamlib functions return the negative value
 * of rig_errcode_e definitions in case of error, or 0 when successful.
 */
enum rig_errcode_e {
	RIG_OK=0,		/*!< No error, operation completed successfully */
	RIG_EINVAL,		/*!< invalid parameter */
	RIG_ECONF,		/*!< invalid configuration (serial,..) */
	RIG_ENOMEM,		/*!< memory shortage */
	RIG_ENIMPL,		/*!< function not implemented, but will be */
	RIG_ETIMEOUT,		/*!< communication timed out */
	RIG_EIO,		/*!< IO error, including open failed */
	RIG_EINTERNAL,		/*!< Internal Hamlib error, huh! */
	RIG_EPROTO,		/*!< Protocol error */
	RIG_ERJCTED,		/*!< Command rejected by the rig */
	RIG_ETRUNC,		/*!< Command performed, but arg truncated */
	RIG_ENAVAIL,		/*!< function not available */
	RIG_ENTARGET,		/*!< VFO not targetable */
	RIG_BUSERROR,		/*!< Error talking on the bus */
	RIG_BUSBUSY,		/*!< Collision on the bus */
	RIG_EARG,		/*!< NULL RIG handle or any invalid pointer parameter in get arg */
	RIG_EVFO,		/*!< Invalid VFO */
	RIG_EDOM		/*!< Argument out of domain of func */
};

/** \brief Token in the netrigctl protocol for returning error code */
#define NETRIGCTL_RET "RPRT "

/**
 *\brief Hamlib debug levels
 *
 * REM: Numeric order matters for debug level
 *
 * \sa rig_set_debug
 */
enum rig_debug_level_e {
  RIG_DEBUG_NONE = 0,		/*!< no bug reporting */
  RIG_DEBUG_BUG,		/*!< serious bug */
  RIG_DEBUG_ERR,		/*!< error case (e.g. protocol, memory allocation) */
  RIG_DEBUG_WARN,		/*!< warning */
  RIG_DEBUG_VERBOSE,		/*!< verbose */
  RIG_DEBUG_TRACE		/*!< tracing */
};

/* --------------- Rig capabilities -----------------*/

/* Forward struct references */

struct rig;
struct rig_state;

/*!
 * \brief Rig structure definition (see rig for details).
 */
typedef struct rig RIG;

#define RIGNAMSIZ 30
#define RIGVERSIZ 8
#define FILPATHLEN 100
#define FRQRANGESIZ 30
#define MAXCHANDESC 30		/* describe channel eg: "WWV 5Mhz" */
#define TSLSTSIZ 20		/* max tuning step list size, zero ended */
#define FLTLSTSIZ 60		/* max mode/filter list size, zero ended */
#define MAXDBLSTSIZ 8		/* max preamp/att levels supported, zero ended */
#define CHANLSTSIZ 16		/* max mem_list size, zero ended */
#define MAX_CAL_LENGTH 32	/* max calibration plots in cal_table_t */


/**
 * \brief CTCSS and DCS type definition.
 *
 * Continuous Tone Controlled Squelch System (CTCSS)
 * sub-audible tone frequency are expressed in \em tenth of Hz.
 * For example, the subaudible tone of 88.5 Hz is represented within
 * Hamlib by 885.
 *
 * Digitally-Coded Squelch codes are simple direct integers.
 */
typedef unsigned int tone_t;

/**
 * \brief Port type
 */
typedef enum rig_port_e {
  RIG_PORT_NONE = 0,		/*!< No port */
  RIG_PORT_SERIAL,		/*!< Serial */
  RIG_PORT_NETWORK,		/*!< Network socket type */
  RIG_PORT_DEVICE,		/*!< Device driver, like the WiNRADiO */
  RIG_PORT_PACKET,		/*!< AX.25 network type, e.g. SV8CS protocol */
  RIG_PORT_DTMF,		/*!< DTMF protocol bridge via another rig, eg. Kenwood Sky Cmd System */
  RIG_PORT_ULTRA,		/*!< IrDA Ultra protocol! */
  RIG_PORT_RPC,			/*!< RPC wrapper */
  RIG_PORT_PARALLEL,		/*!< Parallel port */
  RIG_PORT_USB,			/*!< USB port */
  RIG_PORT_UDP_NETWORK,		/*!< UDP Network socket type */
  RIG_PORT_CM108,		/*!< CM108 GPIO */
  RIG_PORT_GPIO,		/*!< GPIO */
  RIG_PORT_GPION,		/*!< GPIO inverted */
} rig_port_t;

/**
 * \brief Serial parity
 */
enum serial_parity_e {
  RIG_PARITY_NONE = 0,		/*!< No parity */
  RIG_PARITY_ODD,		/*!< Odd */
  RIG_PARITY_EVEN,		/*!< Even */
  RIG_PARITY_MARK,		/*!< Mark */
  RIG_PARITY_SPACE		/*!< Space */
};

/**
 * \brief Serial handshake
 */
enum serial_handshake_e {
  RIG_HANDSHAKE_NONE = 0,	/*!< No handshake */
  RIG_HANDSHAKE_XONXOFF,	/*!< Software XON/XOFF */
  RIG_HANDSHAKE_HARDWARE	/*!< Hardware CTS/RTS */
};


/**
 * \brief Serial control state
 */
enum serial_control_state_e {
  RIG_SIGNAL_UNSET = 0,	/*!< Unset or tri-state */
  RIG_SIGNAL_ON,	/*!< ON */
  RIG_SIGNAL_OFF	/*!< OFF */
};

/** \brief Rig type flags */
typedef enum {
	RIG_FLAG_RECEIVER =	(1<<1),		/*!< Receiver */
	RIG_FLAG_TRANSMITTER =	(1<<2),		/*!< Transmitter */
	RIG_FLAG_SCANNER =	(1<<3),		/*!< Scanner */

	RIG_FLAG_MOBILE =	(1<<4),		/*!< mobile sized */
	RIG_FLAG_HANDHELD =	(1<<5),		/*!< handheld sized */
	RIG_FLAG_COMPUTER =	(1<<6),		/*!< "Computer" rig */
	RIG_FLAG_TRUNKING =	(1<<7),		/*!< has trunking */
	RIG_FLAG_APRS =		(1<<8),		/*!< has APRS */
	RIG_FLAG_TNC =		(1<<9),		/*!< has TNC */
	RIG_FLAG_DXCLUSTER =	(1<<10),	/*!< has DXCluster */
	RIG_FLAG_TUNER =	(1<<11) 	/*!< dumb tuner */
} rig_type_t;

#define RIG_FLAG_TRANSCEIVER (RIG_FLAG_RECEIVER|RIG_FLAG_TRANSMITTER)
#define RIG_TYPE_MASK (RIG_FLAG_TRANSCEIVER|RIG_FLAG_SCANNER|RIG_FLAG_MOBILE|RIG_FLAG_HANDHELD|RIG_FLAG_COMPUTER|RIG_FLAG_TRUNKING|RIG_FLAG_TUNER)

#define RIG_TYPE_OTHER		0
#define RIG_TYPE_TRANSCEIVER	RIG_FLAG_TRANSCEIVER
#define RIG_TYPE_HANDHELD	(RIG_FLAG_TRANSCEIVER|RIG_FLAG_HANDHELD)
#define RIG_TYPE_MOBILE		(RIG_FLAG_TRANSCEIVER|RIG_FLAG_MOBILE)
#define RIG_TYPE_RECEIVER	RIG_FLAG_RECEIVER
#define RIG_TYPE_PCRECEIVER	(RIG_FLAG_COMPUTER|RIG_FLAG_RECEIVER)
#define RIG_TYPE_SCANNER	(RIG_FLAG_SCANNER|RIG_FLAG_RECEIVER)
#define RIG_TYPE_TRUNKSCANNER	(RIG_TYPE_SCANNER|RIG_FLAG_TRUNKING)
#define RIG_TYPE_COMPUTER	(RIG_FLAG_TRANSCEIVER|RIG_FLAG_COMPUTER)
#define RIG_TYPE_TUNER		RIG_FLAG_TUNER


/**
 * \brief Development status of the backend
 */
enum rig_status_e {
  RIG_STATUS_ALPHA = 0,		/*!< Alpha quality, i.e. development */
  RIG_STATUS_UNTESTED,		/*!< Written from available specs, rig unavailable for test, feedback wanted! */
  RIG_STATUS_BETA,		/*!< Beta quality */
  RIG_STATUS_STABLE,		/*!< Stable */
  RIG_STATUS_BUGGY		/*!< Was stable, but something broke it! */
/*  RIG_STATUS_NEW	*	*!< Initial release of code
 *  				!! Use of RIG_STATUS_NEW is deprecated. Do not use it anymore */
};

/** \brief Map all deprecated RIG_STATUS_NEW references to RIG_STATUS_UNTESTED for backward compatibility */
#define RIG_STATUS_NEW RIG_STATUS_UNTESTED

/**
 * \brief Repeater shift type
 */
typedef enum {
  RIG_RPT_SHIFT_NONE = 0,	/*!< No repeater shift */
  RIG_RPT_SHIFT_MINUS,		/*!< "-" shift */
  RIG_RPT_SHIFT_PLUS		/*!< "+" shift */
} rptr_shift_t;

/**
 * \brief Split mode
 */
typedef enum {
  RIG_SPLIT_OFF = 0,		/*!< Split mode disabled */
  RIG_SPLIT_ON			/*!< Split mode enabled */
} split_t;

/**
 * \brief Frequency type,
 * Frequency type unit in Hz, able to hold SHF frequencies.
 */
typedef double freq_t;
/** \brief printf(3) format to be used for freq_t type */
#define PRIfreq "f"
/** \brief scanf(3) format to be used for freq_t type */
#define SCNfreq "lf"
#define FREQFMT SCNfreq

/**
 * \brief Short frequency type
 * Frequency in Hz restricted to 31bits, suitable for offsets, shifts, etc..
 */
typedef signed long shortfreq_t;

#define Hz(f)	((freq_t)(f))
#define kHz(f)	((freq_t)((f)*(freq_t)1000))
#define MHz(f)	((freq_t)((f)*(freq_t)1000000))
#define GHz(f)	((freq_t)((f)*(freq_t)1000000000))

#define s_Hz(f) 	((shortfreq_t)(f))
#define s_kHz(f)	((shortfreq_t)((f)*(shortfreq_t)1000))
#define s_MHz(f)	((shortfreq_t)((f)*(shortfreq_t)1000000))
#define s_GHz(f)	((shortfreq_t)((f)*(shortfreq_t)1000000000))

#define RIG_FREQ_NONE Hz(0)


/**
 * \brief VFO definition
 *
 * There are several ways of using a vfo_t. For most cases, using RIG_VFO_A,
 * RIG_VFO_B, RIG_VFO_CURR, etc., as opaque macros should suffice.
 *
 * Strictly speaking a VFO is Variable Frequency Oscillator.
 * Here, it is referred as a tunable channel, from the radio operator's
 * point of view. The channel can be designated individually by its real
 * number, or by using an alias.
 *
 * Aliases may or may not be honored by a backend and are defined using
 * high significant bits, i.e. RIG_VFO_MEM, RIG_VFO_MAIN, etc.
 *
 */
typedef int vfo_t;

/** \brief '' -- used in caps */
#define RIG_VFO_NONE    0

#define RIG_VFO_TX_FLAG    (1<<30)

/** \brief \c currVFO -- current "tunable channel"/VFO */
#define RIG_VFO_CURR    (1<<29)

/** \brief \c MEM -- means Memory mode, to be used with set_vfo */
#define RIG_VFO_MEM	(1<<28)

/** \brief \c VFO -- means (last or any)VFO mode, with set_vfo */
#define RIG_VFO_VFO	(1<<27)

#define RIG_VFO_TX_VFO(v)	((v)|RIG_VFO_TX_FLAG)

/** \brief \c TX -- alias for split tx or uplink, of VFO_CURR  */
#define RIG_VFO_TX	RIG_VFO_TX_VFO(RIG_VFO_CURR)

/** \brief \c RX -- alias for split rx or downlink */
#define RIG_VFO_RX	RIG_VFO_CURR

/** \brief \c Main -- alias for MAIN */
#define RIG_VFO_MAIN	(1<<26)
/** \brief \c Sub -- alias for SUB */
#define RIG_VFO_SUB	(1<<25)

#define RIG_VFO_N(n) (1<<(n))

/** \brief \c VFOA -- VFO A */
#define RIG_VFO_A RIG_VFO_N(0)
/** \brief \c VFOB -- VFO B */
#define RIG_VFO_B RIG_VFO_N(1)
/** \brief \c VFOC -- VFO C */
#define RIG_VFO_C RIG_VFO_N(2)


/*
 * targetable bitfields, for internal use.
 * RIG_TARGETABLE_PURE means a pure targetable radio on every command
 */
#define RIG_TARGETABLE_NONE 0
#define RIG_TARGETABLE_FREQ (1<<0)
#define RIG_TARGETABLE_MODE (1<<1)
#define RIG_TARGETABLE_PURE (1<<2)
#define RIG_TARGETABLE_TONE (1<<3)
#define RIG_TARGETABLE_FUNC (1<<4)
#define RIG_TARGETABLE_ALL  0x7fffffff


#define RIG_PASSBAND_NORMAL s_Hz(0)
#define RIG_PASSBAND_NOCHANGE s_Hz(-1)
/**
 * \brief Passband width, in Hz
 * \sa rig_passband_normal, rig_passband_narrow, rig_passband_wide
 */
typedef shortfreq_t pbwidth_t;


/**
 * \brief DCD status
 */
typedef enum dcd_e {
  RIG_DCD_OFF = 0,		/*!< Squelch closed */
  RIG_DCD_ON			/*!< Squelch open */
} dcd_t;

/**
 * \brief DCD type
 * \sa rig_get_dcd
 */
typedef enum {
  RIG_DCD_NONE = 0,		/*!< No DCD available */
  RIG_DCD_RIG,			/*!< Rig has DCD status support, i.e. rig has get_dcd cap */
  RIG_DCD_SERIAL_DSR,		/*!< DCD status from serial DSR signal */
  RIG_DCD_SERIAL_CTS,		/*!< DCD status from serial CTS signal */
  RIG_DCD_SERIAL_CAR,		/*!< DCD status from serial CD signal */
  RIG_DCD_PARALLEL,		/*!< DCD status from parallel port pin */
  RIG_DCD_CM108			/*!< DCD status from CM108 vol dn pin */
} dcd_type_t;


/**
 * \brief PTT status
 */
typedef enum {
  RIG_PTT_OFF = 0,		/*!< PTT desactivated */
  RIG_PTT_ON,			/*!< PTT activated */
  RIG_PTT_ON_MIC,		/*!< PTT Mic only, fallbacks on RIG_PTT_ON if unavailable */
  RIG_PTT_ON_DATA		/*!< PTT Data (Mic-muted), fallbacks on RIG_PTT_ON if unavailable */
} ptt_t;

/**
 * \brief PTT type
 * \sa rig_get_ptt
 */
typedef enum {
  RIG_PTT_NONE = 0,		/*!< No PTT available */
  RIG_PTT_RIG,			/*!< Legacy PTT (CAT PTT) */
  RIG_PTT_SERIAL_DTR,		/*!< PTT control through serial DTR signal */
  RIG_PTT_SERIAL_RTS,		/*!< PTT control through serial RTS signal */
  RIG_PTT_PARALLEL,		/*!< PTT control through parallel port */
  RIG_PTT_RIG_MICDATA,		/*!< Legacy PTT (CAT PTT), supports RIG_PTT_ON_MIC/RIG_PTT_ON_DATA */
  RIG_PTT_CM108,		/*!< PTT control through CM108 GPIO pin */
  RIG_PTT_GPIO,			/*!< PTT control through GPIO pin */
  RIG_PTT_GPION,		/*!< PTT control through inverted GPIO pin */
} ptt_type_t;

/**
 * \brief Radio power state
 */
typedef enum {
  RIG_POWER_OFF =	0,		/*!< Power off */
  RIG_POWER_ON =	(1<<0),		/*!< Power on */
  RIG_POWER_STANDBY =	(1<<1)		/*!< Standby */
} powerstat_t;

/**
 * \brief Reset operation
 */
typedef enum {
  RIG_RESET_NONE =	 0,		/*!< No reset */
  RIG_RESET_SOFT =	(1<<0),		/*!< Software reset */
  RIG_RESET_VFO =	(1<<1),		/*!< VFO reset */
  RIG_RESET_MCALL =	(1<<2),		/*!< Memory clear */
  RIG_RESET_MASTER =	(1<<3)		/*!< Master reset */
} reset_t;


/**
 * \brief VFO operation
 *
 * A VFO operation is an action on a VFO (or tunable memory).
 * The difference with a function is that an action has no on/off
 * status, it is performed at once.
 *
 * Note: the vfo argument for some vfo operation may be irrelevant,
 * and thus will be ignored.
 *
 * The VFO/MEM "mode" is set by rig_set_vfo.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_vfo_op() rig_strvfop()
 */
typedef enum {
	RIG_OP_NONE =		0,  /*!< '' No VFO_OP */
	RIG_OP_CPY =		(1<<0),	/*!< \c CPY -- VFO A = VFO B */
	RIG_OP_XCHG =		(1<<1),	/*!< \c XCHG -- Exchange VFO A/B */
	RIG_OP_FROM_VFO =	(1<<2),	/*!< \c FROM_VFO -- VFO->MEM */
	RIG_OP_TO_VFO =		(1<<3),	/*!< \c TO_VFO -- MEM->VFO */
	RIG_OP_MCL =		(1<<4),	/*!< \c MCL -- Memory clear */
	RIG_OP_UP =		(1<<5),	/*!< \c UP -- UP increment VFO freq by tuning step*/
	RIG_OP_DOWN =		(1<<6),	/*!< \c DOWN -- DOWN decrement VFO freq by tuning step*/
	RIG_OP_BAND_UP =	(1<<7),	/*!< \c BAND_UP -- Band UP */
	RIG_OP_BAND_DOWN =	(1<<8),	/*!< \c BAND_DOWN -- Band DOWN */
	RIG_OP_LEFT =		(1<<9),	/*!< \c LEFT -- LEFT */
	RIG_OP_RIGHT =		(1<<10),/*!< \c RIGHT -- RIGHT */
	RIG_OP_TUNE =		(1<<11),/*!< \c TUNE -- Start tune */
	RIG_OP_TOGGLE =		(1<<12) /*!< \c TOGGLE -- Toggle VFOA and VFOB */
} vfo_op_t;


/**
 * \brief Rig Scan operation
 *
 * Various scan operations supported by a rig.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_scan() rig_strscan()
 */
typedef enum {
	RIG_SCAN_NONE =		0,  /*!< '' No-op value */
	RIG_SCAN_MEM =		(1<<0),	/*!< \c MEM -- Scan all memory channels */
	RIG_SCAN_SLCT =		(1<<1),	/*!< \c SLCT -- Scan all selected memory channels */
	RIG_SCAN_PRIO =		(1<<2),	/*!< \c PRIO -- Priority watch (mem or call channel) */
	RIG_SCAN_PROG =		(1<<3),	/*!< \c PROG -- Programmed(edge) scan */
	RIG_SCAN_DELTA =	(1<<4),	/*!< \c DELTA -- delta-f scan */
	RIG_SCAN_VFO =		(1<<5),	/*!< \c VFO -- most basic scan */
	RIG_SCAN_PLT =		(1<<6), /*!< \c PLT -- Scan using pipelined tuning */
	RIG_SCAN_STOP =		(1<<7)  /*!< \c STOP -- Stop scanning */
} scan_t;

/**
 * \brief configuration token
 */
typedef long token_t;

#define RIG_CONF_END 0

/**
 * \brief parameter types
 *
 *   Used with configuration, parameter and extra-parm tables.
 *
 *   Current internal implementation
 *   NUMERIC: val.f or val.i
 *   COMBO: val.i, starting from 0.  Points to a table of strings or asci stored values.
 *   STRING: val.s or val.cs
 *   CHECKBUTTON: val.i 0/1
 */

/* strongly inspired from soundmodem. Thanks Thomas! */

enum rig_conf_e {
	RIG_CONF_STRING,	/*!<	String type */
	RIG_CONF_COMBO,		/*!<	Combo type */
	RIG_CONF_NUMERIC,	/*!<	Numeric type integer or real */
	RIG_CONF_CHECKBUTTON,	/*!<	on/off type */
	RIG_CONF_BUTTON		/*!<    Button type */
};

#define RIG_COMBO_MAX	8

/**
 * \brief Configuration parameter structure.
 */
struct confparams {
  token_t token;		/*!< Conf param token ID */
  const char *name;		/*!< Param name, no spaces allowed */
  const char *label;		/*!< Human readable label */
  const char *tooltip;		/*!< Hint on the parameter */
  const char *dflt;		/*!< Default value */
  enum rig_conf_e type;		/*!< Type of the parameter */
  union {			/*!< */
	struct {		/*!< */
		float min;	/*!< Minimum value */
		float max;	/*!< Maximum value */
		float step;	/*!< Step */
	} n;			/*!< Numeric type */
	struct {		/*!< */
		const char *combostr[RIG_COMBO_MAX];	/*!< Combo list */
	} c;			/*!< Combo type */
  } u;				/*!< Type union */
};

/** \brief Announce
 *
 * Designate optional speech synthesizer.
 */
typedef enum {
	RIG_ANN_NONE =	0,		/*!< None */
	RIG_ANN_OFF = 	RIG_ANN_NONE,	/*!< disable announces */
	RIG_ANN_FREQ =	(1<<0),		/*!< Announce frequency */
	RIG_ANN_RXMODE = (1<<1),	/*!< Announce receive mode */
	RIG_ANN_CW = (1<<2),		/*!< CW */
	RIG_ANN_ENG = (1<<3),		/*!< English */
	RIG_ANN_JAP = (1<<4)		/*!< Japan */
} ann_t;


/**
 * \brief Antenna number
 */
typedef int ant_t;

#define RIG_ANT_NONE	0
#define RIG_ANT_N(n)	((ant_t)1<<(n))
#define RIG_ANT_1	RIG_ANT_N(0)
#define RIG_ANT_2	RIG_ANT_N(1)
#define RIG_ANT_3	RIG_ANT_N(2)
#define RIG_ANT_4	RIG_ANT_N(3)
#define RIG_ANT_5	RIG_ANT_N(4)

/**
 * \brief AGC delay settings
 */
/* TODO: kill me, and replace by real AGC delay */
enum agc_level_e {
	RIG_AGC_OFF = 0,
	RIG_AGC_SUPERFAST,
	RIG_AGC_FAST,
	RIG_AGC_SLOW,
	RIG_AGC_USER,		/*!< user selectable */
	RIG_AGC_MEDIUM,
	RIG_AGC_AUTO
};

/**
 * \brief Level display meters
 */
enum meter_level_e {
  RIG_METER_NONE =	0,		/*< No display meter */
  RIG_METER_SWR =	(1<<0),		/*< Stationary Wave Ratio */
  RIG_METER_COMP =	(1<<1),		/*< Compression level */
  RIG_METER_ALC =	(1<<2),		/*< ALC */
  RIG_METER_IC =	(1<<3),		/*< IC */
  RIG_METER_DB =	(1<<4),		/*< DB */
  RIG_METER_PO =	(1<<5),		/*< Power Out */
  RIG_METER_VDD =	(1<<6)		/*< Final Amp Voltage */
};

/**
 * \brief Universal approach for passing values
 * \sa rig_set_level, rig_get_level, rig_set_parm, rig_get_parm
 */
typedef union {
  signed int i;			/*!< Signed integer */
  float f;			/*!< Single precision float */
  char *s;			/*!< Pointer to char string */
  const char *cs;		/*!< Pointer to constant char string */
} value_t;

/** \brief Rig Level Settings
 *
 * Various operating levels supported by a rig.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_level() rig_strlevel()
 */

enum rig_level_e {
	RIG_LEVEL_NONE =	0,	/*!< '' -- No Level */
	RIG_LEVEL_PREAMP =	(1<<0),	/*!< \c PREAMP -- Preamp, arg int (dB) */
	RIG_LEVEL_ATT =		(1<<1),	/*!< \c ATT -- Attenuator, arg int (dB) */
	RIG_LEVEL_VOX =		(1<<2),	/*!< \c VOX -- VOX delay, arg int (tenth of seconds) */
	RIG_LEVEL_AF =		(1<<3),	/*!< \c AF -- Volume, arg float [0.0 ... 1.0] */
	RIG_LEVEL_RF =		(1<<4),	/*!< \c RF -- RF gain (not TX power), arg float [0.0 ... 1.0] */
	RIG_LEVEL_SQL =		(1<<5),	/*!< \c SQL -- Squelch, arg float [0.0 ... 1.0] */
	RIG_LEVEL_IF =		(1<<6),	/*!< \c IF -- IF, arg int (Hz) */
	RIG_LEVEL_APF =		(1<<7),	/*!< \c APF -- Audio Peak Filter, arg float [0.0 ... 1.0] */
	RIG_LEVEL_NR =		(1<<8),	/*!< \c NR -- Noise Reduction, arg float [0.0 ... 1.0] */
	RIG_LEVEL_PBT_IN =	(1<<9),	/*!< \c PBT_IN -- Twin PBT (inside), arg float [0.0 ... 1.0] */
	RIG_LEVEL_PBT_OUT =	(1<<10),/*!< \c PBT_OUT -- Twin PBT (outside), arg float [0.0 ... 1.0] */
	RIG_LEVEL_CWPITCH =	(1<<11),/*!< \c CWPITCH -- CW pitch, arg int (Hz) */
	RIG_LEVEL_RFPOWER =	(1<<12),/*!< \c RFPOWER -- RF Power, arg float [0.0 ... 1.0] */
	RIG_LEVEL_MICGAIN =	(1<<13),/*!< \c MICGAIN -- MIC Gain, arg float [0.0 ... 1.0] */
	RIG_LEVEL_KEYSPD =	(1<<14),/*!< \c KEYSPD -- Key Speed, arg int (WPM) */
	RIG_LEVEL_NOTCHF =	(1<<15),/*!< \c NOTCHF -- Notch Freq., arg int (Hz) */
	RIG_LEVEL_COMP =	(1<<16),/*!< \c COMP -- Compressor, arg float [0.0 ... 1.0] */
	RIG_LEVEL_AGC =		(1<<17),/*!< \c AGC -- AGC, arg int (see enum agc_level_e) */
	RIG_LEVEL_BKINDL =	(1<<18),/*!< \c BKINDL -- BKin Delay, arg int (tenth of dots) */
	RIG_LEVEL_BALANCE =	(1<<19),/*!< \c BAL -- Balance (Dual Watch), arg float [0.0 ... 1.0] */
	RIG_LEVEL_METER =	(1<<20),/*!< \c METER -- Display meter, arg int (see enum meter_level_e) */

	RIG_LEVEL_VOXGAIN =	(1<<21),/*!< \c VOXGAIN -- VOX gain level, arg float [0.0 ... 1.0] */
	RIG_LEVEL_VOXDELAY =  RIG_LEVEL_VOX,	/*!< Synonym of RIG_LEVEL_VOX */
	RIG_LEVEL_ANTIVOX =	(1<<22),/*!< \c ANTIVOX -- anti-VOX level, arg float [0.0 ... 1.0] */
	RIG_LEVEL_SLOPE_LOW =	(1<<23),/*!< \c SLOPE_LOW -- Slope tune, low frequency cut, */
	RIG_LEVEL_SLOPE_HIGH =	(1<<24),/*!< \c SLOPE_HIGH -- Slope tune, high frequency cut, */
	RIG_LEVEL_BKIN_DLYMS =	(1<<25),/*!< \c BKIN_DLYMS -- BKin Delay, arg int Milliseconds */

	/*!< These are not settable */
	RIG_LEVEL_RAWSTR =	(1<<26),/*!< \c RAWSTR -- Raw (A/D) value for signal strength, specific to each rig, arg int */
	RIG_LEVEL_SQLSTAT =	(1<<27),/*!< \c SQLSTAT -- SQL status, arg int (open=1/closed=0). Deprecated, use get_dcd instead */
	RIG_LEVEL_SWR =		(1<<28),/*!< \c SWR -- SWR, arg float [0.0 ... infinite] */
	RIG_LEVEL_ALC =		(1<<29),/*!< \c ALC -- ALC, arg float */
	RIG_LEVEL_STRENGTH =	(1<<30) /*!< \c STRENGTH -- Effective (calibrated) signal strength relative to S9, arg int (dB) */
	/*RIG_LEVEL_BWC =	(1<<31)*/ /*!< Bandwidth Control, arg int (Hz) */
};

#define RIG_LEVEL_FLOAT_LIST (RIG_LEVEL_AF|RIG_LEVEL_RF|RIG_LEVEL_SQL|RIG_LEVEL_APF|RIG_LEVEL_NR|RIG_LEVEL_PBT_IN|RIG_LEVEL_PBT_OUT|RIG_LEVEL_RFPOWER|RIG_LEVEL_MICGAIN|RIG_LEVEL_COMP|RIG_LEVEL_BALANCE|RIG_LEVEL_SWR|RIG_LEVEL_ALC|RIG_LEVEL_VOXGAIN|RIG_LEVEL_ANTIVOX)

#define RIG_LEVEL_READONLY_LIST (RIG_LEVEL_SQLSTAT|RIG_LEVEL_SWR|RIG_LEVEL_ALC|RIG_LEVEL_STRENGTH|RIG_LEVEL_RAWSTR)

#define RIG_LEVEL_IS_FLOAT(l) ((l)&RIG_LEVEL_FLOAT_LIST)
#define RIG_LEVEL_SET(l) ((l)&~RIG_LEVEL_READONLY_LIST)


/**
 * \brief Rig Parameters
 *
 * Parameters are settings that are not VFO specific.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_parm() rig_strparm()
 */
enum rig_parm_e {
	RIG_PARM_NONE =		0,	/*!< '' -- No Parm */
	RIG_PARM_ANN =		(1<<0),	/*!< \c ANN -- "Announce" level, see ann_t */
	RIG_PARM_APO =		(1<<1),	/*!< \c APO -- Auto power off, int in minute */
	RIG_PARM_BACKLIGHT =	(1<<2),	/*!< \c BACKLIGHT -- LCD light, float [0.0 ... 1.0] */
	RIG_PARM_BEEP =		(1<<4),	/*!< \c BEEP -- Beep on keypressed, int (0,1) */
	RIG_PARM_TIME =		(1<<5),	/*!< \c TIME -- hh:mm:ss, int in seconds from 00:00:00 */
	RIG_PARM_BAT =		(1<<6),	/*!< \c BAT -- battery level, float [0.0 ... 1.0] */
	RIG_PARM_KEYLIGHT =	(1<<7)  /*!< \c KEYLIGHT -- Button backlight, on/off */
};

#define RIG_PARM_FLOAT_LIST (RIG_PARM_BACKLIGHT|RIG_PARM_BAT)
#define RIG_PARM_READONLY_LIST (RIG_PARM_BAT)

#define RIG_PARM_IS_FLOAT(l) ((l)&RIG_PARM_FLOAT_LIST)
#define RIG_PARM_SET(l) ((l)&~RIG_PARM_READONLY_LIST)

#define RIG_SETTING_MAX 32
/**
 * \brief Setting
 *
 * This can be a func, a level or a parm.
 * Each bit designates one of them.
 */
typedef unsigned long setting_t;

/*
 * tranceive mode, ie. the rig notify the host of any event,
 * like freq changed, mode changed, etc.
 */
#define	RIG_TRN_OFF 0
#define	RIG_TRN_RIG 1
#define	RIG_TRN_POLL 2


/**
 * \brief Rig Function Settings
 *
 * Various operating functions supported by a rig.\n
 * \c STRING used in rigctl/rigctld
 *
 * \sa rig_parse_func() rig_strfunc()
 */
/*
 * The C standard dictates that an enum constant is a 32 bit signed integer.
 * Setting a constant's bit 31 created a negative value that on amd64 had the
 * upper 32 bits set as well when assigned to the misc.c:func_str structure.
 * This caused misc.c:rig_strfunc() to fail its comparison for RIG_FUNC_XIT
 * on amd64 (x86_64).  To use bit 31 as an unsigned long, preprocessor macros
 * have been used instead as a 'const unsigned long' which cannot be used to
 * initialize the func_str.func members.  TNX KA6MAL, AC6SL.  - N0NB
 */
#define RIG_FUNC_NONE    0         /*!< '' -- No Function */
#define RIG_FUNC_FAGC    (1UL<<0)  /*!< \c FAGC -- Fast AGC */
#define RIG_FUNC_NB      (1UL<<1)  /*!< \c NB -- Noise Blanker */
#define RIG_FUNC_COMP    (1UL<<2)  /*!< \c COMP -- Speech Compression */
#define RIG_FUNC_VOX     (1UL<<3)  /*!< \c VOX -- Voice Operated Relay */
#define RIG_FUNC_TONE    (1UL<<4)  /*!< \c TONE -- CTCSS Tone */
#define RIG_FUNC_TSQL    (1UL<<5)  /*!< \c TSQL -- CTCSS Activate/De-activate */
#define RIG_FUNC_SBKIN   (1UL<<6)  /*!< \c SBKIN -- Semi Break-in (CW mode) */
#define RIG_FUNC_FBKIN   (1UL<<7)  /*!< \c FBKIN -- Full Break-in (CW mode) */
#define RIG_FUNC_ANF     (1UL<<8)  /*!< \c ANF -- Automatic Notch Filter (DSP) */
#define RIG_FUNC_NR      (1UL<<9)  /*!< \c NR -- Noise Reduction (DSP) */
#define RIG_FUNC_AIP     (1UL<<10) /*!< \c AIP -- RF pre-amp (AIP on Kenwood, IPO on Yaesu, etc.) */
#define RIG_FUNC_APF     (1UL<<11) /*!< \c APF -- Auto Passband/Audio Peak Filter */
#define RIG_FUNC_MON     (1UL<<12) /*!< \c MON -- Monitor transmitted signal */
#define RIG_FUNC_MN      (1UL<<13) /*!< \c MN -- Manual Notch */
#define RIG_FUNC_RF      (1UL<<14) /*!< \c RF -- RTTY Filter */
#define RIG_FUNC_ARO     (1UL<<15) /*!< \c ARO -- Auto Repeater Offset */
#define RIG_FUNC_LOCK    (1UL<<16) /*!< \c LOCK -- Lock */
#define RIG_FUNC_MUTE    (1UL<<17) /*!< \c MUTE -- Mute */
#define RIG_FUNC_VSC     (1UL<<18) /*!< \c VSC -- Voice Scan Control */
#define RIG_FUNC_REV     (1UL<<19) /*!< \c REV -- Reverse transmit and receive frequencies */
#define RIG_FUNC_SQL     (1UL<<20) /*!< \c SQL -- Turn Squelch Monitor on/off */
#define RIG_FUNC_ABM     (1UL<<21) /*!< \c ABM -- Auto Band Mode */
#define RIG_FUNC_BC      (1UL<<22) /*!< \c BC -- Beat Canceller */
#define RIG_FUNC_MBC     (1UL<<23) /*!< \c MBC -- Manual Beat Canceller */
#define RIG_FUNC_RIT     (1UL<<24) /*!< \c RIT -- Receiver Incremental Tuning */
#define RIG_FUNC_AFC     (1UL<<25) /*!< \c AFC -- Auto Frequency Control ON/OFF */
#define RIG_FUNC_SATMODE (1UL<<26) /*!< \c SATMODE -- Satellite mode ON/OFF */
#define RIG_FUNC_SCOPE   (1UL<<27) /*!< \c SCOPE -- Simple bandscope ON/OFF */
#define RIG_FUNC_RESUME  (1UL<<28) /*!< \c RESUME -- Scan auto-resume */
#define RIG_FUNC_TBURST  (1UL<<29) /*!< \c TBURST -- 1750 Hz tone burst */
#define RIG_FUNC_TUNER   (1UL<<30) /*!< \c TUNER -- Enable automatic tuner */
#define RIG_FUNC_XIT     (1UL<<31) /*!< \c XIT -- Transmitter Incremental Tuning */


/*
 * power unit macros, converts to mW
 * This is limited to 2MW on 32 bits systems.
 */
#define mW(p)	 ((int)(p))
#define Watts(p) ((int)((p)*1000))
#define W(p)	 Watts(p)
#define kW(p)	 ((int)((p)*1000000L))

/**
 * \brief Radio mode
 *
 * Various modes supported by a rig.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_mode() rig_strrmode()
 */
typedef enum {
	RIG_MODE_NONE =  	0,	/*!< '' -- None */
	RIG_MODE_AM =    	(1<<0),	/*!< \c AM -- Amplitude Modulation */
	RIG_MODE_CW =    	(1<<1),	/*!< \c CW -- CW "normal" sideband */
	RIG_MODE_USB =		(1<<2),	/*!< \c USB -- Upper Side Band */
	RIG_MODE_LSB =		(1<<3),	/*!< \c LSB -- Lower Side Band */
	RIG_MODE_RTTY =		(1<<4),	/*!< \c RTTY -- Radio Teletype */
	RIG_MODE_FM =    	(1<<5),	/*!< \c FM -- "narrow" band FM */
	RIG_MODE_WFM =   	(1<<6),	/*!< \c WFM -- broadcast wide FM */
	RIG_MODE_CWR =   	(1<<7),	/*!< \c CWR -- CW "reverse" sideband */
	RIG_MODE_RTTYR =	(1<<8),	/*!< \c RTTYR -- RTTY "reverse" sideband */
	RIG_MODE_AMS =    	(1<<9),	/*!< \c AMS -- Amplitude Modulation Synchronous */
	RIG_MODE_PKTLSB =       (1<<10),/*!< \c PKTLSB -- Packet/Digital LSB mode (dedicated port) */
	RIG_MODE_PKTUSB =       (1<<11),/*!< \c PKTUSB -- Packet/Digital USB mode (dedicated port) */
	RIG_MODE_PKTFM =        (1<<12),/*!< \c PKTFM -- Packet/Digital FM mode (dedicated port) */
	RIG_MODE_ECSSUSB =      (1<<13),/*!< \c ECSSUSB -- Exalted Carrier Single Sideband USB */
	RIG_MODE_ECSSLSB =      (1<<14),/*!< \c ECSSLSB -- Exalted Carrier Single Sideband LSB */
	RIG_MODE_FAX =          (1<<15),/*!< \c FAX -- Facsimile Mode */
	RIG_MODE_SAM =          (1<<16),/*!< \c SAM -- Synchronous AM double sideband */
	RIG_MODE_SAL =          (1<<17),/*!< \c SAL -- Synchronous AM lower sideband */
	RIG_MODE_SAH =          (1<<18),/*!< \c SAH -- Synchronous AM upper (higher) sideband */
	RIG_MODE_DSB =		(1<<19),/*!< \c DSB -- Double sideband suppressed carrier */
	RIG_MODE_FMN =		(1<<21),/*!< \c FMN -- FM Narrow Kenwood ts990s */
	RIG_MODE_TESTS_MAX              /*!< \c MUST ALWAYS BE LAST, Max Count for dumpcaps.c */
} rmode_t;


/** \brief macro for backends, not to be used by rig_set_mode et al. */
#define RIG_MODE_SSB  	(RIG_MODE_USB|RIG_MODE_LSB)

/** \brief macro for backends, not to be used by rig_set_mode et al. */
#define RIG_MODE_ECSS   (RIG_MODE_ECSSUSB|RIG_MODE_ECSSLSB)


#define RIG_DBLST_END 0		/* end marker in a preamp/att level list */
#define RIG_IS_DBLST_END(d) ((d)==0)

/**
 * \brief Frequency range
 *
 * Put together a group of this struct in an array to define
 * what frequencies your rig has access to.
 */
typedef struct freq_range_list {
  freq_t start;		/*!< Start frequency */
  freq_t end;		/*!< End frequency */
  rmode_t modes;	/*!< Bit field of RIG_MODE's */
  int low_power;	/*!< Lower RF power in mW, -1 for no power (ie. rx list) */
  int high_power;	/*!< Higher RF power in mW, -1 for no power (ie. rx list) */
  vfo_t vfo;		/*!< VFO list equipped with this range */
  ant_t ant;		/*!< Antenna list equipped with this range, 0 means all */
} freq_range_t;

#define RIG_FRNG_END     {Hz(0),Hz(0),RIG_MODE_NONE,0,0,RIG_VFO_NONE}
#define RIG_IS_FRNG_END(r) ((r).start == Hz(0) && (r).end == Hz(0))

#define RIG_ITU_REGION1 1
#define RIG_ITU_REGION2 2
#define RIG_ITU_REGION3 3

/**
 * \brief Tuning step definition
 *
 * Lists the tuning steps available for each mode.
 *
 * If a ts field in the list has RIG_TS_ANY value,
 * this means the rig allows its tuning step to be
 * set to any value ranging from the lowest to the
 * highest (if any) value in the list for that mode.
 * The tuning step must be sorted in the ascending
 * order, and the RIG_TS_ANY value, if present, must
 * be the last one in the list.
 *
 * Note also that the minimum frequency resolution
 * of the rig is determined by the lowest value
 * in the Tuning step list.
 *
 * \sa rig_set_ts, rig_get_resolution
 */
struct tuning_step_list {
  rmode_t modes;	/*!< Bit field of RIG_MODE's */
  shortfreq_t ts;	/*!< Tuning step in Hz */
};

#define RIG_TS_ANY     0
#define RIG_TS_END     {RIG_MODE_NONE,0}
#define RIG_IS_TS_END(t)	((t).modes == RIG_MODE_NONE && (t).ts == 0)

/**
 * \brief Filter definition
 *
 * Lists the filters available for each mode.
 *
 * If more than one filter is available for a given mode,
 * the first entry in the array will be the default
 * filter to use for the normal passband of this mode.
 * The first entry in the array below the default normal passband
 * is the default narrow passband and the first entry in the array
 * above the default normal passband is the default wide passband.
 * Note: if there's no lower width or upper width, then narrow or
 * respectively wide passband is equal to the default normal passband.
 *
 * If a width field in the list has RIG_FLT_ANY value,
 * this means the rig allows its passband width to be
 * set to any value ranging from the lowest to the
 * highest value (if any) in the list for that mode.
 * The RIG_FLT_ANY value, if present, must
 * be the last one in the list.
 *
 * The width field is the narrowest passband in a transmit/receive chain
 * with regard to different IF.
 *
 * \sa rig_set_mode, rig_passband_normal, rig_passband_narrow, rig_passband_wide
 */
struct filter_list {
  rmode_t modes;	/*!< Bit field of RIG_MODE's */
  pbwidth_t width;	/*!< Passband width in Hz */
};

#define RIG_FLT_ANY     0
#define RIG_FLT_END     {RIG_MODE_NONE,0}
#define RIG_IS_FLT_END(f)	((f).modes == RIG_MODE_NONE)


/** \brief Empty channel_t.flags field */
#define RIG_CHFLAG_NONE	0
/** \brief skip memory channel during scan (lock out), channel_t.flags */
#define RIG_CHFLAG_SKIP	(1<<0)
/** \brief DATA port mode flag */
#define RIG_CHFLAG_DATA (1<<1)

/**
 * \brief Extension attribute definition
 *
 */
struct ext_list {
  token_t token;	/*!< Token ID */
  value_t val;		/*!< Value */
};

#define RIG_EXT_END     {0, {.i=0}}
#define RIG_IS_EXT_END(x)	((x).token == 0)

/**
 * \brief Channel structure
 *
 * The channel struct stores all the attributes peculiar to a VFO.
 *
 * \sa rig_set_channel, rig_get_channel
 */
struct channel {
  int channel_num;		/*!< Channel number */
  int bank_num;			/*!< Bank number */
  vfo_t vfo;			/*!< VFO */
  int ant;			/*!< Selected antenna */
  freq_t freq;			/*!< Receive frequency */
  rmode_t mode;			/*!< Receive mode */
  pbwidth_t width;		/*!< Receive passband width associated with mode */

  freq_t tx_freq;		/*!< Transmit frequency */
  rmode_t tx_mode;		/*!< Transmit mode */
  pbwidth_t tx_width;		/*!< Transmit passband width associated with mode */

  split_t split;		/*!< Split mode */
  vfo_t tx_vfo;			/*!< Split transmit VFO */

  rptr_shift_t rptr_shift;	/*!< Repeater shift */
  shortfreq_t rptr_offs;	/*!< Repeater offset */
  shortfreq_t tuning_step;	/*!< Tuning step */
  shortfreq_t rit;		/*!< RIT */
  shortfreq_t xit;		/*!< XIT */
  setting_t funcs;		/*!< Function status */
  value_t levels[RIG_SETTING_MAX];	/*!< Level values */
  tone_t ctcss_tone;		/*!< CTCSS tone */
  tone_t ctcss_sql;		/*!< CTCSS squelch tone */
  tone_t dcs_code;		/*!< DCS code */
  tone_t dcs_sql;		/*!< DCS squelch code */
  int scan_group;		/*!< Scan group */
  int flags;			/*!< Channel flags, see RIG_CHFLAG's */
  char channel_desc[MAXCHANDESC];	/*!< Name */
  struct ext_list *ext_levels;	/*!< Extension level value list, NULL ended. ext_levels can be NULL */
};
/** \brief Channel structure typedef */
typedef struct channel channel_t;

/**
 * \brief Channel capability definition
 *
 * Definition of the attributes that can be stored/retrieved in/from memory
 */
struct channel_cap {
  unsigned bank_num:1;		/*!< Bank number */
  unsigned vfo:1;		/*!< VFO */
  unsigned ant:1;		/*!< Selected antenna */
  unsigned freq:1;		/*!< Receive frequency */
  unsigned mode:1;		/*!< Receive mode */
  unsigned width:1;		/*!< Receive passband width associated with mode */

  unsigned tx_freq:1;		/*!< Transmit frequency */
  unsigned tx_mode:1;		/*!< Transmit mode */
  unsigned tx_width:1;		/*!< Transmit passband width associated with mode */

  unsigned split:1;		/*!< Split mode */
  unsigned tx_vfo:1;		/*!< Split transmit VFO */
  unsigned rptr_shift:1;	/*!< Repeater shift */
  unsigned rptr_offs:1;		/*!< Repeater offset */
  unsigned tuning_step:1;	/*!< Tuning step */
  unsigned rit:1;		/*!< RIT */
  unsigned xit:1;		/*!< XIT */
  setting_t funcs;		/*!< Function status */
  setting_t levels;		/*!< Level values */
  unsigned ctcss_tone:1;	/*!< CTCSS tone */
  unsigned ctcss_sql:1;		/*!< CTCSS squelch tone */
  unsigned dcs_code:1;		/*!< DCS code */
  unsigned dcs_sql:1;		/*!< DCS squelch code */
  unsigned scan_group:1;	/*!< Scan group */
  unsigned flags:1;		/*!< Channel flags */
  unsigned channel_desc:1;	/*!< Name */
  unsigned ext_levels:1;	/*!< Extension level value list */
};

/** \brief Channel cap */
typedef struct channel_cap channel_cap_t;


/**
 * \brief Memory channel type definition
 *
 * Definition of memory types. Depending on the type, the content
 * of the memory channel has to be interpreted accordingly.
 * For instance, a RIG_MTYPE_EDGE channel_t will hold only a start
 * or stop frequency.
 *
 * \sa chan_list
 */
typedef enum {
  RIG_MTYPE_NONE=0,		/*!< None */
  RIG_MTYPE_MEM,		/*!< Regular */
  RIG_MTYPE_EDGE,		/*!< Scan edge */
  RIG_MTYPE_CALL,		/*!< Call channel */
  RIG_MTYPE_MEMOPAD,		/*!< Memory pad */
  RIG_MTYPE_SAT,		/*!< Satellite */
  RIG_MTYPE_BAND,		/*!< VFO/Band channel */
  RIG_MTYPE_PRIO		/*!< Priority channel */
} chan_type_t;

/**
 * \brief Memory channel list definition
 *
 * Example for the Ic706MkIIG (99 memory channels, 2 scan edges, 2 call chans):
\code
	chan_t chan_list[] = {
		{ 1, 99, RIG_MTYPE_MEM  },
		{ 100, 103, RIG_MTYPE_EDGE },
		{ 104, 105, RIG_MTYPE_CALL },
		RIG_CHAN_END
	}
\endcode
 */
struct chan_list {
  int start;			/*!< Starting memory channel \b number */
  int end;			/*!< Ending memory channel \b number */
  chan_type_t type;		/*!< Memory type. see chan_type_t */
  channel_cap_t mem_caps;	/*!< Definition of attributes that can be stored/retrieved */
};

#define RIG_CHAN_END     {0,0,RIG_MTYPE_NONE}
#define RIG_IS_CHAN_END(c)	((c).type == RIG_MTYPE_NONE)
/** \brief Special memory channel value to tell rig_lookup_mem_caps() to retrieve all the ranges */
#define RIG_MEM_CAPS_ALL -1

/** \brief chan_t type */
typedef struct chan_list chan_t;

/**
 * \brief level/parm granularity definition
 *
 * The granularity is undefined if min=0, max=0, and step=0.
 *
 * For float settings, if min.f=0 and max.f=0 (and step.f!=0),
 * max.f is assumed to be actually equal to 1.0.
 *
 * If step=0 (and min and/or max are not null), then this means step
 * can have maximum resolution, depending on type (int or float).
 */
struct gran {
	value_t min;		/*!< Minimum value */
	value_t max;		/*!< Maximum value */
	value_t step;		/*!< Step */
};

/** \brief gran_t type */
typedef	struct gran gran_t;


/** \brief Calibration table struct */
struct cal_table {
	int size;		/*!< number of plots in the table */
	struct {
		int raw;	/*!< raw (A/D) value, as returned by \a RIG_LEVEL_RAWSTR */
		int val;	/*!< associated value, basically the measured dB value */
	} table[MAX_CAL_LENGTH];	/*!< table of plots */
};

/**
 * \brief calibration table type
 *
 * cal_table_t is a data type suited to hold linear calibration.
 * cal_table_t.size tells the number of plots cal_table_t.table contains.
 *
 * If a value is below or equal to cal_table_t.table[0].raw,
 * rig_raw2val() will return cal_table_t.table[0].val.
 *
 * If a value is greater or equal to cal_table_t.table[cal_table_t.size-1].raw,
 * rig_raw2val() will return cal_table_t.table[cal_table_t.size-1].val.
 */
typedef struct cal_table cal_table_t;

#define EMPTY_STR_CAL { 0, { { 0, 0 }, } }


typedef int (*chan_cb_t) (RIG *, channel_t**, int, const chan_t*, rig_ptr_t);
typedef int (*confval_cb_t) (RIG *, const struct confparams *, value_t *, rig_ptr_t);

/**
 * \brief Rig data structure.
 *
 * Basic rig type, can store some useful info about different radios.
 * Each lib must be able to populate this structure, so we can make
 * useful inquiries about capabilities.
 *
 * The main idea of this struct is that it will be defined by the backend
 * rig driver, and will remain readonly for the application.
 * Fields that need to be modifiable by the application are
 * copied into the struct rig_state, which is a kind of private
 * of the RIG instance.
 * This way, you can have several rigs running within the same application,
 * sharing the struct rig_caps of the backend, while keeping their own
 * customized data.
 * NB: don't move fields around, as backend depends on it when initializing
 *     their caps.
 */
struct rig_caps {
  rig_model_t rig_model;	/*!< Rig model. */
  const char *model_name;	/*!< Model name. */
  const char *mfg_name;		/*!< Manufacturer. */
  const char *version;		/*!< Driver version. */
  const char *copyright;	/*!< Copyright info. */
  enum rig_status_e status;	/*!< Driver status. */

  int rig_type;			/*!< Rig type. */
  ptt_type_t ptt_type;		/*!< Type of the PTT port. */
  dcd_type_t dcd_type;		/*!< Type of the DCD port. */
  rig_port_t port_type;		/*!< Type of communication port. */

  int serial_rate_min;		/*!< Minimum serial speed. */
  int serial_rate_max;		/*!< Maximum serial speed. */
  int serial_data_bits;		/*!< Number of data bits. */
  int serial_stop_bits;		/*!< Number of stop bits. */
  enum serial_parity_e serial_parity;		/*!< Parity. */
  enum serial_handshake_e serial_handshake;	/*!< Handshake. */

  int write_delay;		/*!< Delay between each byte sent out, in mS */
  int post_write_delay;		/*!< Delay between each commands send out, in mS */
  int timeout;			/*!< Timeout, in mS */
  int retry;			/*!< Maximum number of retries if command fails, 0 to disable */

  setting_t has_get_func;	/*!< List of get functions */
  setting_t has_set_func;	/*!< List of set functions */
  setting_t has_get_level;	/*!< List of get level */
  setting_t has_set_level;	/*!< List of set level */
  setting_t has_get_parm;	/*!< List of get parm */
  setting_t has_set_parm;	/*!< List of set parm */

  gran_t level_gran[RIG_SETTING_MAX];	/*!< level granularity (i.e. steps) */
  gran_t parm_gran[RIG_SETTING_MAX];	/*!< parm granularity (i.e. steps) */

  const struct confparams *extparms;	/*!< Extension parm list, \sa ext.c */
  const struct confparams *extlevels;	/*!< Extension level list, \sa ext.c */

  const tone_t *ctcss_list;	/*!< CTCSS tones list, zero ended */
  const tone_t *dcs_list;	/*!< DCS code list, zero ended */

  int preamp[MAXDBLSTSIZ];	/*!< Preamp list in dB, 0 terminated */
  int attenuator[MAXDBLSTSIZ];	/*!< Preamp list in dB, 0 terminated */
  shortfreq_t max_rit;		/*!< max absolute RIT */
  shortfreq_t max_xit;		/*!< max absolute XIT */
  shortfreq_t max_ifshift;	/*!< max absolute IF-SHIFT */

  ann_t announces;	/*!< Announces bit field list */

  vfo_op_t vfo_ops;	/*!< VFO op bit field list */
  scan_t scan_ops;	/*!< Scan bit field list */
  int targetable_vfo;	/*!< Bit field list of direct VFO access commands */
  int transceive;	/*!< Supported transceive mode */

  int bank_qty;		/*!< Number of banks */
  int chan_desc_sz;	/*!< Max length of memory channel name */

  chan_t chan_list[CHANLSTSIZ];	/*!< Channel list, zero ended */

  freq_range_t rx_range_list1[FRQRANGESIZ];	/*!< Receive frequency range list for ITU region 1 */
  freq_range_t tx_range_list1[FRQRANGESIZ];	/*!< Transmit frequency range list for ITU region 1 */
  freq_range_t rx_range_list2[FRQRANGESIZ];	/*!< Receive frequency range list for ITU region 2 */
  freq_range_t tx_range_list2[FRQRANGESIZ];	/*!< Transmit frequency range list for ITU region 2 */

  struct tuning_step_list tuning_steps[TSLSTSIZ];	/*!< Tuning step list */
  struct filter_list filters[FLTLSTSIZ];		/*!< mode/filter table, at -6dB */

  cal_table_t str_cal;				/*!< S-meter calibration table */

  const struct confparams *cfgparams;            /*!< Configuration parametres. */
  const rig_ptr_t priv;                          /*!< Private data. */

	/*
	 * Rig API
	 *
	 */

  int (*rig_init) (RIG * rig);
  int (*rig_cleanup) (RIG * rig);
  int (*rig_open) (RIG * rig);
  int (*rig_close) (RIG * rig);

	/*
	 *  General API commands, from most primitive to least.. :()
	 *  List Set/Get functions pairs
	 */

  int (*set_freq) (RIG * rig, vfo_t vfo, freq_t freq);
  int (*get_freq) (RIG * rig, vfo_t vfo, freq_t * freq);

  int (*set_mode) (RIG * rig, vfo_t vfo, rmode_t mode,
			 pbwidth_t width);
  int (*get_mode) (RIG * rig, vfo_t vfo, rmode_t * mode,
			 pbwidth_t * width);

  int (*set_vfo) (RIG * rig, vfo_t vfo);
  int (*get_vfo) (RIG * rig, vfo_t * vfo);

  int (*set_ptt) (RIG * rig, vfo_t vfo, ptt_t ptt);
  int (*get_ptt) (RIG * rig, vfo_t vfo, ptt_t * ptt);
  int (*get_dcd) (RIG * rig, vfo_t vfo, dcd_t * dcd);

  int (*set_rptr_shift) (RIG * rig, vfo_t vfo,
			       rptr_shift_t rptr_shift);
  int (*get_rptr_shift) (RIG * rig, vfo_t vfo,
			       rptr_shift_t * rptr_shift);

  int (*set_rptr_offs) (RIG * rig, vfo_t vfo, shortfreq_t offs);
  int (*get_rptr_offs) (RIG * rig, vfo_t vfo, shortfreq_t * offs);

  int (*set_split_freq) (RIG * rig, vfo_t vfo, freq_t tx_freq);
  int (*get_split_freq) (RIG * rig, vfo_t vfo, freq_t * tx_freq);
  int (*set_split_mode) (RIG * rig, vfo_t vfo, rmode_t tx_mode,
			       pbwidth_t tx_width);
  int (*get_split_mode) (RIG * rig, vfo_t vfo, rmode_t * tx_mode,
			       pbwidth_t * tx_width);
  int (*set_split_freq_mode) (RIG * rig, vfo_t vfo, freq_t tx_freq,
             rmode_t tx_mode, pbwidth_t tx_width);
  int (*get_split_freq_mode) (RIG * rig, vfo_t vfo, freq_t * tx_freq,
             rmode_t * tx_mode, pbwidth_t * tx_width);

  int (*set_split_vfo) (RIG * rig, vfo_t vfo, split_t split, vfo_t tx_vfo);
  int (*get_split_vfo) (RIG * rig, vfo_t vfo, split_t * split, vfo_t *tx_vfo);

  int (*set_rit) (RIG * rig, vfo_t vfo, shortfreq_t rit);
  int (*get_rit) (RIG * rig, vfo_t vfo, shortfreq_t * rit);
  int (*set_xit) (RIG * rig, vfo_t vfo, shortfreq_t xit);
  int (*get_xit) (RIG * rig, vfo_t vfo, shortfreq_t * xit);

  int (*set_ts) (RIG * rig, vfo_t vfo, shortfreq_t ts);
  int (*get_ts) (RIG * rig, vfo_t vfo, shortfreq_t * ts);

  int (*set_dcs_code) (RIG * rig, vfo_t vfo, tone_t code);
  int (*get_dcs_code) (RIG * rig, vfo_t vfo, tone_t * code);
  int (*set_tone) (RIG * rig, vfo_t vfo, tone_t tone);
  int (*get_tone) (RIG * rig, vfo_t vfo, tone_t * tone);
  int (*set_ctcss_tone) (RIG * rig, vfo_t vfo, tone_t tone);
  int (*get_ctcss_tone) (RIG * rig, vfo_t vfo, tone_t * tone);

  int (*set_dcs_sql) (RIG * rig, vfo_t vfo, tone_t code);
  int (*get_dcs_sql) (RIG * rig, vfo_t vfo, tone_t * code);
  int (*set_tone_sql) (RIG * rig, vfo_t vfo, tone_t tone);
  int (*get_tone_sql) (RIG * rig, vfo_t vfo, tone_t * tone);
  int (*set_ctcss_sql) (RIG * rig, vfo_t vfo, tone_t tone);
  int (*get_ctcss_sql) (RIG * rig, vfo_t vfo, tone_t * tone);

  int (*power2mW) (RIG * rig, unsigned int *mwpower, float power,
			 freq_t freq, rmode_t mode);
  int (*mW2power) (RIG * rig, float *power, unsigned int mwpower,
			 freq_t freq, rmode_t mode);

  int (*set_powerstat) (RIG * rig, powerstat_t status);
  int (*get_powerstat) (RIG * rig, powerstat_t * status);
  int (*reset) (RIG * rig, reset_t reset);

  int (*set_ant) (RIG * rig, vfo_t vfo, ant_t ant);
  int (*get_ant) (RIG * rig, vfo_t vfo, ant_t * ant);

  int (*set_level) (RIG * rig, vfo_t vfo, setting_t level,
			  value_t val);
  int (*get_level) (RIG * rig, vfo_t vfo, setting_t level,
			  value_t * val);

  int (*set_func) (RIG * rig, vfo_t vfo, setting_t func, int status);
  int (*get_func) (RIG * rig, vfo_t vfo, setting_t func,
			 int *status);

  int (*set_parm) (RIG * rig, setting_t parm, value_t val);
  int (*get_parm) (RIG * rig, setting_t parm, value_t * val);

  int (*set_ext_level)(RIG *rig, vfo_t vfo, token_t token, value_t val);
  int (*get_ext_level)(RIG *rig, vfo_t vfo, token_t token, value_t *val);

  int (*set_ext_parm)(RIG *rig, token_t token, value_t val);
  int (*get_ext_parm)(RIG *rig, token_t token, value_t *val);

  int (*set_conf) (RIG * rig, token_t token, const char *val);
  int (*get_conf) (RIG * rig, token_t token, char *val);

  int (*send_dtmf) (RIG * rig, vfo_t vfo, const char *digits);
  int (*recv_dtmf) (RIG * rig, vfo_t vfo, char *digits, int *length);
  int (*send_morse) (RIG * rig, vfo_t vfo, const char *msg);

  int (*set_bank) (RIG * rig, vfo_t vfo, int bank);
  int (*set_mem) (RIG * rig, vfo_t vfo, int ch);
  int (*get_mem) (RIG * rig, vfo_t vfo, int *ch);
  int (*vfo_op) (RIG * rig, vfo_t vfo, vfo_op_t op);
  int (*scan) (RIG * rig, vfo_t vfo, scan_t scan, int ch);

  int (*set_trn) (RIG * rig, int trn);
  int (*get_trn) (RIG * rig, int *trn);

  int (*decode_event) (RIG * rig);

  int (*set_channel) (RIG * rig, const channel_t * chan);
  int (*get_channel) (RIG * rig, channel_t * chan);

  const char *(*get_info) (RIG * rig);

  int (*set_chan_all_cb) (RIG * rig, chan_cb_t chan_cb, rig_ptr_t);
  int (*get_chan_all_cb) (RIG * rig, chan_cb_t chan_cb, rig_ptr_t);

  int (*set_mem_all_cb) (RIG * rig, chan_cb_t chan_cb, confval_cb_t parm_cb, rig_ptr_t);
  int (*get_mem_all_cb) (RIG * rig, chan_cb_t chan_cb, confval_cb_t parm_cb, rig_ptr_t);

  const char *clone_combo_set;	/*!< String describing key combination to enter load cloning mode */
  const char *clone_combo_get;	/*!< String describing key combination to enter save cloning mode */
};

/**
 * \brief Port definition
 *
 * Of course, looks like OO painstakingly programmed in C, sigh.
 */
typedef struct hamlib_port {
  union {
	rig_port_t rig;		/*!< Communication port type */
	ptt_type_t ptt;		/*!< PTT port type */
	dcd_type_t dcd;		/*!< DCD port type */
  } type;
  int fd;			/*!< File descriptor */
  void* handle;			/*!< handle for USB */

  int write_delay;		/*!< Delay between each byte sent out, in mS */
  int post_write_delay;		/*!< Delay between each commands send out, in mS */
  struct { int tv_sec,tv_usec; } post_write_date;	/*!< hamlib internal use */
  int timeout;			/*!< Timeout, in mS */
  int retry;			/*!< Maximum number of retries, 0 to disable */

  char pathname[FILPATHLEN];	/*!< Port pathname */
  union {
	struct {
		int rate;	/*!< Serial baud rate */
		int data_bits;	/*!< Number of data bits */
		int stop_bits;	/*!< Number of stop bits */
		enum serial_parity_e parity;		/*!< Serial parity */
		enum serial_handshake_e handshake;	/*!< Serial handshake */
		enum serial_control_state_e rts_state;	/*!< RTS set state */
		enum serial_control_state_e dtr_state;	/*!< DTR set state */
	} serial;		/*!< serial attributes */
	struct {
		int pin;	/*!< Parallel port pin number */
	} parallel;		/*!< parallel attributes */
	struct {
		int ptt_bitnum;	/*< Bit number for CM108 GPIO PTT */
	} cm108;		/*!< CM108 attributes */
	struct {
		int vid;	/*!< Vendor ID */
		int pid;	/*!< Product ID */
		int conf;	/*!< Configuration */
		int iface;	/*!< interface */
		int alt;	/*!< alternate */
        char *vendor_name; /*!< Vendor name (opt.) */
        char *product;     /*!< Product (opt.) */
	} usb;			/*!< USB attributes */
	struct {
		int on_value;
		int value;
	} gpio;
  } parm;			/*!< Port parameter union */
} hamlib_port_t;

#if !defined(__APPLE__) || !defined(__cplusplus)
typedef hamlib_port_t port_t;
#endif


/**
 * \brief Rig state containing live data and customized fields.
 *
 * This struct contains live data, as well as a copy of capability fields
 * that may be updated (ie. customized)
 *
 * It is fine to move fields around, as this kind of struct should
 * not be initialized like caps are.
 */
struct rig_state {
	/*
	 * overridable fields
	 */
  hamlib_port_t rigport;	/*!< Rig port (internal use). */
  hamlib_port_t pttport;	/*!< PTT port (internal use). */
  hamlib_port_t dcdport;	/*!< DCD port (internal use). */

  double vfo_comp;	/*!< VFO compensation in PPM, 0.0 to disable */

  int itu_region;	/*!< ITU region to select among freq_range_t */
  freq_range_t rx_range_list[FRQRANGESIZ];	/*!< Receive frequency range list */
  freq_range_t tx_range_list[FRQRANGESIZ];	/*!< Transmit frequency range list */

  struct tuning_step_list tuning_steps[TSLSTSIZ];	/*!< Tuning step list */

  struct filter_list filters[FLTLSTSIZ];	/*!< Mode/filter table, at -6dB */

  cal_table_t str_cal;				/*!< S-meter calibration table */

  chan_t chan_list[CHANLSTSIZ];	/*!< Channel list, zero ended */

  shortfreq_t max_rit;		/*!< max absolute RIT */
  shortfreq_t max_xit;		/*!< max absolute XIT */
  shortfreq_t max_ifshift;	/*!< max absolute IF-SHIFT */

  ann_t announces;		/*!< Announces bit field list */

  int preamp[MAXDBLSTSIZ];	/*!< Preamp list in dB, 0 terminated */
  int attenuator[MAXDBLSTSIZ];	/*!< Preamp list in dB, 0 terminated */

  setting_t has_get_func;	/*!< List of get functions */
  setting_t has_set_func;	/*!< List of set functions */
  setting_t has_get_level;	/*!< List of get level */
  setting_t has_set_level;	/*!< List of set level */
  setting_t has_get_parm;	/*!< List of get parm */
  setting_t has_set_parm;	/*!< List of set parm */

  gran_t level_gran[RIG_SETTING_MAX];	/*!< level granularity */
  gran_t parm_gran[RIG_SETTING_MAX];	/*!< parm granularity */


	/*
	 * non overridable fields, internal use
	 */

  int hold_decode;	/*!< set to 1 to hold the event decoder (async) otherwise 0 */
  vfo_t current_vfo;	/*!< VFO currently set */
  int vfo_list;		/*!< Complete list of VFO for this rig */
  int comm_state;	/*!< Comm port state, opened/closed. */
  rig_ptr_t priv;	/*!< Pointer to private rig state data. */
  rig_ptr_t obj;	/*!< Internal use by hamlib++ for event handling. */

  int transceive;	/*!< Whether the transceive mode is on */
  int poll_interval;	/*!< Event notification polling period in milliseconds */
  freq_t current_freq;	/*!< Frequency currently set */
  rmode_t current_mode;	/*!< Mode currently set */
  pbwidth_t current_width;	/*!< Passband width currently set */
  vfo_t tx_vfo;		/*!< Tx VFO currently set */
  int mode_list;		/*!< Complete list of modes for this rig */

};


typedef int (*vprintf_cb_t) (enum rig_debug_level_e, rig_ptr_t, const char *, va_list);

typedef int (*freq_cb_t) (RIG *, vfo_t, freq_t, rig_ptr_t);
typedef int (*mode_cb_t) (RIG *, vfo_t, rmode_t, pbwidth_t, rig_ptr_t);
typedef int (*vfo_cb_t) (RIG *, vfo_t, rig_ptr_t);
typedef int (*ptt_cb_t) (RIG *, vfo_t, ptt_t, rig_ptr_t);
typedef int (*dcd_cb_t) (RIG *, vfo_t, dcd_t, rig_ptr_t);
typedef int (*pltune_cb_t) (RIG *, vfo_t, freq_t *, rmode_t *, pbwidth_t *, rig_ptr_t);

/**
 * \brief Callback functions and args for rig event.
 *
 * Some rigs are able to notify the host computer the operator changed
 * the freq/mode from the front panel, depressed a button, etc.
 *
 * Events from the rig are received through async io,
 * so callback functions will be called from the SIGIO sighandler context.
 *
 * Don't set these fields directly, use rig_set_freq_callback et. al. instead.
 *
 * Callbacks suit event based programming very well,
 * really appropriate in a GUI.
 *
 * \sa rig_set_freq_callback, rig_set_mode_callback, rig_set_vfo_callback,
 *	 rig_set_ptt_callback, rig_set_dcd_callback
 */
struct rig_callbacks {
  freq_cb_t freq_event;	/*!< Frequency change event */
  rig_ptr_t freq_arg;	/*!< Frequency change argument */
  mode_cb_t mode_event;	/*!< Mode change event */
  rig_ptr_t mode_arg;	/*!< Mode change argument */
  vfo_cb_t vfo_event;	/*!< VFO change event */
  rig_ptr_t vfo_arg;	/*!< VFO change argument */
  ptt_cb_t ptt_event;	/*!< PTT change event */
  rig_ptr_t ptt_arg;	/*!< PTT change argument */
  dcd_cb_t dcd_event;	/*!< DCD change event */
  rig_ptr_t dcd_arg;	/*!< DCD change argument */
  pltune_cb_t pltune;   /*!< Pipeline tuning module freq/mode/width callback */
  rig_ptr_t pltune_arg; /*!< Pipeline tuning argument */
  /* etc.. */
};

/**
 * \brief The Rig structure
 *
 * This is the master data structure, acting as a handle for the controlled
 * rig. A pointer to this structure is returned by the rig_init() API
 * function and is passed as a parameter to every rig specific API call.
 *
 * \sa rig_init(), rig_caps, rig_state
 */
struct rig {
  struct rig_caps *caps;		/*!< Pointer to rig capabilities (read only) */
  struct rig_state state;		/*!< Rig state */
  struct rig_callbacks callbacks;	/*!< registered event callbacks */
};



/* --------------- API function prototypes -----------------*/

extern HAMLIB_EXPORT(RIG *) rig_init HAMLIB_PARAMS((rig_model_t rig_model));
extern HAMLIB_EXPORT(int) rig_open HAMLIB_PARAMS((RIG *rig));

  /*
   *  General API commands, from most primitive to least.. :()
   *  List Set/Get functions pairs
   */

extern HAMLIB_EXPORT(int) rig_set_freq HAMLIB_PARAMS((RIG *rig, vfo_t vfo, freq_t freq));
extern HAMLIB_EXPORT(int) rig_get_freq HAMLIB_PARAMS((RIG *rig, vfo_t vfo, freq_t *freq));

extern HAMLIB_EXPORT(int) rig_set_mode HAMLIB_PARAMS((RIG *rig, vfo_t vfo, rmode_t mode, pbwidth_t width));
extern HAMLIB_EXPORT(int) rig_get_mode HAMLIB_PARAMS((RIG *rig, vfo_t vfo, rmode_t *mode, pbwidth_t *width));

extern HAMLIB_EXPORT(int) rig_set_vfo HAMLIB_PARAMS((RIG *rig, vfo_t vfo));
extern HAMLIB_EXPORT(int) rig_get_vfo HAMLIB_PARAMS((RIG *rig, vfo_t *vfo));

extern HAMLIB_EXPORT(int) rig_set_ptt HAMLIB_PARAMS((RIG *rig, vfo_t vfo, ptt_t ptt));
extern HAMLIB_EXPORT(int) rig_get_ptt HAMLIB_PARAMS((RIG *rig, vfo_t vfo, ptt_t *ptt));

extern HAMLIB_EXPORT(int) rig_get_dcd HAMLIB_PARAMS((RIG *rig, vfo_t vfo, dcd_t *dcd));

extern HAMLIB_EXPORT(int) rig_set_rptr_shift HAMLIB_PARAMS((RIG *rig, vfo_t vfo, rptr_shift_t rptr_shift));
extern HAMLIB_EXPORT(int) rig_get_rptr_shift HAMLIB_PARAMS((RIG *rig, vfo_t vfo, rptr_shift_t *rptr_shift));
extern HAMLIB_EXPORT(int) rig_set_rptr_offs HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t rptr_offs));
extern HAMLIB_EXPORT(int) rig_get_rptr_offs HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t *rptr_offs));

extern HAMLIB_EXPORT(int) rig_set_ctcss_tone HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t tone));
extern HAMLIB_EXPORT(int) rig_get_ctcss_tone HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t *tone));
extern HAMLIB_EXPORT(int) rig_set_dcs_code HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t code));
extern HAMLIB_EXPORT(int) rig_get_dcs_code HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t *code));

extern HAMLIB_EXPORT(int) rig_set_ctcss_sql HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t tone));
extern HAMLIB_EXPORT(int) rig_get_ctcss_sql HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t *tone));
extern HAMLIB_EXPORT(int) rig_set_dcs_sql HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t code));
extern HAMLIB_EXPORT(int) rig_get_dcs_sql HAMLIB_PARAMS((RIG *rig, vfo_t vfo, tone_t *code));

extern HAMLIB_EXPORT(int) rig_set_split_freq HAMLIB_PARAMS((RIG *rig, vfo_t vfo, freq_t tx_freq));
extern HAMLIB_EXPORT(int) rig_get_split_freq HAMLIB_PARAMS((RIG *rig, vfo_t vfo, freq_t *tx_freq));
extern HAMLIB_EXPORT(int) rig_set_split_mode HAMLIB_PARAMS((RIG *rig, vfo_t vfo, rmode_t tx_mode, pbwidth_t tx_width));
extern HAMLIB_EXPORT(int) rig_get_split_mode HAMLIB_PARAMS((RIG *rig, vfo_t vfo, rmode_t *tx_mode, pbwidth_t *tx_width));
extern HAMLIB_EXPORT(int) rig_set_split_freq_mode HAMLIB_PARAMS((RIG *rig, vfo_t vfo, freq_t tx_freq, rmode_t tx_mode, pbwidth_t tx_width));
extern HAMLIB_EXPORT(int) rig_get_split_freq_mode HAMLIB_PARAMS((RIG *rig, vfo_t vfo, freq_t * tx_freq, rmode_t *tx_mode, pbwidth_t *tx_width));
extern HAMLIB_EXPORT(int) rig_set_split_vfo HAMLIB_PARAMS((RIG*, vfo_t rx_vfo, split_t split, vfo_t tx_vfo));
extern HAMLIB_EXPORT(int) rig_get_split_vfo HAMLIB_PARAMS((RIG*, vfo_t rx_vfo, split_t *split, vfo_t *tx_vfo));
#define rig_set_split(r,v,s) rig_set_split_vfo((r),(v),(s),RIG_VFO_CURR)
#define rig_get_split(r,v,s) ({ vfo_t _tx_vfo; rig_get_split_vfo((r),(v),(s),&_tx_vfo); })

extern HAMLIB_EXPORT(int) rig_set_rit HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t rit));
extern HAMLIB_EXPORT(int) rig_get_rit HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t *rit));
extern HAMLIB_EXPORT(int) rig_set_xit HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t xit));
extern HAMLIB_EXPORT(int) rig_get_xit HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t *xit));

extern HAMLIB_EXPORT(int) rig_set_ts HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t ts));
extern HAMLIB_EXPORT(int) rig_get_ts HAMLIB_PARAMS((RIG *rig, vfo_t vfo, shortfreq_t *ts));

extern HAMLIB_EXPORT(int) rig_power2mW HAMLIB_PARAMS((RIG *rig, unsigned int *mwpower, float power, freq_t freq, rmode_t mode));
extern HAMLIB_EXPORT(int) rig_mW2power HAMLIB_PARAMS((RIG *rig, float *power, unsigned int mwpower, freq_t freq, rmode_t mode));

extern HAMLIB_EXPORT(shortfreq_t) rig_get_resolution HAMLIB_PARAMS((RIG *rig, rmode_t mode));

extern HAMLIB_EXPORT(int) rig_set_level HAMLIB_PARAMS((RIG *rig, vfo_t vfo, setting_t level, value_t val));
extern HAMLIB_EXPORT(int) rig_get_level HAMLIB_PARAMS((RIG *rig, vfo_t vfo, setting_t level, value_t *val));

#define rig_get_strength(r,v,s) rig_get_level((r),(v),RIG_LEVEL_STRENGTH, (value_t*)(s))

extern HAMLIB_EXPORT(int) rig_set_parm HAMLIB_PARAMS((RIG *rig, setting_t parm, value_t val));
extern HAMLIB_EXPORT(int) rig_get_parm HAMLIB_PARAMS((RIG *rig, setting_t parm, value_t *val));

extern HAMLIB_EXPORT(int) rig_set_conf HAMLIB_PARAMS((RIG *rig, token_t token, const char *val));
extern HAMLIB_EXPORT(int) rig_get_conf HAMLIB_PARAMS((RIG *rig, token_t token, char *val));

extern HAMLIB_EXPORT(int) rig_set_powerstat HAMLIB_PARAMS((RIG *rig, powerstat_t status));
extern HAMLIB_EXPORT(int) rig_get_powerstat HAMLIB_PARAMS((RIG *rig, powerstat_t *status));

extern HAMLIB_EXPORT(int) rig_reset HAMLIB_PARAMS((RIG *rig, reset_t reset));	/* dangerous! */

extern HAMLIB_EXPORT(int) rig_set_ext_level HAMLIB_PARAMS((RIG *rig, vfo_t vfo,
			token_t token, value_t val));
extern HAMLIB_EXPORT(int) rig_get_ext_level HAMLIB_PARAMS((RIG *rig, vfo_t vfo,
			token_t token, value_t *val));

extern HAMLIB_EXPORT(int) rig_set_ext_parm HAMLIB_PARAMS((RIG *rig, token_t token, value_t val));
extern HAMLIB_EXPORT(int) rig_get_ext_parm HAMLIB_PARAMS((RIG *rig, token_t token, value_t *val));

extern HAMLIB_EXPORT(int) rig_ext_level_foreach HAMLIB_PARAMS((RIG *rig, int (*cfunc)(RIG*, const struct confparams *, rig_ptr_t), rig_ptr_t data));
extern HAMLIB_EXPORT(int) rig_ext_parm_foreach HAMLIB_PARAMS((RIG *rig, int (*cfunc)(RIG*, const struct confparams *, rig_ptr_t), rig_ptr_t data));
extern HAMLIB_EXPORT(const struct confparams*) rig_ext_lookup HAMLIB_PARAMS((RIG *rig, const char *name));
extern HAMLIB_EXPORT(const struct confparams *)  rig_ext_lookup_tok HAMLIB_PARAMS((RIG *rig, token_t token));
extern HAMLIB_EXPORT(token_t) rig_ext_token_lookup HAMLIB_PARAMS((RIG *rig, const char *name));


extern HAMLIB_EXPORT(int) rig_token_foreach HAMLIB_PARAMS((RIG *rig, int (*cfunc)(const struct confparams *, rig_ptr_t), rig_ptr_t data));
extern HAMLIB_EXPORT(const struct confparams*) rig_confparam_lookup HAMLIB_PARAMS((RIG *rig, const char *name));
extern HAMLIB_EXPORT(token_t) rig_token_lookup HAMLIB_PARAMS((RIG *rig, const char *name));

extern HAMLIB_EXPORT(int) rig_close HAMLIB_PARAMS((RIG *rig));
extern HAMLIB_EXPORT(int) rig_cleanup HAMLIB_PARAMS((RIG *rig));

extern HAMLIB_EXPORT(int) rig_set_ant HAMLIB_PARAMS((RIG *rig, vfo_t vfo, ant_t ant));	/* antenna */
extern HAMLIB_EXPORT(int) rig_get_ant HAMLIB_PARAMS((RIG *rig, vfo_t vfo, ant_t *ant));

extern HAMLIB_EXPORT(setting_t) rig_has_get_level HAMLIB_PARAMS((RIG *rig, setting_t level));
extern HAMLIB_EXPORT(setting_t) rig_has_set_level HAMLIB_PARAMS((RIG *rig, setting_t level));

extern HAMLIB_EXPORT(setting_t) rig_has_get_parm HAMLIB_PARAMS((RIG *rig, setting_t parm));
extern HAMLIB_EXPORT(setting_t) rig_has_set_parm HAMLIB_PARAMS((RIG *rig, setting_t parm));

extern HAMLIB_EXPORT(setting_t) rig_has_get_func HAMLIB_PARAMS((RIG *rig, setting_t func));
extern HAMLIB_EXPORT(setting_t) rig_has_set_func HAMLIB_PARAMS((RIG *rig, setting_t func));

extern HAMLIB_EXPORT(int) rig_set_func HAMLIB_PARAMS((RIG *rig, vfo_t vfo, setting_t func, int status));
extern HAMLIB_EXPORT(int) rig_get_func HAMLIB_PARAMS((RIG *rig, vfo_t vfo, setting_t func, int *status));

extern HAMLIB_EXPORT(int) rig_send_dtmf HAMLIB_PARAMS((RIG *rig, vfo_t vfo, const char *digits));
extern HAMLIB_EXPORT(int) rig_recv_dtmf HAMLIB_PARAMS((RIG *rig, vfo_t vfo, char *digits, int *length));
extern HAMLIB_EXPORT(int) rig_send_morse HAMLIB_PARAMS((RIG *rig, vfo_t vfo, const char *msg));

extern HAMLIB_EXPORT(int) rig_set_bank HAMLIB_PARAMS((RIG *rig, vfo_t vfo, int bank));
extern HAMLIB_EXPORT(int) rig_set_mem HAMLIB_PARAMS((RIG *rig, vfo_t vfo, int ch));
extern HAMLIB_EXPORT(int) rig_get_mem HAMLIB_PARAMS((RIG *rig, vfo_t vfo, int *ch));
extern HAMLIB_EXPORT(int) rig_vfo_op HAMLIB_PARAMS((RIG *rig, vfo_t vfo, vfo_op_t op));
extern HAMLIB_EXPORT(vfo_op_t) rig_has_vfo_op HAMLIB_PARAMS((RIG *rig, vfo_op_t op));
extern HAMLIB_EXPORT(int) rig_scan HAMLIB_PARAMS((RIG *rig, vfo_t vfo, scan_t scan, int ch));
extern HAMLIB_EXPORT(scan_t) rig_has_scan HAMLIB_PARAMS((RIG *rig, scan_t scan));

extern HAMLIB_EXPORT(int) rig_set_channel HAMLIB_PARAMS((RIG *rig, const channel_t *chan));	/* mem */
extern HAMLIB_EXPORT(int) rig_get_channel HAMLIB_PARAMS((RIG *rig, channel_t *chan));

extern HAMLIB_EXPORT(int) rig_set_chan_all HAMLIB_PARAMS((RIG *rig, const channel_t chans[]));
extern HAMLIB_EXPORT(int) rig_get_chan_all HAMLIB_PARAMS((RIG *rig, channel_t chans[]));
extern HAMLIB_EXPORT(int) rig_set_chan_all_cb HAMLIB_PARAMS((RIG *rig, chan_cb_t chan_cb, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_get_chan_all_cb HAMLIB_PARAMS((RIG *rig, chan_cb_t chan_cb, rig_ptr_t));

extern HAMLIB_EXPORT(int) rig_set_mem_all_cb HAMLIB_PARAMS((RIG *rig, chan_cb_t chan_cb, confval_cb_t parm_cb, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_get_mem_all_cb HAMLIB_PARAMS((RIG *rig, chan_cb_t chan_cb, confval_cb_t parm_cb, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_set_mem_all HAMLIB_PARAMS((RIG *rig, const channel_t *chan, const struct confparams *, const value_t *));
extern HAMLIB_EXPORT(int) rig_get_mem_all HAMLIB_PARAMS((RIG *rig, channel_t *chan, const struct confparams *, value_t *));
extern HAMLIB_EXPORT(const chan_t *) rig_lookup_mem_caps HAMLIB_PARAMS((RIG *rig, int ch));
extern HAMLIB_EXPORT(int) rig_mem_count HAMLIB_PARAMS((RIG *rig));


extern HAMLIB_EXPORT(int) rig_set_trn HAMLIB_PARAMS((RIG *rig, int trn));
extern HAMLIB_EXPORT(int) rig_get_trn HAMLIB_PARAMS((RIG *rig, int *trn));
extern HAMLIB_EXPORT(int) rig_set_freq_callback HAMLIB_PARAMS((RIG *, freq_cb_t, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_set_mode_callback HAMLIB_PARAMS((RIG *, mode_cb_t, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_set_vfo_callback HAMLIB_PARAMS((RIG *, vfo_cb_t, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_set_ptt_callback HAMLIB_PARAMS((RIG *, ptt_cb_t, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_set_dcd_callback HAMLIB_PARAMS((RIG *, dcd_cb_t, rig_ptr_t));
extern HAMLIB_EXPORT(int) rig_set_pltune_callback HAMLIB_PARAMS((RIG *, pltune_cb_t, rig_ptr_t));

extern HAMLIB_EXPORT(const char *) rig_get_info HAMLIB_PARAMS((RIG *rig));

extern HAMLIB_EXPORT(const struct rig_caps *) rig_get_caps HAMLIB_PARAMS((rig_model_t rig_model));
extern HAMLIB_EXPORT(const freq_range_t *) rig_get_range HAMLIB_PARAMS((const freq_range_t range_list[], freq_t freq, rmode_t mode));

extern HAMLIB_EXPORT(pbwidth_t) rig_passband_normal HAMLIB_PARAMS((RIG *rig, rmode_t mode));
extern HAMLIB_EXPORT(pbwidth_t) rig_passband_narrow HAMLIB_PARAMS((RIG *rig, rmode_t mode));
extern HAMLIB_EXPORT(pbwidth_t) rig_passband_wide HAMLIB_PARAMS((RIG *rig, rmode_t mode));

extern HAMLIB_EXPORT(const char *) rigerror HAMLIB_PARAMS((int errnum));

extern HAMLIB_EXPORT(int) rig_setting2idx HAMLIB_PARAMS((setting_t s));
#define rig_idx2setting(i) (1UL<<(i))

/*
 * Even if these functions are prefixed with "rig_", they are not rig specific
 * Maybe "hamlib_" would have been better. Let me know. --SF
 */
extern HAMLIB_EXPORT(void) rig_set_debug HAMLIB_PARAMS((enum rig_debug_level_e debug_level));
#define rig_set_debug_level(level) rig_set_debug(level)
extern HAMLIB_EXPORT(int) rig_need_debug HAMLIB_PARAMS((enum rig_debug_level_e debug_level));
extern HAMLIB_EXPORT(void) rig_debug HAMLIB_PARAMS((enum rig_debug_level_e debug_level, const char *fmt, ...));
extern HAMLIB_EXPORT(vprintf_cb_t) rig_set_debug_callback HAMLIB_PARAMS((vprintf_cb_t cb, rig_ptr_t arg));
extern HAMLIB_EXPORT(FILE*) rig_set_debug_file HAMLIB_PARAMS((FILE *stream));

extern HAMLIB_EXPORT(int) rig_register HAMLIB_PARAMS((const struct rig_caps *caps));
extern HAMLIB_EXPORT(int) rig_unregister HAMLIB_PARAMS((rig_model_t rig_model));
extern HAMLIB_EXPORT(int) rig_list_foreach HAMLIB_PARAMS((int (*cfunc)(const struct rig_caps*, rig_ptr_t), rig_ptr_t data));
extern HAMLIB_EXPORT(int) rig_load_backend HAMLIB_PARAMS((const char *be_name));
extern HAMLIB_EXPORT(int) rig_check_backend HAMLIB_PARAMS((rig_model_t rig_model));
extern HAMLIB_EXPORT(int) rig_load_all_backends HAMLIB_PARAMS((void));

typedef int (*rig_probe_func_t)(const hamlib_port_t *, rig_model_t, rig_ptr_t);
extern HAMLIB_EXPORT(int) rig_probe_all HAMLIB_PARAMS((hamlib_port_t *p, rig_probe_func_t, rig_ptr_t));
extern HAMLIB_EXPORT(rig_model_t) rig_probe HAMLIB_PARAMS((hamlib_port_t *p));


/* Misc calls */
extern HAMLIB_EXPORT(const char *) rig_strrmode(rmode_t mode);
extern HAMLIB_EXPORT(const char *) rig_strvfo(vfo_t vfo);
extern HAMLIB_EXPORT(const char *) rig_strfunc(setting_t);
extern HAMLIB_EXPORT(const char *) rig_strlevel(setting_t);
extern HAMLIB_EXPORT(const char *) rig_strparm(setting_t);
extern HAMLIB_EXPORT(const char *) rig_strptrshift(rptr_shift_t);
extern HAMLIB_EXPORT(const char *) rig_strvfop(vfo_op_t op);
extern HAMLIB_EXPORT(const char *) rig_strscan(scan_t scan);
extern HAMLIB_EXPORT(const char *) rig_strstatus(enum rig_status_e status);
extern HAMLIB_EXPORT(const char *) rig_strmtype(chan_type_t mtype);

extern HAMLIB_EXPORT(rmode_t) rig_parse_mode(const char *s);
extern HAMLIB_EXPORT(vfo_t) rig_parse_vfo(const char *s);
extern HAMLIB_EXPORT(setting_t) rig_parse_func(const char *s);
extern HAMLIB_EXPORT(setting_t) rig_parse_level(const char *s);
extern HAMLIB_EXPORT(setting_t) rig_parse_parm(const char *s);
extern HAMLIB_EXPORT(vfo_op_t) rig_parse_vfo_op(const char *s);
extern HAMLIB_EXPORT(scan_t) rig_parse_scan(const char *s);
extern HAMLIB_EXPORT(rptr_shift_t) rig_parse_rptr_shift(const char *s);
extern HAMLIB_EXPORT(chan_type_t) rig_parse_mtype(const char *s);

extern HAMLIB_EXPORT(const char *) rig_license HAMLIB_PARAMS(());
extern HAMLIB_EXPORT(const char *) rig_version HAMLIB_PARAMS(());
extern HAMLIB_EXPORT(const char *) rig_copyright HAMLIB_PARAMS(());

HAMLIB_EXPORT(void) rig_no_restore_ai();

__END_DECLS

#endif /* _RIG_H */

/*! @} */
