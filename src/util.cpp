#include "pchheader.hpp"
#include "util.hpp"

namespace util
{
    const std::string hex2bin(std::string_view hex)
    {
        std::string_view s = (hex.size() > 2 && hex[0] == '0' && (hex[1] == 'x' || hex[1] == 'X')) ? hex.substr(2) : hex;

        std::string sOut;
        sOut.reserve(s.length() / 2);

        std::string extract;
        for (auto pos = s.begin(); pos < s.end(); pos += 2)
        {
            extract.assign(pos, pos + 2);
            sOut.push_back(std::stoi(extract, nullptr, 16));
        }
        return sOut;
    }

    const std::string bin2hex(std::string_view s)
    {
        std::ostringstream oss;

        for (unsigned char ch : s)
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)ch;

        return oss.str();
    }

    bool file_exists(std::string_view path)
    {
        std::ifstream infile(path.data());
        return infile.good();
    }
}