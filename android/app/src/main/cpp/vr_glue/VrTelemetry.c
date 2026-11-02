/* Telemetría por UDP: el motor manda una línea por segundo (fps, ms, tiempos
 * de GPU/CPU, tris, draw calls...) a la dirección de broadcast de cada
 * interfaz Wi-Fi, puerto 5231. tools/apk/serve.mjs la escucha, la imprime y la
 * guarda en dist/telemetry.log. Sin configuración: no hace falta saber la IP
 * del PC. Si no hay red, sendto falla en silencio y no cuesta nada. */
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <ifaddrs.h>
#include <net/if.h>
#include <unistd.h>
#include <time.h>

#include "VrCommon.h"
#include "VrTelemetry.h"

#define VR_TELEMETRY_PORT 5231

static int sock = -1;
static struct sockaddr_in targets[4];
static int numtargets = 0;
static time_t lastscan = 0;

static void scanInterfaces(void)
{
    struct ifaddrs *list = NULL;
    numtargets = 0;
    if (getifaddrs(&list) != 0) return;
    for (struct ifaddrs *a = list; a && numtargets < 4; a = a->ifa_next) {
        if (!a->ifa_addr || a->ifa_addr->sa_family != AF_INET || !a->ifa_netmask) continue;
        if (a->ifa_flags & IFF_LOOPBACK) continue;
        if (!(a->ifa_flags & IFF_UP)) continue;
        uint32_t ip = ((struct sockaddr_in *)a->ifa_addr)->sin_addr.s_addr;
        uint32_t mask = ((struct sockaddr_in *)a->ifa_netmask)->sin_addr.s_addr;
        struct sockaddr_in *t = &targets[numtargets++];
        memset(t, 0, sizeof(*t));
        t->sin_family = AF_INET;
        t->sin_port = htons(VR_TELEMETRY_PORT);
        t->sin_addr.s_addr = ip | ~mask; /* broadcast dirigido de la subred */
    }
    freeifaddrs(list);
}

void VrTelemetry_Send(const char *line)
{
    if (sock < 0) {
        sock = socket(AF_INET, SOCK_DGRAM, 0);
        if (sock < 0) return;
        int on = 1;
        setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    }
    time_t now = time(NULL);
    if (numtargets == 0 || now - lastscan >= 10) { scanInterfaces(); lastscan = now; }
    size_t len = strlen(line);
    for (int i = 0; i < numtargets; i++)
        sendto(sock, line, len, MSG_DONTWAIT, (struct sockaddr *)&targets[i], sizeof(targets[i]));
}
