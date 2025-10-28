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
#include <errno.h> // FIXME: Is this POSIX?
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h> // FIXME: What is this for?
#include <arpa/inet.h> // FIXME: What is this for?
#include <sys/wait.h>
#include <signal.h> // FIXME: Is this POSIX or Unix?

/***************************** Local Declarations *****************************/
// Macros
#define PORT_NUMSTR "5555"

// Macros that I prefer to be typed constants!
static const int POSIX_API_NO_ERROR = 0;

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
// FIXME: Declare errno as external import? Or is that covered in the hdr includes?

/******************************* Main Function ********************************/

int main( int argc, char * argv[] )
{
   // Local vars to be re-used
   int posix_api_err;

   // Create a socket on which to listen in on for connections to this server's
   // port. Accepted connections are fed into separate processes with separate
   // sockets, of course.
   int sockfd_connections; // FIXME: Can we use a more expressive type than int?
   struct addrinfo   server_addr_cfg_hints = {0}; // Holds up-front address cfg pre-getaddrinfo
   struct addrinfo * server_addr_info;

   server_addr_cfg_hints.ai_family   =  AF_INET; // IPv4
   server_addr_cfg_hints.ai_socktype =  SOCK_STREAM;
   server_addr_cfg_hints.ai_flags    |= AI_PASSIVE /* Use local host IP */;

   posix_api_err = getaddrinfo(
                        NULL, /* FIXME: What is this? */
                        PORT_NUMSTR,
                        &server_addr_cfg_hints,
                        &server_addr_info /* Fnc's returned parameter */
                  );
   if ( posix_api_err != POSIX_API_NO_ERROR )
   {
      fprintf( stderr, "getaddressinfo: %s\n", gai_strerror( posix_api_err) );
      return (int)ADDRESS_INFO_ERR;
   }

   // Loop through linked-list until you find a valid addrinfo object

   // Free the address info object

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
   return NULL;
}
