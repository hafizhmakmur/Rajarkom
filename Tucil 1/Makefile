HEADERS = dcomm.h

all: default
default: transmitter receiver

transmitter.o: transmitter.c $(HEADERS)
	gcc -c transmitter.c -o transmitter.o

receiver.o: receiver.c $(HEADERS)
	gcc -c receiver.c -o receiver.o

transmitter: transmitter.o
	gcc transmitter.o -o transmitter

receiver: receiver.o
	gcc receiver.o -o receiver

clean:
	-rm -f receiver.o transmitter.o
	-rm -f transmitter receiver
