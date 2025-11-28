// Socket-Specific Headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
// Signal System Headers
#include <signal.h>
// General-Purpose Headers
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <assert.h>
#include <ctype.h>
// Table String Lookups Src Inclusion for Unity Build
#include "ai_flags_lookup.c"
#include "ai_family_lookup.c"
#include "ai_protocol_lookup.c"
#include "ai_socktype_lookup.c"

constexpr int GETADDRINFO_SUCCESS = 0;

static volatile sig_atomic_t UserEndedSession = false;

void handleSIGINT(int sig_num);
#ifndef NDEBUG
bool checkNullTermination(char arr[], size_t len);
#endif // NDEBUG
void printAddrInfoObject( struct addrinfo * obj, size_t idx );

/******************************************************************************/
int main(void)
{
   struct sigaction sa_cfg;
   memset( &sa_cfg, 0x00, sizeof(sa_cfg) );
   sa_cfg.sa_handler = handleSIGINT;
   sigemptyset(&sa_cfg.sa_mask);
   sa_cfg.sa_flags = 0; // Default signal handling behavior, including no SA_RESTART
                        // if system call was interrupted by signal.
   sigaction(SIGINT, &sa_cfg, NULL);

   printf("Enter one of the following commands:\n"
          "- service <service_name>\n"
          "- hostname <hostname>\n"
          "- local\n"
          "- listen\n" );

   size_t nreps = 0;
   constexpr size_t NREPS_MAX = 1'000;
   while ( !UserEndedSession
           && nreps < NREPS_MAX )
   {
      printf("> ");
      fflush(stdout);

      constexpr size_t MAX_STRLEN = 30;
      char buf[MAX_STRLEN];

      if ( fgets(buf, sizeof(buf), stdin) == NULL )
      {
         // EOF reached or system call interrupted by signal and errno == EINTR
         // Time to gracefully stop
         break;
      }

      // Clear newline from buf, or identify that too many characters have been entered
      char * newline_ptr = memchr( buf, '\n', sizeof(buf) );
      if ( nullptr == newline_ptr )
      {
         // Too many characters entered! Clear remaining line input and print err msg.
         int c;
         while ( (c = getchar()) != '\n' && c != EOF );
         fprintf( stderr,
                  "Too many characters entered. Max is: %zu. Please try again.",
                  MAX_STRLEN );
         continue;
      }
      else
      {
         *newline_ptr = '\0';
      }
      assert( checkNullTermination(buf, sizeof buf) );

      // Pattern match command
      size_t cmd_len = strlen(buf);
      // Convert input to lower-case
      for ( size_t i=0; i < cmd_len && buf[i] != '\0'; ++i )
         buf[i] = tolower(buf[i]);
      // Start the comparisons
      if ( strncmp( buf, "service", sizeof("service")-1 ) == 0 )
      {
         // Find the argument portion
         char * space_ptr = memchr(buf, ' ', cmd_len);
         if ( nullptr == space_ptr )
         {
            fprintf(stderr, "No argument for service command found. Try again.\n");
            continue;
         }

         // Argument portion is character that follows the space
         char * service = space_ptr + 1;

         // TODO: Check past first argument!

         // Begin the getaddrinfo() action
         printf("----------------------------------------------------------------------\n");
         printf("First, let's do a NULL node argument.\n");
         printf("----------------------------------------------------------------------\n");
         struct addrinfo * result;
         int retcode = getaddrinfo( NULL, // node
                                    service,
                                    NULL, // hints
                                    &result );
         if ( retcode != GETADDRINFO_SUCCESS )
         {
            fprintf( stderr,
                     "Error: getaddrinfo() returned: %s\n",
                     gai_strerror(retcode) );
            continue;
         }

         size_t i = 1;
         for ( struct addrinfo * it = result; it != NULL; it = it->ai_next, ++i )
            printAddrInfoObject(it, i);

         freeaddrinfo(result);

         printf("----------------------------------------------------------------------\n");
         printf("Now, let's try google.com.\n");
         printf("----------------------------------------------------------------------\n");
         retcode = getaddrinfo( "google.com", // node
                                service,
                                NULL, // hints
                                &result );
         if ( retcode != GETADDRINFO_SUCCESS )
         {
            fprintf( stderr,
                     "Error: getaddrinfo() returned: %s\n",
                     gai_strerror(retcode) );
            continue;
         }

         i = 1;
         for ( struct addrinfo * it = result; it != NULL; it = it->ai_next, ++i )
            printAddrInfoObject(it, i);

         freeaddrinfo(result);
      }
      else if ( strncmp( buf, "hostname", sizeof("hostname")-1 ) == 0 )
      {

      }
      else if ( strncmp( buf, "local", sizeof("local")-1 ) == 0 )
      {

      }
      else if ( strncmp( buf, "listen", sizeof("listen")-1 ) == 0 )
      {

      }
      else
      {
         fprintf(stderr, "Invalid command: %s. Try again.\n", buf);
         continue;
      }
   }
   assert(nreps <= NREPS_MAX);
   assert(UserEndedSession == false || UserEndedSession == true);

   puts("");
   if ( nreps >= NREPS_MAX )
      puts("Maximum loops reached.");
   else
      puts("User ended session.");
   puts("See ya again soon! Goodbye for now :).");

   return 0;
}
/******************************************************************************/

void handleSIGINT(int sig_num)
{
   (void)sig_num; // Only SIGINT is handled here
   UserEndedSession = true;
}

#ifndef NDEBUG
bool checkNullTermination(char arr[], size_t len)
{
   for ( size_t i=0; i < len; ++i )
   {
      if ( arr[i] == '\0' )
         return true;
   }
   return false;
}
#endif // NDEBUG

void printAddrInfoObject( struct addrinfo * obj, size_t idx )
{
   assert(obj != nullptr);

   printf("Address %zu:\n", idx);

   printf("\t->ai_flags:\n");
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0001] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0001]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0002] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0002]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0004] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0004]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0008] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0008]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0010] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0010]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0020] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0020]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0040] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0040]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0080] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0080]);
   if ( AiFlagsStringLookup[(obj->ai_flags) & 0x0400] != nullptr )
      printf("\t\t%s\n", AiFlagsStringLookup[(obj->ai_flags) & 0x0400]);

   printf("\t->ai_family: %s\n", AiFamilyStringLookup[obj->ai_family]);
   printf("\t->ai_socktype: %s\n", AiSocktypeStringLookup[obj->ai_socktype]);
   printf("\t->ai_protocol: %s\n", AiProtocolStringLookup[obj->ai_protocol]);
   printf("\t->ai_addrlen: %d\n", obj->ai_addrlen);
   printf("\t->ai_addr->sa_family: %s\n", AiFamilyStringLookup[obj->ai_addr->sa_family]);
   if ( obj->ai_addr->sa_family == AF_INET )
   {
      printf( "\t((struct sockaddr_in *)(->ai_addr))->sin_port: %d\n",
              ntohs( ((struct sockaddr_in *)(obj->ai_addr))->sin_port ) );
      char ipaddress_str[INET_ADDRSTRLEN];
      const char * retcode = inet_ntop( AF_INET,
                                        &((struct sockaddr_in *)(obj->ai_addr))->sin_addr,
                                        ipaddress_str,
                                        sizeof ipaddress_str );
      if ( retcode != NULL )
         printf( "\t((struct sockaddr_in *)(->ai_addr))->sin_addr: %s\n", ipaddress_str );
      else
         fprintf( stderr, "Error: inet_ntop() unable to convert ->ai_addr->sin_addr member.\n");
   }
   else if ( obj->ai_addr->sa_family == AF_INET6 )
   {
      printf( "\t((struct sockaddr_in6 *)(->ai_addr))->sin6_port: %d\n",
              ntohs( ((struct sockaddr_in6 *)(obj->ai_addr))->sin6_port ) );

      char ipaddress_str[INET6_ADDRSTRLEN];
      const char * retcode = inet_ntop( AF_INET6,
                                        &((struct sockaddr_in6 *)(obj->ai_addr))->sin6_addr,
                                        ipaddress_str,
                                        sizeof ipaddress_str );
      if ( retcode != NULL )
         printf( "\t((struct sockaddr_in *)(->ai_addr))->sin6_addr: %s\n", ipaddress_str );
      else
         fprintf( stderr, "Error: inet_ntop() unable to convert ->ai_addr->sin_addr member.\n");

      printf( "\t((struct sockaddr_in6 *)(->ai_addr))->sin6_flowinfo: 0x%04X\n",
              ntohs( ((struct sockaddr_in6 *)(obj->ai_addr))->sin6_flowinfo ) );
      printf( "\t((struct sockaddr_in6 *)(->ai_addr))->sin6_scope_id: 0x%04X\n",
              ntohs( ((struct sockaddr_in6 *)(obj->ai_addr))->sin6_scope_id ) );
   }
   else
   {
      // TODO: Not sure what to do with non AF_INETx family types...
   }
   puts("");
}
