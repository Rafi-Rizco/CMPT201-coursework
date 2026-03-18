/*
Questions to answer at top of client.c:
(You should not need to change the code in client.c)
1. What is the address of the server it is trying to connect to (IP address and
port number).

  127.0.0.1,  port: 8000

2. Is it UDP or TCP? How do you know?
  it is TCP, since we are using connect and UDP is connectionless while TCP uses
connections. we are also using the SOCK_STREAM for socket(), which is for a
connection mode which would be TCP.

3. The client is going to send some data to the server. Where does it get this
data from? How can you tell in the code? it gets the data from the standard
input stream, std_in

4. How does the client program end? How can you tell that in the code?

it ends when the client presses enter with no other inputs so there is only 1
byte read. it also ends when there is an error while reading so that num_read is
negative, it also ends if num of bytes read is 0 (end of file).

  i could see from the while loop that keeps going until the read from input
stream is less than 1
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUF_SIZE 64
#define ADDR "127.0.0.1"

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int main() {
  struct sockaddr_in addr;
  int sfd;
  ssize_t num_read;
  char buf[BUF_SIZE];

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
    handle_error("inet_pton");
  }

  int res = connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (res == -1) {
    handle_error("connect");
  }

  while ((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) > 1) {
    if (write(sfd, buf, num_read) != num_read) {
      handle_error("write");
    }
    printf("Just sent %zd bytes.\n", num_read);
  }

  if (num_read == -1) {
    handle_error("read");
  }

  close(sfd);
  exit(EXIT_SUCCESS);
}




#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 64
#define PORT 8000
#define LISTEN_BACKLOG 32

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

// Shared counters for: total # messages, and counter of clients (used for
// assigning client IDs)
int total_message_count = 0;
int client_id_counter = 1;

// Mutexs to protect above global state.
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;

struct client_info {
  int cfd;
  int client_id;
};

void *handle_client(void *arg) {
  struct client_info *client = (struct client_info *)arg;

  printf("file descriptor: %d\n", client->cfd);
  printf("client id: %d\n", client->client_id);
  fflush(stdout);
  // TODO: print the message received from client
  // TODO: increase total_message_count per message

  int num_read = 0;

  char buf[BUF_SIZE];
  while ((num_read = read(client->cfd, buf, BUF_SIZE)) > 1) {

    pthread_mutex_lock(&count_mutex);
    total_message_count++;
    printf("Msg #   %d; Client ID %d: ", total_message_count,
           client->client_id);
    fflush(stdout);
    write(STDOUT_FILENO, buf, num_read);
    pthread_mutex_unlock(&count_mutex);
  }

  close(client->cfd);
  printf("client %d is done\n", client->client_id);
  fflush(stdout);

  if (client != NULL)
    free(client);

  return NULL;
}

int main() {
  struct sockaddr_in addr;
  int sfd;

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  addr.sin_addr.s_addr = htonl(INADDR_ANY);

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) {
    handle_error("bind");
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) {
    handle_error("listen");
  }

  for (;;) {
    // TODO: create a new thread when a new connection is encountered
    // TODO: call handle_client() when launching a new thread, and provide
    // client_info
    pthread_t thread;
    socklen_t client_addrlen = sizeof(struct sockaddr_in);
    int file = accept(sfd, (struct sockaddr *)&addr, &client_addrlen);
    if (file == -1) {
      handle_error("accept");
    }

    struct client_info *info = malloc(sizeof(struct client_info));
    info->cfd = file;

    pthread_mutex_lock(&client_id_mutex);
    info->client_id = client_id_counter;
    client_id_counter++;
    pthread_mutex_unlock(&client_id_mutex);

    if (pthread_create(&thread, NULL, handle_client, info) != 0) {
      handle_error("pthread_create");
    }

    pthread_detach(thread);
  }

  if (close(sfd) == -1) {
    handle_error("close");
  }

  return 0;
}
