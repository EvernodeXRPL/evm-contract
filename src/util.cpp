#include "pchheader.hpp"
#include "util.hpp"

namespace util
{
    std::string hex2bin(const std::string &s)
    {
        std::string sOut;
        sOut.reserve(s.length() / 2);

        std::string extract;
        for (std::string::const_iterator pos = s.begin(); pos < s.end(); pos += 2)
        {
            extract.assign(pos, pos + 2);
            sOut.push_back(std::stoi(extract, nullptr, 16));
        }
        return sOut;
    }

    std::string bin2hex(const std::string &s)
    {
        std::ostringstream oss;

        for (unsigned char ch : s)
            oss << std::hex << std::setw(2) << std::setfill('0') << (int)ch;

        return oss.str();
    }

    const std::string read_file(const std::string &path)
    {
        std::string buf;
        std::ifstream infile(path);

        //get length of file
        infile.seekg(0, std::ios::end);
        const size_t length = infile.tellg();
        infile.seekg(0, std::ios::beg);
        buf.resize(length);

        infile.read(buf.data(), length);
        return buf;
    }
}