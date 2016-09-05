# vmtrace
JVMTI agent for tracing VM events:
 - Class loading
 - Garbage collection
 - JIT compilation

## Compilation
### Linux
gcc -O2 -fPIC -shared -Wl,-soname,libvmtrace.so -olibvmtrace.so vmtrace.c
### Windows 
cl /O2 /LD vmtrace.c

## Running
java -agentpath:/path/to/libvmtrace.so Main
