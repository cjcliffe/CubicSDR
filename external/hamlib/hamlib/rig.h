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

#define __FILENAME__ (strrchr(__FILE__, '/') ? strrchr(__FILE__, '/') + 1 : __FILE__)

#include <stdio.h>
#include <stdarg.h>
#include <string.h>
#include <inttypes.h>
#include <time.h>

/* Rig list is in a separate file so as not to mess up w/ this one */
#include <hamlib/riglist.h>

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
//! @cond Doxygen_Suppress
#undef __BEGIN_DECLS
#undef __END_DECLS
#ifdef __cplusplus
#  define __BEGIN_DECLS extern "C" {
#  define __END_DECLS }
#else
#  define __BEGIN_DECLS      /* empty */
#  define __END_DECLS        /* empty */
#endif
//! @endcond

/* HAMLIB_PARAMS is a macro used to wrap function prototypes, so that compilers
 * that don't understand ANSI C prototypes still work, and ANSI C
 * compilers can issue warnings about type mismatches. */
//! @cond Doxygen_Suppress
#undef HAMLIB_PARAMS
#if defined (__STDC__)                                                  \
    || defined (_AIX)                                                   \
    || (defined (__mips) && defined (_SYSTYPE_SVR4))                    \
    || defined(__CYGWIN__)                                              \
    || defined(_WIN32)                                                  \
    || defined(__cplusplus)
#  define HAMLIB_PARAMS(protos) protos
#  define rig_ptr_t     void *
#  define amp_ptr_t     void *
#else
#  define HAMLIB_PARAMS(protos) ()
#  define rig_ptr_t     char *
#  define amp_ptr_t     char *
#endif
//! @endcond

#include <hamlib/rig_dll.h>

#ifndef SWIGLUA
//! @cond Doxygen_Suppress
#define CONSTANT_64BIT_FLAG(BIT) (1ull << (BIT))
//! @endcond
#else
/* SWIG's older Lua generator doesn't grok ull due to Lua using a
   double-precision floating point type internally for number
   representations (max 53 bits of precision) so makes a string
   constant from a constant number literal using ull */
// #define CONSTANT_64BIT_FLAG(BIT) (1 << (BIT))
// #define SWIGLUAHIDE
/* But this appears to have been fixed so we'll use the correct one now 
   If you have the older version of SWIG comment out this line and use
   the two above */
// This 1ul definition works on swig 4.0.1 and lua 5.3.5
#define CONSTANT_64BIT_FLAG(BIT) (1ul << (BIT))
#endif

__BEGIN_DECLS

//! @cond Doxygen_Suppress
extern HAMLIB_EXPORT_VAR(const char) hamlib_version[];
extern HAMLIB_EXPORT_VAR(const char) hamlib_copyright[];
extern HAMLIB_EXPORT_VAR(const char *) hamlib_version2;
extern HAMLIB_EXPORT_VAR(const char *) hamlib_copyright2;
//! @endcond

/**
 * \brief Hamlib error codes
 * Error code definition that can be returned by the Hamlib functions.
 * Unless stated otherwise, Hamlib functions return the negative value
 * of rig_errcode_e definitions in case of error, or 0 when successful.
 */
enum rig_errcode_e {
    RIG_OK = 0,     /*!< 0 No error, operation completed successfully */
    RIG_EINVAL,     /*!< 1 invalid parameter */
    RIG_ECONF,      /*!< 2 invalid configuration (serial,..) */
    RIG_ENOMEM,     /*!< 3 memory shortage */
    RIG_ENIMPL,     /*!< 4 function not implemented, but will be */
    RIG_ETIMEOUT,   /*!< 5 communication timed out */
    RIG_EIO,        /*!< 6 IO error, including open failed */
    RIG_EINTERNAL,  /*!< 7 Internal Hamlib error, huh! */
    RIG_EPROTO,     /*!< 8 Protocol error */
    RIG_ERJCTED,    /*!< 9 Command rejected by the rig */
    RIG_ETRUNC,     /*!< 10 Command performed, but arg truncated */
    RIG_ENAVAIL,    /*!< 11 Function not available */
    RIG_ENTARGET,   /*!< 12 VFO not targetable */
    RIG_BUSERROR,   /*!< 13 Error talking on the bus */
    RIG_BUSBUSY,    /*!< 14 Collision on the bus */
    RIG_EARG,       /*!< 15 NULL RIG handle or any invalid pointer parameter in get arg */
    RIG_EVFO,       /*!< 16 Invalid VFO */
    RIG_EDOM        /*!< 17 Argument out of domain of func */
};

/**
 * \brief Determines if the given error code indicates a "soft" error
 * Soft errors are caused by invalid parameters and software/hardware features
 * and cannot be fixed by retries or by re-initializing hardware.
 */
#define RIG_IS_SOFT_ERRCODE(errcode) (errcode == RIG_EINVAL || errcode == RIG_ENIMPL || errcode == RIG_ERJCTED \
    || errcode == RIG_ETRUNC || errcode == RIG_ENAVAIL || errcode == RIG_ENTARGET \
    || errcode == RIG_EVFO || errcode == RIG_EDOM)

/**
 * \brief Token in the netrigctl protocol for returning error code
 */
#define NETRIGCTL_RET "RPRT "


/**
 *\brief Hamlib debug levels
 *
 * NOTE: Numeric order matters for debug level
 *
 * \sa rig_set_debug()
 */
enum rig_debug_level_e {
    RIG_DEBUG_NONE = 0, /*!< no bug reporting */
    RIG_DEBUG_BUG,      /*!< serious bug */
    RIG_DEBUG_ERR,      /*!< error case (e.g. protocol, memory allocation) */
    RIG_DEBUG_WARN,     /*!< warning */
    RIG_DEBUG_VERBOSE,  /*!< verbose */
    RIG_DEBUG_TRACE,    /*!< tracing */
    RIG_DEBUG_CACHE     /*!< caching */
};


/* --------------- Rig capabilities -----------------*/

/* Forward struct references */

struct rig;
struct rig_state;

/**
 * \brief Rig structure definition (see rig for details).
 */
typedef struct s_rig RIG;

//! @cond Doxygen_Suppress
#define HAMLIB_RIGNAMSIZ 30
#define HAMLIB_RIGVERSIZ 8
#define HAMLIB_FILPATHLEN 512
#define HAMLIB_FRQRANGESIZ 30
#define HAMLIB_MAXCHANDESC 30      /* describe channel eg: "WWV 5Mhz" */
#define HAMLIB_TSLSTSIZ 20         /* max tuning step list size, zero ended */
#define HAMLIB_FLTLSTSIZ 60        /* max mode/filter list size, zero ended */
#define HAMLIB_MAXDBLSTSIZ 8       /* max preamp/att levels supported, zero ended */
#define HAMLIB_CHANLSTSIZ 16       /* max mem_list size, zero ended */
#define HAMLIB_MAX_CAL_LENGTH 32   /* max calibration plots in cal_table_t */
#define HAMLIB_MAX_MODES 63
#define HAMLIB_MAX_VFOS 31
#define HAMLIB_MAX_ROTORS 63
#define HAMLIB_MAX_VFO_OPS 31
#define HAMLIB_MAX_RSCANS 31
//! @endcond


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
#define CTCSS_LIST_SIZE 60
#define DCS_LIST_SIZE 128
typedef unsigned int tone_t;


/**
 * \brief Port type
 */
typedef enum rig_port_e {
    RIG_PORT_NONE = 0,      /*!< No port */
    RIG_PORT_SERIAL,        /*!< Serial */
    RIG_PORT_NETWORK,       /*!< Network socket type */
    RIG_PORT_DEVICE,        /*!< Device driver, like the WiNRADiO */
    RIG_PORT_PACKET,        /*!< AX.25 network type, e.g. SV8CS protocol */
    RIG_PORT_DTMF,          /*!< DTMF protocol bridge via another rig, eg. Kenwood Sky Cmd System */
    RIG_PORT_ULTRA,         /*!< IrDA Ultra protocol! */
    RIG_PORT_RPC,           /*!< RPC wrapper */
    RIG_PORT_PARALLEL,      /*!< Parallel port */
    RIG_PORT_USB,           /*!< USB port */
    RIG_PORT_UDP_NETWORK,   /*!< UDP Network socket type */
    RIG_PORT_CM108,         /*!< CM108 GPIO */
    RIG_PORT_GPIO,          /*!< GPIO */
    RIG_PORT_GPION,         /*!< GPIO inverted */
} rig_port_t;


/**
 * \brief Serial parity
 */
enum serial_parity_e {
    RIG_PARITY_NONE = 0,    /*!< No parity */
    RIG_PARITY_ODD,         /*!< Odd */
    RIG_PARITY_EVEN,        /*!< Even */
    RIG_PARITY_MARK,        /*!< Mark */
    RIG_PARITY_SPACE        /*!< Space */
};


/**
 * \brief Serial handshake
 */
enum serial_handshake_e {
    RIG_HANDSHAKE_NONE = 0, /*!< No handshake */
    RIG_HANDSHAKE_XONXOFF,  /*!< Software XON/XOFF */
    RIG_HANDSHAKE_HARDWARE  /*!< Hardware CTS/RTS */
};


/**
 * \brief Serial control state
 */
enum serial_control_state_e {
    RIG_SIGNAL_UNSET = 0,   /*!< Unset or tri-state */
    RIG_SIGNAL_ON,          /*!< ON */
    RIG_SIGNAL_OFF          /*!< OFF */
};


/**
 * \brief Rig type flags
 */
typedef enum {
    RIG_FLAG_RECEIVER = (1 << 1),       /*!< Receiver */
    RIG_FLAG_TRANSMITTER = (1 << 2),    /*!< Transmitter */
    RIG_FLAG_SCANNER = (1 << 3),        /*!< Scanner */
    RIG_FLAG_MOBILE = (1 << 4),         /*!< mobile sized */
    RIG_FLAG_HANDHELD = (1 << 5),       /*!< handheld sized */
    RIG_FLAG_COMPUTER = (1 << 6),       /*!< "Computer" rig */
    RIG_FLAG_TRUNKING = (1 << 7),       /*!< has trunking */
    RIG_FLAG_APRS = (1 << 8),           /*!< has APRS */
    RIG_FLAG_TNC = (1 << 9),            /*!< has TNC */
    RIG_FLAG_DXCLUSTER = (1 << 10),     /*!< has DXCluster */
    RIG_FLAG_TUNER = (1 << 11)          /*!< dumb tuner */
} rig_type_t;

//! @cond Doxygen_Suppress
#define RIG_FLAG_TRANSCEIVER (RIG_FLAG_RECEIVER|RIG_FLAG_TRANSMITTER)
#define RIG_TYPE_MASK (RIG_FLAG_TRANSCEIVER|RIG_FLAG_SCANNER|RIG_FLAG_MOBILE|RIG_FLAG_HANDHELD|RIG_FLAG_COMPUTER|RIG_FLAG_TRUNKING|RIG_FLAG_TUNER)

#define RIG_TYPE_OTHER      0
#define RIG_TYPE_TRANSCEIVER    RIG_FLAG_TRANSCEIVER
#define RIG_TYPE_HANDHELD       (RIG_FLAG_TRANSCEIVER|RIG_FLAG_HANDHELD)
#define RIG_TYPE_MOBILE         (RIG_FLAG_TRANSCEIVER|RIG_FLAG_MOBILE)
#define RIG_TYPE_RECEIVER       RIG_FLAG_RECEIVER
#define RIG_TYPE_PCRECEIVER     (RIG_FLAG_COMPUTER|RIG_FLAG_RECEIVER)
#define RIG_TYPE_SCANNER        (RIG_FLAG_SCANNER|RIG_FLAG_RECEIVER)
#define RIG_TYPE_TRUNKSCANNER   (RIG_TYPE_SCANNER|RIG_FLAG_TRUNKING)
#define RIG_TYPE_COMPUTER       (RIG_FLAG_TRANSCEIVER|RIG_FLAG_COMPUTER)
#define RIG_TYPE_TUNER          RIG_FLAG_TUNER
//! @endcond


/**
 * \brief Development status of the backend
 */
enum rig_status_e {
    RIG_STATUS_ALPHA = 0,   /*!< Alpha quality, i.e. development */
    RIG_STATUS_UNTESTED,    /*!< Written from available specs, rig unavailable for test, feedback wanted! */
    RIG_STATUS_BETA,        /*!< Beta quality */
    RIG_STATUS_STABLE,      /*!< Stable */
    RIG_STATUS_BUGGY        /*!< Was stable, but something broke it! */
    /* RIG_STATUS_NEW  *     *!< Initial release of code
     * !! Use of RIG_STATUS_NEW is deprecated. Do not use it anymore */
};

/**
 * \brief Map all deprecated RIG_STATUS_NEW references to
 *        RIG_STATUS_UNTESTED for backward compatibility
 */
#define RIG_STATUS_NEW  RIG_STATUS_UNTESTED


/**
 * \brief Repeater shift type
 */
typedef enum {
    RIG_RPT_SHIFT_NONE = 0, /*!< No repeater shift */
    RIG_RPT_SHIFT_MINUS,    /*!< "-" shift */
    RIG_RPT_SHIFT_PLUS      /*!< "+" shift */
} rptr_shift_t;


/**
 * \brief Split mode
 */
typedef enum {
    RIG_SPLIT_OFF = 0,  /*!< Split mode disabled */
    RIG_SPLIT_ON        /*!< Split mode enabled */
} split_t;


/**
 * \brief Frequency type,
 *
 * Frequency type unit in Hz, able to hold SHF frequencies.
 */
typedef double freq_t;

/**
 * \brief printf(3) format to be used for freq_t type
 */
#define PRIfreq ".0f"

/**
 * \brief scanf(3) format to be used for freq_t type
 */
#define SCNfreq "lf"
/**
 * \brief printf(3) format to be used for freq_t type
 */
#define FREQFMT SCNfreq


/**
 * \brief Short frequency type
 *
 * Frequency in Hz restricted to 31bits, suitable for offsets, shifts, etc..
 */
typedef signed long shortfreq_t;

/** \brief \c Macro to return Hz when f=Hz  */
#define Hz(f)   ((freq_t)(f))
/** \brief \c Macro to return Hz when f=kHz  */
#define kHz(f)  ((freq_t)((f)*(freq_t)1000))
/** \brief \c Macro to return Hz when f=MHz  */
#define MHz(f)  ((freq_t)((f)*(freq_t)1000000))
/** \brief \c Macro to return Hz when f=GHz  */
#define GHz(f)  ((freq_t)((f)*(freq_t)1000000000))

/** \brief \c Macro to return short Hz when f=Hz  */
#define s_Hz(f)     ((shortfreq_t)(f))
/** \brief \c Macro to return short Hz when f=kHz  */
#define s_kHz(f)    ((shortfreq_t)((f)*(shortfreq_t)1000))
/** \brief \c Macro to return short Hz when f=MHz  */
#define s_MHz(f)    ((shortfreq_t)((f)*(shortfreq_t)1000000))
/** \brief \c Macro to return short Hz when f=GHz  */
#define s_GHz(f)    ((shortfreq_t)((f)*(shortfreq_t)1000000000))

/** \brief \c Frequency none -- used as default value for checking  */
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
 */
typedef unsigned int vfo_t;

/** \brief '' -- used in caps */

#define RIG_VFO_N(n)        ((vfo_t)(1u<<(n)))

/** \brief \c VFONone -- vfo unknown */
#define RIG_VFO_NONE        0

/** \brief \c VFOA -- VFO A */
#define RIG_VFO_A           RIG_VFO_N(0)

/** \brief \c VFOB -- VFO B */
#define RIG_VFO_B           RIG_VFO_N(1)

/** \brief \c VFOC -- VFO C */
#define RIG_VFO_C           RIG_VFO_N(2)

// Any addition VFOS need to go from 3-20
// To maintain backward compatibility these values cannot change

/** \brief \c SubA -- alias for SUB_A */
#define RIG_VFO_SUB_A       RIG_VFO_N(21)

/** \brief \c SubB -- alias for SUB_B */
#define RIG_VFO_SUB_B       RIG_VFO_N(22)

/** \brief \c SubC -- alias for SUB_B */
#define RIG_VFO_SUB_C       RIG_VFO_N(3)

/** \brief \c MainA -- alias for MAIN_A */
#define RIG_VFO_MAIN_A      RIG_VFO_N(23)

/** \brief \c MainB -- alias for MAIN_B */
#define RIG_VFO_MAIN_B      RIG_VFO_N(24)

/** \brief \c MainC -- alias for MAIN_C */
#define RIG_VFO_MAIN_C      RIG_VFO_N(4)

/** \brief \c Sub -- alias for SUB */
#define RIG_VFO_SUB         RIG_VFO_N(25)

/** \brief \c Main -- alias for MAIN */
#define RIG_VFO_MAIN        RIG_VFO_N(26)

/** \brief \c VFO -- means (last or any)VFO mode, with set_vfo */
#define RIG_VFO_VFO         RIG_VFO_N(27)

/** \brief \c MEM -- means Memory mode, to be used with set_vfo */
#define RIG_VFO_MEM         RIG_VFO_N(28)

/** \brief \c currVFO -- current "tunable channel"/VFO */
#define RIG_VFO_CURR        RIG_VFO_N(29)

/** \brief \c Flag to set if VFO can transmit */
#define RIG_VFO_TX_FLAG     RIG_VFO_N(30)

/** \brief \c Flag to set all VFOS */
#define RIG_VFO_ALL     RIG_VFO_N(31)

// we can also use RIG_VFO_N(31) if needed

// Misc VFO Macros

/** \brief \c Macro to tell you if VFO can transmit */
#define RIG_VFO_TX_VFO(v)   ((v)|RIG_VFO_TX_FLAG)

/** \brief \c TX -- alias for split tx or uplink, of VFO_CURR  */
#define RIG_VFO_TX          RIG_VFO_TX_VFO(RIG_VFO_CURR)

/** \brief \c RX -- alias for split rx or downlink */
#define RIG_VFO_RX          RIG_VFO_CURR


/*
 * targetable bitfields, for internal use.
 * In rig.c lack of a flag will case a VFO change if needed
 * So setting this flag will mean the backend handles any VFO needs
 * For many rigs RITXIT, PTT, MEM, and BANK are non-VFO commands so need these flags to avoid unnecessary VFO swapping
 */
//! @cond Doxygen_Suppress
#define RIG_TARGETABLE_NONE 0
#define RIG_TARGETABLE_FREQ (1<<0)
#define RIG_TARGETABLE_MODE (1<<1) // mode by vfo or same mode on both vfos
#define RIG_TARGETABLE_PURE (1<<2) // deprecated -- not used -- reuse it
#define RIG_TARGETABLE_TONE (1<<3)
#define RIG_TARGETABLE_FUNC (1<<4)
#define RIG_TARGETABLE_LEVEL (1<<5)
#define RIG_TARGETABLE_RITXIT (1<<6)
#define RIG_TARGETABLE_PTT (1<<7)
#define RIG_TARGETABLE_MEM (1<<8)
#define RIG_TARGETABLE_BANK (1<<9)
#define RIG_TARGETABLE_ANT (1<<10)
#define RIG_TARGETABLE_ROOFING (1<<11) // roofing filter targetable by VFO
#define RIG_TARGETABLE_COMMON (RIG_TARGETABLE_RITXIT | RIG_TARGETABLE_PTT | RIG_TARGETABLE_MEM | RIG_TARGETABLE_BANK)
#define RIG_TARGETABLE_ALL  0x7fffffff
//! @endcond
//
//
// Newer Icoms like the 9700 and 910 have VFOA/B on both Main & Sub
// Compared to older rigs which have one or the other
// So we need to distinguish between them
//! @cond Doxygen_Suppress
#define VFO_HAS_A_B ((rig->state.vfo_list & (RIG_VFO_A|RIG_VFO_B)) == (RIG_VFO_A|RIG_VFO_B))
#define VFO_HAS_MAIN_SUB ((rig->state.vfo_list & (RIG_VFO_MAIN|RIG_VFO_SUB)) == (RIG_VFO_MAIN|RIG_VFO_SUB))
#define VFO_HAS_MAIN_SUB_ONLY ((!VFO_HAS_A_B) & VFO_HAS_MAIN_SUB)
#define VFO_HAS_MAIN_SUB_A_B_ONLY (VFO_HAS_A_B & VFO_HAS_MAIN_SUB)
#define VFO_HAS_A_B_ONLY (VFO_HAS_A_B & (!VFO_HAS_MAIN_SUB))
#define VFO_DUAL (RIG_VFO_MAIN_A|RIG_VFO_MAIN_B|RIG_VFO_SUB_A|RIG_VFO_SUB_B)
#define VFO_HAS_DUAL ((rig->state.vfo_list & VFO_DUAL) == VFO_DUAL)
//! @endcond

/**
 * \brief Macro for bandpass to be set to normal
 * \def RIG_PASSBAND_NORMAL
 */
#define RIG_PASSBAND_NORMAL     s_Hz(0)

/**
 * \brief Macro for bandpass to be left alone
 */
#define RIG_PASSBAND_NOCHANGE   s_Hz(-1)

/**
 *
 * \sa rig_passband_normal(), rig_passband_narrow(), rig_passband_wide()
 */
typedef shortfreq_t pbwidth_t;


/**
 * \brief DCD status
 */
typedef enum dcd_e {
    RIG_DCD_OFF = 0,    /*!< Squelch closed */
    RIG_DCD_ON          /*!< Squelch open */
} dcd_t;


/**
 * \brief DCD type
 *
 * \sa rig_get_dcd()
 */
typedef enum {
    RIG_DCD_NONE = 0,   /*!< No DCD available */
    RIG_DCD_RIG,        /*!< Rig has DCD status support, i.e. rig has get_dcd cap */
    RIG_DCD_SERIAL_DSR, /*!< DCD status from serial DSR signal */
    RIG_DCD_SERIAL_CTS, /*!< DCD status from serial CTS signal */
    RIG_DCD_SERIAL_CAR, /*!< DCD status from serial CD signal */
    RIG_DCD_PARALLEL,   /*!< DCD status from parallel port pin */
    RIG_DCD_CM108,      /*!< DCD status from CM108 vol dn pin */
    RIG_DCD_GPIO,       /*!< DCD status from GPIO pin */
    RIG_DCD_GPION,      /*!< DCD status from inverted GPIO pin */
} dcd_type_t;


/**
 * \brief PTT status
 */
typedef enum {
    RIG_PTT_OFF = 0,    /*!< PTT deactivated */
    RIG_PTT_ON,         /*!< PTT activated */
    RIG_PTT_ON_MIC,     /*!< PTT Mic only, fallbacks on RIG_PTT_ON if unavailable */
    RIG_PTT_ON_DATA     /*!< PTT Data (Mic-muted), fallbacks on RIG_PTT_ON if unavailable */
} ptt_t;


/**
 * \brief PTT type
 *
 * \sa rig_get_ptt()
 */
typedef enum {
    RIG_PTT_NONE = 0,       /*!< No PTT available */
    RIG_PTT_RIG,            /*!< Legacy PTT (CAT PTT) */
    RIG_PTT_SERIAL_DTR,     /*!< PTT control through serial DTR signal */
    RIG_PTT_SERIAL_RTS,     /*!< PTT control through serial RTS signal */
    RIG_PTT_PARALLEL,       /*!< PTT control through parallel port */
    RIG_PTT_RIG_MICDATA,    /*!< Legacy PTT (CAT PTT), supports RIG_PTT_ON_MIC/RIG_PTT_ON_DATA */
    RIG_PTT_CM108,          /*!< PTT control through CM108 GPIO pin */
    RIG_PTT_GPIO,           /*!< PTT control through GPIO pin */
    RIG_PTT_GPION,          /*!< PTT control through inverted GPIO pin */
} ptt_type_t;


/**
 * \brief Radio power state
 */
typedef enum {
    RIG_POWER_OFF =     0,          /*!< Power off */
    RIG_POWER_ON =      (1 << 0),   /*!< Power on */
    RIG_POWER_STANDBY = (1 << 1),   /*!< Standby */
    RIG_POWER_OPERATE = (1 << 2),   /*!< Operate (from Standby) */
    RIG_POWER_UNKNOWN = (1 << 3)    /*!< Unknown power status */
} powerstat_t;


/**
 * \brief Reset operation
 */
typedef enum {
    RIG_RESET_NONE =    0,          /*!< No reset */
    RIG_RESET_SOFT =    (1 << 0),   /*!< Software reset */
    RIG_RESET_VFO =     (1 << 1),   /*!< VFO reset */
    RIG_RESET_MCALL =   (1 << 2),   /*!< Memory clear */
    RIG_RESET_MASTER =  (1 << 3)    /*!< Master reset */
} reset_t;


/**
 * \brief VFO operation
 *
 * A VFO operation is an action on a VFO (or tunable memory).
 * The difference with a function is that an action has no on/off
 * status, it is performed at once.
 *
 * NOTE: the vfo argument for some vfo operation may be irrelevant,
 * and thus will be ignored.
 *
 * The VFO/MEM "mode" is set by rig_set_vfo.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_vfo_op(), rig_strvfop()
 */
typedef enum {
    RIG_OP_NONE =       0,          /*!< '' No VFO_OP */
    RIG_OP_CPY =        (1 << 0),   /*!< \c CPY -- VFO A = VFO B */
    RIG_OP_XCHG =       (1 << 1),   /*!< \c XCHG -- Exchange VFO A/B */
    RIG_OP_FROM_VFO =   (1 << 2),   /*!< \c FROM_VFO -- VFO->MEM */
    RIG_OP_TO_VFO =     (1 << 3),   /*!< \c TO_VFO -- MEM->VFO */
    RIG_OP_MCL =        (1 << 4),   /*!< \c MCL -- Memory clear */
    RIG_OP_UP =         (1 << 5),   /*!< \c UP -- UP increment VFO freq by tuning step*/
    RIG_OP_DOWN =       (1 << 6),   /*!< \c DOWN -- DOWN decrement VFO freq by tuning step*/
    RIG_OP_BAND_UP =    (1 << 7),   /*!< \c BAND_UP -- Band UP */
    RIG_OP_BAND_DOWN =  (1 << 8),   /*!< \c BAND_DOWN -- Band DOWN */
    RIG_OP_LEFT =       (1 << 9),   /*!< \c LEFT -- LEFT */
    RIG_OP_RIGHT =      (1 << 10),  /*!< \c RIGHT -- RIGHT */
    RIG_OP_TUNE =       (1 << 11),  /*!< \c TUNE -- Start tune */
    RIG_OP_TOGGLE =     (1 << 12)   /*!< \c TOGGLE -- Toggle VFOA and VFOB */
} vfo_op_t;


/**
 * \brief Rig Scan operation
 *
 * Various scan operations supported by a rig.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_scan(), rig_strscan()
 */
typedef enum {
    RIG_SCAN_NONE =     0,          /*!< '' No-op value */
    RIG_SCAN_MEM =      (1 << 0),   /*!< \c MEM -- Scan all memory channels */
    RIG_SCAN_SLCT =     (1 << 1),   /*!< \c SLCT -- Scan all selected memory channels */
    RIG_SCAN_PRIO =     (1 << 2),   /*!< \c PRIO -- Priority watch (mem or call channel) */
    RIG_SCAN_PROG =     (1 << 3),   /*!< \c PROG -- Programmed(edge) scan */
    RIG_SCAN_DELTA =    (1 << 4),   /*!< \c DELTA -- delta-f scan */
    RIG_SCAN_VFO =      (1 << 5),   /*!< \c VFO -- most basic scan */
    RIG_SCAN_PLT =      (1 << 6),   /*!< \c PLT -- Scan using pipelined tuning */
    RIG_SCAN_STOP =     (1 << 7)    /*!< \c STOP -- Stop scanning */
} scan_t;


/**
 * \brief configuration token
 */
typedef long token_t;


//! @cond Doxygen_Suppress
#define RIG_CONF_END 0
//! @endcond


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
 *   BINARY: val.b
 */

/* strongly inspired from soundmodem. Thanks Thomas! */
enum rig_conf_e {
    RIG_CONF_STRING,        /*!<    String type */
    RIG_CONF_COMBO,         /*!<    Combo type */
    RIG_CONF_NUMERIC,       /*!<    Numeric type integer or real */
    RIG_CONF_CHECKBUTTON,   /*!<    on/off type */
    RIG_CONF_BUTTON,        /*!<    Button type */
    RIG_CONF_BINARY         /*!<    Binary buffer type */
};

//! @cond Doxygen_Suppress
#define RIG_COMBO_MAX   16
#define RIG_BIN_MAX  80
//! @endcond

/**
 * \brief Configuration parameter structure.
 */
struct confparams {
    token_t token;          /*!< Conf param token ID */
    const char *name;       /*!< Param name, no spaces allowed */
    const char *label;      /*!< Human readable label */
    const char *tooltip;    /*!< Hint on the parameter */
    const char *dflt;       /*!< Default value */
    enum rig_conf_e type;   /*!< Type of the parameter */
    union {                 /*!< */
        struct {            /*!< */
            float min;      /*!< Minimum value */
            float max;      /*!< Maximum value */
            float step;     /*!< Step */
        } n;                /*!< Numeric type */
        struct {            /*!< */
            const char *combostr[RIG_COMBO_MAX];    /*!< Combo list */
        } c;                /*!< Combo type */
    } u;                    /*!< Type union */
};


/**
 * \brief Announce
 *
 * Designate optional speech synthesizer.
 */
typedef enum {
    RIG_ANN_NONE =      0,              /*!< None */
    RIG_ANN_OFF =       RIG_ANN_NONE,   /*!< disable announces */
    RIG_ANN_FREQ =      (1 << 0),       /*!< Announce frequency */
    RIG_ANN_RXMODE =    (1 << 1),       /*!< Announce receive mode */
    RIG_ANN_CW =        (1 << 2),       /*!< CW */
    RIG_ANN_ENG =       (1 << 3),       /*!< English */
    RIG_ANN_JAP =       (1 << 4)        /*!< Japan */
} ann_t;


/**
 * \brief Antenna typedef
 * \typedef ant_t
 */
/**
 * \brief Antenna number
 * \def RIG_ANT_NONE
 * No antenna set yet or unknown 
 */
/**
 * \brief Antenna conversion macro
 * \def RIG_ANT_N
 * Convert antenna number to bit mask
 */
/**
 * \brief Macro for Ant#1
 * \def RIG_ANT_1
 */
/**
 * \brief Macro for Ant#2
 * \def RIG_ANT_2
 */
/**
 * \brief Macro for Ant#3
 * \def RIG_ANT_3
 */
/**
 * \brief Macro for Ant#4
 * \def RIG_ANT_4
 */
/**
 * \brief Macro for Ant#5
 * \def RIG_ANT_5
 */
/**
 * \brief Antenna is on whatever "current" means
 * \def RIG_ANT_CURR
 */
/**
 * \brief Macro for unknown antenna
 * \def RIG_ANT_UNKNOWN
 */
/**
 * \brief Maximum antenna#
 * \def RIG_ANT_MAX
 */
typedef unsigned int ant_t;

#define RIG_ANT_NONE    0
#define RIG_ANT_N(n)    ((ant_t)1<<(n))
#define RIG_ANT_1       RIG_ANT_N(0)
#define RIG_ANT_2       RIG_ANT_N(1)
#define RIG_ANT_3       RIG_ANT_N(2)
#define RIG_ANT_4       RIG_ANT_N(3)
#define RIG_ANT_5       RIG_ANT_N(4)

#define RIG_ANT_UNKNOWN RIG_ANT_N(30)
#define RIG_ANT_CURR    RIG_ANT_N(31)

#define RIG_ANT_MAX 32


/**
 * \brief AGC delay settings
 */
/* TODO: kill me, and replace by real AGC delay */
enum agc_level_e {
    RIG_AGC_OFF =       0,
    RIG_AGC_SUPERFAST,
    RIG_AGC_FAST,
    RIG_AGC_SLOW,
    RIG_AGC_USER,           /*!< user selectable */
    RIG_AGC_MEDIUM,
    RIG_AGC_AUTO
};

//! @cond Doxygen_Suppress
#define RIG_AGC_LAST RIG_AGC_AUTO
//! @endcond

/**
 * \brief Level display meters
 */
enum meter_level_e {
    RIG_METER_NONE =    0,          /*< No display meter */
    RIG_METER_SWR =     (1 << 0),   /*< Stationary Wave Ratio */
    RIG_METER_COMP =    (1 << 1),   /*< Compression level */
    RIG_METER_ALC =     (1 << 2),   /*< ALC */
    RIG_METER_IC =      (1 << 3),   /*< IC */
    RIG_METER_DB =      (1 << 4),   /*< DB */
    RIG_METER_PO =      (1 << 5),   /*< Power Out */
    RIG_METER_VDD =     (1 << 6),   /*< Final Amp Voltage */
    RIG_METER_TEMP =    (1 << 7)    /*< Final Amp Voltage */
};


/**
 * \brief Universal approach for passing values
 *
 * \sa rig_set_level(), rig_get_level(), rig_set_parm(), rig_get_parm()
 */
typedef union {
    signed int i;       /*!< Signed integer */
    float f;            /*!< Single precision float */
    char *s;            /*!< Pointer to char string */
    const char *cs;     /*!< Pointer to constant char string */
//! @cond Doxygen_Suppress
    struct {
        int l;          /*!< Length of data */
        unsigned char *d; /* Pointer to data buffer */
    } b;
//! @endcond
} value_t;


/**
 * \brief Rig Level Settings
 *
 * Various operating levels supported by a rig.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_level(), rig_strlevel()
 */
typedef uint64_t rig_level_e;
#define RIG_LEVEL_NONE       0              /*!< '' -- No Level */
#define RIG_LEVEL_PREAMP     CONSTANT_64BIT_FLAG(0)       /*!< \c PREAMP -- Preamp, arg int (dB) */
#define RIG_LEVEL_ATT        CONSTANT_64BIT_FLAG(1)       /*!< \c ATT -- Attenuator, arg int (dB) */
#define RIG_LEVEL_VOXDELAY   CONSTANT_64BIT_FLAG(2)       /*!< \c VOXDELAY -- VOX delay, arg int (tenth of seconds) */
#define RIG_LEVEL_AF         CONSTANT_64BIT_FLAG(3)       /*!< \c AF -- Volume, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_RF         CONSTANT_64BIT_FLAG(4)       /*!< \c RF -- RF gain (not TX power) arg float [0.0 ... 1.0] */
#define RIG_LEVEL_SQL        CONSTANT_64BIT_FLAG(5)       /*!< \c SQL -- Squelch, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_IF         CONSTANT_64BIT_FLAG(6)       /*!< \c IF -- IF, arg int (Hz) */
#define RIG_LEVEL_APF        CONSTANT_64BIT_FLAG(7)       /*!< \c APF -- Audio Peak Filter, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_NR         CONSTANT_64BIT_FLAG(8)       /*!< \c NR -- Noise Reduction, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_PBT_IN     CONSTANT_64BIT_FLAG(9)       /*!< \c PBT_IN -- Twin PBT (inside) arg float [0.0 ... 1.0] */
#define RIG_LEVEL_PBT_OUT    CONSTANT_64BIT_FLAG(10)      /*!< \c PBT_OUT -- Twin PBT (outside) arg float [0.0 ... 1.0] */
#define RIG_LEVEL_CWPITCH    CONSTANT_64BIT_FLAG(11)      /*!< \c CWPITCH -- CW pitch, arg int (Hz) */
#define RIG_LEVEL_RFPOWER    CONSTANT_64BIT_FLAG(12)      /*!< \c RFPOWER -- RF Power, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_MICGAIN    CONSTANT_64BIT_FLAG(13)      /*!< \c MICGAIN -- MIC Gain, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_KEYSPD     CONSTANT_64BIT_FLAG(14)      /*!< \c KEYSPD -- Key Speed, arg int (WPM) */
#define RIG_LEVEL_NOTCHF     CONSTANT_64BIT_FLAG(15)      /*!< \c NOTCHF -- Notch Freq., arg int (Hz) */
#define RIG_LEVEL_COMP       CONSTANT_64BIT_FLAG(16)      /*!< \c COMP -- Compressor, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_AGC        CONSTANT_64BIT_FLAG(17)      /*!< \c AGC -- AGC, arg int (see enum agc_level_e) */
#define RIG_LEVEL_BKINDL     CONSTANT_64BIT_FLAG(18)      /*!< \c BKINDL -- BKin Delay, arg int (tenth of dots) */
#define RIG_LEVEL_BALANCE    CONSTANT_64BIT_FLAG(19)      /*!< \c BAL -- Balance (Dual Watch) arg float [0.0 ... 1.0] */
#define RIG_LEVEL_METER      CONSTANT_64BIT_FLAG(20)      /*!< \c METER -- Display meter, arg int (see enum meter_level_e) */
#define RIG_LEVEL_VOXGAIN    CONSTANT_64BIT_FLAG(21)      /*!< \c VOXGAIN -- VOX gain level, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_ANTIVOX    CONSTANT_64BIT_FLAG(22)      /*!< \c ANTIVOX -- anti-VOX level, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_SLOPE_LOW  CONSTANT_64BIT_FLAG(23)      /*!< \c SLOPE_LOW -- Slope tune, low frequency cut, arg int (Hz) */
#define RIG_LEVEL_SLOPE_HIGH CONSTANT_64BIT_FLAG(24)      /*!< \c SLOPE_HIGH -- Slope tune, high frequency cut, arg int (Hz) */
#define RIG_LEVEL_BKIN_DLYMS CONSTANT_64BIT_FLAG(25)      /*!< \c BKIN_DLYMS -- BKin Delay, arg int Milliseconds */

    /*!< These are not settable */
#define RIG_LEVEL_RAWSTR     CONSTANT_64BIT_FLAG(26)      /*!< \c RAWSTR -- Raw (A/D) value for signal strength, specific to each rig, arg int */
//#define RIG_LEVEL_SQLSTAT    CONSTANT_64BIT_FLAG(27)      /*!< \c SQLSTAT -- SQL status, arg int (open=1/closed=0). Deprecated, use get_dcd instead */
#define RIG_LEVEL_SWR        CONSTANT_64BIT_FLAG(28)      /*!< \c SWR -- SWR, arg float [0.0 ... infinite] */
#define RIG_LEVEL_ALC        CONSTANT_64BIT_FLAG(29)      /*!< \c ALC -- ALC, arg float */
#define RIG_LEVEL_STRENGTH   CONSTANT_64BIT_FLAG(30)      /*!< \c STRENGTH -- Effective (calibrated) signal strength relative to S9, arg int (dB) */
    /* RIG_LEVEL_BWC        (1<<31) */        /*!< Bandwidth Control, arg int (Hz) */
#define RIG_LEVEL_RFPOWER_METER  CONSTANT_64BIT_FLAG(32)      /*!< \c RFPOWER_METER -- RF power output meter, arg float [0.0 ... 1.0] (percentage of maximum power) */
#define RIG_LEVEL_COMP_METER   CONSTANT_64BIT_FLAG(33)      /*!< \c COMP_METER -- Audio output level compression meter, arg float (dB) */
#define RIG_LEVEL_VD_METER     CONSTANT_64BIT_FLAG(34)      /*!< \c VD_METER -- Input voltage level meter, arg float (V, volts) */
#define RIG_LEVEL_ID_METER     CONSTANT_64BIT_FLAG(35)      /*!< \c ID_METER -- Current draw meter, arg float (A, amperes) */

#define RIG_LEVEL_NOTCHF_RAW   CONSTANT_64BIT_FLAG(36)      /*!< \c NOTCHF_RAW -- Notch Freq., arg float [0.0 ... 1.0] */
#define RIG_LEVEL_MONITOR_GAIN CONSTANT_64BIT_FLAG(37)      /*!< \c MONITOR_GAIN -- Monitor gain (level for monitoring of transmitted audio) arg float [0.0 ... 1.0] */
#define RIG_LEVEL_NB           CONSTANT_64BIT_FLAG(38)      /*!< \c NB -- Noise Blanker level, arg float [0.0 ... 1.0] */
#define RIG_LEVEL_RFPOWER_METER_WATTS  CONSTANT_64BIT_FLAG(39)      /*!< \c RFPOWER_METER_WATTS -- RF power output meter, arg float [0.0 ... MAX] (output power in watts) */
#define RIG_LEVEL_40           CONSTANT_64BIT_FLAG(40)      /*!< \c Future use */
#define RIG_LEVEL_41           CONSTANT_64BIT_FLAG(41)      /*!< \c Future use */
#define RIG_LEVEL_42           CONSTANT_64BIT_FLAG(42)      /*!< \c Future use */
#define RIG_LEVEL_43           CONSTANT_64BIT_FLAG(43)      /*!< \c Future use */
#define RIG_LEVEL_44           CONSTANT_64BIT_FLAG(44)      /*!< \c Future use */
#define RIG_LEVEL_45           CONSTANT_64BIT_FLAG(45)      /*!< \c Future use */
#define RIG_LEVEL_46           CONSTANT_64BIT_FLAG(46)      /*!< \c Future use */
#define RIG_LEVEL_47           CONSTANT_64BIT_FLAG(47)      /*!< \c Future use */
#define RIG_LEVEL_48           CONSTANT_64BIT_FLAG(48)      /*!< \c Future use */
#define RIG_LEVEL_49           CONSTANT_64BIT_FLAG(49)      /*!< \c Future use */
#define RIG_LEVEL_50           CONSTANT_64BIT_FLAG(50)      /*!< \c Future use */
#define RIG_LEVEL_51           CONSTANT_64BIT_FLAG(51)      /*!< \c Future use */
#define RIG_LEVEL_52           CONSTANT_64BIT_FLAG(52)      /*!< \c Future use */
#define RIG_LEVEL_53           CONSTANT_64BIT_FLAG(53)      /*!< \c Future use */
#define RIG_LEVEL_54           CONSTANT_64BIT_FLAG(54)      /*!< \c Future use */
#define RIG_LEVEL_55           CONSTANT_64BIT_FLAG(55)      /*!< \c Future use */
#define RIG_LEVEL_56           CONSTANT_64BIT_FLAG(56)      /*!< \c Future use */
#define RIG_LEVEL_57           CONSTANT_64BIT_FLAG(57)      /*!< \c Future use */
#define RIG_LEVEL_58           CONSTANT_64BIT_FLAG(58)      /*!< \c Future use */
#define RIG_LEVEL_59           CONSTANT_64BIT_FLAG(59)      /*!< \c Future use */
#define RIG_LEVEL_60           CONSTANT_64BIT_FLAG(60)      /*!< \c Future use */
#define RIG_LEVEL_61           CONSTANT_64BIT_FLAG(61)      /*!< \c Future use */
#define RIG_LEVEL_62           CONSTANT_64BIT_FLAG(62)      /*!< \c Future use */
#define RIG_LEVEL_63           CONSTANT_64BIT_FLAG(63)      /*!< \c Future use */

//! @cond Doxygen_Suppress
#define RIG_LEVEL_FLOAT_LIST (RIG_LEVEL_AF|RIG_LEVEL_RF|RIG_LEVEL_SQL|RIG_LEVEL_APF|RIG_LEVEL_NR|RIG_LEVEL_PBT_IN|RIG_LEVEL_PBT_OUT|RIG_LEVEL_RFPOWER|RIG_LEVEL_MICGAIN|RIG_LEVEL_COMP|RIG_LEVEL_BALANCE|RIG_LEVEL_SWR|RIG_LEVEL_ALC|RIG_LEVEL_VOXGAIN|RIG_LEVEL_ANTIVOX|RIG_LEVEL_RFPOWER_METER|RIG_LEVEL_RFPOWER_METER_WATTS|RIG_LEVEL_COMP_METER|RIG_LEVEL_VD_METER|RIG_LEVEL_ID_METER|RIG_LEVEL_NOTCHF_RAW|RIG_LEVEL_MONITOR_GAIN|RIG_LEVEL_NB)

#define RIG_LEVEL_READONLY_LIST (RIG_LEVEL_SWR|RIG_LEVEL_ALC|RIG_LEVEL_STRENGTH|RIG_LEVEL_RAWSTR|RIG_LEVEL_RFPOWER_METER|RIG_LEVEL_COMP_METER|RIG_LEVEL_VD_METER|RIG_LEVEL_ID_METER)

#define RIG_LEVEL_IS_FLOAT(l) ((l)&RIG_LEVEL_FLOAT_LIST)
#define RIG_LEVEL_SET(l) ((l)&~RIG_LEVEL_READONLY_LIST)
//! @endcond


/**
 * \brief Rig Parameters
 *
 * Parameters are settings that are not VFO specific.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_parm(), rig_strparm()
 */
enum rig_parm_e {
    RIG_PARM_NONE =         0,          /*!< '' -- No Parm */
    RIG_PARM_ANN =          (1 << 0),   /*!< \c ANN -- "Announce" level, see ann_t */
    RIG_PARM_APO =          (1 << 1),   /*!< \c APO -- Auto power off, int in minute */
    RIG_PARM_BACKLIGHT =    (1 << 2),   /*!< \c BACKLIGHT -- LCD light, float [0.0 ... 1.0] */
    RIG_PARM_BEEP =         (1 << 4),   /*!< \c BEEP -- Beep on keypressed, int (0,1) */
    RIG_PARM_TIME =         (1 << 5),   /*!< \c TIME -- hh:mm:ss, int in seconds from 00:00:00 */
    RIG_PARM_BAT =          (1 << 6),   /*!< \c BAT -- battery level, float [0.0 ... 1.0] */
    RIG_PARM_KEYLIGHT =     (1 << 7),   /*!< \c KEYLIGHT -- Button backlight, on/off */
    RIG_PARM_SCREENSAVER =  (1 << 8)    /*!< \c SCREENSAVER -- rig specific timeouts */
};

//! @cond Doxygen_Suppress
#define RIG_PARM_FLOAT_LIST (RIG_PARM_BACKLIGHT|RIG_PARM_BAT|RIG_PARM_KEYLIGHT)
#define RIG_PARM_READONLY_LIST (RIG_PARM_BAT)

#define RIG_PARM_IS_FLOAT(l) ((l)&RIG_PARM_FLOAT_LIST)
#define RIG_PARM_SET(l) ((l)&~RIG_PARM_READONLY_LIST)
//! @endcond

/**
 * \brief Setting
 *
 * This can be a func, a level or a parm.
 * Each bit designates one of them.
 */
typedef uint64_t setting_t;

/**
 * \brief Maximum # of rig settings
 *
 */
#define RIG_SETTING_MAX 64

/**
 * \brief Tranceive mode
 * The rig notifies the host of any event, like freq changed, mode changed, etc.
 * \def RIG_TRN_OFF
 * Turn it off
 * \brief Tranceive mode
 * \def RIG_TRN_RIG
 * RIG_TRN_RIG means the rig acts asynchrousnly
 * \brief Tranceive mode
 * \def RIG_TRN_POLL
 * RIG_TRN_POLL means we have to poll the rig
 *
 */
#define RIG_TRN_OFF 0
#define RIG_TRN_RIG 1
#define RIG_TRN_POLL 2


/**
 * \brief Rig Function Settings
 *
 * Various operating functions supported by a rig.\n
 * \c STRING used in rigctl/rigctld
 *
 * \sa rig_parse_func(), rig_strfunc()
 */
/*
 * The C standard dictates that an enum constant is a 32 bit signed integer.
 * Setting a constant's bit 31 created a negative value that on amd64 had the
 * upper 32 bits set as well when assigned to the misc.c:rig_func_str structure.
 * This caused misc.c:rig_strfunc() to fail its comparison for RIG_FUNC_XIT
 * on amd64 (x86_64).  To use bit 31 as an unsigned long, preprocessor macros
 * have been used instead as a 'const unsigned long' which cannot be used to
 * initialize the rig_func_str.func members.  TNX KA6MAL, AC6SL.  - N0NB
 */
#define RIG_FUNC_NONE       0                          /*!< '' -- No Function */
#define RIG_FUNC_FAGC       CONSTANT_64BIT_FLAG (0)    /*!< \c FAGC -- Fast AGC */
#define RIG_FUNC_NB         CONSTANT_64BIT_FLAG (1)    /*!< \c NB -- Noise Blanker */
#define RIG_FUNC_COMP       CONSTANT_64BIT_FLAG (2)    /*!< \c COMP -- Speech Compression */
#define RIG_FUNC_VOX        CONSTANT_64BIT_FLAG (3)    /*!< \c VOX -- Voice Operated Relay */
#define RIG_FUNC_TONE       CONSTANT_64BIT_FLAG (4)    /*!< \c TONE -- CTCSS Tone TX */
#define RIG_FUNC_TSQL       CONSTANT_64BIT_FLAG (5)    /*!< \c TSQL -- CTCSS Activate/De-activate RX */
#define RIG_FUNC_SBKIN      CONSTANT_64BIT_FLAG (6)    /*!< \c SBKIN -- Semi Break-in (CW mode) */
#define RIG_FUNC_FBKIN      CONSTANT_64BIT_FLAG (7)    /*!< \c FBKIN -- Full Break-in (CW mode) */
#define RIG_FUNC_ANF        CONSTANT_64BIT_FLAG (8)    /*!< \c ANF -- Automatic Notch Filter (DSP) */
#define RIG_FUNC_NR         CONSTANT_64BIT_FLAG (9)    /*!< \c NR -- Noise Reduction (DSP) */
#define RIG_FUNC_AIP        CONSTANT_64BIT_FLAG (10)   /*!< \c AIP -- RF pre-amp (AIP on Kenwood, IPO on Yaesu, etc.) */
#define RIG_FUNC_APF        CONSTANT_64BIT_FLAG (11)   /*!< \c APF -- Auto Passband/Audio Peak Filter */
#define RIG_FUNC_MON        CONSTANT_64BIT_FLAG (12)   /*!< \c MON -- Monitor transmitted signal */
#define RIG_FUNC_MN         CONSTANT_64BIT_FLAG (13)   /*!< \c MN -- Manual Notch */
#define RIG_FUNC_RF         CONSTANT_64BIT_FLAG (14)   /*!< \c RF -- RTTY Filter */
#define RIG_FUNC_ARO        CONSTANT_64BIT_FLAG (15)   /*!< \c ARO -- Auto Repeater Offset */
#define RIG_FUNC_LOCK       CONSTANT_64BIT_FLAG (16)   /*!< \c LOCK -- Lock */
#define RIG_FUNC_MUTE       CONSTANT_64BIT_FLAG (17)   /*!< \c MUTE -- Mute */
#define RIG_FUNC_VSC        CONSTANT_64BIT_FLAG (18)   /*!< \c VSC -- Voice Scan Control */
#define RIG_FUNC_REV        CONSTANT_64BIT_FLAG (19)   /*!< \c REV -- Reverse transmit and receive frequencies */
#define RIG_FUNC_SQL        CONSTANT_64BIT_FLAG (20)   /*!< \c SQL -- Turn Squelch Monitor on/off */
#define RIG_FUNC_ABM        CONSTANT_64BIT_FLAG (21)   /*!< \c ABM -- Auto Band Mode */
#define RIG_FUNC_BC         CONSTANT_64BIT_FLAG (22)   /*!< \c BC -- Beat Canceller */
#define RIG_FUNC_MBC        CONSTANT_64BIT_FLAG (23)   /*!< \c MBC -- Manual Beat Canceller */
#define RIG_FUNC_RIT        CONSTANT_64BIT_FLAG (24)   /*!< \c RIT -- Receiver Incremental Tuning */
#define RIG_FUNC_AFC        CONSTANT_64BIT_FLAG (25)   /*!< \c AFC -- Auto Frequency Control ON/OFF */
#define RIG_FUNC_SATMODE    CONSTANT_64BIT_FLAG (26)   /*!< \c SATMODE -- Satellite mode ON/OFF */
#define RIG_FUNC_SCOPE      CONSTANT_64BIT_FLAG (27)   /*!< \c SCOPE -- Simple bandscope ON/OFF */
#define RIG_FUNC_RESUME     CONSTANT_64BIT_FLAG (28)   /*!< \c RESUME -- Scan auto-resume */
#define RIG_FUNC_TBURST     CONSTANT_64BIT_FLAG (29)   /*!< \c TBURST -- 1750 Hz tone burst */
#define RIG_FUNC_TUNER      CONSTANT_64BIT_FLAG (30)   /*!< \c TUNER -- Enable automatic tuner */
#define RIG_FUNC_XIT        CONSTANT_64BIT_FLAG (31)   /*!< \c XIT -- Transmitter Incremental Tuning */
#ifndef SWIGLUAHIDE
/* Hide the top 32 bits from the old Lua binding as they can't be represented */
#define RIG_FUNC_NB2        CONSTANT_64BIT_FLAG (32)   /*!< \c NB2 -- 2nd Noise Blanker */
#define RIG_FUNC_CSQL       CONSTANT_64BIT_FLAG (33)   /*!< \c CSQL -- DCS Squelch setting */
#define RIG_FUNC_AFLT       CONSTANT_64BIT_FLAG (34)   /*!< \c AFLT -- AF Filter setting */
#define RIG_FUNC_ANL        CONSTANT_64BIT_FLAG (35)   /*!< \c ANL -- Noise limiter setting */
#define RIG_FUNC_BC2        CONSTANT_64BIT_FLAG (36)   /*!< \c BC2 -- 2nd Beat Cancel */
#define RIG_FUNC_DUAL_WATCH CONSTANT_64BIT_FLAG (37)   /*!< \c DUAL_WATCH -- Dual Watch / Sub Receiver */
#define RIG_FUNC_DIVERSITY  CONSTANT_64BIT_FLAG (38)   /*!< \c DIVERSITY -- Diversity receive */
#define RIG_FUNC_DSQL       CONSTANT_64BIT_FLAG (39)   /*!< \c DSQL -- Digital modes squelch */
#define RIG_FUNC_SCEN       CONSTANT_64BIT_FLAG (40)   /*!< \c SCEN -- scrambler/encryption */
#define RIG_FUNC_SLICE      CONSTANT_64BIT_FLAG (41)   /*!< \c Rig slice selection -- Flex */
#define RIG_FUNC_BIT42      CONSTANT_64BIT_FLAG (42)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT43      CONSTANT_64BIT_FLAG (43)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT44      CONSTANT_64BIT_FLAG (44)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT45      CONSTANT_64BIT_FLAG (45)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT46      CONSTANT_64BIT_FLAG (46)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT47      CONSTANT_64BIT_FLAG (47)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT48      CONSTANT_64BIT_FLAG (48)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT49      CONSTANT_64BIT_FLAG (49)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT50      CONSTANT_64BIT_FLAG (50)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT51      CONSTANT_64BIT_FLAG (51)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT52      CONSTANT_64BIT_FLAG (52)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT53      CONSTANT_64BIT_FLAG (53)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT54      CONSTANT_64BIT_FLAG (54)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT55      CONSTANT_64BIT_FLAG (55)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT56      CONSTANT_64BIT_FLAG (56)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT57      CONSTANT_64BIT_FLAG (57)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT58      CONSTANT_64BIT_FLAG (58)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT59      CONSTANT_64BIT_FLAG (59)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT60      CONSTANT_64BIT_FLAG (60)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT61      CONSTANT_64BIT_FLAG (61)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT62      CONSTANT_64BIT_FLAG (62)   /*!< \c available for future RIG_FUNC items */
#define RIG_FUNC_BIT63      CONSTANT_64BIT_FLAG (63)   /*!< \c available for future RIG_FUNC items */
/* 63 is this highest bit number that can be used */
#endif

/**
 * \brief power unit macros
 * \def mW
 * Converts a power level integer to milliwatts.  This is limited to 2
 * megawatts on 32 bit systems.
 */
#define mW(p)       ((int)(p))
/**
 * \brief power unit macros
 * \def Watts
 *
 * Converts a power level integer to watts.  This is limited to 2
 * gigawatts on 32 bit systems.
 */
#define Watts(p)    ((int)((p)*1000))
/**
 * \brief power unit macros
 * \def W
 *
 * Same as Watts for the person who is too lazy to type Watts :-)
 */
#define W(p)        Watts(p)
#if 0 // deprecating kW macro as this doesn't make sense
/**
 * \brief power unit macros
 * \def kW
 *
 * Same as Watts for the person who is too lazy to type Watts :-)
 */
#define kW(p)       ((int)((p)*1000000L))
#endif


/**
 * \brief Radio mode
 *
 * Various modes supported by a rig.\n
 * \c STRING used in rigctl
 *
 * \sa rig_parse_mode(), rig_strrmode()
 * TODO: Add new 8600 modes to rig2icom_mode() and icom2rig_mode() in frame.c
 */
typedef uint64_t rmode_t;

#define    RIG_MODE_NONE      0                         /*!< '' -- None */
#define    RIG_MODE_AM        CONSTANT_64BIT_FLAG (0)   /*!< \c AM -- Amplitude Modulation */
#define    RIG_MODE_CW        CONSTANT_64BIT_FLAG (1)   /*!< \c CW -- CW "normal" sideband */
#define    RIG_MODE_USB       CONSTANT_64BIT_FLAG (2)   /*!< \c USB -- Upper Side Band */
#define    RIG_MODE_LSB       CONSTANT_64BIT_FLAG (3)   /*!< \c LSB -- Lower Side Band */
#define    RIG_MODE_RTTY      CONSTANT_64BIT_FLAG (4)   /*!< \c RTTY -- Radio Teletype */
#define    RIG_MODE_FM        CONSTANT_64BIT_FLAG (5)   /*!< \c FM -- "narrow" band FM */
#define    RIG_MODE_WFM       CONSTANT_64BIT_FLAG (6)   /*!< \c WFM -- broadcast wide FM */
#define    RIG_MODE_CWR       CONSTANT_64BIT_FLAG (7)   /*!< \c CWR -- CW "reverse" sideband */
#define    RIG_MODE_RTTYR     CONSTANT_64BIT_FLAG (8)   /*!< \c RTTYR -- RTTY "reverse" sideband */
#define    RIG_MODE_AMS       CONSTANT_64BIT_FLAG (9)   /*!< \c AMS -- Amplitude Modulation Synchronous */
#define    RIG_MODE_PKTLSB    CONSTANT_64BIT_FLAG (10)  /*!< \c PKTLSB -- Packet/Digital LSB mode (dedicated port) */
#define    RIG_MODE_PKTUSB    CONSTANT_64BIT_FLAG (11)  /*!< \c PKTUSB -- Packet/Digital USB mode (dedicated port) */
#define    RIG_MODE_PKTFM     CONSTANT_64BIT_FLAG (12)  /*!< \c PKTFM -- Packet/Digital FM mode (dedicated port) */
#define    RIG_MODE_ECSSUSB   CONSTANT_64BIT_FLAG (13)  /*!< \c ECSSUSB -- Exalted Carrier Single Sideband USB */
#define    RIG_MODE_ECSSLSB   CONSTANT_64BIT_FLAG (14)  /*!< \c ECSSLSB -- Exalted Carrier Single Sideband LSB */
#define    RIG_MODE_FAX       CONSTANT_64BIT_FLAG (15)  /*!< \c FAX -- Facsimile Mode */
#define    RIG_MODE_SAM       CONSTANT_64BIT_FLAG (16)  /*!< \c SAM -- Synchronous AM double sideband */
#define    RIG_MODE_SAL       CONSTANT_64BIT_FLAG (17)  /*!< \c SAL -- Synchronous AM lower sideband */
#define    RIG_MODE_SAH       CONSTANT_64BIT_FLAG (18)  /*!< \c SAH -- Synchronous AM upper (higher) sideband */
#define    RIG_MODE_DSB       CONSTANT_64BIT_FLAG (19)  /*!< \c DSB -- Double sideband suppressed carrier */
#define    RIG_MODE_FMN       CONSTANT_64BIT_FLAG (21)  /*!< \c FMN -- FM Narrow Kenwood ts990s */
#define    RIG_MODE_PKTAM     CONSTANT_64BIT_FLAG (22)  /*!< \c PKTAM -- Packet/Digital AM mode e.g. IC7300 */
#define    RIG_MODE_P25       CONSTANT_64BIT_FLAG (23)  /*!< \c P25 -- APCO/P25 VHF,UHF digital mode IC-R8600 */
#define    RIG_MODE_DSTAR     CONSTANT_64BIT_FLAG (24)  /*!< \c D-Star -- VHF,UHF digital mode IC-R8600 */
#define    RIG_MODE_DPMR      CONSTANT_64BIT_FLAG (25)  /*!< \c dPMR -- digital PMR, VHF,UHF digital mode IC-R8600 */
#define    RIG_MODE_NXDNVN    CONSTANT_64BIT_FLAG (26)  /*!< \c NXDN-VN -- VHF,UHF digital mode IC-R8600 */
#define    RIG_MODE_NXDN_N    CONSTANT_64BIT_FLAG (27)  /*!< \c NXDN-N -- VHF,UHF digital mode IC-R8600 */
#define    RIG_MODE_DCR       CONSTANT_64BIT_FLAG (28)  /*!< \c DCR -- VHF,UHF digital mode IC-R8600 */
#define    RIG_MODE_AMN       CONSTANT_64BIT_FLAG (29)  /*!< \c AM-N -- Narrow band AM mode IC-R30 */
#define    RIG_MODE_PSK       CONSTANT_64BIT_FLAG (30)  /*!< \c PSK - Kenwood PSK and others */
#define    RIG_MODE_PSKR      CONSTANT_64BIT_FLAG (31)  /*!< \c PSKR - Kenwood PSKR and others */
#ifndef SWIGLUAHIDE
/* hide the top 32 bits from the Lua binding as they will not work */
#define    RIG_MODE_DD        CONSTANT_64BIT_FLAG (32)  /*!< \c DD Mode IC-9700 */
#define    RIG_MODE_C4FM      CONSTANT_64BIT_FLAG (33)  /*!< \c Yaesu C4FM mode */
#define    RIG_MODE_PKTFMN    CONSTANT_64BIT_FLAG (34)  /*!< \c Yaesu DATA-FM-N */
#define    RIG_MODE_SPEC      CONSTANT_64BIT_FLAG (35)  /*!< \c Unfiltered as in PowerSDR */
#define    RIG_MODE_BIT36     CONSTANT_64BIT_FLAG (36)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT37     CONSTANT_64BIT_FLAG (37)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT38     CONSTANT_64BIT_FLAG (38)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT39     CONSTANT_64BIT_FLAG (39)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT40     CONSTANT_64BIT_FLAG (40)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT41     CONSTANT_64BIT_FLAG (41)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT42     CONSTANT_64BIT_FLAG (42)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT43     CONSTANT_64BIT_FLAG (43)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT44     CONSTANT_64BIT_FLAG (44)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT45     CONSTANT_64BIT_FLAG (45)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT46     CONSTANT_64BIT_FLAG (46)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT47     CONSTANT_64BIT_FLAG (47)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT48     CONSTANT_64BIT_FLAG (48)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT49     CONSTANT_64BIT_FLAG (49)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT50     CONSTANT_64BIT_FLAG (50)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT51     CONSTANT_64BIT_FLAG (51)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT52     CONSTANT_64BIT_FLAG (52)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT53     CONSTANT_64BIT_FLAG (53)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT54     CONSTANT_64BIT_FLAG (54)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT55     CONSTANT_64BIT_FLAG (55)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT56     CONSTANT_64BIT_FLAG (56)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT57     CONSTANT_64BIT_FLAG (57)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT58     CONSTANT_64BIT_FLAG (58)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT59     CONSTANT_64BIT_FLAG (59)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT60     CONSTANT_64BIT_FLAG (60)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT61     CONSTANT_64BIT_FLAG (61)  /*!< \c reserved for future expansion */
#define    RIG_MODE_BIT62     CONSTANT_64BIT_FLAG (62)  /*!< \c reserved for future expansion */
#define    RIG_MODE_TESTS_MAX CONSTANT_64BIT_FLAG (63)  /*!< \c last bit available for 64-bit enum MUST ALWAYS BE LAST, Max Count for dumpcaps.c */
#endif

/**
 * \brief macro for backends, not to be used by rig_set_mode et al.
 */
#define RIG_MODE_SSB    (RIG_MODE_USB|RIG_MODE_LSB)

/**
 * \brief macro for backends, not to be used by rig_set_mode et al.
 */
#define RIG_MODE_ECSS   (RIG_MODE_ECSSUSB|RIG_MODE_ECSSLSB)

//! @cond Doxygen_Suppress
#define RIG_DBLST_END 0     /* end marker in a preamp/att level list */
#define RIG_IS_DBLST_END(d) ((d)==0)
//! @endcond


/**
 * \brief Frequency range
 *
 * Put together a group of this struct in an array to define
 * what frequencies your rig has access to.
 */
typedef struct freq_range_list {
    freq_t startf;      /*!< Start frequency */
    freq_t endf;        /*!< End frequency */
    rmode_t modes;      /*!< Bit field of RIG_MODE's */
    int low_power;      /*!< Lower RF power in mW, -1 for no power (ie. rx list) */
    int high_power;     /*!< Higher RF power in mW, -1 for no power (ie. rx list) */
    vfo_t vfo;          /*!< VFO list equipped with this range */
    ant_t ant;          /*!< Antenna list equipped with this range, 0 means all, RIG_ANT_CURR means dedicated to certain bands and automatically switches, no set_ant command */
    char *label;        /*!< Label for this range that explains why.  e.g. Icom rigs USA, EUR, ITR, TPE, KOR */
} freq_range_t;

//! @cond Doxygen_Suppress
#define RIG_FRNG_END        {Hz(0),Hz(0),RIG_MODE_NONE,0,0,RIG_VFO_NONE}
#define RIG_IS_FRNG_END(r)  ((r).startf == Hz(0) && (r).endf == Hz(0))
//! @endcond

/**
 * \brief Tuning step definition
 *
 * Lists the tuning steps available for each mode.
 *
 * If a ts field in the list has RIG_TS_ANY value, this means the rig allows
 * its tuning step to be set to any value ranging from the lowest to the
 * highest (if any) value in the list for that mode.  The tuning step must be
 * sorted in the ascending order, and the RIG_TS_ANY value, if present, must
 * be the last one in the list.
 *
 * Note also that the minimum frequency resolution of the rig is determined by
 * the lowest value in the Tuning step list.
 *
 * \sa rig_set_ts(), rig_get_resolution()
 */
struct tuning_step_list {
    rmode_t modes;      /*!< Bit field of RIG_MODE's */
    shortfreq_t ts;     /*!< Tuning step in Hz */
};

//! @cond Doxygen_Suppress
#define RIG_TS_ANY          0
#define RIG_TS_END          {RIG_MODE_NONE, 0}
#define RIG_IS_TS_END(t)    ((t).modes == RIG_MODE_NONE && (t).ts == 0)
//! @endcond


/**
 * \brief Filter definition
 *
 * Lists the filters available for each mode.
 *
 * If more than one filter is available for a given mode, the first entry in
 * the array will be the default filter to use for the normal passband of this
 * mode.  The first entry in the array below the default normal passband is
 * the default narrow passband and the first entry in the array above the
 * default normal passband is the default wide passband.  Note: if there's no
 * lower width or upper width, then narrow or respectively wide passband is
 * equal to the default normal passband.
 *
 * If a width field in the list has RIG_FLT_ANY value, this means the rig
 * allows its passband width to be set to any value ranging from the lowest to
 * the highest value (if any) in the list for that mode.  The RIG_FLT_ANY
 * value, if present, must be the last one in the list.
 *
 * The width field is the narrowest passband in a transmit/receive chain with
 * regard to different IF.
 *
 * \sa rig_set_mode(), rig_passband_normal(), rig_passband_narrow(), rig_passband_wide()
 */
struct filter_list {
    rmode_t modes;      /*!< Bit field of RIG_MODE's */
    pbwidth_t width;    /*!< Passband width in Hz */
};
//! @cond Doxygen_Suppress
#define RIG_FLT_ANY         0
#define RIG_FLT_END         {RIG_MODE_NONE, 0}
#define RIG_IS_FLT_END(f)   ((f).modes == RIG_MODE_NONE)
//! @endcond


/**
 * \brief Empty channel_t.flags field
 */
#define RIG_CHFLAG_NONE     0
/**
 * \brief skip memory channel during scan (lock out), channel_t.flags
 */
#define RIG_CHFLAG_SKIP     (1<<0)
/**
 * \brief DATA port mode flag
 */
#define RIG_CHFLAG_DATA     (1<<1)
/**
 * \brief programmed skip (PSKIP) memory channel during scan (lock out), channel_t.flags
 */
#define RIG_CHFLAG_PSKIP    (1<<2)

/**
 * \brief Extension attribute definition
 *
 */
struct ext_list {
    token_t token;      /*!< Token ID */
    value_t val;        /*!< Value */
};

//! @cond Doxygen_Suppress
#define RIG_EXT_END     {0, {.i=0}}
#define RIG_IS_EXT_END(x)   ((x).token == 0)
//! @endcond

/**
 * \brief Channel structure
 *
 * The channel struct stores all the attributes peculiar to a VFO.
 *
 * \sa rig_set_channel(), rig_get_channel()
 */
struct channel {
    int channel_num;                    /*!< Channel number */
    int bank_num;                       /*!< Bank number */
    vfo_t vfo;                          /*!< VFO */
    ant_t ant;                            /*!< Selected antenna */
    freq_t freq;                        /*!< Receive frequency */
    rmode_t mode;                       /*!< Receive mode */
    pbwidth_t width;                    /*!< Receive passband width associated with mode */

    freq_t tx_freq;                     /*!< Transmit frequency */
    rmode_t tx_mode;                    /*!< Transmit mode */
    pbwidth_t tx_width;                 /*!< Transmit passband width associated with mode */

    split_t split;                      /*!< Split mode */
    vfo_t tx_vfo;                       /*!< Split transmit VFO */

    rptr_shift_t rptr_shift;            /*!< Repeater shift */
    shortfreq_t rptr_offs;              /*!< Repeater offset */
    shortfreq_t tuning_step;            /*!< Tuning step */
    shortfreq_t rit;                    /*!< RIT */
    shortfreq_t xit;                    /*!< XIT */
    setting_t funcs;                    /*!< Function status */
    value_t levels[RIG_SETTING_MAX];    /*!< Level values */
    tone_t ctcss_tone;                  /*!< CTCSS tone */
    tone_t ctcss_sql;                   /*!< CTCSS squelch tone */
    tone_t dcs_code;                    /*!< DCS code */
    tone_t dcs_sql;                     /*!< DCS squelch code */
    int scan_group;                     /*!< Scan group */
    unsigned int flags;                 /*!< Channel flags, see RIG_CHFLAG's */
    char channel_desc[HAMLIB_MAXCHANDESC];     /*!< Name */
    struct ext_list
            *ext_levels;                /*!< Extension level value list, NULL ended. ext_levels can be NULL */
};

/**
 * \brief Channel structure typedef
 */
typedef struct channel channel_t;

/**
 * \brief Channel capability definition
 *
 * Definition of the attributes that can be stored/retrieved in/from memory
 */
struct channel_cap {
    unsigned bank_num:      1;  /*!< Bank number */
    unsigned vfo:           1;  /*!< VFO */
    unsigned ant:           1;  /*!< Selected antenna */
    unsigned freq:          1;  /*!< Receive frequency */
    unsigned mode:          1;  /*!< Receive mode */
    unsigned width:         1;  /*!< Receive passband width associated with mode */

    unsigned tx_freq:       1;  /*!< Transmit frequency */
    unsigned tx_mode:       1;  /*!< Transmit mode */
    unsigned tx_width:      1;  /*!< Transmit passband width associated with mode */

    unsigned split:         1;  /*!< Split mode */
    unsigned tx_vfo:        1;  /*!< Split transmit VFO */
    unsigned rptr_shift:    1;  /*!< Repeater shift */
    unsigned rptr_offs:     1;  /*!< Repeater offset */
    unsigned tuning_step:   1;  /*!< Tuning step */
    unsigned rit:           1;  /*!< RIT */
    unsigned xit:           1;  /*!< XIT */
    setting_t funcs;            /*!< Function status */
    setting_t levels;           /*!< Level values */
    unsigned ctcss_tone:    1;  /*!< CTCSS tone */
    unsigned ctcss_sql:     1;  /*!< CTCSS squelch tone */
    unsigned dcs_code:      1;  /*!< DCS code */
    unsigned dcs_sql:       1;  /*!< DCS squelch code */
    unsigned scan_group:    1;  /*!< Scan group */
    unsigned flags:         1;  /*!< Channel flags */
    unsigned channel_desc:  1;  /*!< Name */
    unsigned ext_levels:    1;  /*!< Extension level value list */
};

/**
 * \brief Channel cap
 */
typedef struct channel_cap channel_cap_t;


/**
 * \brief Memory channel type definition
 *
 * Definition of memory types. Depending on the type, the content
 * of the memory channel has to be interpreted accordingly.
 * For instance, a RIG_MTYPE_EDGE channel_t will hold only a start
 * or stop frequency.
 *
 * \sa chan_list()
 */
typedef enum {
    RIG_MTYPE_NONE =    0,  /*!< None */
    RIG_MTYPE_MEM,          /*!< Regular */
    RIG_MTYPE_EDGE,         /*!< Scan edge */
    RIG_MTYPE_CALL,         /*!< Call channel */
    RIG_MTYPE_MEMOPAD,      /*!< Memory pad */
    RIG_MTYPE_SAT,          /*!< Satellite */
    RIG_MTYPE_BAND,         /*!< VFO/Band channel */
    RIG_MTYPE_PRIO          /*!< Priority channel */
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
    int startc;          /*!< Starting memory channel \b number */
    int endc;            /*!< Ending memory channel \b number */
    chan_type_t type;   /*!< Memory type. see chan_type_t */
    channel_cap_t
    mem_caps;           /*!< Definition of attributes that can be stored/retrieved */
};

//! @cond Doxygen_Suppress
#define RIG_CHAN_END        {0,0,RIG_MTYPE_NONE}
#define RIG_IS_CHAN_END(c)  ((c).type == RIG_MTYPE_NONE)
//! @endcond

/**
 * \brief Special memory channel value to tell rig_lookup_mem_caps() to retrieve all the ranges
 */
#define RIG_MEM_CAPS_ALL    -1

/**
 * \brief chan_t type
 */
typedef struct chan_list chan_t;


/**
 * \brief level/parm granularity definition
 *
 * The granularity is undefined if min = 0, max = 0, and step = 0.
 *
 * For float settings, if min.f = 0 and max.f = 0 (and step.f! = 0), max.f is
 * assumed to be actually equal to 1.0.
 *
 * If step = 0 (and min and/or max are not null), then this means step can
 * have maximum resolution, depending on type (int or float).
 */
struct gran {
    value_t min;        /*!< Minimum value */
    value_t max;        /*!< Maximum value */
    value_t step;       /*!< Step */
};

/**
 * \brief gran_t type
 */
typedef struct gran gran_t;


/**
 * \brief Calibration table struct
 */
struct cal_table {
    int size;                   /*!< number of plots in the table */
    struct {
        int raw;                /*!< raw (A/D) value, as returned by \a RIG_LEVEL_RAWSTR */
        int val;                /*!< associated value, basically the measured dB value */
    } table[HAMLIB_MAX_CAL_LENGTH];    /*!< table of plots */
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

//! @cond Doxygen_Suppress
#define EMPTY_STR_CAL { 0, { { 0, 0 }, } }
//! @endcond Doxygen_Suppress


/**
 * \brief Calibration table struct for float values
 */
struct cal_table_float {
  int size;                   /*!< number of plots in the table */
  struct {
    int raw;                  /*!< raw (A/D) value */
    float val;                /*!< associated value */
  } table[HAMLIB_MAX_CAL_LENGTH];    /*!< table of plots */
};

/**
 * \brief calibration table type for float values
 *
 * cal_table_float_t is a data type suited to hold linear calibration.
 * cal_table_float_t.size tells the number of plots cal_table_float_t.table contains.
 *
 * If a value is below or equal to cal_table_float_t.table[0].raw,
 * rig_raw2val_float() will return cal_table_float_t.table[0].val.
 *
 * If a value is greater or equal to cal_table_float_t.table[cal_table_float_t.size-1].raw,
 * rig_raw2val_float() will return cal_table_float_t.table[cal_table_float_t.size-1].val.
 */
typedef struct cal_table_float cal_table_float_t;

//! @cond Doxygen_Suppress
#define EMPTY_FLOAT_CAL { 0, { { 0, 0f }, } }

typedef int (* chan_cb_t)(RIG *, channel_t **, int, const chan_t *, rig_ptr_t);
typedef int (* confval_cb_t)(RIG *,
                             const struct confparams *,
                             value_t *,
                             rig_ptr_t);
//! @endcond


/**
 * \brief Rig data structure.
 *
 * Basic rig type, can store some useful info about different radios.  Each
 * backend must be able to populate this structure, so we can make useful
 * inquiries about capabilities.
 *
 * The main idea of this struct is that it will be defined by the backend rig
 * driver, and will remain readonly for the application.  Fields that need to
 * be modifiable by the application are copied into the struct rig_state,
 * which is a kind of private storage of the RIG instance.
 *
 * This way, you can have several rigs running within the same application,
 * sharing the struct rig_caps of the backend, while keeping their own
 * customized data.
 *
 * mdblack: Don't move or add fields around without bumping the version numbers
 *          DLL or shared library replacement depends on order
 */
//! @cond Doxygen_Suppress
#define RIG_MODEL(arg) .rig_model=arg,.macro_name=#arg
struct rig_caps {
    rig_model_t rig_model;      /*!< Rig model. */
    const char *model_name;     /*!< Model name. */
    const char *mfg_name;       /*!< Manufacturer. */
    const char *version;        /*!< Driver version. */
    const char *copyright;      /*!< Copyright info. */
    enum rig_status_e status;   /*!< Driver status. */

    int rig_type;               /*!< Rig type. */
    ptt_type_t ptt_type;        /*!< Type of the PTT port. */
    dcd_type_t dcd_type;        /*!< Type of the DCD port. */
    rig_port_t port_type;       /*!< Type of communication port. */

    int serial_rate_min;        /*!< Minimum serial speed. */
    int serial_rate_max;        /*!< Maximum serial speed. */
    int serial_data_bits;       /*!< Number of data bits. */
    int serial_stop_bits;       /*!< Number of stop bits. */
    enum serial_parity_e serial_parity;         /*!< Parity. */
    enum serial_handshake_e serial_handshake;   /*!< Handshake. */

    int write_delay;            /*!< Delay between each byte sent out, in mS */
    int post_write_delay;       /*!< Delay between each commands send out, in mS */
    int timeout;                /*!< Timeout, in mS */
    int retry;                  /*!< Maximum number of retries if command fails, 0 to disable */

    setting_t has_get_func;     /*!< List of get functions */
    setting_t has_set_func;     /*!< List of set functions */
    setting_t has_get_level;    /*!< List of get level */
    setting_t has_set_level;    /*!< List of set level */
    setting_t has_get_parm;     /*!< List of get parm */
    setting_t has_set_parm;     /*!< List of set parm */

    gran_t level_gran[RIG_SETTING_MAX]; /*!< level granularity (i.e. steps) */
    gran_t parm_gran[RIG_SETTING_MAX];  /*!< parm granularity (i.e. steps) */

    const struct confparams *extparms;  /*!< Extension parm list, \sa ext.c */
    const struct confparams *extlevels; /*!< Extension level list, \sa ext.c */
    const struct confparams *extfuncs; /*!< Extension func list, \sa ext.c */
    int *ext_tokens;                    /*!< Extension token list */

    tone_t *ctcss_list;   /*!< CTCSS tones list, zero ended */
    tone_t *dcs_list;     /*!< DCS code list, zero ended */

    int preamp[HAMLIB_MAXDBLSTSIZ];    /*!< Preamp list in dB, 0 terminated */
    int attenuator[HAMLIB_MAXDBLSTSIZ];    /*!< Preamp list in dB, 0 terminated */
    shortfreq_t max_rit;        /*!< max absolute RIT */
    shortfreq_t max_xit;        /*!< max absolute XIT */
    shortfreq_t max_ifshift;    /*!< max absolute IF-SHIFT */

    ann_t announces;            /*!< Announces bit field list */

    vfo_op_t vfo_ops;           /*!< VFO op bit field list */
    scan_t scan_ops;            /*!< Scan bit field list */
    int targetable_vfo;         /*!< Bit field list of direct VFO access commands */
    int transceive;             /*!< Supported transceive mode */

    int bank_qty;               /*!< Number of banks */
    int chan_desc_sz;           /*!< Max length of memory channel name */

    chan_t chan_list[HAMLIB_CHANLSTSIZ];   /*!< Channel list, zero ended */

    // As of 2020-02-12 we know of 5 models from Icom USA, EUR, ITR, TPE, KOR for the IC-9700
    // So we currently have 5 ranges we need to deal with
    // The backend for the model should fill in the label field to explain what model it is
    // The the IC-9700 in ic7300.c for an example 
    freq_range_t rx_range_list1[HAMLIB_FRQRANGESIZ];   /*!< Receive frequency range list #1 */
    freq_range_t tx_range_list1[HAMLIB_FRQRANGESIZ];   /*!< Transmit frequency range list #1 */
    freq_range_t rx_range_list2[HAMLIB_FRQRANGESIZ];   /*!< Receive frequency range list #2 */
    freq_range_t tx_range_list2[HAMLIB_FRQRANGESIZ];   /*!< Transmit frequency range list #2 */
    freq_range_t rx_range_list3[HAMLIB_FRQRANGESIZ];   /*!< Receive frequency range list #3 */
    freq_range_t tx_range_list3[HAMLIB_FRQRANGESIZ];   /*!< Transmit frequency range list #3 */
    freq_range_t rx_range_list4[HAMLIB_FRQRANGESIZ];   /*!< Receive frequency range list #4 */
    freq_range_t tx_range_list4[HAMLIB_FRQRANGESIZ];   /*!< Transmit frequency range list #4 */
    freq_range_t rx_range_list5[HAMLIB_FRQRANGESIZ];   /*!< Receive frequency range list #5 */
    freq_range_t tx_range_list5[HAMLIB_FRQRANGESIZ];   /*!< Transmit frequency range list #5 */

    struct tuning_step_list tuning_steps[HAMLIB_TSLSTSIZ];     /*!< Tuning step list */
    struct filter_list filters[HAMLIB_FLTLSTSIZ];              /*!< mode/filter table, at -6dB */

    cal_table_t str_cal;                    /*!< S-meter calibration table */
    cal_table_float_t swr_cal;              /*!< SWR meter calibration table */
    cal_table_float_t alc_cal;              /*!< ALC meter calibration table */
    cal_table_float_t rfpower_meter_cal;    /*!< RF power meter calibration table */
    cal_table_float_t comp_meter_cal;       /*!< COMP meter calibration table */
    cal_table_float_t vd_meter_cal;         /*!< Voltage meter calibration table */
    cal_table_float_t id_meter_cal;         /*!< Current draw meter calibration table */

    const struct confparams *cfgparams; /*!< Configuration parametres. */
    const rig_ptr_t priv;               /*!< Private data. */

    /*
     * Rig API
     *
     */

    int (*rig_init)(RIG *rig);
    int (*rig_cleanup)(RIG *rig);
    int (*rig_open)(RIG *rig);
    int (*rig_close)(RIG *rig);

    /*
     *  General API commands, from most primitive to least.. :()
     *  List Set/Get functions pairs
     */

    int (*set_freq)(RIG *rig, vfo_t vfo, freq_t freq);
    int (*get_freq)(RIG *rig, vfo_t vfo, freq_t *freq);

    int (*set_mode)(RIG *rig, vfo_t vfo, rmode_t mode, pbwidth_t width);
    int (*get_mode)(RIG *rig, vfo_t vfo, rmode_t *mode, pbwidth_t *width);

    int (*set_vfo)(RIG *rig, vfo_t vfo);
    int (*get_vfo)(RIG *rig, vfo_t *vfo);

    int (*set_ptt)(RIG *rig, vfo_t vfo, ptt_t ptt);
    int (*get_ptt)(RIG *rig, vfo_t vfo, ptt_t *ptt);

    int (*get_dcd)(RIG *rig, vfo_t vfo, dcd_t *dcd);

    int (*set_rptr_shift)(RIG *rig, vfo_t vfo, rptr_shift_t rptr_shift);
    int (*get_rptr_shift)(RIG *rig, vfo_t vfo, rptr_shift_t *rptr_shift);

    int (*set_rptr_offs)(RIG *rig, vfo_t vfo, shortfreq_t offs);
    int (*get_rptr_offs)(RIG *rig, vfo_t vfo, shortfreq_t *offs);

    int (*set_split_freq)(RIG *rig, vfo_t vfo, freq_t tx_freq);
    int (*get_split_freq)(RIG *rig, vfo_t vfo, freq_t *tx_freq);

    int (*set_split_mode)(RIG *rig,
                          vfo_t vfo,
                          rmode_t tx_mode,
                          pbwidth_t tx_width);
    int (*get_split_mode)(RIG *rig,
                          vfo_t vfo,
                          rmode_t *tx_mode,
                          pbwidth_t *tx_width);

    int (*set_split_freq_mode)(RIG *rig,
                               vfo_t vfo,
                               freq_t tx_freq,
                               rmode_t tx_mode,
                               pbwidth_t tx_width);
    int (*get_split_freq_mode)(RIG *rig,
                               vfo_t vfo,
                               freq_t *tx_freq,
                               rmode_t *tx_mode,
                               pbwidth_t *tx_width);

    int (*set_split_vfo)(RIG *rig, vfo_t vfo, split_t split, vfo_t tx_vfo);
    int (*get_split_vfo)(RIG *rig, vfo_t vfo, split_t *split, vfo_t *tx_vfo);

    int (*set_rit)(RIG *rig, vfo_t vfo, shortfreq_t rit);
    int (*get_rit)(RIG *rig, vfo_t vfo, shortfreq_t *rit);

    int (*set_xit)(RIG *rig, vfo_t vfo, shortfreq_t xit);
    int (*get_xit)(RIG *rig, vfo_t vfo, shortfreq_t *xit);

    int (*set_ts)(RIG *rig, vfo_t vfo, shortfreq_t ts);
    int (*get_ts)(RIG *rig, vfo_t vfo, shortfreq_t *ts);

    int (*set_dcs_code)(RIG *rig, vfo_t vfo, tone_t code);
    int (*get_dcs_code)(RIG *rig, vfo_t vfo, tone_t *code);

    int (*set_tone)(RIG *rig, vfo_t vfo, tone_t tone);
    int (*get_tone)(RIG *rig, vfo_t vfo, tone_t *tone);

    int (*set_ctcss_tone)(RIG *rig, vfo_t vfo, tone_t tone);
    int (*get_ctcss_tone)(RIG *rig, vfo_t vfo, tone_t *tone);

    int (*set_dcs_sql)(RIG *rig, vfo_t vfo, tone_t code);
    int (*get_dcs_sql)(RIG *rig, vfo_t vfo, tone_t *code);

    int (*set_tone_sql)(RIG *rig, vfo_t vfo, tone_t tone);
    int (*get_tone_sql)(RIG *rig, vfo_t vfo, tone_t *tone);

    int (*set_ctcss_sql)(RIG *rig, vfo_t vfo, tone_t tone);
    int (*get_ctcss_sql)(RIG *rig, vfo_t vfo, tone_t *tone);

    int (*power2mW)(RIG *rig,
                    unsigned int *mwpower,
                    float power,
                    freq_t freq,
                    rmode_t mode);
    int (*mW2power)(RIG *rig,
                    float *power,
                    unsigned int mwpower,
                    freq_t freq,
                    rmode_t mode);

    int (*set_powerstat)(RIG *rig, powerstat_t status);
    int (*get_powerstat)(RIG *rig, powerstat_t *status);

    int (*reset)(RIG *rig, reset_t reset);

    int (*set_ant)(RIG *rig, vfo_t vfo, ant_t ant, value_t option);
    int (*get_ant)(RIG *rig, vfo_t vfo, ant_t ant, value_t *option, ant_t *ant_curr, ant_t *ant_tx, ant_t *ant_rx);

    int (*set_level)(RIG *rig, vfo_t vfo, setting_t level, value_t val);
    int (*get_level)(RIG *rig, vfo_t vfo, setting_t level, value_t *val);

    int (*set_func)(RIG *rig, vfo_t vfo, setting_t func, int status);
    int (*get_func)(RIG *rig, vfo_t vfo, setting_t func, int *status);

    int (*set_parm)(RIG *rig, setting_t parm, value_t val);
    int (*get_parm)(RIG *rig, setting_t parm, value_t *val);

    int (*set_ext_level)(RIG *rig, vfo_t vfo, token_t token, value_t val);
    int (*get_ext_level)(RIG *rig, vfo_t vfo, token_t token, value_t *val);

    int (*set_ext_func)(RIG *rig, vfo_t vfo, token_t token, int status);
    int (*get_ext_func)(RIG *rig, vfo_t vfo, token_t token, int *status);

    int (*set_ext_parm)(RIG *rig, token_t token, value_t val);
    int (*get_ext_parm)(RIG *rig, token_t token, value_t *val);

    int (*set_conf)(RIG *rig, token_t token, const char *val);
    int (*get_conf)(RIG *rig, token_t token, char *val);

    int (*send_dtmf)(RIG *rig, vfo_t vfo, const char *digits);
    int (*recv_dtmf)(RIG *rig, vfo_t vfo, char *digits, int *length);

    int (*send_morse)(RIG *rig, vfo_t vfo, const char *msg);
    int (*stop_morse)(RIG *rig, vfo_t vfo);
    int (*wait_morse)(RIG *rig, vfo_t vfo);

    int (*send_voice_mem)(RIG *rig, vfo_t vfo, int ch);

    int (*set_bank)(RIG *rig, vfo_t vfo, int bank);

    int (*set_mem)(RIG *rig, vfo_t vfo, int ch);
    int (*get_mem)(RIG *rig, vfo_t vfo, int *ch);

    int (*vfo_op)(RIG *rig, vfo_t vfo, vfo_op_t op);

    int (*scan)(RIG *rig, vfo_t vfo, scan_t scan, int ch);

    int (*set_trn)(RIG *rig, int trn);
    int (*get_trn)(RIG *rig, int *trn);

    int (*decode_event)(RIG *rig);

    int (*set_channel)(RIG *rig, vfo_t vfo, const channel_t *chan);
    int (*get_channel)(RIG *rig, vfo_t vfo, channel_t *chan, int read_only);

    const char * (*get_info)(RIG *rig);

    int (*set_chan_all_cb)(RIG *rig, vfo_t vfo, chan_cb_t chan_cb, rig_ptr_t);
    int (*get_chan_all_cb)(RIG *rig, vfo_t vfo, chan_cb_t chan_cb, rig_ptr_t);

    int (*set_mem_all_cb)(RIG *rig,
                          chan_cb_t chan_cb,
                          confval_cb_t parm_cb,
                          rig_ptr_t);
    int (*get_mem_all_cb)(RIG *rig,
                          chan_cb_t chan_cb,
                          confval_cb_t parm_cb,
                          rig_ptr_t);

    int (*set_vfo_opt)(RIG *rig, int status); // only for Net Rigctl device
    int (*rig_get_vfo_info) (RIG *rig,
                             vfo_t vfo,
                             freq_t *freq,
                             rmode_t *mode,
                             pbwidth_t *width,
                             split_t *split);

    const char *clone_combo_set;    /*!< String describing key combination to enter load cloning mode */
    const char *clone_combo_get;    /*!< String describing key combination to enter save cloning mode */
    const char *macro_name;     /*!< Rig model macro name */
};
//! @endcond

/**
 * \brief Enumeration of all rig_ functions
 *
 */
//! @cond Doxygen_Suppress
// all functions enumerated for rig_get_function_ptr
enum rig_function_e {
    RIG_FUNCTION_INIT,
    RIG_FUNCTION_CLEANUP,
    RIG_FUNCTION_OPEN,
    RIG_FUNCTION_CLOSE,
    RIG_FUNCTION_SET_FREQ,
    RIG_FUNCTION_GET_FREQ,
    RIG_FUNCTION_SET_MODE,
    RIG_FUNCTION_GET_MODE,
    RIG_FUNCTION_SET_VFO,
    RIG_FUNCTION_GET_VFO,
    RIG_FUNCTION_SET_PTT,
    RIG_FUNCTION_GET_PTT,
    RIG_FUNCTION_GET_DCD,
    RIG_FUNCTION_SET_RPTR_SHIFT,
    RIG_FUNCTION_GET_RPTR_SHIFT,
    RIG_FUNCTION_SET_RPTR_OFFS,
    RIG_FUNCTION_GET_RPTR_OFFS,
    RIG_FUNCTION_SET_SPLIT_FREQ,
    RIG_FUNCTION_GET_SPLIT_FREQ,
    RIG_FUNCTION_SET_SPLIT_MODE,
    RIG_FUNCTION_SET_SPLIT_FREQ_MODE,
    RIG_FUNCTION_GET_SPLIT_FREQ_MODE,
    RIG_FUNCTION_SET_SPLIT_VFO,
    RIG_FUNCTION_GET_SPLIT_VFO,
    RIG_FUNCTION_SET_RIT,
    RIG_FUNCTION_GET_RIT,
    RIG_FUNCTION_SET_XIT,
    RIG_FUNCTION_GET_XIT,
    RIG_FUNCTION_SET_TS,
    RIG_FUNCTION_GET_TS,
    RIG_FUNCTION_SET_DCS_CODE,
    RIG_FUNCTION_GET_DCS_CODE,
    RIG_FUNCTION_SET_TONE,
    RIG_FUNCTION_GET_TONE,
    RIG_FUNCTION_SET_CTCSS_TONE,
    RIG_FUNCTION_GET_CTCSS_TONE,
    RIG_FUNCTION_SET_DCS_SQL,
    RIG_FUNCTION_GET_DCS_SQL,
    RIG_FUNCTION_SET_TONE_SQL,
    RIG_FUNCTION_GET_TONE_SQL,
    RIG_FUNCTION_SET_CTCSS_SQL,
    RIG_FUNCTION_GET_CTCSS_SQL,
    RIG_FUNCTION_POWER2MW,
    RIG_FUNCTION_MW2POWER,
    RIG_FUNCTION_SET_POWERSTAT,
    RIG_FUNCTION_GET_POWERSTAT,
    RIG_FUNCTION_RESET,
    RIG_FUNCTION_SET_ANT,
    RIG_FUNCTION_GET_ANT,
    RIG_FUNCTION_SET_LEVEL,
    RIG_FUNCTION_GET_LEVEL,
    RIG_FUNCTION_SET_FUNC,
    RIG_FUNCTION_GET_FUNC,
    RIG_FUNCTION_SET_PARM,
    RIG_FUNCTION_GET_PARM,
    RIG_FUNCTION_SET_EXT_LEVEL,
    RIG_FUNCTION_GET_EXT_LEVEL,
    RIG_FUNCTION_SET_EXT_FUNC,
    RIG_FUNCTION_GET_EXT_FUNC,
    RIG_FUNCTION_SET_EXT_PARM,
    RIG_FUNCTION_GET_EXT_PARM,
    RIG_FUNCTION_SET_CONF,
    RIG_FUNCTION_GET_CONF,
    RIG_FUNCTION_SEND_DTMF,
    RIG_FUNCTION_SEND_MORSE,
    RIG_FUNCTION_STOP_MORSE,
    RIG_FUNCTION_WAIT_MORSE,
    RIG_FUNCTION_SEND_VOICE_MEM,
    RIG_FUNCTION_SET_BANK,
    RIG_FUNCTION_SET_MEM,
    RIG_FUNCTION_GET_MEM,
    RIG_FUNCTION_VFO_OP,
    RIG_FUNCTION_SCAN,
    RIG_FUNCTION_SET_TRN,
    RIG_FUNCTION_GET_TRN,
    RIG_FUNCTION_DECODE_EVENT,
    RIG_FUNCTION_SET_CHANNEL,
    RIG_FUNCTION_GET_CHANNEL,
    RIG_FUNCTION_GET_INFO,
    RIG_FUNCTION_SET_CHAN_ALL_CB,
    RIG_FUNCTION_GET_CHAN_ALL_CB,
    RIG_FUNCTION_SET_MEM_ALL_CB,
    RIG_FUNCTION_GET_MEM_ALL_CB,
    RIG_FUNCTION_SET_VFO_OPT
};

/**
 * \brief Function to return pointer to rig_* function
 *
 */
//! @cond Doxygen_Suppress
extern HAMLIB_EXPORT (void *) rig_get_function_ptr(rig_model_t rig_model, enum rig_function_e rig_function);

/**
 * \brief Enumeration of rig->caps values
 *
 */
//! @cond Doxygen_Suppress
// values enumerated for rig->caps values
enum rig_caps_int_e {
    RIG_CAPS_TARGETABLE_VFO,
    RIG_CAPS_RIG_MODEL,
    RIG_CAPS_PORT_TYPE,
    RIG_CAPS_PTT_TYPE,
    RIG_CAPS_HAS_GET_LEVEL
};

enum rig_caps_cptr_e {
    RIG_CAPS_VERSION_CPTR,
    RIG_CAPS_MFG_NAME_CPTR,
    RIG_CAPS_MODEL_NAME_CPTR,
    RIG_CAPS_STATUS_CPTR
};

/**
 * \brief Function to return int value from rig->caps
 * Does not support > 32-bit rig_caps values
 */
//! @cond Doxygen_Suppress
extern HAMLIB_EXPORT (long long) rig_get_caps_int(rig_model_t rig_model, enum rig_caps_int_e rig_caps);

/**
 * \brief Function to return char pointer value from rig->caps
 *
 */
//! @cond Doxygen_Suppress
extern HAMLIB_EXPORT (const char *) rig_get_caps_cptr(rig_model_t rig_model, enum rig_caps_cptr_e rig_caps);

/**
 * \brief Port definition
 *
 * Of course, looks like OO painstakingly programmed in C, sigh.
 */
//! @cond Doxygen_Suppress
typedef struct hamlib_port {
    union {
        rig_port_t rig;     /*!< Communication port type */
        ptt_type_t ptt;     /*!< PTT port type */
        dcd_type_t dcd;     /*!< DCD port type */
    } type;

    int fd;                 /*!< File descriptor */
    void *handle;           /*!< handle for USB */

    int write_delay;        /*!< Delay between each byte sent out, in mS */
    int post_write_delay;   /*!< Delay between each commands send out, in mS */

    struct {
        int tv_sec, tv_usec;
    } post_write_date;      /*!< hamlib internal use */

    int timeout;            /*!< Timeout, in mS */
    short retry;            /*!< Maximum number of retries, 0 to disable */
    short flushx;           /*!< If true flush is done with read instead of TCFLUSH - MicroHam */

    char pathname[HAMLIB_FILPATHLEN];      /*!< Port pathname */

    union {
        struct {
            int rate;       /*!< Serial baud rate */
            int data_bits;  /*!< Number of data bits */
            int stop_bits;  /*!< Number of stop bits */
            enum serial_parity_e parity;        /*!< Serial parity */
            enum serial_handshake_e handshake;  /*!< Serial handshake */
            enum serial_control_state_e rts_state;    /*!< RTS set state */
            enum serial_control_state_e dtr_state;    /*!< DTR set state */
        } serial;           /*!< serial attributes */

        struct {
            int pin;        /*!< Parallel port pin number */
        } parallel;         /*!< parallel attributes */

        struct {
            int ptt_bitnum; /*!< Bit number for CM108 GPIO PTT */
        } cm108;            /*!< CM108 attributes */

        struct {
            int vid;        /*!< Vendor ID */
            int pid;        /*!< Product ID */
            int conf;       /*!< Configuration */
            int iface;      /*!< interface */
            int alt;        /*!< alternate */
            char *vendor_name;  /*!< Vendor name (opt.) */
            char *product;      /*!< Product (opt.) */
        } usb;              /*!< USB attributes */

        struct {
            int on_value;   /*!< GPIO: 1 == normal, GPION: 0 == inverted */
            int value;      /*!< Toggle PTT ON or OFF */
        } gpio;             /*!< GPIO attributes */
    } parm;                 /*!< Port parameter union */
} hamlib_port_t;
//! @endcond

#if !defined(__APPLE__) || !defined(__cplusplus)
typedef hamlib_port_t port_t;
#endif

#define HAMLIB_ELAPSED_GET 0
#define HAMLIB_ELAPSED_SET 1
#define HAMLIB_ELAPSED_INVALIDATE 2

typedef enum {
    HAMLIB_CACHE_ALL, // to set all cache timeouts at once
    HAMLIB_CACHE_VFO,
    HAMLIB_CACHE_FREQ,
    HAMLIB_CACHE_MODE,
    HAMLIB_CACHE_PTT,
    HAMLIB_CACHE_SPLIT,
    HAMLIB_CACHE_WIDTH
} hamlib_cache_t;

typedef enum {
    TWIDDLE_OFF,
    TWIDDLE_ON
} twiddle_state_t;

/**
 * \brief Rig cache data
 * 
 * This struct contains all the items we cache at the highest level
 */
struct rig_cache {
    int timeout_ms;  // the cache timeout for invalidating itself
    vfo_t vfo;
    //freq_t freq; // to be deprecated in 4.1 when full Main/Sub/A/B caching is implemented in 4.1
    // other abstraction here is based on dual vfo rigs and mapped to all others
    // So we have four possible states of rig
    // MainA, MainB, SubA, SubB
    // Main is the Main VFO and Sub is for the 2nd VFO
    // Most rigs have MainA and MainB
    // Dual VFO rigs can have SubA and SubB too
    // For dual VFO rigs simplex operations are all done on MainA/MainB -- ergo this abstraction
    freq_t freqMainA; // VFO_A, VFO_MAIN, and VFO_MAINA
    freq_t freqMainB; // VFO_B, VFO_SUB, and VFO_MAINB
    freq_t freqMainC; // VFO_C, VFO_MAINC
    freq_t freqSubA;  // VFO_SUBA -- only for rigs with dual Sub VFOs
    freq_t freqSubB;  // VFO_SUBB -- only for rigs with dual Sub VFOs
    freq_t freqSubC;  // VFO_SUBC -- only for rigs with 3 Sub VFOs
    freq_t freqMem;   // VFO_MEM -- last MEM channel 
    rmode_t modeMainA;
    rmode_t modeMainB;
    rmode_t modeMainC;
    rmode_t modeSubA;
    rmode_t modeSubB;
    rmode_t modeSubC;
    rmode_t modeMem;
    pbwidth_t widthMainA; // if non-zero then rig has separate width for MainA
    pbwidth_t widthMainB; // if non-zero then rig has separate width for MainB
    pbwidth_t widthMainC; // if non-zero then rig has separate width for MainC
    pbwidth_t widthSubA;  // if non-zero then rig has separate width for SubA
    pbwidth_t widthSubB;  // if non-zero then rig has separate width for SubB
    pbwidth_t widthSubC;  // if non-zero then rig has separate width for SubC
    pbwidth_t widthMem;  // if non-zero then rig has separate width for Mem
    ptt_t ptt;
    split_t split;
    vfo_t split_vfo;  // split caches two values
    struct timespec time_freqMainA;
    struct timespec time_freqMainB;
    struct timespec time_freqMainC;
    struct timespec time_freqSubA;
    struct timespec time_freqSubB;
    struct timespec time_freqSubC;
    struct timespec time_freqMem;
    struct timespec time_vfo;
    struct timespec time_modeMainA;
    struct timespec time_modeMainB;
    struct timespec time_modeMainC;
    struct timespec time_modeSubA;
    struct timespec time_modeSubB;
    struct timespec time_modeSubC;
    struct timespec time_modeMem;
    struct timespec time_widthMainA;
    struct timespec time_widthMainB;
    struct timespec time_widthMainC;
    struct timespec time_widthSubA;
    struct timespec time_widthSubB;
    struct timespec time_widthSubC;
    struct timespec time_widthMem;
    struct timespec time_ptt;
    struct timespec time_split;
    int satmode; // if rig is in satellite mode
};


/**
 * \brief Rig state containing live data and customized fields.
 *
 * This struct contains live data, as well as a copy of capability fields
 * that may be updated (ie. customized)
 *
 * It is NOT fine to move fields around as it can break share library offset
 * As of 2021-03-03  vfo_list is the last known item being reference externally
 * So any additions or changes to this structure must be after vfo_list.
 */
struct rig_state {
    /*
     * overridable fields
     */
    hamlib_port_t rigport;  /*!< Rig port (internal use). */
    hamlib_port_t pttport;  /*!< PTT port (internal use). */
    hamlib_port_t dcdport;  /*!< DCD port (internal use). */

    double vfo_comp;        /*!< VFO compensation in PPM, 0.0 to disable */

    int deprecated_itu_region;         /*!< ITU region to select among freq_range_t */
    freq_range_t rx_range_list[HAMLIB_FRQRANGESIZ];    /*!< Receive frequency range list */
    freq_range_t tx_range_list[HAMLIB_FRQRANGESIZ];    /*!< Transmit frequency range list */

    struct tuning_step_list tuning_steps[HAMLIB_TSLSTSIZ]; /*!< Tuning step list */

    struct filter_list filters[HAMLIB_FLTLSTSIZ];      /*!< Mode/filter table, at -6dB */

    cal_table_t str_cal;            /*!< S-meter calibration table */

    chan_t chan_list[HAMLIB_CHANLSTSIZ];   /*!< Channel list, zero ended */

    shortfreq_t max_rit;        /*!< max absolute RIT */
    shortfreq_t max_xit;        /*!< max absolute XIT */
    shortfreq_t max_ifshift;    /*!< max absolute IF-SHIFT */

    ann_t announces;            /*!< Announces bit field list */

    int preamp[HAMLIB_MAXDBLSTSIZ];    /*!< Preamp list in dB, 0 terminated */
    int attenuator[HAMLIB_MAXDBLSTSIZ];    /*!< Preamp list in dB, 0 terminated */

    setting_t has_get_func;     /*!< List of get functions */
    setting_t has_set_func;     /*!< List of set functions */
    setting_t has_get_level;    /*!< List of get level */
    setting_t has_set_level;    /*!< List of set level */
    setting_t has_get_parm;     /*!< List of get parm */
    setting_t has_set_parm;     /*!< List of set parm */

    gran_t level_gran[RIG_SETTING_MAX]; /*!< level granularity */
    gran_t parm_gran[RIG_SETTING_MAX];  /*!< parm granularity */


    /*
     * non overridable fields, internal use
     */

    int hold_decode;    /*!< set to 1 to hold the event decoder (async) otherwise 0 */
    vfo_t current_vfo;  /*!< VFO currently set */
    int vfo_list;       /*!< Complete list of VFO for this rig */
    int comm_state;     /*!< Comm port state, opened/closed. */
    rig_ptr_t priv;     /*!< Pointer to private rig state data. */
    rig_ptr_t obj;      /*!< Internal use by hamlib++ for event handling. */

    int transceive;             /*!< Whether the transceive mode is on */
    int poll_interval;          /*!< Event notification polling period in milliseconds */
    freq_t current_freq;        /*!< Frequency currently set */
    rmode_t current_mode;       /*!< Mode currently set */
    //rmode_t current_modeB;      /*!< Mode currently set VFOB */
    pbwidth_t current_width;    /*!< Passband width currently set */
    vfo_t tx_vfo;               /*!< Tx VFO currently set */
    rmode_t mode_list;              /*!< Complete list of modes for this rig */
    // mode_list is used by some 
    // so anything added to this structure must be below here
    int transmit;               /*!< rig should be transmitting i.e. hard
                                     wired PTT asserted - used by rigs that
                                     don't do CAT while in Tx */
    freq_t lo_freq;             /*!< Local oscillator frequency of any transverter */
    time_t twiddle_time;        /*!< time when vfo twiddling was detected */
    int twiddle_timeout;        /*!< timeout to resume from twiddling */
    // uplink allows gpredict to behave better by no reading the uplink VFO
    int uplink;                 /*!< uplink=1 will not read Sub, uplink=2 will not read Main */
    struct rig_cache cache;
    int vfo_opt;                /*!< Is -o switch turned on? */
    int auto_power_on;          /*!< Allow Hamlib to power on rig
                                   automatically if supported */
    int auto_power_off;          /*!< Allow Hamlib to power off rig
                                   automatically if supported */
    int auto_disable_screensaver; /*!< Allow Hamlib to disable the
                                   rig's screen saver automatically if
                                   supported */
    int ptt_share;              /*!< Share ptt port by open/close during get_ptt, set_ptt hogs the port while active */
    int power_now;              /*!< Current RF power level in rig units */
    int power_min;              /*!< Minimum RF power level in rig units */
    int power_max;              /*!< Maximum RF power level in rig units */
    unsigned char disable_yaesu_bandselect; /*!< Disables Yaeus band select logic */
    int twiddle_rit;            /*!< Suppresses VFOB reading (cached value used) so RIT control can be used */
    int twiddle_state;          /*!< keeps track of twiddle status */
};

//! @cond Doxygen_Suppress
typedef int (*vprintf_cb_t)(enum rig_debug_level_e,
                            rig_ptr_t,
                            const char *,
                            va_list);

typedef int (*freq_cb_t)(RIG *, vfo_t, freq_t, rig_ptr_t);
typedef int (*mode_cb_t)(RIG *, vfo_t, rmode_t, pbwidth_t, rig_ptr_t);
typedef int (*vfo_cb_t)(RIG *, vfo_t, rig_ptr_t);
typedef int (*ptt_cb_t)(RIG *, vfo_t, ptt_t, rig_ptr_t);
typedef int (*dcd_cb_t)(RIG *, vfo_t, dcd_t, rig_ptr_t);
typedef int (*pltune_cb_t)(RIG *,
                           vfo_t, freq_t *,
                           rmode_t *,
                           pbwidth_t *,
                           rig_ptr_t);

//! @endcond

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
 * \sa rig_set_freq_callback(), rig_set_mode_callback(), rig_set_vfo_callback(),
 *     rig_set_ptt_callback(), rig_set_dcd_callback()
 */
struct rig_callbacks {
    freq_cb_t freq_event;   /*!< Frequency change event */
    rig_ptr_t freq_arg;     /*!< Frequency change argument */
    mode_cb_t mode_event;   /*!< Mode change event */
    rig_ptr_t mode_arg;     /*!< Mode change argument */
    vfo_cb_t vfo_event;     /*!< VFO change event */
    rig_ptr_t vfo_arg;      /*!< VFO change argument */
    ptt_cb_t ptt_event;     /*!< PTT change event */
    rig_ptr_t ptt_arg;      /*!< PTT change argument */
    dcd_cb_t dcd_event;     /*!< DCD change event */
    rig_ptr_t dcd_arg;      /*!< DCD change argument */
    pltune_cb_t pltune;     /*!< Pipeline tuning module freq/mode/width callback */
    rig_ptr_t pltune_arg;   /*!< Pipeline tuning argument */
    /* etc.. */
};


/**
 * \brief The Rig structure
 *
 * This is the master data structure, acting as a handle for the controlled
 * rig. A pointer to this structure is returned by the rig_init() API
 * function and is passed as a parameter to every rig specific API call.
 *
 * \sa rig_init(), rig_caps(), rig_state()
 */
struct s_rig {
    struct rig_caps *caps;          /*!< Pointer to rig capabilities (read only) */
    struct rig_state state;         /*!< Rig state */
    struct rig_callbacks callbacks; /*!< registered event callbacks */
};



/* --------------- API function prototypes -----------------*/

//! @cond Doxygen_Suppress

extern HAMLIB_EXPORT(RIG *) rig_init HAMLIB_PARAMS((rig_model_t rig_model));
extern HAMLIB_EXPORT(int) rig_open HAMLIB_PARAMS((RIG *rig));

/*
 *  General API commands, from most primitive to least.. :()
 *  List Set/Get functions pairs
 */

extern HAMLIB_EXPORT(int)
rig_flush(hamlib_port_t *port);

extern HAMLIB_EXPORT(int)
rig_set_freq HAMLIB_PARAMS((RIG *rig,
                            vfo_t vfo,
                            freq_t freq));
extern HAMLIB_EXPORT(int)
rig_get_freq HAMLIB_PARAMS((RIG *rig,
                            vfo_t vfo,
                            freq_t *freq));

extern HAMLIB_EXPORT(int)
rig_set_mode HAMLIB_PARAMS((RIG *rig,
                            vfo_t vfo,
                            rmode_t mode,
                            pbwidth_t width));
extern HAMLIB_EXPORT(int)
rig_get_mode HAMLIB_PARAMS((RIG *rig,
                            vfo_t vfo,
                            rmode_t *mode,
                            pbwidth_t *width));

extern HAMLIB_EXPORT(int)
rig_set_vfo HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo));
extern HAMLIB_EXPORT(int)
rig_get_vfo HAMLIB_PARAMS((RIG *rig,
                           vfo_t *vfo));

extern HAMLIB_EXPORT(int)
rig_get_vfo_info HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo, 
                           freq_t *freq, 
                           rmode_t *mode, 
                           pbwidth_t *width, 
                           split_t *split,
                           int *satmode));

extern HAMLIB_EXPORT(int)
rig_get_vfo_list HAMLIB_PARAMS((RIG *rig, char *buf, int buflen));

extern HAMLIB_EXPORT(int)
netrigctl_get_vfo_mode HAMLIB_PARAMS((RIG *rig));

extern HAMLIB_EXPORT(int)
rig_set_ptt HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           ptt_t ptt));
extern HAMLIB_EXPORT(int)
rig_get_ptt HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           ptt_t *ptt));

extern HAMLIB_EXPORT(int)
rig_get_dcd HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           dcd_t *dcd));

extern HAMLIB_EXPORT(int)
rig_set_rptr_shift HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  rptr_shift_t rptr_shift));
extern HAMLIB_EXPORT(int)
rig_get_rptr_shift HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  rptr_shift_t *rptr_shift));

extern HAMLIB_EXPORT(int)
rig_set_rptr_offs HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 shortfreq_t rptr_offs));
extern HAMLIB_EXPORT(int)
rig_get_rptr_offs HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 shortfreq_t *rptr_offs));

extern HAMLIB_EXPORT(int)
rig_set_ctcss_tone HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  tone_t tone));
extern HAMLIB_EXPORT(int)
rig_get_ctcss_tone HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  tone_t *tone));

extern HAMLIB_EXPORT(int)
rig_set_dcs_code HAMLIB_PARAMS((RIG *rig,
                                vfo_t vfo,
                                tone_t code));
extern HAMLIB_EXPORT(int)
rig_get_dcs_code HAMLIB_PARAMS((RIG *rig,
                                vfo_t vfo,
                                tone_t *code));

extern HAMLIB_EXPORT(int)
rig_set_ctcss_sql HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 tone_t tone));
extern HAMLIB_EXPORT(int)
rig_get_ctcss_sql HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 tone_t *tone));

extern HAMLIB_EXPORT(int)
rig_set_dcs_sql HAMLIB_PARAMS((RIG *rig,
                               vfo_t vfo,
                               tone_t code));
extern HAMLIB_EXPORT(int)
rig_get_dcs_sql HAMLIB_PARAMS((RIG *rig,
                               vfo_t vfo,
                               tone_t *code));

extern HAMLIB_EXPORT(int)
rig_set_split_freq HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  freq_t tx_freq));
extern HAMLIB_EXPORT(int)
rig_get_split_freq HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  freq_t *tx_freq));

extern HAMLIB_EXPORT(int)
rig_set_split_mode HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  rmode_t tx_mode,
                                  pbwidth_t tx_width));
extern HAMLIB_EXPORT(int)
rig_get_split_mode HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  rmode_t *tx_mode,
                                  pbwidth_t *tx_width));

extern HAMLIB_EXPORT(int)
rig_set_split_freq_mode HAMLIB_PARAMS((RIG *rig,
                                       vfo_t vfo,
                                       freq_t tx_freq,
                                       rmode_t tx_mode,
                                       pbwidth_t tx_width));
extern HAMLIB_EXPORT(int)
rig_get_split_freq_mode HAMLIB_PARAMS((RIG *rig,
                                       vfo_t vfo,
                                       freq_t *tx_freq,
                                       rmode_t *tx_mode,
                                       pbwidth_t *tx_width));

extern HAMLIB_EXPORT(int)
rig_set_split_vfo HAMLIB_PARAMS((RIG *,
                                 vfo_t rx_vfo,
                                 split_t split,
                                 vfo_t tx_vfo));
extern HAMLIB_EXPORT(int)
rig_get_split_vfo HAMLIB_PARAMS((RIG *,
                                 vfo_t rx_vfo,
                                 split_t *split,
                                 vfo_t *tx_vfo));

#define rig_set_split(r,v,s) rig_set_split_vfo((r),(v),(s),RIG_VFO_CURR)
#define rig_get_split(r,v,s) ({ vfo_t _tx_vfo; rig_get_split_vfo((r),(v),(s),&_tx_vfo); })

extern HAMLIB_EXPORT(int)
rig_set_rit HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           shortfreq_t rit));
extern HAMLIB_EXPORT(int)
rig_get_rit HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           shortfreq_t *rit));

extern HAMLIB_EXPORT(int)
rig_set_xit HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           shortfreq_t xit));
extern HAMLIB_EXPORT(int)
rig_get_xit HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           shortfreq_t *xit));

extern HAMLIB_EXPORT(int)
rig_set_ts HAMLIB_PARAMS((RIG *rig,
                          vfo_t vfo,
                          shortfreq_t ts));
extern HAMLIB_EXPORT(int)
rig_get_ts HAMLIB_PARAMS((RIG *rig,
                          vfo_t vfo,
                          shortfreq_t *ts));

extern HAMLIB_EXPORT(int)
rig_power2mW HAMLIB_PARAMS((RIG *rig,
                            unsigned int *mwpower,
                            float power,
                            freq_t freq,
                            rmode_t mode));
extern HAMLIB_EXPORT(int)
rig_mW2power HAMLIB_PARAMS((RIG *rig,
                            float *power,
                            unsigned int mwpower,
                            freq_t freq,
                            rmode_t mode));

extern HAMLIB_EXPORT(shortfreq_t)
rig_get_resolution HAMLIB_PARAMS((RIG *rig,
                                  rmode_t mode));

extern HAMLIB_EXPORT(int)
rig_set_level HAMLIB_PARAMS((RIG *rig,
                             vfo_t vfo,
                             setting_t level,
                             value_t val));
extern HAMLIB_EXPORT(int)
rig_get_level HAMLIB_PARAMS((RIG *rig,
                             vfo_t vfo,
                             setting_t level,
                             value_t *val));

#define rig_get_strength(r,v,s) rig_get_level((r),(v),RIG_LEVEL_STRENGTH, (value_t*)(s))

extern HAMLIB_EXPORT(int)
rig_set_parm HAMLIB_PARAMS((RIG *rig,
                            setting_t parm,
                            value_t val));
extern HAMLIB_EXPORT(int)
rig_get_parm HAMLIB_PARAMS((RIG *rig,
                            setting_t parm,
                            value_t *val));

extern HAMLIB_EXPORT(int)
rig_set_conf HAMLIB_PARAMS((RIG *rig,
                            token_t token,
                            const char *val));
extern HAMLIB_EXPORT(int)
rig_get_conf HAMLIB_PARAMS((RIG *rig,
                            token_t token,
                            char *val));

extern HAMLIB_EXPORT(int)
rig_set_powerstat HAMLIB_PARAMS((RIG *rig,
                                 powerstat_t status));
extern HAMLIB_EXPORT(int)
rig_get_powerstat HAMLIB_PARAMS((RIG *rig,
                                 powerstat_t *status));

extern HAMLIB_EXPORT(int)
rig_reset HAMLIB_PARAMS((RIG *rig,
                         reset_t reset));   /* dangerous! */

extern HAMLIB_EXPORT(int)
rig_set_ext_level HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 token_t token,
                                 value_t val));
extern HAMLIB_EXPORT(int)
rig_get_ext_level HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 token_t token,
                                 value_t *val));

extern HAMLIB_EXPORT(int)
rig_set_ext_func HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 token_t token,
                                 int status));
extern HAMLIB_EXPORT(int)
rig_get_ext_func HAMLIB_PARAMS((RIG *rig,
                                 vfo_t vfo,
                                 token_t token,
                                 int *status));

extern HAMLIB_EXPORT(int)
rig_set_ext_parm HAMLIB_PARAMS((RIG *rig,
                                token_t token,
                                value_t val));
extern HAMLIB_EXPORT(int)
rig_get_ext_parm HAMLIB_PARAMS((RIG *rig,
                                token_t token,
                                value_t *val));

extern HAMLIB_EXPORT(int)
rig_ext_func_foreach HAMLIB_PARAMS((RIG *rig,
                                     int (*cfunc)(RIG *,
                                                  const struct confparams *,
                                                  rig_ptr_t),
                                     rig_ptr_t data));
extern HAMLIB_EXPORT(int)
rig_ext_level_foreach HAMLIB_PARAMS((RIG *rig,
                                     int (*cfunc)(RIG *,
                                                  const struct confparams *,
                                                  rig_ptr_t),
                                     rig_ptr_t data));
extern HAMLIB_EXPORT(int)
rig_ext_parm_foreach HAMLIB_PARAMS((RIG *rig,
                                    int (*cfunc)(RIG *,
                                                 const struct confparams *,
                                                 rig_ptr_t),
                                    rig_ptr_t data));

extern HAMLIB_EXPORT(const struct confparams *)
rig_ext_lookup HAMLIB_PARAMS((RIG *rig,
                              const char *name));

extern HAMLIB_EXPORT(const struct confparams *)
rig_ext_lookup_tok HAMLIB_PARAMS((RIG *rig,
                                  token_t token));
extern HAMLIB_EXPORT(token_t)
rig_ext_token_lookup HAMLIB_PARAMS((RIG *rig,
                                    const char *name));


extern HAMLIB_EXPORT(int)
rig_token_foreach HAMLIB_PARAMS((RIG *rig,
                                 int (*cfunc)(const struct confparams *,
                                              rig_ptr_t),
                                 rig_ptr_t data));

extern HAMLIB_EXPORT(const struct confparams *)
rig_confparam_lookup HAMLIB_PARAMS((RIG *rig,
                                    const char *name));
extern HAMLIB_EXPORT(token_t)
rig_token_lookup HAMLIB_PARAMS((RIG *rig,
                                const char *name));

extern HAMLIB_EXPORT(int)
rig_close HAMLIB_PARAMS((RIG *rig));

extern HAMLIB_EXPORT(int)
rig_cleanup HAMLIB_PARAMS((RIG *rig));

extern HAMLIB_EXPORT(int)
rig_set_ant HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           ant_t ant,  /* antenna */
                           value_t option));  /* optional ant info */
extern HAMLIB_EXPORT(int)
rig_get_ant HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           ant_t ant,
                           value_t *option,
                           ant_t *ant_curr,
                           ant_t *ant_tx,
                           ant_t *ant_rx));

extern HAMLIB_EXPORT(setting_t)
rig_has_get_level HAMLIB_PARAMS((RIG *rig,
                                 setting_t level));
extern HAMLIB_EXPORT(setting_t)
rig_has_set_level HAMLIB_PARAMS((RIG *rig,
                                 setting_t level));

extern HAMLIB_EXPORT(setting_t)
rig_has_get_parm HAMLIB_PARAMS((RIG *rig,
                                setting_t parm));
extern HAMLIB_EXPORT(setting_t)
rig_has_set_parm HAMLIB_PARAMS((RIG *rig,
                                setting_t parm));

extern HAMLIB_EXPORT(setting_t)
rig_has_get_func HAMLIB_PARAMS((RIG *rig,
                                setting_t func));
extern HAMLIB_EXPORT(setting_t)
rig_has_set_func HAMLIB_PARAMS((RIG *rig,
                                setting_t func));

extern HAMLIB_EXPORT(int)
rig_set_func HAMLIB_PARAMS((RIG *rig,
                            vfo_t vfo,
                            setting_t func,
                            int status));
extern HAMLIB_EXPORT(int)
rig_get_func HAMLIB_PARAMS((RIG *rig,
                            vfo_t vfo,
                            setting_t func,
                            int *status));

extern HAMLIB_EXPORT(int)
rig_send_dtmf HAMLIB_PARAMS((RIG *rig,
                             vfo_t vfo,
                             const char *digits));
extern HAMLIB_EXPORT(int)
rig_recv_dtmf HAMLIB_PARAMS((RIG *rig,
                             vfo_t vfo,
                             char *digits,
                             int *length));

extern HAMLIB_EXPORT(int)
rig_send_morse HAMLIB_PARAMS((RIG *rig,
                              vfo_t vfo,
                              const char *msg));

extern HAMLIB_EXPORT(int)
rig_stop_morse HAMLIB_PARAMS((RIG *rig,
                              vfo_t vfo));

extern HAMLIB_EXPORT(int)
rig_wait_morse HAMLIB_PARAMS((RIG *rig,
                              vfo_t vfo));

extern HAMLIB_EXPORT(int)
rig_send_voice_mem HAMLIB_PARAMS((RIG *rig,
                              vfo_t vfo,
                              int ch));

extern HAMLIB_EXPORT(int)
rig_set_bank HAMLIB_PARAMS((RIG *rig,
                            vfo_t vfo,
                            int bank));

extern HAMLIB_EXPORT(int)
rig_set_mem HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           int ch));
extern HAMLIB_EXPORT(int)
rig_get_mem HAMLIB_PARAMS((RIG *rig,
                           vfo_t vfo,
                           int *ch));

extern HAMLIB_EXPORT(int)
rig_vfo_op HAMLIB_PARAMS((RIG *rig,
                          vfo_t vfo,
                          vfo_op_t op));

extern HAMLIB_EXPORT(vfo_op_t)
rig_has_vfo_op HAMLIB_PARAMS((RIG *rig,
                              vfo_op_t op));

extern HAMLIB_EXPORT(int)
rig_scan HAMLIB_PARAMS((RIG *rig,
                        vfo_t vfo,
                        scan_t scan,
                        int ch));

extern HAMLIB_EXPORT(scan_t)
rig_has_scan HAMLIB_PARAMS((RIG *rig,
                            scan_t scan));

extern HAMLIB_EXPORT(int)
rig_set_channel HAMLIB_PARAMS((RIG *rig, 
                               vfo_t vfo,
                               const channel_t *chan)); /* mem */
extern HAMLIB_EXPORT(int)
rig_get_channel HAMLIB_PARAMS((RIG *rig, 
                               vfo_t vfo,
                               channel_t *chan, int read_only));

extern HAMLIB_EXPORT(int)
rig_set_chan_all HAMLIB_PARAMS((RIG *rig, 
                                vfo_t vfo,
                                const channel_t chans[]));
extern HAMLIB_EXPORT(int)
rig_get_chan_all HAMLIB_PARAMS((RIG *rig, 
                                vfo_t vfo,
                                channel_t chans[]));

extern HAMLIB_EXPORT(int)
rig_set_chan_all_cb HAMLIB_PARAMS((RIG *rig, 
                                   vfo_t vfo,
                                   chan_cb_t chan_cb,
                                   rig_ptr_t));
extern HAMLIB_EXPORT(int)
rig_get_chan_all_cb HAMLIB_PARAMS((RIG *rig, 
                                   vfo_t vfo,
                                   chan_cb_t chan_cb,
                                   rig_ptr_t));

extern HAMLIB_EXPORT(int)
rig_set_mem_all_cb HAMLIB_PARAMS((RIG *rig,
                                  vfo_t vfo,
                                  chan_cb_t chan_cb,
                                  confval_cb_t parm_cb,
                                  rig_ptr_t));
extern HAMLIB_EXPORT(int)
rig_get_mem_all_cb HAMLIB_PARAMS((RIG *rig, 
                                  vfo_t vfo,
                                  chan_cb_t chan_cb,
                                  confval_cb_t parm_cb,
                                  rig_ptr_t));

extern HAMLIB_EXPORT(int)
rig_set_mem_all HAMLIB_PARAMS((RIG *rig,
                               vfo_t vfo,
                               const channel_t *chan,
                               const struct confparams *,
                               const value_t *));
extern HAMLIB_EXPORT(int)
rig_get_mem_all HAMLIB_PARAMS((RIG *rig,
                               vfo_t vfo,
                               channel_t *chan,
                               const struct confparams *,
                               value_t *));

extern HAMLIB_EXPORT(const chan_t *)
rig_lookup_mem_caps HAMLIB_PARAMS((RIG *rig,
                                   int ch));

extern HAMLIB_EXPORT(int)
rig_mem_count HAMLIB_PARAMS((RIG *rig));

extern HAMLIB_EXPORT(int)
rig_set_trn HAMLIB_PARAMS((RIG *rig,
                           int trn));
extern HAMLIB_EXPORT(int)
rig_get_trn HAMLIB_PARAMS((RIG *rig,
                           int *trn));

extern HAMLIB_EXPORT(int)
rig_set_freq_callback HAMLIB_PARAMS((RIG *,
                                     freq_cb_t,
                                     rig_ptr_t));

extern HAMLIB_EXPORT(int)
rig_set_mode_callback HAMLIB_PARAMS((RIG *,
                                     mode_cb_t,
                                     rig_ptr_t));
extern HAMLIB_EXPORT(int)
rig_set_vfo_callback HAMLIB_PARAMS((RIG *,
                                    vfo_cb_t,
                                    rig_ptr_t));

extern HAMLIB_EXPORT(int)
rig_set_ptt_callback HAMLIB_PARAMS((RIG *,
                                    ptt_cb_t,
                                    rig_ptr_t));

extern HAMLIB_EXPORT(int)
rig_set_dcd_callback HAMLIB_PARAMS((RIG *,
                                    dcd_cb_t,
                                    rig_ptr_t));

extern HAMLIB_EXPORT(int)
rig_set_pltune_callback HAMLIB_PARAMS((RIG *,
                                       pltune_cb_t,
                                       rig_ptr_t));

extern HAMLIB_EXPORT(int)
rig_set_twiddle HAMLIB_PARAMS((RIG *rig,
                                 int seconds));

extern HAMLIB_EXPORT(int)
rig_get_twiddle HAMLIB_PARAMS((RIG *rig,
                                 int *seconds));

extern HAMLIB_EXPORT(int)
rig_set_uplink HAMLIB_PARAMS((RIG *rig,
                                 int val));

extern HAMLIB_EXPORT(const char *)
rig_get_info HAMLIB_PARAMS((RIG *rig));

extern HAMLIB_EXPORT(const struct rig_caps *)
rig_get_caps HAMLIB_PARAMS((rig_model_t rig_model));

extern HAMLIB_EXPORT(const freq_range_t *)
rig_get_range HAMLIB_PARAMS((const freq_range_t *range_list,
                             freq_t freq,
                             rmode_t mode));

extern HAMLIB_EXPORT(pbwidth_t)
rig_passband_normal HAMLIB_PARAMS((RIG *rig,
                                   rmode_t mode));
extern HAMLIB_EXPORT(pbwidth_t)
rig_passband_narrow HAMLIB_PARAMS((RIG *rig,
                                   rmode_t mode));
extern HAMLIB_EXPORT(pbwidth_t)
rig_passband_wide HAMLIB_PARAMS((RIG *rig,
                                 rmode_t mode));

extern HAMLIB_EXPORT(const char *)
rigerror HAMLIB_PARAMS((int errnum));

extern HAMLIB_EXPORT(int)
rig_setting2idx HAMLIB_PARAMS((setting_t s));

extern HAMLIB_EXPORT(setting_t)
rig_idx2setting(int i);
/*
 * Even if these functions are prefixed with "rig_", they are not rig specific
 * Maybe "hamlib_" would have been better. Let me know. --SF
 */
extern HAMLIB_EXPORT(void)
rig_set_debug HAMLIB_PARAMS((enum rig_debug_level_e debug_level));

extern HAMLIB_EXPORT(void)
rig_set_debug_time_stamp HAMLIB_PARAMS((int flag));

#define rig_set_debug_level(level) rig_set_debug(level)

extern HAMLIB_EXPORT(int)
rig_need_debug HAMLIB_PARAMS((enum rig_debug_level_e debug_level));


// this need to be fairly big to avoid compiler warnings
#define DEBUGMSGSAVE_SIZE 24000
extern HAMLIB_EXPORT_VAR(char) debugmsgsave[DEBUGMSGSAVE_SIZE];  // last debug msg
extern HAMLIB_EXPORT_VAR(char) debugmsgsave2[DEBUGMSGSAVE_SIZE];  // last-1 debug msg
extern HAMLIB_EXPORT_VAR(char) debugmsgsave3[DEBUGMSGSAVE_SIZE];  // last-2 debug msg
#ifndef __cplusplus
#ifdef __GNUC__
// doing the debug macro with a dummy sprintf allows gcc to check the format string
#define rig_debug(debug_level,fmt,...) do { strncpy(debugmsgsave3, debugmsgsave2,sizeof(debugmsgsave3));strncpy(debugmsgsave2, debugmsgsave, sizeof(debugmsgsave2));snprintf(debugmsgsave,sizeof(debugmsgsave),fmt,__VA_ARGS__);rig_debug(debug_level,fmt,##__VA_ARGS__); } while(0);
#endif
#endif
extern HAMLIB_EXPORT(void)
rig_debug HAMLIB_PARAMS((enum rig_debug_level_e debug_level,
                         const char *fmt, ...));

extern HAMLIB_EXPORT(vprintf_cb_t)
rig_set_debug_callback HAMLIB_PARAMS((vprintf_cb_t cb,
                                      rig_ptr_t arg));

extern HAMLIB_EXPORT(FILE *)
rig_set_debug_file HAMLIB_PARAMS((FILE *stream));

extern HAMLIB_EXPORT(int)
rig_register HAMLIB_PARAMS((const struct rig_caps *caps));

extern HAMLIB_EXPORT(int)
rig_unregister HAMLIB_PARAMS((rig_model_t rig_model));

extern HAMLIB_EXPORT(int)
rig_list_foreach HAMLIB_PARAMS((int (*cfunc)(const struct rig_caps *, rig_ptr_t),
                                rig_ptr_t data));

extern HAMLIB_EXPORT(int)
rig_list_foreach_model HAMLIB_PARAMS((int (*cfunc)(const rig_model_t rig_model, rig_ptr_t),
                                rig_ptr_t data));

extern HAMLIB_EXPORT(int)
rig_load_backend HAMLIB_PARAMS((const char *be_name));

extern HAMLIB_EXPORT(int)
rig_check_backend HAMLIB_PARAMS((rig_model_t rig_model));

extern HAMLIB_EXPORT(int)
rig_load_all_backends HAMLIB_PARAMS((void));

typedef int (*rig_probe_func_t)(const hamlib_port_t *, rig_model_t, rig_ptr_t);

extern HAMLIB_EXPORT(int)
rig_probe_all HAMLIB_PARAMS((hamlib_port_t *p,
                             rig_probe_func_t,
                             rig_ptr_t));

extern HAMLIB_EXPORT(rig_model_t)
rig_probe HAMLIB_PARAMS((hamlib_port_t *p));


/* Misc calls */
extern HAMLIB_EXPORT(const char *) rig_strrmode(rmode_t mode);
extern HAMLIB_EXPORT(int)          rig_strrmodes(rmode_t modes, char *buf, int buflen);
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

extern HAMLIB_EXPORT(void) rig_no_restore_ai();

extern HAMLIB_EXPORT(int) rig_get_cache_timeout_ms(RIG *rig, hamlib_cache_t selection);
extern HAMLIB_EXPORT(int) rig_set_cache_timeout_ms(RIG *rig, hamlib_cache_t selection, int ms);

extern HAMLIB_EXPORT(int) rig_set_vfo_opt(RIG *rig, int status);
extern HAMLIB_EXPORT(int) rig_get_vfo_info(RIG *rig, vfo_t vfo, freq_t *freq, rmode_t *mode, pbwidth_t *width, split_t *split, int *satmode);
extern HAMLIB_EXPORT(int) rig_get_rig_info(RIG *rig, char *response, int max_response_len);
extern HAMLIB_EXPORT(int) rig_get_cache(RIG *rig, vfo_t vfo, freq_t *freq, int * cache_ms_freq, rmode_t *mode, int *cache_ms_mode, pbwidth_t *width, int *cache_ms_width);


typedef unsigned long rig_useconds_t;
extern HAMLIB_EXPORT(int) hl_usleep(rig_useconds_t msec);

//! @endcond

__END_DECLS

#endif /* _RIG_H */

/*! @} */
