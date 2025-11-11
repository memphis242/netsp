/**
 * @file tcp-server.c
 * @brief A demo tcp server application that listens to up to 4 connections and
 *        whos API consists of 2 requests (client → server) and 2 cmds (user →
 *        server → client). The server and clients shall use IPv4. The reqs/cmds
 *        are text-based.
 */

/*************************** File Header Inclusions ***************************/

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>

// POSIX headers
#include <errno.h> // C standard header, used by lots of POSIX API and UNIX system calls
#include <sys/types.h> // Defines many types used by POSIX APIs, e.g., pid_t
#include <sys/socket.h> // POSIX Sockets API
#include <netinet/in.h> // IP address structs
#include <netdb.h> // Hostname address lookups that come from DNS records
#include <arpa/inet.h> // Functions to go from host ↔ network
#include <sys/wait.h> // POSIX: process control through waitpid
#include <signal.h> // POSIX + C subset : signal handling

/***************************** Local Declarations *****************************/
// constexpr's - The better macros!
constexpr char PORT_NUMSTR[] = "5555";
constexpr int POSIX_API_NO_ERROR = 0;

// Local Types
enum MainReturnValues
{
   PROGRAM_RAN_FINE,
   ADDRESS_INFO_ERR,
};

enum LocalFcnError
{
   NoError,
   BadAddressObject
};

// Local Functions
static void   sig_child_handler(int s); // FIXME: What is the actual argument type here? - signal handler
static void * get_ip_addr( const struct sockaddr * sa ); // FIXME: I don't like this API

// Extern declarations

/******************************* Main Function ********************************/

int main( int argc, char * argv[] )
{
   // Local vars to be re-used
   int posix_api_err;

   // Create a socket on which to listen in on for connections to this server's
   // port.
   int sockfd_connections; // FIXME: Can we use a more expressive type than int? :(
   struct addrinfo   server_addr_cfg_hints = {0}; // Holds apriori address cfg for getaddrinfo
   struct addrinfo * server_addr_info = nullptr;

   server_addr_cfg_hints.ai_family   =  AF_INET; // IPv4
   server_addr_cfg_hints.ai_socktype =  SOCK_STREAM; // For TCP. For UDP, this would be SOCK_DGRAM.
   server_addr_cfg_hints.ai_flags    |= AI_PASSIVE /* Use local host IP */;

   posix_api_err = getaddrinfo(
                        nullptr, // node: IP address as a string
                        PORT_NUMSTR, // service: port number as a string, or even a service name
                        &server_addr_cfg_hints,
                        &server_addr_info /* Fnc's returned parameter which has ai_addr member for socket API */
                     );
   if ( posix_api_err != POSIX_API_NO_ERROR )
   {
      fprintf( stderr, "getaddressinfo: %s\n", gai_strerror( posix_api_err) );
      return (int)ADDRESS_INFO_ERR;
   }

   // Loop through linked-list until you find a valid addrinfo object

   // Free the address info object
   freeaddrinfo(server_addr_info);

   // Open a socket to listen to connections on

   // Bind a local port to that socket

   // Declare that we'll listen to X amount of connections in the connection queue at most

   // Accept connections and fork off child processes to handle them

   // Close the connection socket

   return (int)PROGRAM_RAN_FINE;
}

/*********************** Local Function Implementations ***********************/

/**
 * @brief FIXME: Handle what exact OS signal interrupt?
 */
static void sig_child_handler(int s)
{
   (void)s; // Silence unused var warning

   // waitpid() might override errno, so save it first prior to the call and restore afterwards.
   int saved_errno = errno;   // FIXME: Does errno have to be an int...
   while ( waitpid(-1, /* FIXME: What is this? */
                   NULL, /* FIXME: What is this? */
                   WNOHANG /* FIXME: What is this? */ ) > 0 );
   errno = saved_errno;
}

static void * get_ip_addr( const struct sockaddr * sa )
{
   assert(false);
   return nullptr;
}
