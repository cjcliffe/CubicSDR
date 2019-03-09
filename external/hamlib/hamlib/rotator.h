/*
 *  Hamlib Interface - Rotator API header
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

#ifndef _ROTATOR_H
#define _ROTATOR_H 1

#include <hamlib/rig.h>
#include <hamlib/rotlist.h>

/**
 * \addtogroup rotator
 * @{
 */

/**
 *  \file rotator.h
 *  \brief Hamlib rotator data structures.
 *
 *  This file contains the data structures and declarations for the Hamlib
 *  rotator API.  see the rotator.c file for more details on the rotator API.
 */



__BEGIN_DECLS

/* Forward struct references */

struct rot;
struct rot_state;


/**
 *  \typedef typedef struct rot ROT
 *  \brief Rotator structure definition (see rot for details).
 */
typedef struct rot ROT;


/**
 *  \typedef typedef float elevation_t
 *  \brief Type definition for elevation.
 *
 *  The elevation_t type is used as parameter for the rot_set_position() and
 *  rot_get_position() functions.
 *
 *  Unless specified otherwise, the unit of elevation_t is decimal degrees.
 */
typedef float elevation_t;


/**
 *  \typedef typedef float azimuth_t
 *  \brief Type definition for azimuth.
 *
 *  The azimuth_t type is used as parameter for the rot_set_position() and
 *  rot_get_position() functions.
 *
 *  Unless specified otherwise, the unit of azimuth_t is decimal degrees.
 */
typedef float azimuth_t;


/**
 * \brief Token in the netrotctl protocol for returning error code
 */
#define NETROTCTL_RET "RPRT "


/**
 *  \def ROT_RESET_ALL
 *  \brief A macro that returns the flag for the \b reset operation.
 *  \sa rot_reset(), rot_reset_t()
 */
#define ROT_RESET_ALL   1


/**
 *  \typedef typedef int rot_reset_t
 *  \brief Type definition for rotator reset.
 *
 *  The rot_reset_t type is used as parameter for the rot_reset() API
 *  function.
 */
typedef int rot_reset_t;


/**
 * \brief Rotator type flags
 */
typedef enum {
    ROT_FLAG_AZIMUTH =      (1 << 1),   /*!< Azimuth */
    ROT_FLAG_ELEVATION =    (1 << 2)    /*!< Elevation */
} rot_type_t;

#define ROT_TYPE_MASK (ROT_FLAG_AZIMUTH|ROT_FLAG_ELEVATION)

#define ROT_TYPE_OTHER      0
#define ROT_TYPE_AZIMUTH    ROT_FLAG_AZIMUTH
#define ROT_TYPE_ELEVATION  ROT_FLAG_ELEVATION
#define ROT_TYPE_AZEL       (ROT_FLAG_AZIMUTH|ROT_FLAG_ELEVATION)


/**
 *  \def ROT_MOVE_UP
 *  \brief A macro that returns the flag for the \b UP direction.
 *
 *  This macro defines the value of the \b UP direction which can be
 *  used with the rot_move() function.
 *
 *  \sa rot_move(), ROT_MOVE_DOWN, ROT_MOVE_LEFT, ROT_MOVE_CCW,
 *      ROT_MOVE_RIGHT, ROT_MOVE_CW
 */
#define ROT_MOVE_UP         (1<<1)

/**
 *  \def ROT_MOVE_DOWN
 *  \brief A macro that returns the flag for the \b DOWN direction.
 *
 *  This macro defines the value of the \b DOWN direction which can be
 *  used with the rot_move() function.
 *
 *  \sa rot_move(), ROT_MOVE_UP, ROT_MOVE_LEFT, ROT_MOVE_CCW, ROT_MOVE_RIGHT,
 *      ROT_MOVE_CW
*/
#define ROT_MOVE_DOWN       (1<<2)

/**
 *  \def ROT_MOVE_LEFT
 *  \brief A macro that returns the flag for the \b LEFT direction.
 *
 *  This macro defines the value of the \b LEFT direction which can be
 *  used with the rot_move function.
 *
 *  \sa rot_move(), ROT_MOVE_UP, ROT_MOVE_DOWN, ROT_MOVE_CCW, ROT_MOVE_RIGHT,
 *      ROT_MOVE_CW
 */
#define ROT_MOVE_LEFT       (1<<3)

/**
 *  \def ROT_MOVE_CCW
 *  \brief A macro that returns the flag for the \b counterclockwise direction.
 *
 *  This macro defines the value of the \b counterclockwise direction which
 *  can be used with the rot_move() function. This value is equivalent to
 *  ROT_MOVE_LEFT .
 *
 *  \sa rot_move(), ROT_MOVE_UP, ROT_MOVE_DOWN, ROT_MOVE_LEFT, ROT_MOVE_RIGHT,
 *      ROT_MOVE_CW
 */
#define ROT_MOVE_CCW        ROT_MOVE_LEFT

/**
 *  \def ROT_MOVE_RIGHT
 *  \brief A macro that returns the flag for the \b RIGHT direction.
 *
 *  This macro defines the value of the \b RIGHT direction which can be used
 *  with the rot_move() function.
 *
 *  \sa rot_move(), ROT_MOVE_UP, ROT_MOVE_DOWN, ROT_MOVE_LEFT, ROT_MOVE_CCW,
 *      ROT_MOVE_CW
 */
#define ROT_MOVE_RIGHT      (1<<4)

/**
 *  \def ROT_MOVE_CW
 *  \brief A macro that returns the flag for the \b clockwise direction.
 *
 *  This macro defines the value of the \b clockwise direction wich can be
 *  used with the rot_move() function. This value is equivalent to
 *  ROT_MOVE_RIGHT .
 *
 *  \sa rot_move(), ROT_MOVE_UP, ROT_MOVE_DOWN, ROT_MOVE_LEFT, ROT_MOVE_CCW,
 *      ROT_MOVE_RIGHT
 */
#define ROT_MOVE_CW         ROT_MOVE_RIGHT


/* Basic rot type, can store some useful info about different rotators. Each
 * lib must be able to populate this structure, so we can make useful
 * enquiries about capablilities.
 */

/**
 * Rotator Caps
 * \struct rot_caps
 * \brief Rotator data structure.
 *
 * The main idea of this struct is that it will be defined by the backend
 * rotator driver, and will remain readonly for the application.  Fields that
 * need to be modifiable by the application are copied into the struct
 * rot_state, which is a kind of private of the ROT instance.
 *
 * This way, you can have several rigs running within the same application,
 * sharing the struct rot_caps of the backend, while keeping their own
 * customized data.
 *
 * n.b.: Don't move fields around, as the backends depend on it when
 *       initializing their caps.
 */
struct rot_caps {
    rot_model_t rot_model;                      /*!< Rotator model. */
    const char *model_name;                     /*!< Model name. */
    const char *mfg_name;                       /*!< Manufacturer. */
    const char *version;                        /*!< Driver version. */
    const char *copyright;                      /*!< Copyright info. */
    enum rig_status_e status;                   /*!< Driver status. */

    int rot_type;                               /*!< Rotator type. */
    enum rig_port_e port_type;                  /*!< Type of communication port. */

    int serial_rate_min;                        /*!< Minimal serial speed. */
    int serial_rate_max;                        /*!< Maximal serial speed. */
    int serial_data_bits;                       /*!< Number of data bits. */
    int serial_stop_bits;                       /*!< Number of stop bits. */
    enum serial_parity_e serial_parity;         /*!< Parity. */
    enum serial_handshake_e serial_handshake;   /*!< Handshake. */

    int write_delay;                            /*!< Write delay. */
    int post_write_delay;                       /*!< Post-write delay. */
    int timeout;                                /*!< Timeout. */
    int retry;                                  /*!< Number of retry if command fails. */

    /*
     * Movement range, az is relative to North
     * negative values allowed for overlap
     */
    azimuth_t min_az;                           /*!< Lower limit for azimuth (relative to North). */
    azimuth_t max_az;                           /*!< Upper limit for azimuth (relative to North). */
    elevation_t
    min_el;                                     /*!< Lower limit for elevation. */
    elevation_t
    max_el;                                     /*!< Upper limit for elevation. */


    const struct confparams *cfgparams;         /*!< Configuration parametres. */
    const rig_ptr_t priv;                       /*!< Private data. */

    /*
     * Rot Admin API
     *
     */

    int (*rot_init)(ROT *rot);
    int (*rot_cleanup)(ROT *rot);
    int (*rot_open)(ROT *rot);
    int (*rot_close)(ROT *rot);

    int (*set_conf)(ROT *rot, token_t token, const char *val);
    int (*get_conf)(ROT *rot, token_t token, char *val);

    /*
     *  General API commands, from most primitive to least.. :()
     *  List Set/Get functions pairs
     */

    int (*set_position)(ROT *rot, azimuth_t azimuth, elevation_t elevation);
    int (*get_position)(ROT *rot, azimuth_t *azimuth, elevation_t *elevation);

    int (*stop)(ROT *rot);
    int (*park)(ROT *rot);
    int (*reset)(ROT *rot, rot_reset_t reset);
    int (*move)(ROT *rot, int direction, int speed);

    /* get firmware info, etc. */
    const char * (*get_info)(ROT *rot);

    /* more to come... */
};


/**
 * Rotator state
 * \struct rot_state
 * \brief Live data and customized fields.
 *
 * This struct contains live data, as well as a copy of capability fields
 * that may be updated (ie. customized)
 *
 * It is fine to move fields around, as this kind of struct should
 * not be initialized like caps are.
 */
struct rot_state {
    /*
     * overridable fields
     */
    azimuth_t min_az;       /*!< Lower limit for azimuth (overridable). */
    azimuth_t max_az;       /*!< Upper limit for azimuth (overridable). */
    elevation_t min_el;     /*!< Lower limit for elevation (overridable). */
    elevation_t max_el;     /*!< Upper limit for elevation (overridable). */

    /*
     * non overridable fields, internal use
     */
    hamlib_port_t rotport;  /*!< Rotator port (internal use). */

    int comm_state;         /*!< Comm port state, opened/closed. */
    rig_ptr_t priv;         /*!< Pointer to private rotator state data. */
    rig_ptr_t obj;          /*!< Internal use by hamlib++ for event handling. */

    /* etc... */
};


/**
 * Rotator structure
 * \struct rot
 * \brief This is the master data structure,
 * acting as a handle for the controlled rotator.
 *
 * This is the master data structure, acting as a handle for the controlled
 * rotator. A pointer to this structure is returned by the rot_init() API
 * function and is passed as a parameter to every rotator specific API call.
 *
 * \sa rot_init(), rot_caps(), rot_state()
 */
struct rot {
    struct rot_caps *caps;      /*!< Rotator caps. */
    struct rot_state state;     /*!< Rotator state. */
};


/* --------------- API function prototypes -----------------*/

extern HAMLIB_EXPORT(ROT *)
rot_init HAMLIB_PARAMS((rot_model_t rot_model));

extern HAMLIB_EXPORT(int)
rot_open HAMLIB_PARAMS((ROT *rot));

extern HAMLIB_EXPORT(int)
rot_close HAMLIB_PARAMS((ROT *rot));

extern HAMLIB_EXPORT(int)
rot_cleanup HAMLIB_PARAMS((ROT *rot));

extern HAMLIB_EXPORT(int)
rot_set_conf HAMLIB_PARAMS((ROT *rot,
                            token_t token,
                            const char *val));
extern HAMLIB_EXPORT(int)
rot_get_conf HAMLIB_PARAMS((ROT *rot,
                            token_t token,
                            char *val));

/*
 *  General API commands, from most primitive to least.. )
 *  List Set/Get functions pairs
 */
extern HAMLIB_EXPORT(int)
rot_set_position HAMLIB_PARAMS((ROT *rot,
                                azimuth_t azimuth,
                                elevation_t elevation));
extern HAMLIB_EXPORT(int)
rot_get_position HAMLIB_PARAMS((ROT *rot,
                                azimuth_t *azimuth,
                                elevation_t *elevation));

extern HAMLIB_EXPORT(int)
rot_stop HAMLIB_PARAMS((ROT *rot));

extern HAMLIB_EXPORT(int)
rot_park HAMLIB_PARAMS((ROT *rot));

extern HAMLIB_EXPORT(int)
rot_reset HAMLIB_PARAMS((ROT *rot,
                         rot_reset_t reset));

extern HAMLIB_EXPORT(int)
rot_move HAMLIB_PARAMS((ROT *rot,
                        int direction,
                        int speed));

extern HAMLIB_EXPORT(const char *)
rot_get_info HAMLIB_PARAMS((ROT *rot));

extern HAMLIB_EXPORT(int)
rot_register HAMLIB_PARAMS((const struct rot_caps *caps));

extern HAMLIB_EXPORT(int)
rot_unregister HAMLIB_PARAMS((rot_model_t rot_model));

extern HAMLIB_EXPORT(int)
rot_list_foreach HAMLIB_PARAMS((int (*cfunc)(const struct rot_caps *,
                                             rig_ptr_t),
                                rig_ptr_t data));

extern HAMLIB_EXPORT(int)
rot_load_backend HAMLIB_PARAMS((const char *be_name));

extern HAMLIB_EXPORT(int)
rot_check_backend HAMLIB_PARAMS((rot_model_t rot_model));

extern HAMLIB_EXPORT(int)
rot_load_all_backends HAMLIB_PARAMS((void));

extern HAMLIB_EXPORT(rot_model_t)
rot_probe_all HAMLIB_PARAMS((hamlib_port_t *p));

extern HAMLIB_EXPORT(int)
rot_token_foreach HAMLIB_PARAMS((ROT *rot,
                                 int (*cfunc)(const struct confparams *,
                                              rig_ptr_t),
                                 rig_ptr_t data));

extern HAMLIB_EXPORT(const struct confparams *)
rot_confparam_lookup HAMLIB_PARAMS((ROT *rot,
                                   const char *name));

extern HAMLIB_EXPORT(token_t)
rot_token_lookup HAMLIB_PARAMS((ROT *rot,
                                const char *name));

extern HAMLIB_EXPORT(const struct rot_caps *)
rot_get_caps HAMLIB_PARAMS((rot_model_t rot_model));

extern HAMLIB_EXPORT(int)
qrb HAMLIB_PARAMS((double lon1,
                   double lat1,
                   double lon2,
                   double lat2,
                   double *distance,
                   double *azimuth));

extern HAMLIB_EXPORT(double)
distance_long_path HAMLIB_PARAMS((double distance));

extern HAMLIB_EXPORT(double)
azimuth_long_path HAMLIB_PARAMS((double azimuth));

extern HAMLIB_EXPORT(int)
longlat2locator HAMLIB_PARAMS((double longitude,
                               double latitude,
                               char *locator_res,
                               int pair_count));

extern HAMLIB_EXPORT(int)
locator2longlat HAMLIB_PARAMS((double *longitude,
                               double *latitude,
                               const char *locator));

extern HAMLIB_EXPORT(double)
dms2dec HAMLIB_PARAMS((int degrees,
                       int minutes,
                       double seconds,
                       int sw));

extern HAMLIB_EXPORT(int)
dec2dms HAMLIB_PARAMS((double dec,
                       int *degrees,
                       int *minutes,
                       double *seconds,
                       int *sw));

extern HAMLIB_EXPORT(int)
dec2dmmm HAMLIB_PARAMS((double dec,
                        int *degrees,
                        double *minutes,
                        int *sw));

extern HAMLIB_EXPORT(double)
dmmm2dec HAMLIB_PARAMS((int degrees,
                        double minutes,
                        int sw));


/**
 *  \def rot_debug
 *  \brief Convenience definition for debug level.
 *
 *  This is just as convenience definition of the rotator debug level,
 *  and is the same as for the rig debug level.
 *
 *  \sa rig_debug()
 */
#define rot_debug rig_debug

__END_DECLS

#endif /* _ROTATOR_H */

/** @} */
