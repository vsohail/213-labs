#include <stdio.h>
#include "csapp.h"
/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define HTTP 80
/* You won't lose style points for including these long lines in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *accept_hdr = "Accept: text/html,application/xhtml+xml,application/xml;q=0.9,*/*;q=0.8\r\n";
static const char *accept_encoding_hdr = "Accept-Encoding: gzip, deflate\r\n";
void *my_proxy(void *fd);
void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg);
int parse(char *uri, char *host, char *path);
int open_clientfd_r(char *hostname, char *port);
int Open_clientfd_r(char *hostname, char* port);

int main(int argc,char **argv)
{
  int listenfd,port,*connfd;
  socklen_t client_len;
  struct sockaddr_in client_addr;
  pthread_t tid;

  if(argc != 2) {
    fprintf(stderr,"usage: %s <port>\n",argv[0]);
    exit(1);
  }
  Signal(SIGPIPE,SIG_IGN);
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
int parse(char *uri, char *host, char *path)
{
  char *p, *q;
  int port;
  p = strstr(uri, "://");
  if (!p) {
    return -1;
  }

  p += 3;
  q = strpbrk(p, " :/");

  strncpy(host, p, q-p);
  host[q-p] = '\0';

  if (*q == ':') {
    q++;
    port = atoi(q);
  }
  else
  port = HTTP;

  q = strstr(p, "/");
  if (!q) {
    path = "/";
    return port;
  }
  strncpy(path, q, strlen(q));
  path[strlen(q)] = '\0';

  return port;

}

void clienterror(int fd, char *cause, char *errnum, char *shortmsg, char *longmsg)
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
void *my_proxy(void *fd_proxy)
{
  int clientfd, fd,port;
  char port_str[10];
  size_t n;
  char response[MAXBUF], req[MAXBUF];
  char buf[MAXLINE],host[MAXLINE],path[MAXLINE],url[MAXLINE], version[MAXLINE], method[MAXLINE];
  rio_t rio;
  Pthread_detach(pthread_self()); 
  fd = *((int *)fd_proxy);
  Free(fd_proxy);
  Rio_readinitb(&rio, fd);
  Rio_readlineb(&rio, buf, MAXLINE);
  sscanf(buf, "%s %s %s", method, url, version);
  if ((port=parse(url, host, path)) != 0) {
    sprintf(req, "%s %s HTTP/1.0\r\n", method,path);
    while(strcmp(buf, "\r\n")) {
      Rio_readlineb(&rio, buf, MAXLINE);
      if (strstr(buf, ":") && !strstr(buf, "User-Agent:") && !strstr(buf, "Accept:")
          && !strstr(buf, "Accept-Encoding:") && !strstr(buf, "Connection:") && !strstr(buf, "Proxy-Connection:")) {
        strcat(req, buf);
      }
    }
    if (!strstr(req, "Host:")) {
      sprintf(req, "%sHost: %s\r\n",req,host);
    }
    sprintf(req, "%s%s",req,user_agent_hdr);
    sprintf(req, "%s%s",req,accept_hdr);
    sprintf(req, "%s%s",req,accept_encoding_hdr);
    sprintf(req, "%sConnection: close\r\n",req);
    sprintf(req, "%sProxy-Connection: close\r\n",req);
    sprintf(req, "%s\r\n",req);
    printf("%s\n",req);
    sprintf(port_str,"%d",port);
    clientfd = open_clientfd_r(host,port_str);
    Rio_readinitb(&rio, clientfd);
    Rio_writen(clientfd, req, strlen(req));
    while (1) {
      if ((n = Rio_readnb(&rio, response, MAXBUF)) <= 0) {
        break;
      }
      if(rio_writen(fd, response, n)==-1)
        return NULL;
    }
    Close(clientfd);
  }
  Close(fd);
  return NULL;
}
int Open_clientfd_r(char *hostname, char* port) 
{
  int rc;

  if ((rc = open_clientfd_r(hostname, port)) < 0) {
    if (rc == -1)
      unix_error("Open_clientfd_r Unix error");
    else
      dns_error("Open_clientfd_r DNS error");
  }
  return rc;
}
int open_clientfd_r(char *hostname, char *port) {
  int clientfd;
  struct addrinfo *addlist, *p;
  int rv;
  /* Create the socket descriptor */
  if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
    return -1;
  }

  /* Get a list of addrinfo structs */
  if ((rv = getaddrinfo(hostname, port, NULL, &addlist)) != 0) {
    return -1;
  }
  /* Walk the list, using each addrinfo to try to connect */
  for (p = addlist; p; p = p->ai_next) {
    if ((p->ai_family == AF_INET)) {
      if (connect(clientfd, p->ai_addr, p->ai_addrlen) == 0) {
        break; /* success */
      }
    }
  }

  /* Clean up */
  freeaddrinfo(addlist);
  if (!p) { /* all connects failed */
    close(clientfd);
    return -1;
  }
  else { /* one of the connects succeeded */
    return clientfd;
  }
}

