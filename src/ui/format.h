#ifndef PDA_FORMAT_H
#define PDA_FORMAT_H

#include <stdbool.h>
#include <stddef.h>
#include <time.h>

/* How the numbers and the times are written: the General page picks the
   clock, the Language page picks the time zone and the Region. */

void fmt_apply(void);                  /* read the settings and apply them */
bool fmt_12h(void);                     /* the clock: 12 or 24 hours */
bool fmt_metric(void);                  /* km/h or mph */
bool fmt_comma(void);                   /* 1.234,5 or 1.234.5 */

void fmt_clock(time_t t, char * buf, size_t n);   /* the time of day */
void fmt_speed(float mps, char * buf, size_t n);  /* the speed */
void fmt_coord(double v, char * buf, size_t n);    /* a coordinate */

#endif
