#include <stdio.h>

#include <stdlib.h>

#include <string.h>

#include <stdint.h>

#include <string>

#include <sys/types.h>

#include <sys/socket.h>

#include <arpa/inet.h>

#include <netdb.h>

#include <netinet/in.h>

#include <unistd.h>

#include <vector>

#include <set>

#include <mutex>

#include <thread>

 

using namespace std;

char message[1024];

 

 

void usage() {

	printf("syntax : web_proxy <tcp port>\n");

	printf("sample : web_proxy 8080\n");

}

 

void error_handling(char *message) {

	fputs(message, stderr);

	fputc('\n', stderr);

	exit(1);

}

string FindHost(char *message) {

	string data = string((const char*)message, strlen(message));

	string ret = "";

	size_t pos = data.find("Host: ");

	if (pos == string::npos) {

		return ret;

	}

	string host = data.substr(pos + 6);

	pos = host.find("\r\n");

	host = host.substr(0, pos);

	return host;

}

 

void relay(int recvsock, int sendsock) {

	while (true) {

		int len = recv(recvsock, message, 1023, 0);

		if(len<=0){

			perror("recv failed");

			break;

		}

		message[len] = '\0';

		printf("server packet :%s", message);

 

		if (send(sendsock, message, strlen(message), 0) <= 0) {

			perror("send failed");

			break;

		}

	}

	close(recvsock);

	close(sendsock);

}

int listen_sock;

 

int main(int argc, char* argv[]) {

	if (argc != 2) {

		usage();

		return -1;

	}

	int str_len, i;

	int status;

	struct sockaddr_in serv_adr;

	struct sockaddr_in clnt_adr;

	socklen_t clnt_adr_sz;

	clnt_adr_sz = sizeof(clnt_adr);

	listen_sock = socket(PF_INET, SOCK_STREAM, 0);

 

	if (listen_sock == -1)

		error_handling("socket() error");

 

	memset(&serv_adr, 0, sizeof(serv_adr));

	serv_adr.sin_family = AF_INET;

	serv_adr.sin_addr.s_addr = htonl(INADDR_ANY);

	serv_adr.sin_port = htons(atoi(argv[1]));

 

	if (bind(listen_sock, (struct sockaddr*)&serv_adr, sizeof(serv_adr)) == -1)

		error_handling("bind() error");

 

	listen(listen_sock, 10);

	puts("client wait....");

 

	vector<thread> server;

	while (1) {

		int accp = accept(listen_sock, (struct sockaddr *)&clnt_adr, &clnt_adr_sz);

		if (accp < 0) {

			error_handling("accept fail");

		}

		ssize_t len = recv(accp, message, 1023, 0);

		if (len <= 0) {

			error_handling("recv failed");

		}

		message[len] = '\0';

		printf("client packet : %s\n", message);

		string host = FindHost(message);

		if (host == "") {

			close(listen_sock);

			continue;

		}

		int server_sock = socket(PF_INET, SOCK_STREAM, 0);

		if (server_sock == -1) {

			close(server_sock);

			close(listen_sock);

			perror("socket failed");

			continue;

		}

		struct hostent* hostinfo = gethostbyname(host.c_str());

		struct sockaddr_in serv_addr2;

		serv_addr2.sin_family = AF_INET;

		serv_addr2.sin_addr.s_addr = *(unsigned int*)hostinfo->h_addr;

		serv_addr2.sin_port = htons(80);

		memset(serv_addr2.sin_zero, 0, sizeof(serv_addr2.sin_zero));

 

		if (connect(server_sock, reinterpret_cast<struct sockaddr*>(&serv_addr2), sizeof(struct sockaddr)) < 0) {

			perror("connect failed");

			close(server_sock);

			close(listen_sock);

			continue;

		}

		ssize_t sent = send(server_sock, message, strlen(message), 0);

		if (sent == 0) {

			perror("send failed");

			close(server_sock);

			close(listen_sock);

			continue;

		}

		server.push_back(thread(relay, listen_sock, server_sock));

		server.push_back(thread(relay, server_sock, listen_sock));

 

	}

 

	close(listen_sock);

	return 0;

}
