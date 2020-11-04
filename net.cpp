#include <iostream>
#include <string>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <error.h>
#include <mutex>
#include <queue>
#include <vector>
#include <thread>
#include <fstream>
#include <ctime>
#include <errno.h>
#include<vector>
#define Len_buff 1024

struct vp
{
  std::vector<int> in;
  std::string str="trung";
};

void send_mess(std::string ip, int port, std::string &a)
{
  int sock;
  sockaddr_in serv_addr;
  char buffer[Len_buff] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    std::cout << "Socket creation error\n";
    exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
  {
    std::cout << "Invalid address\n";
    exit(1);
  }

  if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cout << "Connection Failed\n";
    fprintf(stderr,"%m\n");
    exit(1);
  }
  int len;
  if ((len=send(sock, a.c_str(), a.length(), 0)) < 0)
  {
    std::cout << "ERROR send\n";
    exit(1);
  }
  close(sock);
}

int main()
{
  std::string str,str1,str2;
  str="slptvp:0\nvtm:0,1,0\r\n";
  send_mess("127.0.0.1",12001,str);
  str2="slptvp:1\n1__0,2,2\nvtm:0,4,3\r\n";
  send_mess("127.0.0.1",12001,str2);
  str1="slptvp:1\n1__0,1,0\nvtm:0,2,2\r\n";
  send_mess("127.0.0.1",12001,str1);
  
  
  
  return 0;
}