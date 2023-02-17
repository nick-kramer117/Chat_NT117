#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <pthread.h>

#define LENGTH 2048

// Global variables.
volatile sig_atomic_t flag = 0;
int sockfd = 0;
char name[32];

void str_overwrite_stdout()
{
	printf("%s", "[>] ");
	fflush(stdout);
}

void str_trim_lf (char* arr, int length)
{
	int i;
	for (i = 0; i < length; i++)
	{
		if (arr[i] == '\n')
		{
      			arr[i] = '\0';
     			break;
    		}
  	}
}

void catch_ctrl_c_and_exit(int sig)
{
    	flag = 1;
}

// Simple crypt function XOR.
void fc_xor(char* s, char* k, char* res)
{
	unsigned int len1 = strlen(s);
	unsigned int len2 = strlen(k);
	unsigned int i = 0;
	unsigned int j = 0;

	while (i <= len1)
	{
		if(j == len2)
		{
			j = 0;
		}

		res[i] = s[i] ^ k[j];

		j++;
		i++;
	}
}

// Simple Send messages.
void send_msg_handler()
{
  	char message[LENGTH] = {};
        char buffer[LENGTH + 32] = {};

  	while(1)
	{
        	str_overwrite_stdout();
    		fgets(message, LENGTH, stdin);
    		str_trim_lf(message, LENGTH);

		if (strcmp(message, "CMD:exit") == 0)
		{
			break;
    		}
		else if (strcmp(message, "CMD:clear") == 0)
		{
			system("clear");
			printf("[+] - Cleaned... \n");
		}
		else
		{
      			sprintf(buffer, "%s: %s\n", name, message);
      			send(sockfd, buffer, strlen(buffer), 0);
    		}

                bzero(message, LENGTH);
    		bzero(buffer, LENGTH + 32);
  	}

	catch_ctrl_c_and_exit(2);
}

// Crypt Send messages.
void send_msg_crpt_handler(char* pswd)
{
	char message[LENGTH] = {};
	char buffer[LENGTH + 32] = {};
	char crypt[LENGTH] = {};

	while(1)
	{
		str_overwrite_stdout();

		fgets(message, LENGTH, stdin);
		str_trim_lf(message, LENGTH);

		if(strcmp(message, "SRV:con") == 0)
		{
			sprintf(buffer, "%s \n", message);
			send(sockfd, buffer, strlen(buffer), 0);
		}
		else if (strcmp(message, "CMD:exit") == 0)
		{
			break;
		}
		else if (strcmp(message, "CMD:pswd") == 0)
		{
			printf("[*] - Key: %s \n", pswd);
		}
		else if (strcmp(message, "CMD:clear") == 0)
		{
			system("clear");
			printf("[+] - Cleaned... \n");
		}
		else
		{
			sprintf(buffer, "%s: %s \n", name, message);
			fc_xor(buffer, pswd, crypt);
			send(sockfd, crypt, strlen(crypt), 0);
		}

		bzero(crypt, LENGTH);
		bzero(message, LENGTH);
		bzero(buffer, LENGTH + 32);
	}

	catch_ctrl_c_and_exit(2);
}

// Simple Recive messages.
void recv_msg_handler()
{
        char message[LENGTH] = {};

	while(1)
	{
		int receive = recv(sockfd, message, LENGTH, 0);

		if(receive > 0)
		{
			printf("%s \n", message);
			str_overwrite_stdout();
		}
		else if(receive == 0)
		{
			break;
		}
		else
		{
                        // todo: error - recive!
		}
		memset(message, 0, sizeof(message));
	}
}

// Crypt Recive messages.
void recv_msg_crpt_handler(char* pswd)
{
	char message[LENGTH] = {};
	char decrypt[LENGTH] = {};

	while(1)
	{
		int receive = recv(sockfd, message, LENGTH, 0);

		if(receive > 0)
		{

			if(strncmp(message, "SRV_BOT", 7) == 0)
			{
				printf("%s", message);
				str_overwrite_stdout();
			}
			else
			{
				fc_xor(message, pswd, decryt);
				printf("%s", decrypt);
				str_overwrite_stdout();
			}

		}
		else if(receive == 0)
		{
			break;
		}
		else
		{
                        // todo:
		}
		memset(message, 0, sizeof(message));
		memset(decrypt, 0, sizeof(decrypt));
	}
}

int main(int argc, char **argv)
{
	// Standard parameters.
	char *ip = "127.0.0.1";
	int port = 10117;
	char *pswd = "117";

	if(argc == 4)
	{
		printf("[+] - Set User setting.\n");
		ip = argv[1];
		port = atoi(argv[2]);
		pswd = argv[3];
	}
	else if(argc == 3)
	{
		printf("[+] - Set User setting.\n");
		ip = argv[1];
		port = atoi(argv[2]);
	}
	else if(argc == 2)
	{
		printf("[+] - Set User setting.\n");
		ip = argv[1];
	}
	else if(argc > 4)
	{
		printf("[!] - Error ENTER KEY!\n");
		return EXIT_FAILURE;
	}
	else
	{
		printf("[+] - Set default setting.\n");
	}

	printf("Server IP address: %s \n", ip);
	printf("Server Port: %d \n", port);
	printf("Server Password: %s \n", pswd);

	signal(SIGINT, catch_ctrl_c_and_exit);

	printf("[?] - Please enter your name: ");
	fgets(name, 32, stdin);
	str_trim_lf(name, strlen(name));

	if(strlen(name) > 32 || strlen(name) < 2)
	{
		printf("[!] - Name must be less than 30 and more than 2 characters!\n");
		return EXIT_FAILURE;
	}

	struct sockaddr_in server_addr;

	// Socket settings.
	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	server_addr.sin_family = AF_INET;
	server_addr.sin_addr.s_addr = inet_addr(ip);
	server_addr.sin_port = htons(port);

	// Connect to Server.
	int err = connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr));
	if(err == -1)
	{
		printf("[!] - ERROR: connect!\n");
		return EXIT_FAILURE;
	}

	// Send name to Server.
	send(sockfd, name, 32, 0);

	printf("--=== WELCOME TO THE CHAT NT117 v0 (Client v0.1.6) ===--\n");

	// Creat threads Send||Recive with||not crypt masseges.
	if(argc == 4)
	{
		// Crypt thread Send.
		pthread_t send_msg_thread;
		if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_crpt_handler, pswd) != 0)
		{
			printf("[!] - ERROR: pthread!\n");
			return EXIT_FAILURE;
		}

		// Crypt thread Recive.
		pthread_t recv_msg_thread;
		if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_crpt_handler, pswd) != 0)
		{
			printf("[!] - ERROR: pthread!\n");
			return EXIT_FAILURE;
		}
	}
	else
	{
		// Simple thread Send.
		pthread_t send_msg_thread;
		if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0)
		{
			printf("[!] - ERROR: pthread!\n");
			return EXIT_FAILURE;
		}

		// Simple thread Recive.
		pthread_t recv_msg_thread;
		if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0)
		{
			printf("[!] - ERROR: pthread!\n");
			return EXIT_FAILURE;
		}
	}

	while(1)
	{
		if(flag)
		{
			printf("[*] - Exit...\n");
			break;
		}
	}

	close(sockfd);
	return EXIT_SUCCESS;
}
