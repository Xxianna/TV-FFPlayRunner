
all:
	g++ ffplayrunner.cpp -o ffplayrunner.exe -static -m32 -liphlpapi -lws2_32