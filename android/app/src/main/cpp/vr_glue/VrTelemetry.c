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
#include <fcntl.h>
#include <errno.h>

#include "VrCommon.h"
#include "VrTelemetry.h"

#define VR_TELEMETRY_PORT 5231

static int sock = -1;
static struct sockaddr_in targets[4];
static int numtargets = 0;
static time_t lastscan = 0;
/* Unicast: cuando el servidor contesta "sauvrack" a una línea de telemetría,
 * guardamos su dirección y todo (telemetría y capturas) va directo a ella.
 * El broadcast Wi-Fi va a la velocidad base de la red y pierde casi todo
 * cuando mandamos una captura de 1 MB. */
static struct sockaddr_in serverAddr;
static int haveServer = 0;
static time_t lastAck = 0;

static void pollAck(void)
{
    char buf[64];
    struct sockaddr_in from; socklen_t fl = sizeof(from);
    for (;;) {
        ssize_t n = recvfrom(sock, buf, sizeof(buf) - 1, MSG_DONTWAIT, (struct sockaddr *)&from, &fl);
        if (n <= 0) break;
        buf[n] = 0;
        if (!strncmp(buf, "sauvrack", 8)) {
            if (!haveServer) ALOGV("telemetría: servidor en %s", inet_ntoa(from.sin_addr));
            serverAddr = from; haveServer = 1; lastAck = time(NULL);
        }
    }
    /* sin acuses en 15 s: volver al broadcast (el PC cambió de IP o se cerró) */
    if (haveServer && time(NULL) - lastAck > 15) haveServer = 0;
}

static void ensureSocket(void)
{
    if (sock >= 0) return;
    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock < 0) return;
    int on = 1;
    setsockopt(sock, SOL_SOCKET, SO_BROADCAST, &on, sizeof(on));
    fcntl(sock, F_SETFL, fcntl(sock, F_GETFL, 0) | O_NONBLOCK);
}

int VrTelemetry_HaveServer(void) { return haveServer; }

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

void VrTelemetry_SendRaw(const void *data, size_t len)
{
    ensureSocket();
    if (sock < 0) return;
    if (haveServer) { sendto(sock, data, len, 0, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); return; }
    time_t now = time(NULL);
    if (numtargets == 0 || now - lastscan >= 10) { scanInterfaces(); lastscan = now; }
    for (int i = 0; i < numtargets; i++)
        sendto(sock, data, len, 0, (struct sockaddr *)&targets[i], sizeof(targets[i]));
}

void VrTelemetry_Send(const char *line)
{
    ensureSocket();
    if (sock < 0) return;
    pollAck();
    size_t len = strlen(line);
    if (haveServer) { sendto(sock, line, len, MSG_DONTWAIT, (struct sockaddr *)&serverAddr, sizeof(serverAddr)); return; }
    time_t now = time(NULL);
    if (numtargets == 0 || now - lastscan >= 10) { scanInterfaces(); lastscan = now; }
    for (int i = 0; i < numtargets; i++)
        sendto(sock, line, len, MSG_DONTWAIT, (struct sockaddr *)&targets[i], sizeof(targets[i]));
}
