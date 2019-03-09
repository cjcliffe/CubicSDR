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

#define ROT_MAKE_MODEL(a,b) ((a)*100+(b))
#define ROT_BACKEND_NUM(a) ((a)/100)


/**
 * \addtogroup rotator
 * @{
 */

/**
 *  \file rotlist.h
 *  \brief Hamlib rotator model definitions.
 *
 *  This file contains rotator model definitions for the Hamlib rotator API.
 *  Each distinct rotator type has a unique model number (ID) and is used by
 *  hamlib to identify and distinguish between the different hardware drivers.
 *  The exact model numbers can be acquired using the macros in this file. To
 *  obtain a list of supported rotator branches, one can use the statically
 *  defined ROT_BACKEND_LIST macro. To obtain a full list of supported
 *  rotators (including each model in every branch), the foreach_opened_rot()
 *  API function can be used.
 *
 *  The model number, or ID, is used to tell Hamlib which rotator the client
 *  whishes to use. It is done with the rot_init() API call.
 */


#define ROT_MODEL_NONE 0


/**
 *  \def ROT_MODEL_DUMMY
 *  \brief A macro that returns the model number for the dummy backend.
 *
 *  The dummy backend, as the name suggests, is a backend which performs
 *  no hardware operations and always behaves as one would expect. It can
 *  be thought of as a hardware simulator and is very usefull for testing
 *  client applications.
 */
/**
 *  \def ROT_MODEL_NETROTCTL
 *  \brief A macro that returns the model number for the Network backend.
 *
 *  This backend allows use of the rotctld daemon through the normal
 *  Hamlib API.
 */
#define ROT_DUMMY 0
#define ROT_BACKEND_DUMMY "dummy"
#define ROT_MODEL_DUMMY ROT_MAKE_MODEL(ROT_DUMMY, 1)
#define ROT_MODEL_NETROTCTL ROT_MAKE_MODEL(ROT_DUMMY, 2)


/*
 * Easycomm
 */

/**
 *  \def ROT_MODEL_EASYCOMM1
 *  \brief A macro that returns the model number of the EasyComm 1 backend.
 *
 *  The EasyComm 1 backend can be used with rotators that support the
 *  EASYCOMM I Standard.
 */
/**
 *  \def ROT_MODEL_EASYCOMM2
 *  \brief A macro that returns the model number of the EasyComm 2 backend.
 *
 *  The EasyComm 2 backend can be used with rotators that support the
 *  EASYCOMM II Standard.
 */
#define ROT_EASYCOMM 2
#define ROT_BACKEND_EASYCOMM "easycomm"
#define ROT_MODEL_EASYCOMM1 ROT_MAKE_MODEL(ROT_EASYCOMM, 1)
#define ROT_MODEL_EASYCOMM2 ROT_MAKE_MODEL(ROT_EASYCOMM, 2)
#define ROT_MODEL_TRAKBOX ROT_MAKE_MODEL(ROT_EASYCOMM, 3)
#define ROT_MODEL_EASYCOMM3 ROT_MAKE_MODEL(ROT_EASYCOMM, 4)


/**
 *  \def ROT_MODEL_FODTRACK
 *  \brief A macro that returns the model number of the Fodtrack backend.
 *
 *  The Fodtrack backend can be used with rotators that support the
 *  FODTRACK Standard.
 */
#define ROT_FODTRACK 3
#define ROT_BACKEND_FODTRACK "fodtrack"
#define ROT_MODEL_FODTRACK ROT_MAKE_MODEL(ROT_FODTRACK, 1)


/**
 *  \def ROT_MODEL_ROTOREZ
 *  \brief A macro that returns the model number of the Rotor-EZ backend.
 *
 *  The Rotor-EZ backend can be used with Hy-Gain rotators that support
 *  the extended DCU command set by Idiom Press Rotor-EZ board.
 */
/**
 *  \def ROT_MODEL_ROTORCARD
 *  \brief A macro that returns the model number of the Rotor Card backend.
 *
 *  The Rotor-EZ backend can be used with Yaesu rotators that support the
 *  extended DCU command set by Idiom Press Rotor Card board.
 */
/**
 *  \def ROT_MODEL_DCU
 *  \brief A macro that returns the model number of the DCU backend.
 *
 *  The Rotor-EZ backend can be used with rotators that support the DCU
 *  command set by Hy-Gain (currently the DCU-1).
 */
/**
 *  \def ROT_MODEL_ERC
 *  \brief A macro that returns the model number of the ERC backend.
 *
 *  The Rotor-EZ backend can be used with rotators that support the DCU
 *  command set by DF9GR (currently the ERC).
 */
/**
 *  \def ROT_MODEL_RT21
 *  \brief A macro that returns the model number of the RT21 backend.
 *
 *  The Rotor-EZ backend can be used with rotators that support the DCU
 *  command set by Green Heron (currently the RT-21).
 */
#define ROT_ROTOREZ 4
#define ROT_BACKEND_ROTOREZ "rotorez"
#define ROT_MODEL_ROTOREZ ROT_MAKE_MODEL(ROT_ROTOREZ, 1)
#define ROT_MODEL_ROTORCARD ROT_MAKE_MODEL(ROT_ROTOREZ, 2)
#define ROT_MODEL_DCU ROT_MAKE_MODEL(ROT_ROTOREZ, 3)
#define ROT_MODEL_ERC ROT_MAKE_MODEL(ROT_ROTOREZ, 4)
#define ROT_MODEL_RT21 ROT_MAKE_MODEL(ROT_ROTOREZ, 5)


/**
 *  \def ROT_MODEL_SARTEK1
 *  \brief A macro that returns the model number of the SARtek-1 backend.
 *
 *  The sartek backend can be used with rotators that support the SARtek
 *  protocol.
 */
#define ROT_SARTEK 5
#define ROT_BACKEND_SARTEK "sartek"
#define ROT_MODEL_SARTEK1 ROT_MAKE_MODEL(ROT_SARTEK, 1)


/**
 *  \def ROT_MODEL_GS232A
 *  \brief A macro that returns the model number of the GS-232A backend.
 *
 *  The GS-232A backend can be used with rotators that support the GS-232A
 *  protocol.
 */
/**
 *  \def ROT_MODEL_GS232
 *  \brief A macro that returns the model number of the GS-232 backend.
 *
 *  The GS-232 backend can be used with rotators that support the GS-232
 *  protocol.
 */
/**
 *  \def ROT_MODEL_GS232B
 *  \brief A macro that returns the model number of the GS-232B backend.
 *
 *  The GS-232B backend can be used with rotators that support the GS-232B
 *  protocol.
 */
/**
 *  \def ROT_MODEL_F1TETRACKER
 *  \brief A macro that returns the model number of the F1TETRACKER backend.
 *
 *  The F1TETRACKER backend can be used with rotators that support the
 *  F1TETRACKER protocol.
 */
#define ROT_GS232A 6
#define ROT_BACKEND_GS232A "gs232a"
#define ROT_MODEL_GS232A ROT_MAKE_MODEL(ROT_GS232A, 1)
#define ROT_MODEL_GS232 ROT_MAKE_MODEL(ROT_GS232A, 2) /* Not A or B */
#define ROT_MODEL_GS232B ROT_MAKE_MODEL(ROT_GS232A, 3)
#define ROT_MODEL_F1TETRACKER ROT_MAKE_MODEL(ROT_GS232A, 4)


/**
 *  \def ROT_MODEL_PCROTOR
 *  \brief A macro that returns the model number of the PcRotor/WA6UFQ backend.
 *
 *  The kit backend can be used with home brewed rotators.
 */
#define ROT_KIT 7
#define ROT_BACKEND_KIT "kit"
#define ROT_MODEL_PCROTOR ROT_MAKE_MODEL(ROT_KIT, 1)


/**
 *  \def ROT_MODEL_HD1780
 *  \brief A macro that returns the model number of the HD 1780 backend.
 */
#define ROT_HEATHKIT 8
#define ROT_BACKEND_HEATHKIT "heathkit"
#define ROT_MODEL_HD1780 ROT_MAKE_MODEL(ROT_HEATHKIT, 1)


/**
 *  \def ROT_MODEL_SPID_ROT2PROG
 *  \brief A macro that returns the model number of the ROT2PROG backend.
 *
 *  The SPID backend can be used with rotators that support the SPID protocol.
 */
/**
 *  \def ROT_MODEL_SPID_ROT1PROG
 *  \brief A macro that returns the model number of the ROT1PROG backend.
 *
 *  The SPID backend can be used with rotators that support the SPID protocol.
 */
/**
 *  \def ROT_MODEL_SPID_MD01_ROT2PROG
 *  \brief A macro that returns the model number of the MD-01/02 (ROT2PROG protocol) backend.
 *
 *  The SPID backend can be used with rotators that support the SPID protocol.
 */
#define ROT_SPID 9
#define ROT_BACKEND_SPID "spid"
#define ROT_MODEL_SPID_ROT2PROG ROT_MAKE_MODEL(ROT_SPID, 1)
#define ROT_MODEL_SPID_ROT1PROG ROT_MAKE_MODEL(ROT_SPID, 2)
#define ROT_MODEL_SPID_MD01_ROT2PROG ROT_MAKE_MODEL(ROT_SPID, 3)


/**
 *  \def ROT_MODEL_RC2800
 *  \brief A macro that returns the model number of the RC2800 backend.
 *
 *  The M2 backend can be used with rotators that support the RC2800 protocol
 *  and alike.
 */
#define ROT_M2 10
#define ROT_BACKEND_M2 "m2"
#define ROT_MODEL_RC2800 ROT_MAKE_MODEL(ROT_M2, 1)


/**
 *  \def ROT_MODEL_RCI_AZEL
 *  \brief A macro that returns the model number of the RCI_AZEL backend.
 *
 *  The ARS backend can be used with rotators that support the ARS protocol.
 */
/**
 *  \def ROT_MODEL_RCI_AZ
 *  \brief A macro that returns the model number of the RCI_AZ backend.
 *
 *  The ARS backend can be used with rotators that support the ARS protocol.
 */
#define ROT_ARS 11
#define ROT_BACKEND_ARS "ars"
#define ROT_MODEL_RCI_AZEL ROT_MAKE_MODEL(ROT_ARS, 1)
#define ROT_MODEL_RCI_AZ ROT_MAKE_MODEL(ROT_ARS, 2)


/**
 *  \def ROT_MODEL_IF100
 *  \brief A macro that returns the model number of the IF-100 backend.
 *
 *  The AMSAT backend can be used with rotators that support, among other, the
 *  IF-100 interface.
 */
#define ROT_AMSAT 12
#define ROT_BACKEND_AMSAT "amsat"
#define ROT_MODEL_IF100 ROT_MAKE_MODEL(ROT_AMSAT, 1)


/**
 *  \def ROT_MODEL_TS7400
 *  \brief A macro that returns the model number of the TS7400 backend.
 *
 *  The TS-7400 backend supports and embedded ARM board using the TS-7400
 *  Linux board.  More information is at http://www.embeddedarm.com
 */
#define ROT_TS7400 13
#define ROT_BACKEND_TS7400 "ts7400"
#define ROT_MODEL_TS7400 ROT_MAKE_MODEL(ROT_TS7400, 1)


/**
 *  \def ROT_MODEL_NEXSTAR
 *  \brief A macro that returns the model number of the NEXSTAR backend.
 *
 *  The CELESTRON backend can be used with rotators that support the Celestron
 *  protocol and alike.
 */
#define ROT_CELESTRON 14
#define ROT_BACKEND_CELESTRON "celestron"
#define ROT_MODEL_NEXSTAR ROT_MAKE_MODEL(ROT_CELESTRON, 1)


/**
 *  \def ROT_MODEL_ETHER6
 *  \brief A macro that returns the model number of the Ether6 backend.
 *
 *  The Ether6 backend can be used with rotators that support the Ether6
 *  protocol and alike.
 */
#define ROT_ETHER6 15
#define ROT_BACKEND_ETHER6 "ether6"
#define ROT_MODEL_ETHER6 ROT_MAKE_MODEL(ROT_ETHER6, 1)


/**
 *  \def ROT_MODEL_CNCTRK
 *  \brief A macro that returns the model number of the CNCTRK backend.
 *
 *  The CNCTRK backend can be used with rotators that support the LinuxCNC
 *  running Axis GUI interface.
 */
#define ROT_CNCTRK 16
#define ROT_BACKEND_CNCTRK "cnctrk"
#define ROT_MODEL_CNCTRK ROT_MAKE_MODEL(ROT_CNCTRK, 1)


/**
 *  \def ROT_MODEL_PROSISTEL
 *  \brief A macro that returns the model number of the PROSISTEL backend.
 *
 */
#define ROT_PROSISTEL 17
#define ROT_BACKEND_PROSISTEL "prosistel"
#define ROT_MODEL_PROSISTEL ROT_MAKE_MODEL(ROT_PROSISTEL, 1)


/**
 *  \typedef typedef int rot_model_t
 *  \brief Convenience type definition for rotator model.
*/
typedef int rot_model_t;


#endif /* _ROTLIST_H */

/** @} */
