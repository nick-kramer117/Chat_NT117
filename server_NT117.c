#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <signal.h>
#include <string.h>

#define MAX_CLIENTS 100
#define BUFFER_SZ 2048

static _Atomic unsigned int cli_count = 0;
static int uid = 10;
char server_key[128];
char server_bot_name[8] = "SRV_BOT";

// Client structure.
typedef struct
{
	struct sockaddr_in address;
	int sockfd;
	int uid;
	char name[32];
	char key[128];
} client_t;

client_t *clients[MAX_CLIENTS];

pthread_mutex_t clients_mutex = PTHREAD_MUTEX_INITIALIZER;

void str_overwrite_stdout()
{
	printf("\r%s", "[>] - ");
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

void print_client_addr(struct sockaddr_in addr)
{
	printf("%d.%d.%d.%d",
        addr.sin_addr.s_addr & 0xff,
        (addr.sin_addr.s_addr & 0xff00) >> 8,
        (addr.sin_addr.s_addr & 0xff0000) >> 16,
        (addr.sin_addr.s_addr & 0xff000000) >> 24);
}

// Add clients to queue.
void queue_add(client_t *cl)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
	{
		if(!clients[i])
		{
			clients[i] = cl;
			break;
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Remove clients to queue.
void queue_remove(int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i < MAX_CLIENTS; ++i)
	{
		if(clients[i])
		{
			if(clients[i]->uid == uid)
			{
				clients[i] = NULL;
				break;
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Send message to all clients except sender.
void send_message(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i)
	{
		if(clients[i])
		{
			if(clients[i]->uid != uid)
			{
				if(write(clients[i]->sockfd, s, strlen(s)) < 0)
				{
					perror("ERROR: write to descriptor failed");
					break;
				}
			}
		}
	}

	pthread_mutex_unlock(&clients_mutex);
}

// Send message to client info sender.
void send_message_u(char *s, int uid)
{
	pthread_mutex_lock(&clients_mutex);

	for(int i=0; i<MAX_CLIENTS; ++i)
        {
                if(clients[i])
                {
                        if(clients[i]->uid == uid)
                        {
                                if(write(clients[i]->sockfd, s, strlen(s)) < 0)
                                {
                                        perror("ERROR: write to descriptor failed");
                                        break;
                                }
                        }
                }
        }

	pthread_mutex_unlock(&clients_mutex);
}

// Handle all communication with the client.
void *handle_client(void *arg)
{
	char buff_out[BUFFER_SZ];
	char name[32];
	char key[128];

	int leave_flag = 0;

	cli_count++;
	client_t *cli = (client_t *)arg;

	// Check server key
	if(recv(cli->sockfd, key, 128, 0) <= 0|| strlen(key) < 1 || strlen(key) >= 128-1)
	{
		printf("ERROR: Client haven't server key (or not enter server key)\n");
		leave_flag = 1;
	}
	else
	{
		if(strcmp(key, server_key) == 0)
		{
			strcpy(cli->key, key);
			printf("INFO: Client enter key server: %s\n", key);

			// Check Name.
        		if(recv(cli->sockfd, name, 32, 0) <= 0 || strlen(name) <  2 || strlen(name) >= 32-1)
			{
				printf("ERROR: Client didn't enter the name\n");
               			leave_flag = 1;
        		}
			else if(strcmp(name, server_bot_name) == 0)
			{
				printf("ERROR: Client enter name as bot %s\n", server_bot_name);
				leave_flag = 1;
			}
        		else
			{
				strcpy(cli->name, name);
               			sprintf(buff_out, "USER: %s has joined.\n", cli->name);
               			printf("%s", buff_out);

                		sprintf(buff_out, "%s: - Hi %s. Welcome to Hellrest server. \n", server_bot_name, cli->name);
               			send_message_u(buff_out, cli->uid);
			}
		}
		else
		{
			printf("ERROR: Client enter fake key: %s\n", key);
			leave_flag = 1;
		}
	}

	bzero(buff_out, BUFFER_SZ);

	while(1)
	{
		if (leave_flag)
		{
			break;
		}

		int receive = recv(cli->sockfd, buff_out, BUFFER_SZ, 0);
		if (receive > 0)
		{
			if(strncmp(buff_out, "SRV:con", 7) == 0 && strlen(buff_out) > 0)
			{
				sprintf(buff_out, "SRV_BOT: ID - %u | Username: %s \n", cli->uid ,cli->name);
				send_message_u(buff_out, cli->uid);
				printf("USER: %s call bot.\n", cli->name);
			}
			else if(strlen(buff_out) > 0)
			{
				send_message(buff_out, cli->uid);
				str_trim_lf(buff_out, strlen(buff_out));
				printf("USER: %s -> Send msg.\n", cli->name);
			}
			else
			{
				//todo:
			}
		}
		else if (receive == 0 || strcmp(buff_out, "CMD:exit") == 0)
		{
			// Output: left user.
			sprintf(buff_out, "USER: %s has left\n", cli->name);
			printf("%s", buff_out);
			leave_flag = 1;
		}
		else
		{
			printf("[!] - ERROR: -1\n");
			leave_flag = 1;
		}

		bzero(buff_out, BUFFER_SZ);
	}

	// Delete client from queue and yield thread.
	close(cli->sockfd);
	queue_remove(cli->uid);
  	free(cli);
  	cli_count--;
  	pthread_detach(pthread_self());

	return NULL;
}

int main(int argc, char **argv)
{
	// Standard parameters.
	char *ip = "127.0.0.1";
	int port = 10117;
	strcpy(server_key, "117");

	if(argc == 4)
	{
		printf("[+] - Set Administrator setting.\n");
		ip = argv[1];
		port = atoi(argv[2]);
		strcpy(server_key, argv[3]);
	}
	else if(argc == 3)
	{
		printf("[+] - Set Administrator setting.\n");
		ip = argv[1];
		port = atoi(argv[2]);
	}
	else if(argc == 2)
	{
		printf("[+] - Set Administrator setting.\n");
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

	printf("\033[90m");
	printf("Server IP address: %s \n", ip);
	printf("Server Port: %d \n", port);
	printf("Server Key: %s \n", server_key);
	printf("\033[0m");

	int option = 1;
	int listenfd = 0, connfd = 0;
  	struct sockaddr_in serv_addr;
  	struct sockaddr_in cli_addr;
  	pthread_t tid;

	// Socket settings.
  	listenfd = socket(AF_INET, SOCK_STREAM, 0);
  	serv_addr.sin_family = AF_INET;
  	serv_addr.sin_addr.s_addr = inet_addr(ip);
  	serv_addr.sin_port = htons(port);

	// Ignore pipe signals.
	signal(SIGPIPE, SIG_IGN);

	if(setsockopt(listenfd, SOL_SOCKET,(SO_REUSEPORT | SO_REUSEADDR),(char*)&option,sizeof(option)) < 0)
	{
		perror("[!] - ERROR: setsockopt failed!");
    		return EXIT_FAILURE;
	}

	// Bind.
  	if(bind(listenfd, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0)
	{
    		perror("[!] - ERROR: Socket binding failed!");
    		return EXIT_FAILURE;
  	}

	// Listen.
  	if (listen(listenfd, 10) < 0)
	{
    		perror("[!] - ERROR: Socket listening failed!");
    		return EXIT_FAILURE;
	}

	printf("--=== WELCOME TO THE CHAT NT117 v0 (Server v0.1.9) ===--\n");

	while(1)
	{
		socklen_t clilen = sizeof(cli_addr);
		connfd = accept(listenfd, (struct sockaddr*)&cli_addr, &clilen);

		// Check if max clients is reached.
		if((cli_count + 1) == MAX_CLIENTS)
		{
			printf("[?] - Max clients reached. Rejected: ");
			print_client_addr(cli_addr);
			printf(":%d\n", cli_addr.sin_port);
			close(connfd);
			continue;
		}

		// Client settings.
		client_t *cli = (client_t *)malloc(sizeof(client_t));
		cli->address = cli_addr;
		cli->sockfd = connfd;
		cli->uid = uid++;

		// Add client to the queue and fork thread.
		queue_add(cli);
		pthread_create(&tid, NULL, &handle_client, (void*)cli);

		// Reduce CPU usage.
		sleep(1);
	}

	return EXIT_SUCCESS;
}
