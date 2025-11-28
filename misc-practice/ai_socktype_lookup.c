#include <sys/socket.h>

char * AiSocktypeStringLookup[] =
{
    [SOCK_STREAM] = "SOCK_STREAM (1)",
    [SOCK_DGRAM] = "SOCK_DGRAM (2)",
    [SOCK_RAW] = "SOCK_RAW (3)",
    [SOCK_RDM] = "SOCK_RDM (4)",
    [SOCK_SEQPACKET] = "SOCK_SEQPACKET (5)",
    [SOCK_DCCP] = "SOCK_DCCP (6)",
    [SOCK_PACKET] = "SOCK_PACKET (10)",
    [SOCK_CLOEXEC] = "SOCK_CLOEXEC (02000000)",
    [SOCK_NONBLOCK] = "SOCK_NONBLOCK (00004000)"
};
