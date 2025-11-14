#include <stdio.h>
#include <arpa/inet.h>
#include <signal.h>
#include <string.h>

constexpr int INET_PTON_SUCCESS = 1;
static volatile sig_atomic_t bUserEndsSession = false;

void handleSIGINT(int sig_num)
{
   (void)sig_num; // Not yet sure what this is for if we already associate
                  // a signal number /w this signal handler in signal()...
   bUserEndsSession = true;
}

int main(void)
{
   struct sigaction sa_cfg;
   memset( &sa_cfg, 0x00, sizeof(sa_cfg) );
   sa_cfg.sa_handler = handleSIGINT;
   sigemptyset(&sa_cfg.sa_mask);
   sa_cfg.sa_flags = 0; // defaults for sigaction - namely, no SA_RESTART to
                        // restart sys calls after sig handler
   sigaction(SIGINT, &sa_cfg, NULL);

   printf(" Enter an IP address in numbers-and-dots notation"
          " (Ctrl-C or Ctrl-D to stop gracefully):\n");

   const size_t NREPS_MAX = 10;
   size_t nreps = 0;
   while ( !bUserEndsSession
           && nreps++ < NREPS_MAX )
   {
      in_addr_t num_ipaddr = INADDR_NONE;
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
      if ( newline_pos <= INET_ADDRSTRLEN )
      {
         buf[ newline_pos ] = '\0';
      }
      else
      {
         // More characters than INET_ADDRSTRLEN. Clear input buffer and
         // print error for this round.
         int c; // Needs to be int for EOF
         while ( (c = getchar()) != '\n' && c != EOF );
         fprintf(stderr, "Too many characters entered. Try again.\n\n");

         continue;
      }
      
      int retcode = inet_pton(AF_INET, buf, &num_ipaddr);
      if ( retcode != INET_PTON_SUCCESS )
      {
         fprintf( stderr,
                  "Error: inet_pton() unable to convert %s."
                  " Returned: %d. Try again.\n\n",
                  buf,
                  retcode );
      }
      else
      {
         printf( "Numerical form of %s is 0x%08X\n\n", buf, ntohl(num_ipaddr) );
      }
   }

   puts("");
   if ( nreps >= NREPS_MAX )
      puts("Maximum loops reached.");
   else
      puts("User ended session.");
   puts("See ya again soon! Goodbye for now :).");

   return 0;
}
