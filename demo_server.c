/**
 * @file demo_server.c
 * @brief TODO Brief description on demo_server
 */

/*************************** File Header Inclusions ***************************/
#define _POSIX_C_SOURCE 200809L // Specify atleast POSIX.1-2008 compatibility
#define _GNU_SOURCE

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
#include <stdatomic.h>
#include <sys/select.h>
#include <unistd.h>
#include <time.h>

#ifndef _GNU_SOURCE
#define strerrorname_np(str) "strerrorname_np() NOT IMPLEMENTED"
#endif

/***************************** Local Declarations *****************************/
#define ARR_LEN(arr) (sizeof(arr) / sizeof(arr[0]))

constexpr size_t MAX_CLIENTS = 1'000;
constexpr size_t MAX_SERVERS = 1'000;
constexpr size_t MAX_THREAD_REPS = 1'000'000;
constexpr int STREAM_LISTEN_QUEUE_SZ = 5;
// Honestly, 1 second is too long, but let's optimize later
constexpr time_t MAX_MTX_LOCK_WAIT_SEC = 1;
constexpr time_t MAX_MTX_PRINTF_LOCK_WAIT_SEC = 3;

static volatile sig_atomic_t bUserEndedSession = false;

static pthread_mutex_t mtxPrintf = PTHREAD_MUTEX_INITIALIZER;

// Some errors shouldn't abort the program, but we will still return a code
// indicating something went wrong. To account for a possible accumulation
// of errors, need to reserve specific bits for each.
// TODO: Check that program is actually using these return codes...
enum MainRetCode
{
   MAIN_RC_FINE                    = 0x0000,
   MAIN_RC_SOCKET_CREATION_ERR     = 0x0001,
   MAIN_RC_SOCKET_BIND_ERR         = 0x0002,
   MAIN_RC_SOCKET_LISTEN_ERR       = 0x0004,
   MAIN_RC_SOCKET_ACCEPT_ERR       = 0x0008,
   MAIN_RC_SIGINT_REGISTRATION_ERR = 0x0010,
   MAIN_RC_NREP_LIM_HIT            = 0x0020,
   MAIN_RC_FAILED_CLOSE            = 0x0040,
};

struct Client
{
   int sfd;
   in_addr_t addr;
   in_port_t port;
   // Linked-list of clients makes arbitrary insertion/removal somewhat easier.
   struct Client * next;
};

struct ClientList
{
   struct Client * head;
   struct Client * tail;
   size_t len;
};

struct ServerContext
{
   atomic_bool enabled;
   struct in_addr listening_addr;
   in_port_t listening_port;
   int listening_sfd;
   pthread_t acceptor;
   pthread_t responder;
   pthread_mutex_t mtx;
   struct ClientList clients;
};

static void handleSIGINT(int sig_num);

static void * acceptorThread(void * arg);
static void * responderThread(void * arg);

static bool addClient( struct ServerContext * ctx,
                       const struct Client * client_info );
static bool rmvClient( struct ServerContext * ctx,
                       in_addr_t ip,
                       in_port_t port );

static int tryClose( int sfd );

#ifndef NDEBUG
bool isFullyNumeric(char * str, size_t len);
bool isNullTerminated(char * str, size_t max_len);
#endif // NDEBUG

/******************************* Main Function ********************************/
int main( int argc, char * argv[] )
{
   int main_retcode = MAIN_RC_FINE;
   int retcode; // for system calls
   static struct ServerContext * SrvCtxList[MAX_SERVERS];
   static size_t SrvCtxListIdx = 0;
   memset( &SrvCtxList, 0x00, sizeof(SrvCtxList) );

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
               "Returned: %d, errno: %s (%d): %s\n"
               "You won't be able to stop the program gracefully /w Ctrl+C, \n"
               " though Ctrl+C will still terminate the program.",
               retcode, strerrorname_np(errno), errno, strerror(errno) );

      main_retcode |= MAIN_RC_SIGINT_REGISTRATION_ERR;
   }

   printf( "Hello! This is the REPL for a demo IPv4-only server.\n"
           "Here is a brief list of the available commands (case-insensitive):\n"
           "\t- udp-create [ip_address : port]\n"
           "\t- udp-listen [timeout in seconds]\n"
           "\t- udp-print-msgs\n"
           "\t- udp-socks\n"
           "\t- udp-close sock_id\n"
           "\t- udp-close-all\n"
           "\t- tcp-create [ip_address : port]\n"
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
                     "socket() returned: %d, errno: %s (%d): %s\n",
                     sfd_listening,
                     strerrorname_np(errno), errno,  strerror(errno) );
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
            const char * rc = inet_ntop(AF_INET, &numerical_addr, addrbuf, INET_ADDRSTRLEN);
            if ( nullptr == rc )
            {
               fprintf( stderr,
                        "inet_ntop() failed to convert, errno: %s (%d): %s\n",
                        strerrorname_np(errno), errno, strerror(errno) );
               assert(rc != nullptr); // Still abort, because this indicates a design issue
            }

            fprintf( stderr,
                     "Error: Failed to bind socket to specified interface:\n"
                     "\tIP Address: %s\n"
                     "\tPort: %d\n"
                     "bind() returned: %d, errno: %s (%d): %s\n"
                     "Socket will be closed. Please try again.\n",
                     addrbuf, ntohs(port), retcode,
                     strerrorname_np(errno), errno, strerror(errno) );

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr,
                        "%s:%d : Error: Unable to close socket %d!\n"
                        "Aborting program.\n",
                        __FILE__, __LINE__, sfd_listening );
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
                     "listen() returned: %d, errno: %s (%d): %s\n"
                     "Socket will be closed. Please try again.\n",
                     retcode, strerrorname_np(errno), errno, strerror(errno) );

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr,
                        "%s:%d : Error: Unable to close socket %d!\n"
                        "Aborting program.\n",
                        __FILE__, __LINE__, sfd_listening );
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }

         struct ServerContext * ctx = calloc( 1, sizeof(struct ServerContext) );
         if ( nullptr == ctx )
         {
            fprintf( stderr,
                     "Error: Failed to allocate context.\n"
                     "errno: %s (%d): %s\n"
                     "Socket will be closed. Please try again.\n",
                     strerrorname_np(errno), errno, strerror(errno) );

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr,
                        "%s:%d : Error: Unable to close socket %d!\n"
                        "Aborting program.\n",
                        __FILE__, __LINE__, sfd_listening );
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }

         ctx->listening_sfd = sfd_listening;
         ctx->listening_addr = numerical_addr;
         ctx->listening_port = port;

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
                     "pthread_create() returned: %d : %s (%d): %s\n"
                     "Socket will be closed and context freed. Please try again.\n",
                     retcode, strerrorname_np(errno), errno, strerror(retcode) );

            free(ctx);

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr,
                        "%s:%d : Error: Unable to close socket %d!\n"
                        "Aborting program.\n",
                        __FILE__, __LINE__, sfd_listening );
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
                     "pthread_create() returned: %d : %s (%d): %s\n"
                     "Socket will be closed and context freed. Please try again.\n",
                     retcode, strerrorname_np(errno), errno, strerror(retcode) );

            free(ctx);

            size_t close_nreps = 0;
            while ( (retcode = close(sfd_listening)) != 0 && close_nreps++ < 10 );
            if ( close_nreps >= 10 )
            {
               fprintf( stderr,
                        "%s:%d : Error: Unable to close socket %d!\n"
                        "Aborting program.\n",
                        __FILE__, __LINE__, sfd_listening );
               // TODO: Figure out a way to gracefully handle this error case...
               abort();
            }

            continue;
         }

         ctx->enabled = true; // strictest memory ordering seq_cst is fine here
         SrvCtxList[ SrvCtxListIdx++ ] = ctx;

         printf("Successfully created listening context.\n");
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
   puts("Closing all contexts...");
   // I'd really like to `for ( auto& ctx : SrvCtxList )`...
   struct ServerContext ** begin = &SrvCtxList[0];
   struct ServerContext ** end   = begin + ARR_LEN(SrvCtxList);
   for ( struct ServerContext ** ctx = &SrvCtxList[0];
         ctx < end;
         ctx++ )
   {
      if ( nullptr == *ctx )
         continue;

      // Disable thread infinite loops
      (*ctx)->enabled = false;

      // Shut down listening socket - this will also unblock any pending accept() calls
      retcode = shutdown( (*ctx)->listening_sfd, SHUT_RD );
      if ( retcode != 0)
      {
         fprintf( stderr,
                  "Unable to shutdown listening socket %d.\n"
                  "errno: %s (%d): %s\n",
                  (*ctx)->listening_sfd,
                  strerrorname_np(errno), errno, strerror(errno) );

         continue;
      }

      // Close listening socket
      printf("Closing listening socket for this context...\n");
      retcode = tryClose((*ctx)->listening_sfd);
      if ( retcode != 0 )
      {
         fprintf( stderr,
                  "Unable to close listening socket %d after repeated attempts.\n",
                  (*ctx)->listening_sfd );
         // TODO: Log?

         continue;
      }

      // Shut down client sockets - this will also unblock any pending accept() calls

      // Close client sockets - this will likewise unblock any blocking receive calls
      printf("Shutting down and closing all client sockets"
             "that were opened for this context...\n");
      assert( (*ctx)->clients.len <= MAX_CLIENTS );
      assert( ( (*ctx)->clients.len == 0
                  && (*ctx)->clients.head == nullptr
                  && (*ctx)->clients.tail == nullptr )
              ||
              ( (*ctx)->clients.len > 0
                  && (*ctx)->clients.head != nullptr
                  && (*ctx)->clients.tail != nullptr ) );

      if ( (*ctx)->clients.len > 0 )
      {
         size_t nclients = 0;
         struct Client * client;

         for ( client = (*ctx)->clients.head;
               client != nullptr && nclients++ <= (*ctx)->clients.len;
               client = client->next )
         {
            retcode = shutdown( (*ctx)->listening_sfd, SHUT_RD );
            if ( retcode != 0)
            {
               fprintf( stderr,
                        "Unable to shutdown client socket %d.\n"
                        "errno: %s (%d): %s\n",
                        (*ctx)->listening_sfd,
                        strerrorname_np(errno), errno, strerror(errno) );
            }

            retcode = tryClose(client->sfd);
            if ( retcode != 0 )
            {
               fprintf( stderr,
                        "Unable to close client socket %d after repeated attempts.\n"
                        "Client's IP Address: 0x%08X, Port: %d\n",
                        client->sfd, ntohl(client->addr), ntohs(client->port) );
               // TODO: Log?
            }
         }
         assert(client == (*ctx)->clients.tail); // Double-check that we reached the tail
      }

      // Now we can await for threads to return and join back in here
      printf("Joining in the acceptor and responder threads of this context...\n");
      retcode = pthread_join((*ctx)->acceptor, nullptr);
      assert(retcode == 0); // pthread_join should not fail with proper design
      retcode = pthread_join((*ctx)->responder, nullptr);
      assert(retcode == 0); // pthread_join should not fail with proper design

      // Close out mutexes
      printf("Closing out mutexes...\n");
      retcode = pthread_mutex_destroy(&(*ctx)->mtx);
      assert(retcode == 0); // The only error would be EBUSY, (mtx is locked),
                            // and that indicates bad design

      // Zero-out client objects and free them
      printf("Zeroing out client objects and freeing them...\n");
      if ( (*ctx)->clients.len > 0 )
      {
         struct Client * prev = (*ctx)->clients.head;
         struct Client * curr = (*ctx)->clients.head->next;
         size_t nclients = 0;

         for ( ;
               curr != nullptr && nclients++ <= (*ctx)->clients.len;
               prev = curr, curr = curr->next )
         {
            memset( prev, 0x00, sizeof(struct Client) );
            free(prev);
         }
         memset( prev, 0x00, sizeof(struct Client) );
         free(prev);
      }

      // Zero-out server context object and free it
      printf("Zeroing out server context object and freeing it...\n");
      memset((*ctx), 0x00, sizeof(struct ServerContext) );
      free((*ctx));

      printf("Done cleaning out this server context!\n");
   }

   puts("");
   puts("");
   if ( nreps >= NMAX )
   {
      printf( "REPL repetitions hit limit: %zu / %zu\n"
              "Session ended. Please restart program.\n",
              nreps, NMAX );

      main_retcode |= MAIN_RC_NREP_LIM_HIT;
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

   if ( bUserEndedSession )
      abort();

   bUserEndedSession = true;
}

static void * acceptorThread(void * arg)
{
   struct ServerContext * ctx = arg;
   static size_t nreps = 0;

   while ( ctx->enabled && nreps++ < MAX_THREAD_REPS )
   {
#ifndef NDEBUG
      if ( nreps == 1 || nreps % 10 == 0 )
      {
         printf("\n\tIn acceptor thread. Iteration: %zu\n", nreps);
      }
#endif

      struct sockaddr_in client_info;
      socklen_t client_info_len = sizeof client_info;
      int new_conn_sfd = accept( ctx->listening_sfd,
                                 (struct sockaddr *)&client_info,
                                 &client_info_len );
      if ( new_conn_sfd < 0 )
      {
         if ( EBADF == errno || EINVAL == errno )
         {
            // This may be because main thread closed this socket to wake us
            // out of accept(), which is desired behavior.
            printf("\tListening file descriptor has been closed and/or shut down.\n");
            break;
         }

         // TODO: Handle other accept() errors
         fprintf( stderr,
                  "\tError: accept() returned: %d, errno: %s (%d): %s\n",
                  new_conn_sfd, strerrorname_np(errno), errno, strerror(errno) );
         continue;
      }
      else if ( client_info_len > sizeof client_info )
      {
         // TODO: Handle address getting truncated in accept()
         fprintf( stderr,
                  "\tError: Client address information got truncated.\n" );
         continue;
      }

      struct Client new_client;
      new_client.sfd  = new_conn_sfd;
      new_client.addr = client_info.sin_addr.s_addr;
      new_client.port = client_info.sin_port;
      new_client.next = nullptr;

      bool addedSuccessfully = addClient(ctx, &new_client);
      if ( !addedSuccessfully )
      {
         // TODO: Log lib error and handle this awkward failed client addition
      }

#ifndef NDEBUG
      char addrstr[INET_ADDRSTRLEN];
      const char * retcode = inet_ntop( AF_INET,
                                        &(struct in_addr){ .s_addr = new_client.addr },
                                        addrstr,
                                        sizeof addrstr );
      assert(retcode != nullptr); // inet_ntop better succeed, since this info
                                  // came from accept()

      printf( "\tNew client added! %d\n"
              "\t\tSrc IP Address: %s\n"
              "\t\tSrc Port: %d\n"
              "\tTalking to us on port: %d\n",
              new_client.sfd, addrstr, ntohs(new_client.port), ctx->listening_port );
#endif
   }

   printf("\tExiting acceptor thread... Performed %zu iterations.\n", nreps);

   return nullptr;
}

static void * responderThread(void * arg)
{
   struct ServerContext * ctx = arg;
   size_t nreps = 0;

   while ( ctx->enabled && nreps++ < MAX_THREAD_REPS )
   {
#ifndef NDEBUG
      //if ( nreps == 1 || nreps % 10 == 0 )
      //{
      //   printf("\n\tIn responder thread. Iteration: %zu\n", nreps);
      //}
#endif

      // Receive data from threads, placing that data into a "last 100 bytes"
      // buffer
      // TODO
   }

   printf("\tExiting responder thread... Performed %zu iterations.\n", nreps);

   return nullptr;
}

static bool addClient( struct ServerContext * ctx,
                       const struct Client * client_info )
{
   assert(ctx != nullptr);
   assert(client_info != nullptr);
   assert( ctx->clients.head != nullptr
           || (ctx->clients.head == nullptr && ctx->clients.len == 0) );
   assert(client_info->sfd >= 0);
   assert(client_info->next == nullptr);

   if ( ctx->clients.len >= MAX_CLIENTS )
   {
      fprintf( stderr, "Clientelle is full! %zu is the max.\n"
                       "Please close a connection first.\n", MAX_CLIENTS);
      return false;
   }

   // It is assumed that the client object is temporary and we need to
   // dynamically allocate a client object here to then place in the client list
   struct Client * new_client = malloc( sizeof(struct Client) );
   if ( nullptr == new_client )
   {
      // TODO: Log error in library log that user can choose to read/print from?
      return false;
   }
   *new_client = *client_info;

   // Add to shared list
   struct timespec lock_timeout;
   int retcode = clock_gettime(CLOCK_REALTIME, &lock_timeout);
   if ( retcode != 0 )
   {
      // TODO: Log error in library log that user can choose to read/print from?
      return false;
   }
   lock_timeout.tv_sec += MAX_MTX_LOCK_WAIT_SEC;
   retcode = pthread_mutex_timedlock(&ctx->mtx, &lock_timeout);
   // Pretty much any reason the timed mutex lock fails is cause for redesign
   // (e.g., different timeout, excessively long critical section elsewhere,
   // etc.), so call assert() to indicate this failure and abort.
   assert(retcode == 0); // FIXME: It'd be good to print out _who_ owned the lock at failure...

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

   retcode = pthread_mutex_unlock(&ctx->mtx);
   // Similarly, any error in unlocking signals a redesign to me. Assert!
   assert(retcode == 0);

   assert(new_client->next == nullptr);
   assert(ctx->clients.head != nullptr);
   assert(ctx->clients.tail != nullptr);
   assert(ctx->clients.tail->next == nullptr);
   assert(ctx->clients.len > 0);

   return true;
}

static bool rmvClient( struct ServerContext * ctx,
                       in_addr_t ip,
                       in_port_t port )
{
   assert(ctx != nullptr);
   assert(ctx->clients.head != nullptr); // list should not be empty
   assert(ctx->clients.len > 0);
   assert(ctx->clients.len <= MAX_CLIENTS); // if list is longer, something went wrong
   assert(ip != 0);
   assert(port != 0);
   
   // Find client in list based on IP and port
   // First, lock list
   struct timespec lock_timeout;
   int retcode = clock_gettime(CLOCK_REALTIME, &lock_timeout);
   if ( retcode != 0 )
   {
      // TODO: lib log...
      return false;
   }
   lock_timeout.tv_sec += MAX_MTX_LOCK_WAIT_SEC;
   retcode = pthread_mutex_timedlock(&ctx->mtx, &lock_timeout);
   // Pretty much any reason the timed mutex lock fails is cause for redesign
   // (e.g., different timeout, excessively long critical section elsewhere,
   // etc.), so call assert() to indicate this failure and abort.
   assert(retcode == 0); // FIXME: It'd be good to print out _who_ owned the lock at failure...

   struct Client * old_client = nullptr;
   struct Client * prv_node = ctx->clients.head;
   size_t iter = 1;

   for ( struct Client * curr = ctx->clients.head;
         curr != nullptr && iter <= ctx->clients.len;
         curr = curr->next, ++iter )
   {
      if ( iter > 2 )
         prv_node = prv_node->next;

      if ( ip == curr->addr && port == curr->port )
      {
         old_client = curr;
         break;
      }
   }

   // If we failed to find a match in the list...
   if ( nullptr == old_client )
   {
      retcode = pthread_mutex_unlock(&ctx->mtx);
      assert(retcode == 0); // If unlock fails, I screwed up
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

   retcode = pthread_mutex_unlock(&ctx->mtx);
   assert(retcode == 0); // If unlock fails, I screwed up

   // Close socket line /w client
   size_t close_nreps = 0;
   while ( (retcode = close(old_client->sfd)) != 0 && close_nreps++ < 10 );
   if ( close_nreps >= 10 )
   {
      fprintf( stderr,
               "%s:%d : Error: Unable to close socket %d!\n"
               "Aborting program.\n",
               __FILE__, __LINE__, old_client->sfd );
      // TODO: Figure out a way to gracefully handle this error case...
      abort();
   }

   // Free client object
   free(old_client);

   assert(ctx->clients.len < SIZE_MAX); // Underflow assertion

   return true;
}

static bool rmvAllClients( struct ServerContext * ctx ); // TODO

static int tryClose( int sfd )
{
   size_t n = 0;
   int retcode;
   do {
      retcode = close(sfd);
   } while ( retcode != 0 && n < 5 );

   return retcode;
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
