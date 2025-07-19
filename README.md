# embedded-crypto-monitor
Crypto market monitor in C++, explicitly made for embedded systems with low resources

> [!NOTE]
> The Javascript (node.js) version with a PostgreSQL database can be found [here](https://github.com/NontasBak/crypto-monitor).
> 
> The current repository is for the C++ version.

Install the necessary dependencies (on debian/ubuntu) with:
```bash
sudo apt install nlohmann-json3-dev libcpp-httplib-dev libwebsockets-dev
```

Compile the project with
```bash
make
```

Finally, run it with
```bash
make run
```
