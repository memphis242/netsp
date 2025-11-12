#include <stdio.h>
#include <arpa/inet.h>
#include <sys/socket.h>

constexpr int INET_PTON_SUCCESS = 1;

int main(void)
{
   int retCode;
   in_addr_t net_ipv4 = INADDR_NONE;
   retCode = inet_pton(AF_INET, "192.168.0.1", &net_ipv4);
   if ( retCode != INET_PTON_SUCCESS || net_ipv4 == INADDR_NONE )
   {
      perror("ERROR: inet_pton() unable to convert string numbers-and-dots "
             "notation into numerical representation.");
      return -1;
   }
   printf("Network Numerical Representation of 192.168.0.1: 0x%08X\n", ntohl(net_ipv4));

   return 0;
}
