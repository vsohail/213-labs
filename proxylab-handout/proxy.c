#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
void pipe_handler(int sig);
void *my_proxy(void *fd);

int main(int argc,char **argv)
{
  user_agent_hdr=0;
  accept_hdr=0;
  accept_encoding_hdr=0;
  int listenfd,port;
  int *connfd;
  socklen_t client_len;
  struct sockaddr_in client_addr;
  pthread_t tid;

  if(argc != 2) {
    fprintf(stderr,"usage: %s <port>\n",argv[0]);
    exit(1);
  }
  Signal(SIGPIPE,pipe_handler);
  port = atoi(argv[1]);
  listenfd = Open_listenfd(port);
  client_len = sizeof(struct sockaddr_in);
  while(1) {
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd,(SA *)&client_addr,&client_len);
    Pthread_create(&tid,NULL,my_proxy,connfd);
  }
  return 0;
}
void pipe_handler(int sig)
{
  return;
}
void clienterror(int fd,char *cause,char *errnum,char *shortmsg.char *longmsg)
{
  char buf[MAXLINE], body[MAXBUF];

  /* Build the HTTP response body */
  sprintf(body, "<html><title>Proxy Error</title>");
  sprintf(body, "%s<body bgcolor=""ffffff"">\r\n", body);
  sprintf(body, "%s%s: %s\r\n", body, errnum, shortmsg);
  sprintf(body, "%s<p>%s: %s\r\n", body, longmsg, cause);
  sprintf(body, "%s<hr><em>The Proxy server</em>\r\n", body);

  /* Print the HTTP response */
  sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  Rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  Rio_writen(fd, buf, strlen(buf));
  Rio_writen(fd, body, strlen(body));
}
void *my_proxy(void *fd)
{
  return NULL;
}
