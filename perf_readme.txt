Hello, this is a test file.

chmod +x *.sh test_files/port.py and run ./perf.sh <n-cli> <runtime> [filename] [port]
n-cli is how many threads will be spawned to send requests to ur server, 
runtime is how long in seconds to make request for.
filename is the get file
port is the port to use. 
if filename isnt provided, it requests this file
if port isn't provided, it launches ./httpserver on an open port
sorry if this doesnt work on anything other than m1 mac thats what i used to build this

dependencies:
clang
bash
python3
probably other stuff...