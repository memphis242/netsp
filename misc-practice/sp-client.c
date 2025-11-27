/**
 * @brief A "Socket Practice" (SP) Client
 * @date Nov 27, 2025 (Happy Thanksgiving!)
 */

// Socket-Specific Headers
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
// General-Purpose System Headers
#include <errno.h>
#include <string.h>
#include <signal.h>
#include <sys/wait.h>
#include <stdint.h>
#include <stdio.h>
#include <assert.h>

/* Macro Functions */
#define LAST_ELEMENT(arr) (arr[ (sizeof(arr)/sizeof(arr[0])) - 1])

/* Constant and Type Definitions */
enum MainReturnCodes
{
   MAIN_RETCODE_GOOD,
   MAIN_RETCODE_UNABLE_TO_CONNECT,
   MAIN_RETCODE_UNHANDLED_KERNEL_SIGNAL
};

enum InputParseRetCode
{
   IPARSE_SUCCESS,
   IPARSE_INVALID_INPUT,
   IPARSE_NONE_PURE_CMD_NUM,
   IPARSE_UNABLE_TO_PARSE
};

#define SP_CMD(cmd_enum, cmd_str, cmd_char, cmd_args) cmd_enum,
enum UserCmdCode
{
#  include "sp-cmds.h"
   UCMD_UNKNOWN
};
#undef SP_CMD

struct UserCmd
{
   enum UserCmdCode code;
   char * str;
   char num_char;
   char * args;
};

#define SP_CMD(cmd_enum, cmd_str, cmd_char, cmd_args) \
   { \
      .code     = cmd_enum, \
      .str      = cmd_str, \
      .num_char = cmd_char, \
      .args     = cmd_args \
   },
static const struct UserCmd UserCmdTbl[] =
{
#  include "sp-cmds.h"
};
#undef SP_CMD

/* Local Variables */
static volatile sig_atomic_t UserCancelledSession = false;

/* Local Function Declarations */
void handleSIGINT(int sig_num);
void printUsageInfo(void);
[[nodiscard]] enum InputParseRetCode getCmdNumber(char str[], enum UserCmdCode * cmd);

#ifndef NDEBUG
[[nodiscard]] bool checkNullTermination(char char_arr[], size_t len);
#endif

/******************************************************************************/

/* Meat of the Program */
int main(void)
{
   // TODO: assertions on UserCmdTbl

   // Register signal handler
   struct sigaction sa_cfg;
   memset( &sa_cfg, 0x00, sizeof(sa_cfg) );
   sa_cfg.sa_handler = handleSIGINT;
   sigemptyset(&sa_cfg.sa_mask);
   sa_cfg.sa_flags = 0x00; // Defaults for sigaction - i.e., no SA_RESTART to
                           // restart system calls after signal handler
   sigaction(SIGINT, &sa_cfg, NULL);
   
   // Connect to the Socket Practice (SP) server
   // TODO
   
   printf("Connected to SP server!");
   puts("");

   // Start REPL interface
   printUsageInfo();
   constexpr size_t MAX_REPL_ITERATIONS = 1'000'000;
   size_t repl_iters = 0;
   while ( UserCancelledSession == false
           && repl_iters < MAX_REPL_ITERATIONS )
   {
      constexpr size_t MAX_USER_INPUT_STRLEN = 100;
      char buf[MAX_USER_INPUT_STRLEN];

      printf("> ");
      fflush(stdout);

      if ( fgets( buf, sizeof(buf), stdin ) == NULL )
      {
         // Either EOF was reached or a signal interrupt + errno == EINTR
         // Time to gracefully stop
      }

      // fgets() also takes in the newline character. Replace /w NULL terminator.
      size_t newline_pos = strcspn(buf, "\n");
      if ( newline_pos < MAX_USER_INPUT_STRLEN )
      {
         buf[newline_pos] = '\0';
      }
      else
      {
         // Too many characters entered.
         // Clear stdin line buffer and print an error msg for this round
         int c; // Needs to be int for EOF
         while ( (c = getchar()) != '\n' && c != EOF );
         fprintf( stderr,
                  "Too many characters entered. The limit is %zu. Try again.\n",
                  MAX_USER_INPUT_STRLEN );
         continue;
      }
      assert( checkNullTermination(buf, sizeof(buf)) );

      // Pattern match on command
      enum UserCmdCode cmd = UCMD_UNKNOWN;
      // First, see if input was just a number...
      enum InputParseRetCode iparse_retcode = getCmdNumber(buf, &cmd);
      if ( iparse_retcode != IPARSE_SUCCESS )
      {
         // Not a number, check for command strings...
         for ( size_t i=0; i < sizeof(UserCmdTbl) / sizeof(UserCmdTbl[0]); ++i )
         {
            if ( strcmp(buf, UserCmdTbl[i].str) == 0 )
            {
               // Found match
               cmd = UserCmdTbl[i].code;
               break;
            }
         }
      }

      // See if a match was found...
      if ( UCMD_UNKNOWN == cmd )
      {
         // Neither, print error msg and go to next iteration
         fprintf(stderr, "Unknown command. Try again.\n");
         continue;
      }

      // Carry out cmd
      assert(cmd >= (enum UserCmdCode)0 && cmd < UCMD_UNKNOWN);
      switch(cmd)
      {
         case UCMD_MARCO:
            // TODO
            break;

         case UCMD_INET_PTON:
            // TODO
            break;

         case UCMD_GETADDRINFO:
            // TODO
            break;

         case UCMD_UNKNOWN:
            // fallthrough
         default:
            // Not supposed to get here!
            assert(true);
            break;

      }
   }
   assert( UserCancelledSession == false || UserCancelledSession == true );
   assert( repl_iters <= MAX_REPL_ITERATIONS );

   puts("");
   if ( repl_iters >= MAX_REPL_ITERATIONS )
      puts("Maximum loops reached.");
   else
      puts("User ended session.");
   puts("See ya again soon! Goodbye for now :).");

   return (int)MAIN_RETCODE_GOOD;
}

/******************************************************************************/

/* Local Function Implementations */

/**
 * @brief Handle the user interrupt signal by cancelling the session gracefully.
 */
void handleSIGINT(int sig_num)
{
   (void)sig_num; // this signal handler is only for SIGINT
   UserCancelledSession = true;
}

/**
 * @brief Print client program usage for REPL interface.
 */
void printUsageInfo(void)
{
   puts("");
   printf( "Usage Information:\n"
           "\tType in one of the following commands, either in string-form (case-insensitive)\n"
           "\tor the number:\n" );
   for ( size_t i=0; i < sizeof(UserCmdTbl) / sizeof(UserCmdTbl[0]); ++i )
   {
      printf( "\t- %s (%c) %s\n",
              UserCmdTbl[i].str,
              UserCmdTbl[i].num_char,
              UserCmdTbl[i].args );
   }
   puts("");
}

/**
 * @brief Attempt to parse the string buffer for a _pure_ command number
 * @note "Pure command number" means the string is just one of the valid cmd nums
 * @note Assumes buf is null-terminated!
 *
 * @param[in] str : string holding potential cmd num
 * @param[in] cmd : if pure cmd num was found, place parsed cmd code in here
 *
 * @return enum InputParseRetCode
 *    - IPARSE_SUCCESS string was a pure command number
 *    - IPARSE_INVALID_INPUT arguments were invalid (e.g., nullptr)
 *    - IPARSE_NONE_PURE_CMD_NUM argument had a valid cmd num but also additional chars
 *    - IPARSE_UNABLE_TO_PARSE string did not start /w a digit
 */
[[nodiscard]] enum InputParseRetCode getCmdNumber(char str[], enum UserCmdCode * cmd)
{
   assert( str != nullptr && cmd != nullptr );

   if ( str[0] < '0' || str[0] > LAST_ELEMENT(UserCmdTbl).num_char )
      return IPARSE_UNABLE_TO_PARSE;
   // Assuming single-digit command codes...
   else if ( str[1] != ' ' && str[1] != '\0' )
      return IPARSE_NONE_PURE_CMD_NUM;

   assert( (size_t)(str[0] - '0') < sizeof(UserCmdTbl) );
   *cmd = UserCmdTbl[str[0] - '0'].code;

   return IPARSE_SUCCESS;
}

#ifndef NDEBUG
/**
 * @brief Check if the character array is null-terminated within len specified
 * @return true if null termination found, false otherwise
 */
bool checkNullTermination(char char_arr[], size_t len)
{
   for ( size_t i=0; i < len; ++i )
   {
      if ( char_arr[i] == '\0' )
         return true;
   }
   return false;
}
#endif
