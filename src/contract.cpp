#include "pchheader.hpp"
#include "evm/evm.hpp"
#include "util.hpp"

int main()
{
    const std::string code_buf = util::hex2bin("4360005543600052596000f3");
    const std::string input_buf = "Hello World!";

    return evm::execute(code_buf, input_buf);

    return 0;
}