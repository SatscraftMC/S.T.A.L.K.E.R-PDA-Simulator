#include "gps.h"

#include <arpa/inet.h>
#include <fcntl.h>
#include <math.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define GPSD_HOST "127.0.0.1"
#define GPSD_PORT 2947

static int s_sock = -1;
static bool s_daemon_seen = false;   /* gpsd answered at least once */
static bool s_demo = false;
static bool s_env_pos = false;
static double s_demo_lat = 32.6478;    /* Funchal */
static double s_demo_lon = -16.9193;
static double s_demo_phase = 0.0;

static double nmea_degrees(const char * value, const char * hemi)
{
    if(!value || !*value) return 0.0;
    double raw = atof(value);
    double deg = floor(raw / 100.0);
    double min = raw - deg * 100.0;
    double out = deg + min / 60.0;
    if(hemi && (*hemi == 'S' || *hemi == 'W')) out = -out;
    return out;
}

static int nmea_int(const char * value, int fallback)
{
    if(!value || !*value) return fallback;
    return atoi(value);
}

/* pull the fields of one NMEA sentence out of a comma separated buffer */
static void nmea_field(const char * line, int index, const char ** out, size_t * len)
{
    const char * p = line;
    int cur = 0;
    while(cur < index && p) {
        p = strchr(p, ',');
        if(p) p++;
        cur++;
    }
    if(!p) { *out = ""; *len = 0; return; }
    const char * end = strchr(p, ',');
    if(!end) end = p + strlen(p);
    *out = p;
    *len = (size_t)(end - p);
}

/* copy one field into a small buffer (NMEA fields are short) */
static void nmea_copy(const char * line, int index, char * dst, size_t dstn)
{
    const char * f;
    size_t len;
    nmea_field(line, index, &f, &len);
    if(len >= dstn) len = dstn - 1;
    memcpy(dst, f, len);
    dst[len] = 0;
}

static bool parse_rmc(const char * line, gps_fix_t * fix)
{
    char status[4] = "";
    char lat[16], ns[4], lon[16], ew[4], spd[16], crs[16];
    nmea_copy(line, 2, status, sizeof(status));
    nmea_copy(line, 3, lat, sizeof(lat));
    nmea_copy(line, 4, ns, sizeof(ns));
    nmea_copy(line, 5, lon, sizeof(lon));
    nmea_copy(line, 6, ew, sizeof(ew));
    nmea_copy(line, 7, spd, sizeof(spd));
    nmea_copy(line, 8, crs, sizeof(crs));
    if(status[0] != 'A') return false;          /* no fix */
    if(!lat[0] || !lon[0]) return false;
    fix->lat = nmea_degrees(lat, ns);
    fix->lon = nmea_degrees(lon, ew);
    fix->speed = (float)(atof(spd) * 0.514444); /* knots -> m/s */
    fix->course = (float)atof(crs);
    return true;
}

static void parse_gga(const char * line, gps_fix_t * fix)
{
    char sats[8];
    nmea_copy(line, 7, sats, sizeof(sats));
    fix->sats = nmea_int(sats, fix->sats);
}

static void gpsd_connect(void)
{
    if(s_sock >= 0) return;
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd < 0) return;
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(GPSD_PORT);
    addr.sin_addr.s_addr = inet_addr(GPSD_HOST);
    if(connect(fd, (struct sockaddr *)&addr, sizeof(addr)) != 0) {
        if(getenv("PDA_DEBUG")) fprintf(stderr, "[gps] connect failed\n");
        close(fd);
        return;
    }
    if(getenv("PDA_DEBUG")) fprintf(stderr, "[gps] connected to gpsd\n");
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
    s_sock = fd;
}

bool gps_daemon_seen(void)
{
    return s_daemon_seen;
}

void gps_init(gps_fix_t * fix)
{
    memset(fix, 0, sizeof(*fix));
    fix->lat = s_demo_lat;
    fix->lon = s_demo_lon;

    const char * lat = getenv("PDA_GPS_LAT");
    const char * lon = getenv("PDA_GPS_LON");
    if(lat && lon) {
        fix->lat = atof(lat);
        fix->lon = atof(lon);
        fix->valid = true;
        fix->simulated = true;
        s_env_pos = true;
    }
    if(getenv("PDA_GPS_DEMO")) s_demo = true;
    gpsd_connect();
}

void gps_poll(gps_fix_t * fix)
{
    if(s_demo) {
        /* slow drift so the live update is visible without hardware */
        /* a slow, clearly visible circle (~350m) so the live update can be
           seen without hardware - this is a test aid, not a real walk */
        s_demo_phase += 0.15;
        fix->lat = s_demo_lat + 0.010 * sin(s_demo_phase);
        fix->lon = s_demo_lon + 0.013 * cos(s_demo_phase * 0.7);
        fix->speed = 0.4f;
        fix->course = (float)(s_demo_phase * 57.3f);
        fix->valid = true;
        fix->simulated = true;
        fix->last_fix = time(NULL);
        return;
    }
    if(s_env_pos) {
        fix->last_fix = time(NULL);
        return;
    }

    if(s_sock < 0) gpsd_connect();
    if(s_sock < 0) return;

    char buf[1024];
    ssize_t n = read(s_sock, buf, sizeof(buf) - 1);
    if(getenv("PDA_DEBUG")) fprintf(stderr, "[gps] read -> %zd (sock %d)\n", n, s_sock);
    if(n == 0) {
        /* gpsd closed the connection (it restarts on the Pi) - drop it and
           reconnect on the next poll */
        close(s_sock);
        s_sock = -1;
        return;
    }
    if(n < 0) return;                 /* EAGAIN: nothing new yet */
    buf[n] = 0;
    s_daemon_seen = true;

    char * save = NULL;
    for(char * line = strtok_r(buf, "\r\n", &save); line; line = strtok_r(NULL, "\r\n", &save)) {
        if(strncmp(line, "$GN", 3) && strncmp(line, "$GP", 3)) continue;
        if(strstr(line, "RMC")) {
            if(parse_rmc(line, fix)) {
                fix->valid = true;
                fix->simulated = false;
                fix->last_fix = time(NULL);
            }
        }
        else if(strstr(line, "GGA")) {
            parse_gga(line, fix);
        }
    }
}

const char * gps_status(const gps_fix_t * fix)
{
    static char buf[64];
    if(!fix->valid && !fix->simulated && !gps_daemon_seen())
        snprintf(buf, sizeof(buf), "GPS: gpsd not found");
    else if(!fix->valid) snprintf(buf, sizeof(buf), "GPS: no fix");
    else if(fix->simulated) snprintf(buf, sizeof(buf), "GPS: simulated");
    else snprintf(buf, sizeof(buf), "GPS: %d sats, %.1f m/s", fix->sats, fix->speed);
    return buf;
}
