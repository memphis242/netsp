/**
 * @file demo_server.c
 * @brief TODO Brief description on demo_server
 */

/*************************** File Header Inclusions ***************************/
#define _POSIX_C_SOURCE 200809L // Specify atleast POSIX.1-2008 compatibility

// General-Purpose Headers
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <ctype.h>
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
   MAINRC_FAILED_CLOSE            = 0x0040,
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

static void * acceptorThread(void * arg);
static void * responderThread(void * arg);

#ifndef NDEBUG
bool isFullyNumeric(char * str, size_t len);
bool isNullTerminated(char * str, size_t max_len);
#endif // NDEBUG

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

   constexpr size_t NMAX = 1'000;
   size_t nreps = 0;
   while ( !bUserEndedSession && ++nreps < NMAX )
   {
      char buf[100] = {0};

      printf("> ");
      fflush(stdout);
      
      if ( fgets(buf, sizeof buf, stdin) == nullptr )
      {
         // Either EOF read /wo other characters preceding it or I/O interruption
         // occured. Either way, time to exit gracefully.
         break;
      }

      // Clear newline character, if found. If not found, prompt user to repeat.
      char * newline_ptr = memchr(buf, '\n', sizeof buf);
      if ( nullptr == newline_ptr )
      {
         // Need to clear input buffer since fgets had to stop prematurely
         int c; // int for the EOF character
         while ( (c = fgetc(stdin)) != '\n' && c != EOF );

         fprintf( stderr,
                  "Error: Too many characters. Max is %zu. Please try again.\n",
                  sizeof buf );

         continue;
      }
      *newline_ptr = '\0';

      assert( isNullTerminated(buf, sizeof buf) );

      // TODO: The -create cmds shall spawn listener + responder threads and an
      //       associated context (e.g., mutex for updating/reading client lists)

      // If "exit" or "quit" are part of the beginning of the cmd string, count
      // that as an exit request.
      if ( strncmp( buf, "exit", (sizeof("exit") - 1) ) == 0
           || strncmp( buf, "quit", (sizeof("quit") - 1) ) == 0 )
      {
         break;
      }

      else if ( strncmp( buf, "tcp-create", (sizeof("tcp-create") - 1) ) == 0 )
      {
         // Attempt to parse out command arguments ip_addr:port
         char * cmd_arg_ptr = buf + sizeof("tcp-create") - 1;
         char * cmd_str_end = memchr(buf, '\0', sizeof buf);
         assert(cmd_str_end != nullptr);
         assert( (cmd_str_end - buf) < (ptrdiff_t)(sizeof buf) );

         char cmd_arg_addr[INET_ADDRSTRLEN + 1] = {0};
         char cmd_arg_port[5 + 1] = {0};

         bool invalid_input = false;

         if ( *cmd_arg_ptr != '\0' )
         {
            // Find arguments
            bool found_arg_delim = false;
            size_t arg_addr_idx = 0;
            size_t arg_port_idx = 0;

            for ( char c = *cmd_arg_ptr;
                  cmd_arg_ptr <= cmd_str_end && c != '\0';
                  c = *(cmd_arg_ptr++) )
            {
               if ( ' ' == c )
                  continue;

               if ( ':' == c )
               {
                  found_arg_delim = true;
                  continue;
               }

               if ( !isalnum(c) && c != '.' )
               {
                  invalid_input = true;
                  fprintf( stderr,
                           "Unrecognized character in argument input: %c\n"
                           "Aborting command. Please try again.\n",
                           c );
                  break;
               }

               if ( !found_arg_delim )
               {
                  if ( arg_addr_idx >= sizeof(cmd_arg_addr) )
                  {
                     invalid_input = true;
                     fprintf( stderr,
                              "IP address argument is too long.\n"
                              "Limit is %zu characters.\n"
                              "Aborting command. Please try again.\n",
                              sizeof cmd_arg_addr );
                     break;
                  }

                  cmd_arg_addr[arg_addr_idx++] = c;
               }
               else if ( isdigit(c) )
               {
                  if ( arg_port_idx >= sizeof(cmd_arg_port) )
                  {
                     invalid_input = true;
                     fprintf( stderr,
                              "Port argument is too long.\n"
                              "Limit is %zu characters.\n"
                              "Aborting command. Please try again.\n",
                              sizeof cmd_arg_port );
                     break;
                  }

                  cmd_arg_port[arg_port_idx++] = c;
               }
               else
               {
                  invalid_input = true;
                  fprintf( stderr,
                           "Non-numeric character for port argument: %c\n"
                           "Aborting command. Please try again.\n",
                           c );
                  break;
               }
            }
         }

         if ( invalid_input )
            continue;
         
         assert( isFullyNumeric(cmd_arg_port, strlen(cmd_arg_port)) );
         assert( isNullTerminated(cmd_arg_addr, sizeof cmd_arg_addr) );
         assert( isNullTerminated(cmd_arg_port, sizeof cmd_arg_port) );

         // Convert string arguments to what are needed for bind()'ing
         // FIXME: Remove this printf "found" section after you're done developing
         printf("\tFound arguments:\n"
                "\t\tIP Address: %s\n"
                "\t\tPort: %s\n",
                cmd_arg_addr,
                cmd_arg_port );
         // TODO: You stopped here

         // Attempt to create socket
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

         // TODO: Move this closing of the socket descriptor to its appropriate
         //       spot once we've got the destroy/cleanup code written up. This
         //       is  here for now so that -fanalyzer stops complaining about
         //       a leaking file.
         retcode = close(sfd_listening);
         if ( retcode != 0 )
         {
            fprintf( stderr,
                     "Error: Failed to close socket!\n"
                     "close() returned: %d, errno: %s (%d)\n",
                     retcode, strerror(errno), errno );

            main_retcode |= MAINRC_FAILED_CLOSE;
         }
      }
      else
      {
         fprintf( stderr,
                  "Error: Invalid command: %s.\n"
                  "Please try again.\n",
                  buf );
         continue;
      }
   }

   puts("");
   puts("");
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
   puts("");

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

static void * acceptorThread(void * arg)
{
   // TODO
   return nullptr;
}

static void * responderThread(void * arg)
{
   // TODO
   return nullptr;
}

#ifndef NDEBUG
bool isFullyNumeric(char * str, size_t len)
{
   for ( size_t i = 0; i < len; ++i )
      if ( !isdigit(str[i]) )
         return false;

   return true;
}

bool isNullTerminated(char * str, size_t max_len)
{
   return memchr(str, '\0', max_len) != nullptr;
}
#endif // NDEBUG
