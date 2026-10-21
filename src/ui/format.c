#include "ui/format.h"
#include "ui/prefs.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* The time zone, the clock and the numbers. Everything the General and the
   Language pages set ends up here. */

static bool s_12h = false;
static bool s_metric = true;
static bool s_comma = false;

bool fmt_12h(void)      { return s_12h; }
bool fmt_metric(void)   { return s_metric; }
bool fmt_comma(void)    { return s_comma; }

void fmt_apply(void)
{
    s_12h = !strcmp(prefs_str("time_fmt", "24"), "12");
    s_metric = !strcmp(prefs_str("units", "metric"), "metric");
    s_comma = !strcmp(prefs_str("region", "dot"), "comma");

    /* the time zone: the clock and the log follow it */
    const char * tz = prefs_str("tz", "local");
    if(!strcmp(tz, "utc")) setenv("TZ", "UTC", 1);
    else if(!strcmp(tz, "lisbon")) setenv("TZ", "Europe/Lisbon", 1);
    else setenv("TZ", "", 1);            /* empty: the system's own */
    tzset();
}

void fmt_clock(time_t t, char * buf, size_t n)
{
    struct tm lt;
    localtime_r(&t, &lt);
    if(s_12h) {
        char h[8];
        strftime(h, sizeof(h), "%I:%M", &lt);
        if(h[0] == '0') memmove(h, h + 1, strlen(h));   /* 06:17 is 6:17 */
        snprintf(buf, n, "%s %s", h, lt.tm_hour < 12 ? "AM" : "PM");
    }
    else {
        strftime(buf, n, "%H:%M", &lt);
    }
}

/* a number with the separators of the region */
static void fmt_num(double v, int decimals, char * buf, size_t n)
{
    char raw[64];
    snprintf(raw, sizeof(raw), "%.*f", decimals, v);

    const char * p = raw;
    bool neg = false;
    if(*p == '-') { neg = true; p++; }

    const char * dot = strchr(p, '.');
    int intlen = dot ? (int)(dot - p) : (int)strlen(p);

    char out[96];
    size_t o = 0;
    for(int i = 0; i < intlen; i++) {
        if(i > 0 && (intlen - i) % 3 == 0 && o + 1 < sizeof(out))
            out[o++] = s_comma ? '.' : ' ';
        if(o + 1 < sizeof(out)) out[o++] = p[i];
    }
    if(dot) {
        if(o + 1 < sizeof(out)) out[o++] = s_comma ? ',' : '.';
        size_t l = strlen(dot + 1);
        for(size_t i = 0; i < l && o + 1 < sizeof(out); i++) out[o++] = dot[1 + i];
    }
    out[o] = 0;
    snprintf(buf, n, "%s%s", neg ? "-" : "", out);
}

void fmt_coord(double v, char * buf, size_t n)
{
    fmt_num(v, 5, buf, n);
}

void fmt_speed(float mps, char * buf, size_t n)
{
    if(s_metric) {
        fmt_num(mps * 3.6f, 1, buf, n);       /* kilometres per hour */
        snprintf(buf + strlen(buf), n - strlen(buf), " km/h");
    }
    else {
        fmt_num(mps * 2.23694f, 1, buf, n);   /* miles per hour */
        snprintf(buf + strlen(buf), n - strlen(buf), " mph");
    }
}
