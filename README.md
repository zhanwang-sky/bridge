# bridge

## Supported platforms
Server:
- Linux

Client:
- Linux
- macOS

## Building
### Linux
```
sudo apt install build-essential libboost-all-dev libgoogle-glog-dev net-tools
git clone https://github.com/zhanwang-sky/bridge.git
cd bridge
make
```
### macOS
```
brew install boost glog
git clone https://github.com/zhanwang-sky/bridge.git
```
Open the project file in Xcode.

## Usage
Server:
`sudo ./bridge -s 0.0.0.0 <port> <client_id>`

Client:
`sudo ./bridge <server_addr> <port> <client_id>`

Follow the tips to configure your network.

## TODO
Configure ip addresses in c code.
