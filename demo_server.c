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
#include <limits.h>
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
constexpr size_t MAX_CLIENTS = 1'000;
constexpr int STREAM_LISTEN_QUEUE_SZ = 5;

static volatile sig_atomic_t bUserEndedSession = false;

// Some errors shouldn't abort the program, but we will still return a code
// indicating something went wrong. To account for a possible accumulation
// of errors, need to reserve specific bits for each.
// TODO: Check that program is actually using these return codes...
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
   in_port_t src_port;
   // linked-list of clients makes arbitrary insertion/removal somewhat easier
   struct Client * next;
};

struct ClientList
{
   struct Client * head;
   struct Client * tail;
   size_t len;
};

struct StreamContext
{
   pthread_t acceptor;
   pthread_t responder;
   pthread_mutex_t mtx;
   int sfd; // socket descriptor
   struct in_addr interface_addr;
   in_port_t port;
   struct ClientList clients;
};

static void handleSIGINT(int sig_num);

static void * acceptorThread(void * arg);
static void * responderThread(void * arg);

static bool addClient( struct StreamContext * ctx,
                       const struct Client * client_info );
static bool rmvClient( struct StreamContext * ctx,
                       in_addr_t ip,
                       in_port_t port );

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

   printf( "Hello! This is the REPL for a demo IPv4-only server.\n"
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

         // FIXME: Remove this printf "found" section after you're done developing
         printf("\tFound arguments:\n"
                "\t\tIP Address: %s\n"
                "\t\tPort: %s\n",
                cmd_arg_addr,
                cmd_arg_port );

         // Attempt to create socket
         int sfd_listening = socket(AF_INET, SOCK_STREAM, 0);
         if ( sfd_listening < 0 )
         {
            fprintf( stderr,
                     "Error: Failed to create listening socket.\n"
                     "socket() returned: %d, errno: %s (%d)\n",
                     sfd_listening, strerror(errno), errno );
            continue;
         }

         // Since this is a TCP socket, let's prevent a lingering socket address
         // already used error.
         retcode = setsockopt( sfd_listening,
                               SOL_SOCKET,
                               SO_REUSEADDR,
                               &(int){1},
                               sizeof(int) );
         assert(retcode == 0); // if setsockopt() failed, we set something up wrong

         // Convert string arguments to what are needed for bind()'ing
         // First the IP address
         struct in_addr numerical_addr;
         retcode = inet_pton(AF_INET, cmd_arg_addr, &numerical_addr);
         assert(retcode != -1); // -1 is invalid address family
         if ( 0 == retcode )
         {
            fprintf( stderr,
                     "Error: Invalid IPv4 address: %s\n"
                     "Please try again.\n",
                     cmd_arg_addr );
            continue;
         }

         // Now the port
         char * strtol_end_ptr = cmd_arg_port;
         in_port_t port = strtol(cmd_arg_port, &strtol_end_ptr, 10);
         // If I did my job right above /w the argument parsing, the following
         // assertions should be true.
         assert(*strtol_end_ptr == '\0'); // Full port string was a number
         port = htons(port); // Convert to network byte order for socket API

         // Now bind to the interface we just defined
         retcode = bind( sfd_listening,
                         (struct sockaddr *)
                         &(struct sockaddr_in) {
                            .sin_family = AF_INET,
                            .sin_port = port,
                            .sin_addr = numerical_addr
                         },
                         sizeof(struct sockaddr_in) );
         if ( retcode != 0 )
         {
            char addrbuf[INET_ADDRSTRLEN];
            const char * rc = inet_ntop(AF_INET, &numerical_addr, addrbuf, sizeof numerical_addr);
            assert(rc != nullptr);
            fprintf( stderr,
                     "Error: Failed to bind socket to specified interface:\n"
                     "\tIP Address: %s\n"
                     "\tPort: %d\n"
                     "bind() returned: %d, errno: %s (%d)\n"
                     "Socket will be closed. Please try again.\n",
                     addrbuf, port, retcode, strerror(errno), errno );

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr, "Error: Unable to close socket! Aborting program.\n");
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }

         retcode = listen(sfd_listening, STREAM_LISTEN_QUEUE_SZ);
         if ( retcode != 0 )
         {
            fprintf( stderr,
                     "Error: Failed to start listening on interface.\n"
                     "listen() returned: %d, errno: %s (%d)\n"
                     "Socket will be closed. Please try again.\n",
                     retcode, strerror(errno), errno );

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr, "Error: Unable to close socket! Aborting program.\n");
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }

         struct StreamContext * ctx = calloc( 1, sizeof(struct StreamContext) );
         if ( nullptr == ctx )
         {
            fprintf( stderr,
                     "Error: Failed to allocate context.\n"
                     "errno: %s (%d)\n"
                     "Socket will be closed. Please try again.\n",
                     strerror(errno), errno );

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr, "Error: Unable to close socket! Aborting program.\n");
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }

         ctx->sfd = sfd_listening;
         ctx->interface_addr = numerical_addr;
         ctx->port = port;

         // Create thread objects and point the thread fcns to their respective
         // local fcns, each of which take this stream context ptr as an arg.
         pthread_mutex_init(&ctx->mtx, nullptr); // default mutex attributes
         retcode = pthread_create( &ctx->acceptor,
                                   nullptr, // default thread attributes
                                   acceptorThread,
                                   ctx );
         if ( retcode != 0 )
         {
            fprintf( stderr,
                     "Error: Failed to create acceptor thread for this context.\n"
                     "pthread_create() returned: %d : %s\n"
                     "Socket will be closed and context freed. Please try again.\n",
                     retcode, strerror(retcode) );

            free(ctx);

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr, "Error: Unable to close socket! Aborting program.\n");
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }

         retcode = pthread_create( &ctx->responder,
                                   nullptr, // default thread attributes
                                   responderThread,
                                   ctx );
         if ( retcode != 0 )
         {
            fprintf( stderr,
                     "Error: Failed to create responder thread for this context.\n"
                     "pthread_create() returned: %d : %s\n"
                     "Socket will be closed and context freed. Please try again.\n",
                     retcode, strerror(retcode) );

            free(ctx);

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr, "Error: Unable to close socket! Aborting program.\n");
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }
         
         // FIXME: Move this closing of the socket descriptor to its appropriate
         //        spot once we've got the destroy/cleanup code written up. This
         //        is  here for now so that -fanalyzer stops complaining about
         //        a leaking file.
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
   struct StreamContext * ctx = arg;
   static size_t nreps = 0;

   printf("In acceptor thread. Iteration: %zu", nreps);

   return nullptr;
}

static void * responderThread(void * arg)
{
   // TODO
   struct StreamContext * ctx = arg;
   static size_t nreps = 0;

   printf("In responder thread. Iteration: %zu", nreps);

   return nullptr;
}

static bool addClient( struct StreamContext * ctx,
                       const struct Client * client_info )
{
   assert(ctx != nullptr);
   assert(client_info != nullptr);
   assert( ctx->clients.head != nullptr
           || (ctx->clients.head == nullptr && ctx->clients.len == 0) );
   assert(client_info->sfd >= 0);
   assert(client_info->next == nullptr);

   // It is assumed that the client object is temporary and we need to
   // dynamically allocate a client object here to then place in the client list
   struct Client * new_client = malloc( sizeof(struct Client) );
   if ( nullptr == new_client )
   {
      // TODO: Log error in library log that user can choose to read/print from?
      return false;
   }
   *new_client = *client_info;

   // Add to list
   if ( 0 == ctx->clients.len )
   {
      ctx->clients.head = new_client;
      ctx->clients.tail = new_client;
   }
   else
   {
      ctx->clients.tail->next = new_client;
      ctx->clients.tail = ctx->clients.tail->next;
   }

   ctx->clients.len++;

   assert(new_client->next == nullptr);
   assert(ctx->clients.head != nullptr);
   assert(ctx->clients.tail != nullptr);
   assert(ctx->clients.tail->next == nullptr);
   assert(ctx->clients.len > 0);

   return true;
}

static bool rmvClient( struct StreamContext * ctx,
                       in_addr_t ip,
                       in_port_t port )
{
   assert(ctx != nullptr);
   assert(ctx->clients.head != nullptr); // list should not be empty
   assert(ctx->clients.len > 0);
   assert(ip != 0);
   assert(port != 0);

   // Find client in list based on IP and port
   struct Client * old_client = nullptr;
   struct Client * prv_node = ctx->clients.head;
   size_t iter = 1;

   for ( struct Client * curr = ctx->clients.head;
         curr != nullptr && iter <= ctx->clients.len;
         curr = curr->next, ++iter )
   {
      if ( iter > 2 )
         prv_node = prv_node->next;

      struct sockaddr_in sock_info;
      socklen_t sock_info_len = sizeof(struct sockaddr_in);
      int retcode = getsockname( curr->sfd, (struct sockaddr *)&sock_info, &sock_info_len );
      // If getsockname() fails, we did something wrong in constructing the client...
      assert(retcode == 0);
      assert( sock_info_len <= sizeof(struct sockaddr_in) );
      
      if ( ip == sock_info.sin_addr.s_addr
           && port == sock_info.sin_port )
      {
         old_client = curr;
         break;
      }
   }

   // If we failed to find a match in the list...
   if ( nullptr == old_client )
      return false;

   // Close socket line /w client
   int retcode = close(old_client->sfd);
   if ( retcode != 0 )
   {
      // TODO: Log error in library log that user can choose to read/print from?
      return false;
   }

   // Remove from list
   if ( 1 == ctx->clients.len )
   {
      ctx->clients.head = nullptr;
      ctx->clients.tail = nullptr;
   }
   else
   {
      prv_node->next = old_client->next;
   }

   ctx->clients.len--;

   // Free client object
   free(old_client);

   return true;
}

static void rmvAllClients( struct StreamContext * ctx ); // TODO

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
