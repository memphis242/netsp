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
   std::uint16_t src_port;
   std::uint16_t dst_port;
   std::string udp_pld_str;

   // Retrieve user input
   // Enable exceptions from std::cin on invalid inputs for integer conversion
   std::cin.exceptions(std::ios_base::failbit | std::ios_base::badbit );
   std::cout << "Let's compute a UDP checksum!" << '\n'
             << "\nProvide the following information:" << '\n'
             <<   "----------------------------------" << '\n';
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
   // Since this input is space separated, we'll read it as a line, and make
   // sure any remnants in the input buffer from previous std::cin >> reads are
   // cleared.
   std::cin.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
   std::getline(std::cin, udp_pld_str);
   std::cout << "----------------------------------" << std::endl;

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
   if ( udp_pld_str.length() > 0 )
   {
      std::cout << "Processing payload bytes..." << std::endl;
      // First, obtain individual token bytes, by manually doing a split()
      std::vector<std::string> pld_tokens;
      std::size_t pos_start = 0;
      constexpr std::size_t NREPS_MAX = 50;
      for ( std::size_t nreps = 0, pos_end = udp_pld_str.find(" ", pos_start);
            (nreps++ < NREPS_MAX && pos_end != std::string::npos);
            pos_end = udp_pld_str.find(" ", pos_start) )
      {
         std::string token = udp_pld_str.substr(pos_start, pos_end - pos_start);
         pld_tokens.push_back(token);
         pos_start = pos_end + 1; // 1 for space character
      }
      pld_tokens.push_back( udp_pld_str.substr(pos_start) );

      // Now parse each hex byte into a uint8 number
      std::cout << "\tBytes parsed: ";
      for ( auto token : pld_tokens )
      {
         assert( token.length() > 0 );
         try {
            std::uint8_t pld_byte = std::stoi(token, nullptr, 16);
            std::cout << std::format("0x{:02X} ", pld_byte);
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
      std::cout << '\n';
      std::cout << "Processing payload bytes COMPLETE!" << std::endl;
   }
   else
   {
      std::cout << "No payload bytes given." << std::endl;
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
      pld_byte_pairs.push_back(static_cast<std::uint16_t>(pld_bytes.back()));
   }

   std::cout << "\tByte pairs formed: ";
   for ( auto bpair : pld_byte_pairs )
   {
      std::cout << std::format("0x{:04X} ", bpair);
   }
   std::cout << std::endl;

   // Compute what the correct length would be given the UDP payload
   std::uint16_t udp_len = 8 + pld_bytes.size();
   std::cout << "UDP Packet Length: " << udp_len << " octets\n" << std::endl;
   assert( udp_len >= 8 );

   // Compute one's complement sum
   std::cout << "Summing 16-bit Words: " << '\n'
             << "---------------------"  << '\n'
             << std::format( "0x{:04X}", htons(src_ip_num.s_addr  & 0x0000FFFF) ) << '\n'
             << std::format( "0x{:04X}", htons(src_ip_num.s_addr >> 16) ) << '\n'
             << std::format( "0x{:04X}", htons(dst_ip_num.s_addr  & 0x0000FFFF) ) << '\n'
             << std::format( "0x{:04X}", htons(dst_ip_num.s_addr >> 16) ) << '\n'
             << "0x0011" << '\n'
             << std::format("0x{:04X}", udp_len) << '\n'
             << std::format( "0x{:04X}", src_port ) << '\n'
             << std::format( "0x{:04X}", dst_port ) << '\n'
             << std::format("0x{:04X}", udp_len) << '\n';
   for ( auto wrd : pld_byte_pairs )
   {
      std::cout << std::format("0x{:04X}", wrd) << '\n';
   }
   std::cout << "------" << '\n';
   std::uint16_t initial_ones_complement_sum = 0;
   std::uint32_t running_sum = 0;
   running_sum += htons(src_ip_num.s_addr & 0x0000FFFF);
   running_sum += htons(src_ip_num.s_addr >> 16);
   running_sum += htons(dst_ip_num.s_addr & 0x0000FFFF);
   running_sum += htons(dst_ip_num.s_addr >> 16);
   running_sum += 0x0011;
   running_sum += udp_len;
   running_sum += src_port;
   running_sum += dst_port;
   running_sum += udp_len;
   for ( auto wrd : pld_byte_pairs )
   {
      running_sum += wrd;
   }
   std::cout << std::format("0x{:04X}", running_sum) << std::endl;
   // Carry wrap-around
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
   std::cout << "\nCompleted computations!\n\n";
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
