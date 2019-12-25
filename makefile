all: web_proxy

web_proxy: web_proxy.cpp
	g++ -o web_proxy web_proxy.cpp -pthread -std=gnu++11
clean:
	rm web_proxy

