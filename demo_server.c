/**
 * @file demo_server.c
 * @brief TODO Brief description on demo_server
 */

/*************************** File Header Inclusions ***************************/
// General-Purpose Headers
#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>
// Networking-Related Headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
// Tangential Headers
#include <signal.h>

/***************************** Local Declarations *****************************/
// constexpr's - The better macros!

// Some errors shouldn't abort the program, but we will still return a code
// indicating something went wrong. To account for a possible accumulation
// of errors, need to reserve specific bits for each.
enum MainRetCode
{
   MAINRC_FINE                    = 0x0000,
   MAINRC_SOCKET_CREATION_ERR     = 0x0001,
   MAINRC_SOCKET_BIND_ERR         = 0x0002,
   MAINRC_SOCKET_LISTEN_ERR       = 0x0004,
   MAINRC_SOCKET_ACCEPT_ERR       = 0x0008,
   MAINRC_SIGINT_REGISTRATION_ERR = 0x0010,
};

// Local Functions
static void handleSIGINT(int sig_num);

/******************************* Main Function ********************************/
int main( int argc, char * argv[] )
{
   int main_retcode = MAINRC_FINE;
   int retcode; // for system calls

   // Register signal handlers
   struct sigaction sa_cfg;
   memset(&sa_cfg, 0x00, sizeof sa_cfg);
   sigemptyset(&sa_cfg.sa_mask); // No need to mask any signals during handle
   sa_cfg.sa_handler = handleSIGINT;
   retcode = sigaction( SIGINT,
                        &sa_cfg,
                        nullptr /* old sig action */ );
   if ( retcode != 0 )
   {
      fprintf( stderr,
               "sigaction() failed to register interrupt signal.\n"
               "Returned: %d, errno: %s (%d)\n",
               retcode, strerror(errno), errno );
      main_retcode |= MAINRC_SIGINT_REGISTRATION_ERR;
   }

   printf( "Hello! This is the REPL for a demo server.\n"
           "Here is a brief list of the available commands (case-insensitive):\n"
           "\t- udp-create [ip_address : port]\n"
           "\t- udp-listen [timeout in seconds]\n"
           "\t- udp-print-msgs\n"
           "\t- udp-socks\n"
           "\t- udp-close sock_id\n"
           "\t- udp-close-all\n"
           "\t- tcp-create-listener [ip_address : port]\n"
           "\t- tcp-begin-accepting\n"
           "\t- tcp-stop-accepting\n"
           "\t- tcp-close sock_id\n"
           "\t- tcp-print-msgs\n"
           "\t- tcp-close-all\n"
           "\t- close-all\n" );

   // Create a listening socket for accept()'ing on connections on any interface (ix)
   // TODO: May want to add functionality to support listening on a singular ix...
   int sfd_listening = socket( AF_INET,
                               SOCK_STREAM,
                               0 /* protocol within AF */ );
   if ( sfd_listening < 0 )
   {
      
   }

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
