#include <iostream>
#include <string>
#include <vector>
#include <format>

#include <arpa/inet.h>
#include <cstdint>
#include <cassert>

enum ProgramReturnVals : int
{
   ALL_IS_WELL = 0,
   INVALID_SRC_IP,
   INVALID_DST_IP,
   INVALID_SRC_PORT,
   INVALID_DST_PORT,
   UNKNOWN_ERR
};

constexpr int INET_PTON_SUCCESS = 1;

int main(void)
{
   std::string src_ip;
   std::string dst_ip;
   std::uint16_t    src_port;
   std::uint16_t    dst_port;
   std::string udp_pld;

   // Retrieve user input
   // Enable exceptions from std::cin on invalid inputs for integer conversion
   std::cin.exceptions(std::ios_base::failbit | std::ios_base::badbit );
   std::cout << "Let's compute a UDP checksum!" << '\n'
             << "Provide the following information:" << '\n';
   std::cout << "Source IPv4 Address: ";
   std::cin >> src_ip;
   std::cout << "Destination IPv4 Address: ";
   std::cin >> dst_ip;
   std::cout << "Source Port: ";
   try {
      std::cin >> src_port;
   }
   catch ( const std::ios_base::failure& e ) {
      std::cerr << "Error: Unable to convert entry into integer." << std::endl;
      return INVALID_SRC_PORT;
   }
   std::cout << "Destination Port: ";
   try {
      std::cin >> dst_port;
   }
   catch ( const std::ios_base::failure& e ) {
      std::cerr << "Error: Unable to convert entry into integer." << std::endl;
      return INVALID_DST_PORT;
   }
   std::cout << "UDP Payload (space separate the bytes, in hex, optional 0x prefix): ";
   std::cin >> udp_pld;
   std::cout << std::endl;

   // Convert IP address strings to network-byte order bytes
   struct in_addr src_ip_num;
   int retcode = inet_pton( AF_INET, src_ip.c_str(), &src_ip_num );
   if ( retcode != INET_PTON_SUCCESS )
   {
      std::cerr << "Error: Unable to convert " << src_ip << " into numerical form. "
                   "inet_pton() returned " << retcode << std::endl;
      return INVALID_SRC_IP;
   }

   struct in_addr dst_ip_num;
   retcode = inet_pton( AF_INET, dst_ip.c_str(), &dst_ip_num );
   if ( retcode != INET_PTON_SUCCESS )
   {
      std::cerr << "Error: Unable to convert " << dst_ip << " into numerical form. "
                   "inet_pton() returned " << retcode << std::endl;
      return INVALID_DST_IP;
   }

   // Split UDP payload string entry by space, and parse individual byte entries into vector.
   std::vector<std::uint8_t> pld_bytes;
   if ( udp_pld.length() > 0 )
   {
      std::cout << "Processing payload bytes..." << std::endl;
      // First, obtain individual token bytes, by manually doing a split()
      std::vector<std::string> pld_tokens;
      std::size_t pos_start = 0;
      std::size_t nreps = 0;
      constexpr std::size_t NREPS_MAX = 50;
      std::size_t pos_end;
      while ( pos_end = udp_pld.find(" ", pos_start),
              (nreps++ < NREPS_MAX && pos_end != std::string::npos) )
      {
         std::string token = udp_pld.substr(pos_start, pos_end - pos_start);
         pld_tokens.push_back(token);
         pos_start = pos_end + 1; // 1 for space character
      }
      pld_tokens.push_back( udp_pld.substr(pos_start) );

      // Now parse each hex byte into a uint8 number
      for ( auto token : pld_tokens )
      {
         assert( token.length() > 0 );
         try {
            std::uint8_t pld_byte = std::stoi(token, nullptr, 16);
            pld_bytes.push_back(pld_byte);
         }
         catch (const std::invalid_argument& e) {
            std::cerr << "Warning: Failed to process byte: " << token
                      << " (invalid byte). Continuing..." << std::endl;
         }
         catch (const std::out_of_range& e) {
            std::cerr << "Warning: Failed to process byte: " << token
                      << " (invalid byte). Continuing..." << std::endl;
         }
      }
      std::cout << "Processing payload bytes COMPLETE!" << std::endl;
   }

   // Now form 16-bit pairs for the sum
   std::vector<std::uint16_t> pld_byte_pairs;
   for ( std::size_t i=0; (i+1) < pld_bytes.size(); i+=2 )
   {
      std::uint16_t byte_pair =
            (static_cast<std::uint16_t>(pld_bytes[i]) << 8)
          | (static_cast<std::uint16_t>(pld_bytes[i+1]) );
      pld_byte_pairs.push_back(byte_pair);
   }
   if ( pld_bytes.size() % 2 != 0 )
   {
      pld_byte_pairs.push_back(static_cast<std::uint16_t>(pld_bytes[0]));
   }

   // Compute what the correct length would be given the UDP payload
   std::uint16_t udp_len = 8 + pld_bytes.size();
   assert( udp_len >= 8 );

   // Compute one's complement sum
   std::uint16_t initial_ones_complement_sum = 0;
   std::uint32_t running_sum = 0;
   running_sum += (src_ip_num.s_addr & 0xFF00) >> 16;
   running_sum += src_ip_num.s_addr >> 16;
   running_sum += (dst_ip_num.s_addr & 0xFF00) >> 16;
   running_sum += dst_ip_num.s_addr >> 16;
   running_sum += 0x0011;
   running_sum += udp_len;
   running_sum += htons(src_port);
   running_sum += htons(dst_port);
   running_sum += udp_len;
   if ( running_sum > 0xFFFF )
   {
      initial_ones_complement_sum = static_cast<std::uint16_t>(
                                       (running_sum & 0xFFFF) + (running_sum >> 16)
                                     );
   }
   else
   {
      initial_ones_complement_sum = static_cast<std::uint16_t>(running_sum);
   }

   // Complement the result above
   std::uint16_t checksum = ~initial_ones_complement_sum;

   // Print each intermediate result and the final checksum result
   std::locale::global( std::locale("en_US.UTF-8") );
   std::cout << "Completed computations!\n\n";
   std::cout << std::format("Initial Sum: {:L} (0x{:02X})\n",
                            running_sum,
                            running_sum );
   std::cout << std::format("Initial One's Complement Sum: {:L} (0x{:02X})\n",
                            initial_ones_complement_sum,
                            initial_ones_complement_sum );
   std::cout << std::format("Checksum: {:L} (0x{:02X})\n",
                            checksum,
                            checksum );
   std::cout << std::endl;

   return 0;
}
