#include "pchheader.hpp"
#include "util.hpp"

namespace util
{
    std::string hex2bin(std::string_view s)
    {
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

    std::string bin2hex(std::string_view s)
    {
        std::ostringstream oss;

        for (unsigned char ch : s)
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)ch;

        return oss.str();
    }

    const std::string read_file(std::string_view path)
    {
        std::string buf;
        std::ifstream infile(path.data());

        //get length of file
        infile.seekg(0, std::ios::end);
        const size_t length = infile.tellg();
        infile.seekg(0, std::ios::beg);
        buf.resize(length);

        infile.read(buf.data(), length);
        return buf;
    }
}