# EVM Contract
Hot Pocket C++ contract that can execute solidity contracts using [evmone](https://github.com/ethereum/evmone) library.

# Setting up development environment

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