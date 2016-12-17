#include <errno.h>
#include <inttypes.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <unistd.h>

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <string.h>

#define MAX_ATTEMPTS 32

bool RecvAll(int sockfd, char* data, size_t len) {
    while (len != 0) {
        ssize_t ret = recv(sockfd, data, len, 0);
        if (ret == -1) {
            perror("recv");
            return false;
        }
        if (ret == 0) {
            fprintf(stderr, "recv: connection closed by server\n");
            return false;
        }
        data += ret;
        len -= ret;
    }
    return true;
}

bool SendAll(int sockfd, const char* data, size_t len) {
    while (len != 0) {
        ssize_t ret = send(sockfd, data, len, 0);
        if (ret == -1) {
            perror("send");
            return false;
        }
        if (ret == 0) {
            fprintf(stderr, "send: connection closed by server\n");
            return false;
        }
        data += ret;
        len -= ret;
    }
    return true;
}

int ServeServer(int sockfd) {    
	uint32_t a = 0;
	uint32_t b = 1000000000;	
	int attempt;
    for (attempt = 1; attempt <= MAX_ATTEMPTS; ++attempt) {        
		uint32_t guess = (b+a)/2;
		fprintf(stderr, "a=%d b=%d guess=%d\n", a,b,guess);
		uint32_t tosend = htonl(guess);
		if (!SendAll(sockfd, (char*)&tosend, sizeof(tosend))) {
            break;
        }
		char result;
		if (!RecvAll(sockfd, (char*)&result, sizeof(result))) {
            break;
        }		
		if (result == '=')
		{
			fprintf(stderr, "got it!\n");
			return 0;
		}
		else if (result == '>')
		{
			fprintf(stderr, ">\n");
			a = guess;
		}
		else if (result == '<')
		{
			fprintf(stderr, "<\n");
			b = guess;
		}
		else {
			fprintf(stderr, "hmmmm, rules are broken.");
			return 1;
		}
    }
    if (attempt > MAX_ATTEMPTS) {
        fprintf(stderr, "limit of attempts reached, how....\n");
    }

    return 0;
}

void PrintUsage(const char* prog) {
    fprintf(stderr, "=== number guessing client ===\n");
    fprintf(stderr, "Usage: %s UNIX_SOCKET_PATH\n\n", prog);
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        PrintUsage(argv[0]);
        return 2;
    }

    const char* socketPath = argv[1];

	int s, t, len;
    struct sockaddr_un remote;
    char str[100];

    if ((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }

    printf("Trying to connect...\n");

    remote.sun_family = AF_UNIX;
    strcpy(remote.sun_path, socketPath);
    len = strlen(remote.sun_path) + sizeof(remote.sun_family);
    if (connect(s, (struct sockaddr *)&remote, len) == -1) {
        perror("connect");
        exit(1);
    }

    printf("Connected.\n");

	ServeServer(s);
	if (close(s) == -1) {
		perror("close socket");
	}

    return 0;
}