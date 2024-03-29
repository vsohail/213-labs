/*
 * Name: Sohil Habib
 * Andrew Id:snhabib
 *
 * A simple concurrent proxy server
 * for communication between a client and a server.
 * Handles GET requests using HTTP 1.0
 * The proxy has a cache which caches blocks of
 * MAX_OBJECT_SIZE upto a maximum of MAX_CACHE_SIZE
 */
#include <stdio.h>
#include "cache.h"
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
void clienterror(int fd, char *cause, char *errnum, char *shortmsg,
    char *longmsg);
int parse(char *uri, char *host, char *path);
int open_clientfd_r(char *hostname, char *port);
/*
 * The main routine which keeps
 * opening sockets to service client requests
 * and creates threads per request to make the
 * server concurrent.
 */
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
  Signal(SIGPIPE,SIG_IGN); /* Disable SIGPIPE */
  port = atoi(argv[1]);
  cache_init(MAX_CACHE_SIZE,MAX_OBJECT_SIZE); /* Initialize Cache */
  listenfd = Open_listenfd(port);
  client_len = sizeof(struct sockaddr_in);
  while(1) {
    connfd = Malloc(sizeof(int));
    *connfd = Accept(listenfd,(SA *)&client_addr,&client_len);
    Pthread_create(&tid,NULL,my_proxy,connfd); /* Create a new thread */
  }
  return 0;
}
/*
 * Parse the url to obtain
 * host, port number and path
 * ->if uri is invalid, return
 * ->if no path is specified,
 *   append a '/'
 * ->if no port is specified
 *   default to 80(HTTP)
 * return type: int port
 */
int parse(char *uri, char *host, char *path)
{
  char *p, *q; /* dummy variables to traverse the url */
  int port;
  p = strstr(uri, "://");
  if (!p) {
    return -1;
  }

  p += 3; /* progress to end of '://' */
  q = strpbrk(p, ":/"); /* Detect a :,/ */

  strncpy(host, p, q-p); /* get the host */
  host[q-p] = '\0'; /* terminate host with NULL */

  if (*q == ':') {
    q++;
    port = atoi(q); /* set port */
  }
  else
    port = HTTP; /* else default to 80 (HTTP) */

  q = strstr(p, "/");
  if (!q) {
    path = "/";
    return port;
  }
  strncpy(path, q, strlen(q)); /* update path is specified */
  path[strlen(q)] = '\0';

  return port;

}
/*
 * Error handling function
 */
void clienterror(int fd, char *cause, char *errnum,
    char *shortmsg, char *longmsg)
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
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-type: text/html\r\n");
  rio_writen(fd, buf, strlen(buf));
  sprintf(buf, "Content-length: %d\r\n\r\n", (int)strlen(body));
  rio_writen(fd, buf, strlen(buf));
  rio_writen(fd, body, strlen(body));
}
/*
 * Main proxt threat function
 * Gets the fd, communictaes with server,
 * redirects requests to client, kills threat
 * and handles all the other errors related to
 * the proxy. Also calls cache insert and retreive
 * functions.
 */
void *my_proxy(void *fd_proxy)
{
  int clientfd, fd,port;
  char port_str[10];
  size_t n;
  int size;
  char response[MAXBUF],req[MAXBUF];
  char buf[MAXLINE],host[MAXLINE];
  char path[MAXLINE],url[MAXLINE];
  char version[MAXLINE],method[MAXLINE];
  rio_t rio,rio_c;
  void *to_cache;
  Pthread_detach(pthread_self()); /* detatch thread */
  fd = *((int *)fd_proxy);
  Free(fd_proxy);
  rio_readinitb(&rio, fd);
  rio_readlineb(&rio, buf, MAXLINE);
  /* Parse the method,url and version */
  sscanf(buf, "%s %s %s", method, url, version);
  if (strcasecmp(method, "GET")) { /* throw error for Non-GET requests */
    clienterror(fd, method, "501", "Method Not Implemented",
        "This request is not currently supported");
    Close(fd);
    return NULL;
  }
  if ((port=parse(url, host, path)) != -1) { /* If successfully parsed */
    if((to_cache=retreive(url,&size))!=NULL) {
      rio_writen(fd,to_cache,size); /* If present in cache */
    }
    else {
      /* The main request consisting of method,path,version */
      sprintf(req, "%s %s HTTP/1.0\r\n", method,path);
      while(strcmp(buf, "\r\n")) {
        rio_readlineb(&rio, buf, MAXLINE);
        if (strstr(buf, ":")
            && !strstr(buf, "User-Agent:")
            && !strstr(buf, "Accept:")
            && !strstr(buf, "Accept-Encoding:")
            && !strstr(buf, "Connection:")
            && !strstr(buf, "Proxy-Connection:")) {
          strcat(req, buf); /* Form the request leaving out the above */
        }
      }
      if (!strstr(req, "Host:")) { /* Enter host */
        sprintf(req, "%sHost: %s\r\n",req,host);
      }
      /* Update all the headers ignored above */
      sprintf(req, "%s%s",req,user_agent_hdr);
      sprintf(req, "%s%s",req,accept_hdr);
      sprintf(req, "%s%s",req,accept_encoding_hdr);
      sprintf(req, "%sConnection: close\r\n",req);
      sprintf(req, "%sProxy-Connection: close\r\n",req);
      sprintf(req, "%s\r\n",req); /* Terminating the header */
      sprintf(port_str,"%d",port);
      clientfd = open_clientfd_r(host,port_str);

      if (clientfd < 0) {
        clienterror(fd, "Clientfd", "502", "Open Client Error",
            "Error Opening Client FD");
        Close(fd);
        return NULL;
      }
      rio_readinitb(&rio_c, clientfd);
      rio_writen(clientfd, req, strlen(req));
      size=0;
      while (1) {
        if ((n = rio_readnb(&rio_c, response, MAXBUF)) <= 0) {
          break;
        }
        size+=n;
        if(size<=MAX_OBJECT_SIZE) {
          /* cache till block less than MAX_OBJECT_SIZE */
          to_cache = Realloc(to_cache,size);
          memcpy(to_cache + (size-n),response,n);
        }
        else {
          Free(to_cache);to_cache=NULL;
        }
        rio_writen(fd, response, n);
      }
      if(to_cache) {
        /* if cachable then insert in cache */
        insert(url,to_cache,size);
      }
      Close(clientfd);
    }
  }
  else {
    clienterror(fd, "Parser", "400", "Bad Request",
        "parse error");
  }
  Close(fd);
  return NULL;
}
/*
 * Threadsafe open_clientfd function (Thanksgiving Gift)
 */
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
    Close(clientfd);
    return -1;
  }
  else { /* one of the connects succeeded */
    return clientfd;
  }
}

