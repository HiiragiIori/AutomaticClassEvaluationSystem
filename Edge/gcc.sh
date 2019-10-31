gcc -c -g -fPIC -DDEBUG  ptz_control.c -o ptz_control.o
gcc -c -g -fPIC -DDEBUG  client.c -o client.o
gcc -c -g -fPIC -DDEBUG  server.c -o server.o
gcc -c -g -fPIC -DDEBUG  wsaapi.c -o wsaapi.o
gcc -c -g -fPIC -DDEBUG  -DWITH_OPENSSL stdsoap2.c -o stdsoap2.o
gcc -c -g -fPIC -DDEBUG  soapC.c -o soapC.o
gcc -c -g -fPIC -DDEBUG  duration.c -o duration.o
gcc -c -g -fPIC -DDEBUG  soapClient.c -o soapClient.o
gcc -c -g -fPIC -DDEBUG  wsseapi.c -o wsseapi.o
gcc -c -g -fPIC -DDEBUG  mecevp.c -o mecevp.o
gcc -c -g -fPIC -DDEBUG  smdevp.c -o smdevp.o
gcc -c -g -fPIC -DDEBUG  threads.c -o threads.o
gcc -c -g -fPIC -DDEBUG  dom.c -o dom.o
