/*
 * extendes sockets
 */
#include <string.h>
#include <unistd.h>
#include <vdr/plugin.h>

#if VDRMANAGER_USE_SSL
#include <openssl/err.h>
#endif

#include "serversock.h"
#include "helpers.h"
#include "compressor.h"

static int clientno = 0;

/*
 * cVdrmonServerSocket
 */
cVdrmanagerServerSocket::cVdrmanagerServerSocket() : cVdrmanagerSocket() {
  port = -1;
}

cVdrmanagerServerSocket::~cVdrmanagerServerSocket() {
}

bool cVdrmanagerServerSocket::Create(int port, const char * password, bool forceCheckSvrp, int compressionMode,
                                     const char * certFile, const char * keyFile) {

  this->port = port;
  this->password = password;
  this->forceCheckSvdrp = forceCheckSvrp;
  this->compressionMode = compressionMode;
  this->certFile = certFile;
  this->keyFile = keyFile;

	// create socket
	sock = socket(AF_INET6, SOCK_STREAM, 0);
	if (sock < 0) {
		LOG_ERROR;
		return false;
	}

  isyslog("[vdrmanager] created %sSSL server socket on port %d", certFile ? "" : "non ", port);

	// allow it to always reuse the same port:
	int ReUseAddr = 1;
	setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &ReUseAddr, sizeof(ReUseAddr));

	// bind to address
	struct sockaddr_in6 addr;
	memset(&addr, 0, sizeof(addr));
	addr.sin6_family = AF_INET6;
	addr.sin6_port = htons(port);
	addr.sin6_addr = in6addr_any;
	if (bind(sock, (struct sockaddr *) &addr, sizeof(addr)) < 0) {
		LOG_ERROR;
		Close();
		return false;
	}

	// make it non-blocking:
	if (!MakeDontBlock()) {
		Close();
		return false;
	}

	// listen to the socket:
	if (listen(sock, 100) < 0) {
		LOG_ERROR;
		Close();
		return false;
	}

#if VDRMANAGER_USE_SSL
	if (certFile) {
    isyslog("[vdrmanager] initialize SSL");

    OpenSSL_add_all_algorithms();
    OpenSSL_add_all_ciphers();
    OpenSSL_add_all_digests();
    OpenSSL_add_ssl_algorithms();

    SSL_load_error_strings();
    SSL_library_init();
	}
#endif
	return true;
}

cVdrmanagerClientSocket * cVdrmanagerServerSocket::Accept() {
  cVdrmanagerClientSocket * newsocket = NULL;

  isyslog("[vdrmanager] new client on port %d", port);

  // accept the connection
  struct sockaddr_in6 clientname;
  uint size = sizeof(clientname);
  int newsock = accept(sock, (struct sockaddr *) &clientname, &size);
  if (newsock > 0) {
    // create client socket
    newsocket = new cVdrmanagerClientSocket(password, compressionMode, certFile, keyFile);
    if (!newsocket->Attach(newsock)) {
      delete newsocket;
      return NULL;
    }

    if (!IsPasswordSet() || forceCheckSvdrp == true) {
/*
      bool accepted = SVDRPhosts.Acceptable(clientname.sin6_addr);
      if (!accepted) {
        newsocket->Write(string("NACC Access denied.\n"));
        newsocket->Flush();
        delete newsocket;
        newsocket = NULL;
      }
      dsyslog("[vdrmanager] connect from %s, port %hd - %s",
          inet_ntoa(clientname.sin_addr), ntohs(clientname.sin_port),
          accepted ? "accepted" : "DENIED");
*/
    }
  } else if (errno != EINTR && errno != EAGAIN)
    LOG_ERROR;

  return newsocket;
}

int cVdrmanagerServerSocket::GetPort() {
  return port;
}
