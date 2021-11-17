# EVM Contract
Hot Pocket C++ contract that can execute compiled solidity contracts using [evmone](https://github.com/ethereum/evmone) library.

## Setting up development environment

### 1. Install EVMC library
```
git clone https://github.com/ethereum/evmc.git
cd evmc
cmake .
sudo make install
```

### 2. Install evmone library
- Download evmone [release package](https://github.com/ethereum/evmone/releases/download/v0.8.2/evmone-0.8.2-linux-x86_64.tar.gz) and extract the contents to `/usr`.
- Run `ldconfig`

### 3. Build
```
cmake .
make
```
Contents in the 'build' directory must be copied to initial contract state.

## Input/output message interface
For implicity, this contract uses a character-based i/o message format. Clients can use the following messages to deploy and call compiled Solidity contracts.

### Input messages
  - Deploy: `d<hex addr><hex bytecode>` - Deploys the bytecode at given address (account is created with the max balance).
  - Call: `c<hex addr><hex input>` - Calls the contract located at given address using given input.

### Output messages
  - Deploy result: `d<deployed hex address>`
  - Call result: `c<output hex>`
  - Error: `e<reason>`