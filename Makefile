complie:
	g++ main.cpp -o main -pthread
	g++ net.cpp -o net
debug:
	g++ main.cpp -o main -pthread -g
	