// Networking header(s)
#include <arpa/inet.h>
// General-purpose headers
#include <stdio.h>
#include <signal.h>
#include <string.h>
#include <errno.h>
#include <assert.h>

constexpr int INET_PTON_SUCCESS      =  1;
constexpr int INET_PTON_FAIL_INV_STR =  0;
constexpr int INET_PTON_FAIL_INV_AF  = -1;

static volatile sig_atomic_t bUserEndsSession = false;

enum MainRetCode
{
   MAIN_RETCODE_FINE = 0,
   MAIN_RETCODE_
};

void handleSIGINT(int sig_num);
bool checkNullTermination(char arr[], size_t len);

int main(void)
{
   // Register SIGINT for user interrupt signal
   struct sigaction sa_cfg;
   // memset'ing to 0 includes no SA_RESTART within sa_flags
   memset( &sa_cfg, 0x00, sizeof(sa_cfg) );
   sa_cfg.sa_handler = handleSIGINT;
   sigemptyset(&sa_cfg.sa_mask);
   int retcode = sigaction(SIGINT, &sa_cfg, NULL);
   if ( retcode != 0 )
   {
      fprintf( stderr,
               "sigaction() failed! Returned %d, errno: %d\n",
               retcode,
               errno );
      return 1;
   }

   printf(" Enter an IP address in numbers-and-dots notation"
          " (Ctrl-C or Ctrl-D to stop gracefully):\n");

   constexpr size_t NREPS_MAX = 10;
   size_t nreps = 0;
   while ( !bUserEndsSession
           && nreps++ < NREPS_MAX )
   {
      char buf[INET_ADDRSTRLEN + 2] = {0}; // Extra space to help handle input
                                           // too long case. See further down.

      printf("> ");
      fflush(stdout); // Make sure stdout actually goes out

      if ( fgets(buf, sizeof(buf), stdin) == NULL )
      {
         // Either EOF reached or signal interrupt + errno == EINTR
         // Time to gracefully stop.
         break;
      }

      // Replace newline /w NULL termination
      size_t newline_pos = strcspn(buf, "\n");
      // Alternative to strcspn:
      // char * ptr_newline_pos = memchr(buf, '\n', sizeof buf);
      // if ( nullptr == ptr_newline_pos ) ...
      if ( newline_pos > INET_ADDRSTRLEN )
      {
         // More characters than INET_ADDRSTRLEN. Clear input buffer and
         // print error for this round.
         int c; // Needs to be int for EOF
         while ( (c = getchar()) != '\n' && c != EOF );
         fprintf(stderr, "Too many characters entered. Try again.\n\n");

         continue;
      }
      assert( newline_pos <= INET_ADDRSTRLEN );
      buf[ newline_pos ] = '\0';
      assert( checkNullTermination(buf, sizeof buf) );

      // Check for non-signal non-EOF based exit request...
      if (   strcmp(buf, "exit") == 0
          || strcmp(buf, "quit") == 0 )
      {
         break;
      }

      // Convert IP address!
      in_addr_t num_ipaddr = INADDR_NONE;
      int retcode = inet_pton(AF_INET, buf, &num_ipaddr);
      switch ( retcode )
      {
         case INET_PTON_SUCCESS:
            printf( "Numerical form of %s is 0x%08X\n\n", buf, ntohl(num_ipaddr) );
            break;

         case INET_PTON_FAIL_INV_AF:
            fprintf( stderr,
                     "Error: inet_pton() unable to convert %s."
                     "\nReturned: %d, errno: %s (%d)."
                     "\nTry again.\n\n",
                     buf,
                     retcode,
                     strerror(errno),
                     errno );
            break;

         case INET_PTON_FAIL_INV_STR:
            fprintf( stderr,
                     "Error: inet_pton() unable to convert %s. "
                     "\nReturned: %d (invalid string IP address notation)."
                     "\nTry again.\n\n",
                     buf,
                     retcode );
            break;

         default:
            fprintf( stderr,
                     "Somehow, inet_pton() returned a value out-of-spec..."
                     "\nReturned: %d"
                     "Aborting!\n\n",
                     retcode );
            assert(false); // Should never get here
      }
   }

   assert(nreps <= NREPS_MAX);
   assert(bUserEndsSession == false || bUserEndsSession == true);

   puts("");
   if ( nreps >= NREPS_MAX )
      puts("Maximum loops reached.");
   else
      puts("User ended session.");
   puts("See ya again soon! Goodbye for now :).");

   return 0;
}

void handleSIGINT(int sig_num)
{
   (void)sig_num; // No use of this here when this handler is only for SIGINT
   bUserEndsSession = true;
}

bool checkNullTermination(char arr[], size_t len)
{
   for (size_t i=0; i<len; ++i)
      if (arr[i] == '\0') return true;

   return false;
}
