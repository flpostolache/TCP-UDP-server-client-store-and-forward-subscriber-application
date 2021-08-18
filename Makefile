build: server subscriber

clean: server server.o subscriber subscriber.o
	rm -f server
	rm -f server.o
	rm -f subscriber
	rm -f subscriber.o

server: server.o
	g++ -o server server.o

server.o: server.cpp
	g++ -c server.cpp

subscriber: subscriber.o
	g++ -o subscriber subscriber.o

subscriber.o: subscriber.cpp
	g++ -c subscriber.cpp
