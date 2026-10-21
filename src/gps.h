#ifndef PDA_GPS_H
#define PDA_GPS_H

#include <stdbool.h>
#include <time.h>

/* Position source for the "me" marker.
 *
 * On the Pi this reads gpsd's NMEA socket (127.0.0.1:2947) - the standard
 * Linux stack, already planned for the device. On a desktop there is no GPS,
 * so the position can be forced with PDA_GPS_LAT / PDA_GPS_LON, and
 * PDA_GPS_DEMO=1 makes the marker drift slowly so the live update can be
 * seen without hardware. */

typedef struct {
    double lat, lon;
    float speed;          /* m/s */
    float course;         /* degrees */
    int sats;
    bool valid;           /* a usable fix is available */
    bool simulated;       /* env override or demo drift, not a real fix */
    time_t last_fix;
} gps_fix_t;

void gps_init(gps_fix_t * fix);
void gps_poll(gps_fix_t * fix);
const char * gps_status(const gps_fix_t * fix);
bool gps_daemon_seen(void);   /* true once gpsd has sent us something */

#endif
