/*
 *  Hamlib Interface - list of known rotators
 *  Copyright (c) 2000-2011 by Stephane Fillod
 *  Copyright (c) 2000-2002 by Frank Singleton
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

#ifndef _ROTLIST_H
#define _ROTLIST_H 1

//! @cond Doxygen_Suppress
#define ROT_MAKE_MODEL(a,b) ((a)*100+(b))
#define ROT_BACKEND_NUM(a) ((a)/100)
//! @endcond


/**
 * \addtogroup rotator
 * @{
 */

/**
 * \file rotlist.h
 * \brief Hamlib rotator model definitions.
 *
 * This file contains rotator model definitions for the Hamlib rotator
 * Application Programming Interface (API).  Each distinct rotator type has a
 * unique model number (ID) and is used by Hamlib to identify and distinguish
 * between the different hardware drivers.  The exact model numbers can be
 * acquired using the macros in this file.  To obtain a list of supported
 * rotator branches, one can use the statically defined ROT_BACKEND_LIST macro
 * (defined in configure.ac).  To obtain a full list of supported rotators
 * (including each model in every branch), the foreach_opened_rot() API
 * function can be used.
 *
 * The model number, or ID, is used to tell Hamlib which rotator the client
 * wishes to use which is done with the rot_init() API call.
 */

/**
 * \def ROT_MODEL_NONE
 * \brief A macro that returns the model number for an unknown model.
 *
 * The none backend, as the name suggests, does nothing.  It is mainly for
 * internal use.
 */
#define ROT_MODEL_NONE 0


/**
 * \brief A macro that returns the model number for the DUMMY backend.
 *
 * \def ROT_MODEL_DUMMY
 *
 * The DUMMY backend, as the name suggests, is a backend which performs
 * no hardware operations and always behaves as one would expect.  It can
 * be thought of as a hardware simulator and is very useful for testing
 * client applications.
 */
/**
 * \brief A macro that returns the model number for the NETROTCTL backend.
 *
 * \def ROT_MODEL_NETROTCTL
 *
 * The NETROTCTL backend allows use of the `rotctld` daemon through the normal
 * Hamlib API.
 */
//! @cond Doxygen_Suppress
#define ROT_DUMMY 0
#define ROT_BACKEND_DUMMY "dummy"
//! @endcond
#define ROT_MODEL_DUMMY ROT_MAKE_MODEL(ROT_DUMMY, 1)
#define ROT_MODEL_NETROTCTL ROT_MAKE_MODEL(ROT_DUMMY, 2)


/**
 * \brief A macro that returns the model number of the EASYCOMM 1 backend.
 *
 * \def ROT_MODEL_EASYCOMM1
 *
 * The EASYCOMM1 backend can be used with rotators that support the EASYCOMM
 * I Standard.
 */
/**
 * \brief A macro that returns the model number of the EASYCOMM 2 backend.
 *
 * \def ROT_MODEL_EASYCOMM2
 *
 * The EASYCOMM2 backend can be used with rotators that support the EASYCOMM
 * II Standard.
 */
/**
 * \brief A macro that returns the model number of the EASYCOMM 3 backend.
 *
 * \def ROT_MODEL_EASYCOMM3
 *
 * The EASYCOMM3 backend can be used with rotators that support the EASYCOMM
 * III Standard.
 */
//! @cond Doxygen_Suppress
#define ROT_EASYCOMM 2
#define ROT_BACKEND_EASYCOMM "easycomm"
//! @endcond
#define ROT_MODEL_EASYCOMM1 ROT_MAKE_MODEL(ROT_EASYCOMM, 1)
#define ROT_MODEL_EASYCOMM2 ROT_MAKE_MODEL(ROT_EASYCOMM, 2)
#define ROT_MODEL_EASYCOMM3 ROT_MAKE_MODEL(ROT_EASYCOMM, 4)


/**
 * \brief A macro that returns the model number of the FODTRACK backend.
 *
 * \def ROT_MODEL_FODTRACK
 *
 * The FODTRACK backend can be used with rotators that support the FODTRACK
 * Standard.
 */
//! @cond Doxygen_Suppress
#define ROT_FODTRACK 3
#define ROT_BACKEND_FODTRACK "fodtrack"
//! @endcond
#define ROT_MODEL_FODTRACK ROT_MAKE_MODEL(ROT_FODTRACK, 1)


/**
 * \brief A macro that returns the model number of the ROTOREZ backend.
 *
 * \def ROT_MODEL_ROTOREZ
 *
 * The ROTOREZ backend can be used with Hy-Gain rotators that support the
 * extended DCU command set by the Idiom Press Rotor-EZ board.
 */
/**
 * \brief A macro that returns the model number of the ROTORCARD backend.
 *
 * \def ROT_MODEL_ROTORCARD
 *
 * The ROTORCARD backend can be used with Yaesu rotators that support the
 * extended DCU command set by the Idiom Press Rotor Card board.
 */
/**
 * \brief A macro that returns the model number of the DCU backend.
 *
 * \def ROT_MODEL_DCU
 *
 * The DCU backend can be used with rotators that support the DCU command set
 * by Hy-Gain (currently the DCU-1).
 */
/**
 * \brief A macro that returns the model number of the ERC backend.
 *
 * \def ROT_MODEL_ERC
 *
 * The ERC backend can be used with rotators that support the DCU command set
 * by DF9GR (currently the ERC).
 */
/**
 * \brief A macro that returns the model number of the RT21 backend.
 *
 * \def ROT_MODEL_RT21
 *
 * The RT21 backend can be used with rotators that support the DCU command set
 * by Green Heron (currently the RT-21).
 */
//! @cond Doxygen_Suppress
#define ROT_ROTOREZ 4
#define ROT_BACKEND_ROTOREZ "rotorez"
//! @endcond
#define ROT_MODEL_ROTOREZ ROT_MAKE_MODEL(ROT_ROTOREZ, 1)
#define ROT_MODEL_ROTORCARD ROT_MAKE_MODEL(ROT_ROTOREZ, 2)
#define ROT_MODEL_DCU ROT_MAKE_MODEL(ROT_ROTOREZ, 3)
#define ROT_MODEL_ERC ROT_MAKE_MODEL(ROT_ROTOREZ, 4)
#define ROT_MODEL_RT21 ROT_MAKE_MODEL(ROT_ROTOREZ, 5)


/**
 * \brief A macro that returns the model number of the SARTEK1 backend.
 *
 * \def ROT_MODEL_SARTEK1
 *
 * The SARTEK1 backend can be used with rotators that support the SARtek
 * protocol.
 */
//! @cond Doxygen_Suppress
#define ROT_SARTEK 5
#define ROT_BACKEND_SARTEK "sartek"
//! @endcond
#define ROT_MODEL_SARTEK1 ROT_MAKE_MODEL(ROT_SARTEK, 1)


/**
 * \brief A macro that returns the model number of the GS232A backend.
 *
 * \def ROT_MODEL_GS232A
 *
 * The GS232A backend can be used with rotators that support the GS-232A
 * protocol.
 */
/**
 * \brief A macro that returns the model number of the GS232 backend.
 *
 * \def ROT_MODEL_GS232_GENERIC
 *
 * The GS232_GENERIC backend can be used with rotators that support the
 * generic (even if not coded correctly) GS-232 protocol.
 */
/**
 * \brief A macro that returns the model number of the GS232B backend.
 *
 * \def ROT_MODEL_GS232B
 *
 * The GS232B backend can be used with rotators that support the GS232B
 * protocol.
 */
/**
 * \brief A macro that returns the model number of the F1TETRACKER backend.
 *
 * \def ROT_MODEL_F1TETRACKER
 *
 * The F1TETRACKER backend can be used with rotators that support the F1TE
 * Tracker protocol.
 */
/**
 * \brief A macro that returns the model number of the GS23 backend.
 *
 * \def ROT_MODEL_GS23
 *
 * The GS23 backend can be used with rotators that support the GS-23 protocol.
 */
/**
 * \brief A macro that returns the model number of the GS232 backend.
 *
 * \def ROT_MODEL_GS232
 *
 * The GS232 backend can be used with rotators that support the GS-232
 * protocol.
 */
/**
 * \brief A macro that returns the model number of the LVB  backend.
 *
 * \def ROT_MODEL_LVB
 *
 * The LVB backend can be used with rotators that support the G6LVB AMSAT LVB
 * Tracker GS-232 based protocol.
 */
/**
 * \brief A macro that returns the model number of the ST2 backend.
 *
 * \def ROT_MODEL_ST2
 *
 * The ST2 backend can be used with rotators that support the Fox Delta ST2
 * GS-232 based protocol.
 */
/**
 * \brief A macro that returns the model number of the GS232A_AZ Azimuth backend.
 *
 * \def ROT_MODEL_GS232A_AZ
 *
 * The GS232A_AZ backend can be used with azimuth rotators that support the
 * GS-232A protocol.
 */
/**
 * \brief A macro that returns the model number of the GS232A_EL Elevation backend.
 *
 * \def ROT_MODEL_GS232A_EL
 *
 * The GS232A_EL backend can be used with elevation rotators that support the
 * GS-232A protocol.
 */
/**
 * \brief A macro that returns the model number of the GS232B_AZ Azimuth backend.
 *
 * \def ROT_MODEL_GS232B_AZ
 *
 * The GS232B_AZ backend can be used with azimuth rotators that support the
 * GS-232B protocol.
 */
/**
 * \brief A macro that returns the model number of the GS232B_EL Elevation backend.
 *
 * \def ROT_MODEL_GS232B_EL
 *
 * The GS232B_EL backend can be used with elevation rotators that support the
 * GS-232B protocol.
 */

//! @cond Doxygen_Suppress
#define ROT_GS232A 6
#define ROT_BACKEND_GS232A "gs232a"
//! @endcond
#define ROT_MODEL_GS232A ROT_MAKE_MODEL(ROT_GS232A, 1)
#define ROT_MODEL_GS232_GENERIC ROT_MAKE_MODEL(ROT_GS232A, 2) /* GENERIC */
#define ROT_MODEL_GS232B ROT_MAKE_MODEL(ROT_GS232A, 3)
#define ROT_MODEL_F1TETRACKER ROT_MAKE_MODEL(ROT_GS232A, 4)
#define ROT_MODEL_GS23 ROT_MAKE_MODEL(ROT_GS232A, 5)
#define ROT_MODEL_GS232 ROT_MAKE_MODEL(ROT_GS232A, 6) /* Not A or B */
#define ROT_MODEL_LVB ROT_MAKE_MODEL(ROT_GS232A, 7)
#define ROT_MODEL_ST2 ROT_MAKE_MODEL(ROT_GS232A, 8)
#define ROT_MODEL_GS232A_AZ ROT_MAKE_MODEL(ROT_GS232A, 9)
#define ROT_MODEL_GS232A_EL ROT_MAKE_MODEL(ROT_GS232A, 10)
#define ROT_MODEL_GS232B_AZ ROT_MAKE_MODEL(ROT_GS232A, 11)
#define ROT_MODEL_GS232B_EL ROT_MAKE_MODEL(ROT_GS232A, 12)


/**
 * \brief A macro that returns the model number of the PCROTOR backend.
 *
 * \def ROT_MODEL_PCROTOR
 *
 * The PCROTOR backend is a member of the kit backend group that can be used
 * with home brewed rotators.
 */
//! @cond Doxygen_Suppress
#define ROT_KIT 7
#define ROT_BACKEND_KIT "kit"
//! @endcond
#define ROT_MODEL_PCROTOR ROT_MAKE_MODEL(ROT_KIT, 1)


/**
 * \brief A macro that returns the model number of the HD1780 backend.
 *
 * \def ROT_MODEL_HD1780
 *
 * The HD1780 backend can be used with rotators that support the Heathkit
 * HD-1780 protocol.
 */
//! @cond Doxygen_Suppress
#define ROT_HEATHKIT 8
#define ROT_BACKEND_HEATHKIT "heathkit"
//! @endcond
#define ROT_MODEL_HD1780 ROT_MAKE_MODEL(ROT_HEATHKIT, 1)


/**
 * \brief A macro that returns the model number of the ROT2PROG backend.
 *
 * \def ROT_MODEL_SPID_ROT2PROG
 *
 * The SPID_ROT2PROG backend can be used with rotators that support the SPID
 * azimuth and elevation protocol.
 */
/**
 * \brief A macro that returns the model number of the ROT1PROG backend.
 *
 * \def ROT_MODEL_SPID_ROT1PROG
 *
 * The SPID_ROT1PROG backend can be used with rotators that support the SPID
 * azimuth protocol.
 */
/**
 * \brief A macro that returns the model number of the SPID_MD01_ROT2PROG backend.
 *
 * \def ROT_MODEL_SPID_MD01_ROT2PROG
 *
 * The SPID_MD01_ROT2PROG backend can be used with rotators that support the
 * extended SPID ROT2PROG azimuth and elevation protocol.
 */
//! @cond Doxygen_Suppress
#define ROT_SPID 9
#define ROT_BACKEND_SPID "spid"
//! @endcond
#define ROT_MODEL_SPID_ROT2PROG ROT_MAKE_MODEL(ROT_SPID, 1)
#define ROT_MODEL_SPID_ROT1PROG ROT_MAKE_MODEL(ROT_SPID, 2)
#define ROT_MODEL_SPID_MD01_ROT2PROG ROT_MAKE_MODEL(ROT_SPID, 3)


/**
 * \brief A macro that returns the model number of the RC2800 backend.
 *
 * \def ROT_MODEL_RC2800
 *
 * The RC2800 backend can be used with rotators that support the M2 (M
 * Squared) RC2800 protocol.
 */
/**
 * \brief A macro that returns the model number of the RC2800_EARLY_AZ
 * backend.
 *
 * \def ROT_MODEL_RC2800_EARLY_AZ
 *
 * The RC2800_EARLY_AZ backend can be used with rotators that support the M2
 * (M Squared) RC2800 early azimuth protocol.
 */
/**
 * \brief A macro that returns the model number of the RC2800_EARLY_AZEL
 * backend.
 *
 * \def ROT_MODEL_RC2800_EARLY_AZEL
 *
 * The RC2800_EARLY_AZEL backend can be used with rotators that support the M2
 * (M Squared) RC2800 early azimuth and elevation protocol.
 */
//! @cond Doxygen_Suppress
#define ROT_M2 10
#define ROT_BACKEND_M2 "m2"
//! @endcond
#define ROT_MODEL_RC2800 ROT_MAKE_MODEL(ROT_M2, 1)
#define ROT_MODEL_RC2800_EARLY_AZ ROT_MAKE_MODEL(ROT_M2, 2)
#define ROT_MODEL_RC2800_EARLY_AZEL ROT_MAKE_MODEL(ROT_M2, 3)


/**
 * \brief A macro that returns the model number of the RCI_AZEL backend.
 *
 * \def ROT_MODEL_RCI_AZEL
 *
 * The RCI_AZEL backend can be used with rotators that support the ARS azimuth
 * and elevation protocol.
 */
/**
 * \brief A macro that returns the model number of the RCI_AZ backend.
 *
 * \def ROT_MODEL_RCI_AZ
 *
 * The RCI_AZ backend can be used with rotators that support the ARS azimuth
 * protocol.
 */
//! @cond Doxygen_Suppress
#define ROT_ARS 11
#define ROT_BACKEND_ARS "ars"
//! @endcond
#define ROT_MODEL_RCI_AZEL ROT_MAKE_MODEL(ROT_ARS, 1)
#define ROT_MODEL_RCI_AZ ROT_MAKE_MODEL(ROT_ARS, 2)


/**
 * \brief A macro that returns the model number of the IF100 backend.
 *
 * \def ROT_MODEL_IF100
 *
 * The IF100 backend can be used with rotators that support the AMSAT IF-100
 * interface.
 */
//! @cond Doxygen_Suppress
#define ROT_AMSAT 12
#define ROT_BACKEND_AMSAT "amsat"
//! @endcond
#define ROT_MODEL_IF100 ROT_MAKE_MODEL(ROT_AMSAT, 1)


/**
 * \brief A macro that returns the model number of the TS7400 backend.
 *
 * \def ROT_MODEL_TS7400
 *
 * The TS7400 backend supports an embedded ARM board using the TS-7400 Linux
 * board.  More information is at https://www.embeddedarm.com
 */
//! @cond Doxygen_Suppress
#define ROT_TS7400 13
#define ROT_BACKEND_TS7400 "ts7400"
//! @endcond
#define ROT_MODEL_TS7400 ROT_MAKE_MODEL(ROT_TS7400, 1)


/**
 * \brief A macro that returns the model number of the NEXSTAR backend.
 *
 * \def ROT_MODEL_NEXSTAR
 *
 * The NEXSTAR backend can be used with rotators that support the Celestron
 * NexStar protocol and alike.
 */
//! @cond Doxygen_Suppress
#define ROT_CELESTRON 14
#define ROT_BACKEND_CELESTRON "celestron"
//! @endcond
#define ROT_MODEL_NEXSTAR ROT_MAKE_MODEL(ROT_CELESTRON, 1)


/**
 * \brief A macro that returns the model number of the ETHER6 backend.
 *
 * \def ROT_MODEL_ETHER6
 *
 * The ETHER6 backend can be used with rotators that support the Ether6
 * protocol.
 */
//! @cond Doxygen_Suppress
#define ROT_ETHER6 15
#define ROT_BACKEND_ETHER6 "ether6"
//! @endcond
#define ROT_MODEL_ETHER6 ROT_MAKE_MODEL(ROT_ETHER6, 1)


/**
 * \brief A macro that returns the model number of the CNCTRK backend.
 *
 * \def ROT_MODEL_CNCTRK
 *
 * The CNCTRK backend can be used with rotators that support the LinuxCNC
 * running Axis GUI interface.
 */
//! @cond Doxygen_Suppress
#define ROT_CNCTRK 16
#define ROT_BACKEND_CNCTRK "cnctrk"
//! @endcond
#define ROT_MODEL_CNCTRK ROT_MAKE_MODEL(ROT_CNCTRK, 1)


/**
 * \brief A macro that returns the model number of the PROSISTEL_D_AZ backend.
 *
 * \def ROT_MODEL_PROSISTEL_D_AZ
 *
 * The PROSISTEL_D_AZ backend can be used with rotators that support the Prosistel
 * azimuth protocol.
 */
/**
 * \brief A macro that returns the model number of the PROSISTEL_D_EL backend.
 *
 * \def ROT_MODEL_PROSISTEL_D_EL
 *
 * The PROSISTEL_D_EL backend can be used with rotators that support the Prosistel
 * elevation protocol.
 */
/**
 * \brief A macro that returns the model number of the
 * PROSISTEL_COMBI_TRACK_AZEL backend.
 *
 * \def ROT_MODEL_PROSISTEL_COMBI_TRACK_AZEL
 *
 * The PROSISTEL_AZEL_COMBI_TRACK_AZEL backend can be used with rotators that
 * support the Prosistel combination azimuth and elevation protocol.
 */
//! @cond Doxygen_Suppress
#define ROT_PROSISTEL 17
#define ROT_BACKEND_PROSISTEL "prosistel"
//! @endcond
#define ROT_MODEL_PROSISTEL_D_AZ ROT_MAKE_MODEL(ROT_PROSISTEL, 1)
#define ROT_MODEL_PROSISTEL_D_EL ROT_MAKE_MODEL(ROT_PROSISTEL, 2)
#define ROT_MODEL_PROSISTEL_COMBI_TRACK_AZEL ROT_MAKE_MODEL(ROT_PROSISTEL, 3)


/**
 * \brief A macro that returns the model number of the MEADE backend.
 *
 * \def ROT_MODEL_MEADE
 *
 * The MEADE backend can be used with Meade telescope rotators like the
 * DS-2000.
 */
//! @cond Doxygen_Suppress
#define ROT_MEADE 18
#define ROT_BACKEND_MEADE "meade"
//! @endcond
#define ROT_MODEL_MEADE ROT_MAKE_MODEL(ROT_MEADE, 1)

/**
 * \brief A macro that returns the model number of the IOPTRON backend.
 *
 * \def ROT_MODEL_IOPTRON
 *
 * The IOPTRON backend can be used with IOPTRON telescope mounts.
 */
//! @cond Doxygen_Suppress
#define ROT_IOPTRON 19
#define ROT_BACKEND_IOPTRON "ioptron"
//! @endcond
#define ROT_MODEL_IOPTRON ROT_MAKE_MODEL(ROT_IOPTRON, 1)


/**
 * \brief A macro that returns the model number of the INDI backend.
 *
 * \def ROT_MODEL_INDI
 *
 * The INDI backend can be used with rotators that support the INDI interface.
 */
//! @cond Doxygen_Suppress
#define ROT_INDI 20
#define ROT_BACKEND_INDI "indi"
//! @endcond
#define ROT_MODEL_INDI ROT_MAKE_MODEL(ROT_INDI, 1)


/**
 * \brief A macro that returns the model number of the SATEL backend.
 *
 * \def ROT_MODEL_SATEL
 *
 * The SATEL backend can be used with rotators that support the VE5FP
 * interface.
 */
//! @cond Doxygen_Suppress
#define ROT_SATEL 21
#define ROT_BACKEND_SATEL "satel"
//! @endcond
#define ROT_MODEL_SATEL ROT_MAKE_MODEL(ROT_SATEL, 1)


/**
 * \brief A macro that returns the model number of the RADANT backend.
 *
 * \def ROT_MODEL_RADANT
 *
 * The RADANT backend can be used with rotators that support the MS232
 * interface.
 */
//! @cond Doxygen_Suppress
#define ROT_RADANT 22
#define ROT_BACKEND_RADANT "radant"
//! @endcond
#define ROT_MODEL_RADANT ROT_MAKE_MODEL(ROT_RADANT, 1)


/**
 * \brief Convenience type definition for a rotator model.
 *
 * \typedef typedef int rot_model_t
*/
typedef int rot_model_t;


#endif /* _ROTLIST_H */

/** @} */
