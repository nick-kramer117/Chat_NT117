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
char key[128];

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

		//todo: Server bot cmd.
		if(strcmp(message, "CMD:help") == 0)
		{
			printHelp("-r");
		}
		else if (strcmp(message, "CMD:exit") == 0)
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
		else if(strcmp(message, "CMD:help") == 0)
		{
			printHelp("-r");
		}
		else if(strcmp(message, "CMD:exit") == 0)
		{
			break;
		}
		else if(strcmp(message, "CMD:pswd") == 0)
		{
			printf("[*] - Key: %s \n", pswd);
		}
		else if(strcmp(message, "CMD:clear") == 0)
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
			if(strncmp(message, "SRV_BOT", 7) == 0)
			{
				printf("\033[96m");
                                printf("%s", message);
                                printf("\033[0m");
                                str_overwrite_stdout();
			}
			else
			{
				printf("%s", message);
				// printf("%s\n", message);
				str_overwrite_stdout();
			}
		}
		else
		{
			break;
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
				printf("\033[96m");
				printf("%s", message);
				printf("\033[0m");
				str_overwrite_stdout();
			}
			else
			{
				fc_xor(message, pswd, decrypt);
				printf("%s", decrypt);
				str_overwrite_stdout();
			}

		}
		else
		{
			break;
		}
		memset(message, 0, sizeof(message));
		memset(decrypt, 0, sizeof(decrypt));
	}
}

// Output header text.
void printHeader(char* st, int v, int vw, int vu )
{
	printf("--=== WELCOME TO THE CHAT NT117 v%i (%s %i.%i.%i) ===--\n", v, st, v, vw, vu);
}

// Output text help.
int printHelp(char* key)
{
	printf("\033[92m");
	printf("[i] - Help menu. v0.0.2\n");
	printf("\033[0m");
	if(key == "-h")
	{
		printf("[*] - Help: Starting...\n");
		printf("Startup description (simple):./client_NT117 -cd < ip address > < port >\n");
		printf("Startup description (with crypt message):./client_NT117 -cs < ip address > < port > < encryption key >\n");
		printf("\033[90mStarting the help menu:./client_NT117 -h (or --help).\033[0m\n");
		printf("\033[93m");
		printf("Reference run (with crypt message):./client_NT117 -cs 127.0.0.1 10117 uArtScam1012 \n");
		printf("\033[0m");
		printf("< -h > or < --help > \033[90m........|\033[0m Call help menu.\n");
		printf("< -cd > \033[90m.....................|\033[0m Simple connection.\n");
		printf("< -cs > \033[90m.....................|\033[0m Connection with crypt message.\n");
		printf("< -cs -dd > \033[90m.................|\033[0m Connection with crypt message (Default setting 127.0.0.1:10117.\n");
		printf("< ip address > \033[90m..............|\033[0m Your ip address IPv4 (defult localhost:127.0.0.1).\n");
		printf("< port > \033[90m....................|\033[0m Your port (default port = 10117).\n");
		printf("< encryption key > \033[90m..........|\033[0m Message encryption key (defult key = 117).\n");
		printf("\033[0m");
		printf("To call help when the client is running, type \"CMD:help\" and press \"Enter\"\n");
	}
	else if(key == "-r")
	{
		// TODO: List helper for runtime.
		printf("[*] - Help: In running...\n");
		printf("=> Commands description: < object >:< key >\n");
		printf("\033[94m");
		printf("=> Objects list: CMD (Command for client app), SRV (Server bot).\n");
		printf("\033[0m");
		printf("\033[93m");
		printf("-> Descriptions keys - CMD:< key > (CMD:help and press \"Enter\").\n");
		printf("\033[0m");
		printf("CMD:help \033[90m....................|\033[0m Call helper.\n");
		printf("CMD:clear \033[90m...................|\033[0m Clear console.\n");
		printf("CMD:exit \033[90m....................|\033[0m Exit application.\n");
		printf("CMD:pswd \033[90m....................|\033[0m Show your password.\n");
		printf("\033[93m");
		printf("-> Descriptions keys - SRV:< key > (SRV:con and press \"Enter\").\n");
		printf("\033[0m");
		printf("SRV:con \033[90m.....................|\033[0m Find out the received server ID.\n");
		printf("[e] - End help.\n");
	}
}

int main(int argc, char **argv)
{
	// Local constants.
	const int Vesrsion = 0;
	const char *AppName = "Client";
	const int Release = 1;
	const int Revision = 11;

	// Standard parameters.
	char *ip = "127.0.0.1";
	int port = 10117;
	char *pswd="117";

	//Set default server key (for Server 0.1.7 < ...)
	strcpy(key, "117");

	// Check keys.
	if(strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help") == 0)
	{
		printHeader(AppName, Vesrsion, Release, Revision);
		printHelp("-h");
		return EXIT_SUCCESS;
	}
	else if(strcmp(argv[1], "-cd") == 0)
	{
		// Simple setting connection ([-cd] - Connection Default).
		if(argc == 4)
		{
			// Set < id addres > , < port > .
			printf("[+] - Set User setting.\n");
			ip = argv[2];
			port = atoi(argv[3]);
		}
		else if(argc == 3)
		{
			// Set < id addres > .
			printf("[+] - Set User setting.\n");
			ip = argv[2];
		}
		else if(argc > 4)
		{
			// Output msg "Error key".
			printf("\033[91m");
			printf("[!] - Error ENTER KEY!\n");
			return EXIT_FAILURE;
		}
		else
		{
			printf("[+] - Set default setting.\n");
		}
	}
	else if(strcmp(argv[1], "-cs") == 0)
	{
		// Conectino with crypt messages ([-cs] - Connection Secured).
		if(argc == 5)
		{
			// Set < id addres > , < port > , < pswd > .
			printf("[+] - Set User setting.\n");
			ip = argv[2];
			port = atoi(argv[3]);
			pswd = argv[4];
		}
		else if(argc == 4 && strcmp(argv[2], "-dd") == 0)
		{
			// Set < -dd > , < pswd > .
			printf("[+] - Set default setting.\n");
			pswd = argv[3];
		}
		else
		{
			// Output msg "Error secure key".
			printf("\033[91m");
			printf("[!] - Error ENTER Secure KEY!\n");
			return EXIT_FAILURE;
		}
	}
	else
	{
		printf("\033[91m");
    		printf("[!] - Pleace set key for run Server!!!\n");
    		printf("[?] - Helper key -h or --help.\n");
    		return EXIT_SUCCESS;
	}

	// Output client options.
	printf("\033[90m");
	printf("Server IP address: %s \n", ip);
	printf("Server Port: %d \n", port);
	printf("\033[0m");

	signal(SIGINT, catch_ctrl_c_and_exit);

	// Cient - Enter server key.
	printf("[?] - Please enter server key: ");
	fgets(key, 128, stdin);
	str_trim_lf(key, strlen(key));

	if(strlen(key) > 128 || strlen(key) < 1)
        {
                printf("[!] - Server key must be less than 128 and more than 1 characters!\n");
                return EXIT_FAILURE;
        }

	// Client - Enter name.
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

	// Send key server to Server
	send(sockfd, key, 128, 0);

	// Send name to Server.
	send(sockfd, name, 32, 0);

	// Output header text.
	printHeader(AppName, Vesrsion, Release, Revision);

	// Creat threads Send || Recive with || not crypt masseges.
	if(strcmp(argv[1], "-cs") == 0)
	{
		// Crypt thread Send.
		pthread_t send_msg_thread;
		if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_crpt_handler, pswd) != 0)
		{
			printf("[!] - ERROR: pthread (Send)!\n");
			return EXIT_FAILURE;
		}

		// Crypt thread Recive.
		pthread_t recv_msg_thread;
		if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_crpt_handler, pswd) != 0)
		{
			printf("[!] - ERROR: pthread (Recive)!\n");
			return EXIT_FAILURE;
		}
	}
	else
	{
		// Simple thread Send.
		pthread_t send_msg_thread;
		if(pthread_create(&send_msg_thread, NULL, (void *) send_msg_handler, NULL) != 0)
		{
			printf("[!] - ERROR: pthread (Send)!\n");
			return EXIT_FAILURE;
		}

		// Simple thread Recive.
		pthread_t recv_msg_thread;
		if(pthread_create(&recv_msg_thread, NULL, (void *) recv_msg_handler, NULL) != 0)
		{
			printf("[!] - ERROR: pthread (Recive)!\n");
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
