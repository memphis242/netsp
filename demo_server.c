/**
 * @file demo_server.c
 * @brief TODO Brief description on demo_server
 */

/*************************** File Header Inclusions ***************************/
#define _POSIX_C_SOURCE 200809L // Specify atleast POSIX.1-2008 compatibility
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
#include <pthread.h>
#include <sys/select.h>
#include <unistd.h>

/***************************** Local Declarations *****************************/
static volatile sig_atomic_t bUserEndedSession = false;

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
   MAINRC_NREP_LIM_HIT            = 0x0020,
};

struct Client
{
   int sfd; // socket descriptor of server socket communicating /w this client
   in_addr_t src_ipaddr;
   uint16_t src_port;
   struct Client * next; // linked-list of clients makes arbitrary removal
                         // somewhat easier
};

struct ClientList
{
   struct Client * head;
   size_t len;
};

struct StreamContext
{
   pthread_t acceptor;
   pthread_t responder;
   int sfd; // socket descriptor
   pthread_mutex_t mtx;
   struct ClientList clients;
};

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
               "Warning: sigaction() failed to register interrupt signal.\n"
               "Returned: %d, errno: %s (%d)\n"
               "You won't be able to stop the program gracefully /w Ctrl+C, \n"
               " though Ctrl+C will still terminate the program.",
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

   printf("> ");
   constexpr size_t NMAX = 1'000;
   size_t nreps = 0;
   while ( !bUserEndedSession && nreps++ < NMAX )
   {
      char buf[100] = {0};

      // TODO: The -create cmds shall spawn listener + responder threads and an
      //       associated context (e.g., mutex for updating/reading client lists)

      if ( strncmp( buf, "tcp-create", strlen("udp-create") ) == 0 )
      {
         int sfd_listening = socket( AF_INET,
                                     SOCK_STREAM,
                                     0 /* protocol within AF */ );
         if ( sfd_listening < 0 )
         {
            // TODO: Error creating listening TCP socket...
         }

         struct StreamContext * ctx = calloc( 1, sizeof(struct StreamContext) );
         // Create thread objects and point the thread fcns to their respective
         // local fcns, each of which take this stream context ptr as an arg.
         // TODO:
      }
   }

   if ( nreps >= NMAX )
   {
      printf( "REPL repetitions hit limit: %zu / %zu\n"
              "Session ended. Please restart program.\n",
              nreps, NMAX );

      main_retcode |= MAINRC_NREP_LIM_HIT;
   }
   else
   {
      printf("User has ended the session. Goodbye!\n");
   }

   return main_retcode;
}

/*********************** Local Function Implementations ***********************/

/**
 * @brief Handle the interrupt signal that a user would trigger /w Ctrl+C
 */
static void handleSIGINT(int sig_num)
{
   (void)sig_num; // Signal number is not necessary here
   bUserEndedSession = true;
}
