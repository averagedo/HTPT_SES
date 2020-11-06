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
#include <condition_variable>
#include <utility>
#include <errno.h>

#define FILE "config.txt"
#define Len_buff 5000
#define sl_mess 10
#define LOGFILE "_log.txt"
#define LOGFOLDER "./log/"

struct V_P
{
  std::vector<int> VT;
  int procID;
};

// Cau truc V_M dung de gui mess
struct V_M
{
  std::vector<V_P> v_p;
  std::vector<int> VTm;
};

struct Proc_Info
{
  int procID; // Mot so nguyen duong dung cho viec xac dinh vi tri tren vector time
  std::string IP;
  int port;
};

std::vector<int> Vtime;
std::vector<V_P> Vproc;

std::vector<std::string> vec_str;
std::vector<std::pair<V_M, std::string>> array_buffer;

std::mutex mutex_VTP;
std::mutex mutex_VTP2;
std::mutex mutex_buffrecv;
std::mutex mutex_log;
std::mutex mutex_arrbuff;

std::condition_variable cv;

void printVM(V_M vm)
{
  std::cout << "vp: \n";
  for (int i = 0; i < vm.v_p.size(); i++)
  {
    std::cout << vm.v_p[i].procID << "__";
    for (int j = 0; j < vm.v_p[i].VT.size() - 1; j++)
    {
      std::cout << vm.v_p[i].VT[j] << ",";
    }
    std::cout << vm.v_p[i].VT.back() << "\n";
  }
  std::cout << "vt: ";
  for (int i = 0; i < vm.VTm.size() - 1; i++)
  {
    std::cout << vm.VTm[i] << ",";
  }
  std::cout << vm.VTm.back() << "\n";
}

std::string str_VTP()
{
  std::string f;
  //mutex_VTP.lock();
  f = ">====== VT ======<\n";
  for (int i = 0; i < Vtime.size() - 1; i++)
  {
    f += std::to_string(Vtime[i]) + ",";
  }
  f += std::to_string(Vtime.back()) + "\n";
  f += ">====== VP ======<\n";
  for (int i = 0; i < Vproc.size(); i++)
  {
    f += std::to_string(Vproc[i].procID) + "__";
    for (int j = 0; j < Vproc[i].VT.size() - 1; j++)
    {
      f += std::to_string(Vproc[i].VT[j]) + ",";
    }
    f += std::to_string(Vproc[i].VT.back()) + "\n";
  }
  //mutex_VTP.unlock();
  return f;
}

std::string str_VM(V_M vm)
{
  std::string f;
  f = ">====== VM ======<\n";
  f += "> VT:\t";
  for (int i = 0; i < vm.VTm.size() - 1; i++)
  {
    f += std::to_string(vm.VTm[i]) + ",";
  }
  f += std::to_string(vm.VTm.back()) + "\n";
  f += "> VP:\t";
  if (vm.v_p.size() != 0)
  {
    for (int i = 0; i < vm.v_p.size(); i++)
    {
      f += std::to_string(vm.v_p[i].procID) + "__";
      for (int j = 0; j < vm.v_p[i].VT.size() - 1; j++)
      {
        f += std::to_string(vm.v_p[i].VT[j]) + ",";
      }
      f += std::to_string(vm.v_p[i].VT.back()) + "\n\t";
    }
    f.erase(f.end() - 1);
  }
  else
    f += "\n";

  return f;
}

void log_recv(int procID, V_M vm, std::string m, std::string mess)
{
  std::fstream f;
  f.open(LOGFOLDER + std::to_string(procID) + LOGFILE, std::ios::out | std::ios::app);
  if (!f.is_open())
  {
    std::cout << "open logfile error\n";
    exit(0);
  }
  f << ">>>>>>> " << m << " <<<<<<<\n";
  f << mess << "\n";
  f << str_VM(vm);
  f << str_VTP();
  f << "\n\n";
  f.close();
}

int vec_compare_greater(std::vector<int> a, std::vector<int> b)
{

  if (a.size() != b.size())
  {
    std::cout << "VEC_COMPARE ERROR\n";
    return 3;
  }

  int kq;
  int be = 0, lon = 0;
  for (int i = 0; i < a.size(); i++)
  {
    if (a[i] < b[i])
      be++;
    else if (a[i] > b[i])
      lon++;
  }
  if (be == 0 && lon == 0)
    return 1;
  else if (be > 0 && lon == 0)
    return 0;
  else if (be == 0 && lon > 0)
    return 2;
  else
    return 3;
}

void mergeVT(std::vector<int> vt)
{
  for (int i = 0; i < Vtime.size(); i++)
  {
    if (vt[i] > Vtime[i])
      Vtime[i] = vt[i];
  }
}

void addVP(int procID_SendTo)
{
  V_P vp;
  //mutex_VTP2.lock();
  vp.procID = procID_SendTo;
  vp.VT = Vtime;
  if (Vproc.size() == 0)
  {
    std::cout << "Vo duoc cai dau bang\n";
    Vproc.push_back(vp);
    return;
  }
  else
  {
    for (int i = 0; i < Vproc.size(); i++)
    {
      if (Vproc[i].procID == procID_SendTo)
      {
        Vproc[i].VT = Vtime;
        return;
      }
    }

    for (int i = 0; i < Vproc.size() - 1; i++)
    {
      if (Vproc[i].procID < procID_SendTo && Vproc[i + 1].procID > procID_SendTo)
      {
        Vproc.insert(Vproc.begin() + i + 1, vp);
        return;
      }
    }
    Vproc.push_back(vp);
  }
  //mutex_VTP2.unlock();
  return;
}

std::string ConvertData(V_M vm)
{
  //  ======  Cau truc  =========
  //  slptvp:???\n
  //  procID__0,0,...,0\n
  //  ...
  //  vtm:0,0,...,0\r\n
  //  ===== END ======

  std::string str;
  int sl_vec = vm.v_p.size();
  str += "slptvp:" + std::to_string(sl_vec) + "\n";
  for (int i = 0; i < sl_vec; i++)
  {
    str += std::to_string(vm.v_p[i].procID) + "__";
    for (int j = 0; j < vm.v_p[i].VT.size(); j++)
    {
      str += std::to_string(vm.v_p[i].VT[j]) + ",";
    }
    str.pop_back();
    str += "\n";
  }
  str += "vtm:";
  for (int i = 0; i < vm.VTm.size(); i++)
  {
    str += std::to_string(vm.VTm[i]) + ",";
  }
  str.pop_back();
  str += "\r\n";
  return str;
}

V_M DeConvertData(std::string str)
{
  V_M vm;
  for (int i = 0; i < 7; i++)
    str.erase(str.begin());

  std::string temp;
  while (str[0] != '\n')
  {
    temp.push_back(str[0]);
    str.erase(str.begin());
  }
  str.erase(str.begin());

  int len_vp = std::stoi(temp);

  for (int i = 0; i < len_vp; i++)
  {
    V_P vp;
    // procID
    temp.clear();
    while (str[0] != '_')
    {
      temp.push_back(str[0]);
      str.erase(str.begin());
    }
    str.erase(str.begin());
    str.erase(str.begin());

    vp.procID = std::stoi(temp);

    while (str[0] != '\n')
    {
      std::string ss;
      while (str[0] != ',' && str[0] != '\n')
      {
        ss.push_back(str[0]);
        str.erase(str.begin());
      }

      vp.VT.push_back(std::stoi(ss));
      if (str[0] == '\n')
        break;
      str.erase(str.begin());
    }
    str.erase(str.begin());
    vm.v_p.push_back(vp);
  }

  for (int i = 0; i < 4; i++)
    str.erase(str.begin());

  while (str[0] != '\r')
  {
    std::string ss;
    while (str[0] != ',' && str[0] != '\r')
    {
      ss.push_back(str[0]);
      str.erase(str.begin());
    }
    vm.VTm.push_back(std::stoi(ss));

    if (str[0] == '\r')
      break;
    str.erase(str.begin());
  }
  return vm;
}

bool check_data_recv() // Kiem tra du lieu trong
{
  if (vec_str.size() > 0)
    return true;
  return false;
}

void add_vec_p(V_P a)
{
  if (Vproc[0].procID < a.procID)
  {
    Vproc.insert(Vproc.begin(), a);
    return;
  }
  for (int i = 0; i < Vproc.size() - 1; i++)
  {
    if (Vproc[i].procID <= a.procID && Vproc[i + 1].procID > a.procID)
    {
      Vproc.insert(Vproc.begin() + i + 1, a);
      return;
    }
  }
  Vproc.push_back(a);
}

void mergeVP(std::vector<V_P> a)
{
  if (Vproc.size() == 0)
  {
    Vproc = a;
  }

  std::vector<V_P> copy = a;
  for (int i = 0; i < a.size(); i++)
  {
    int dk1 = 0;
    for (int j = 0; j < Vproc.size(); j++)
    {
      if (Vproc[j].procID == a[i].procID)
      {
        dk1 = 1;
        for (int k = 0; k < Vproc[j].VT.size(); k++)
        {
          if (Vproc[j].VT[k] < a[i].VT[k])
            Vproc[j].VT[k] = a[i].VT[k];
        }
      }
    }

    if (dk1 == 0)
      add_vec_p(a[i]);
  }
}

void send_mess(std::string ip, int port, std::string mess)
{
  int sock;
  sockaddr_in serv_addr;
  char buffer[Len_buff] = {0};
  if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
  {
    std::cout << "Socket creation error\n";
    //exit(1);
  }

  serv_addr.sin_family = AF_INET;
  serv_addr.sin_port = htons(port);

  // Convert IPv4 and IPv6 addresses from text to binary form
  if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0)
  {
    std::cout << "Invalid address\n";
    //exit(1);
  }

  if (connect(sock, (sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
  {
    std::cout << "Connection Failed\n";
    //exit(1);
  }

  if (send(sock, mess.c_str(), mess.length(), 0) < 0)
  {
    std::cout << "ERROR send\n";
    //exit(1);
  }

  close(sock);
}

void thr_send(Proc_Info procinfo, Proc_Info procinfo2)
{
  int i = 0;
  while (i < sl_mess)
  {

    std::string mess;
    V_M vm;

    srand(time(NULL));
    int res = rand() % (8 - 1 + 1) + 1;
    sleep(res);

    mutex_VTP.lock();
    vm.v_p = Vproc;

    addVP(procinfo2.procID);

    Vtime[procinfo.procID - 1] += 1;
    vm.VTm = Vtime;
    mutex_VTP.unlock();
    mess = ConvertData(vm);

    mess = mess + "proc " + std::to_string(procinfo.procID) + " - message " + std::to_string(i);
    i++;

    std::cout << "proc " << procinfo.procID << " gui mess " << i << " to proc " << procinfo2.procID << "\n";
    send_mess(procinfo2.IP, procinfo2.port, mess);
  }
}

void recv_mess(int procID, int port)
{
  int server_fd, new_socket;
  sockaddr_in address;
  int addrlen = sizeof(address);
  char buffer[Len_buff] = {0};

  // Creating socket file descriptor
  if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
  {
    std::cout << "Socket creation error\n";
    //exit(1);
  }

  address.sin_family = AF_INET;
  address.sin_addr.s_addr = INADDR_ANY;
  address.sin_port = htons(port);

  if (bind(server_fd, (sockaddr *)&address, sizeof(address)) < 0)
  {
    std::cout << "Bind failed\n";
    //exit(1);
  }

  if (listen(server_fd, 3) < 0)
  {
    std::cout << "Listen ERROR\n";
  }

  int i=0;
  while (1)
  {
    std::cout << "accepting\n\n";
    if ((new_socket = accept(server_fd, (sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
    {
      std::cout << "ERROR accept\n";
    }

    int recv_len;
    memset(buffer, 0, Len_buff);
    if ((recv_len = recv(new_socket, buffer, Len_buff, 0)) < 0)
    {
      std::cout << "ERROR recv\n";
      //exit(1);
    }

    std::string str = buffer;
    //std::cout << str << std::endl<< std::endl;
    std::unique_lock<std::mutex> mlock(mutex_buffrecv);
    mutex_arrbuff.lock();
    vec_str.push_back(str);
    mutex_arrbuff.unlock();
    cv.notify_one();

    i++;
    std::cout<<"i ne: "<<i<<"\n";
    close(new_socket);
  }
}

void S_E_S(Proc_Info procinfo, std::vector<Proc_Info> vec_proc)
{
  int sl_proc = vec_proc.size();

  std::thread recv_thr(recv_mess, procinfo.procID, procinfo.port);
  recv_thr.detach();
  sleep(4);

  // GUI
  std::thread *thrs = new std::thread[sl_proc];
  for (int i = 0; i < sl_proc; i++)
  {
    thrs[i] = std::thread(thr_send, procinfo, vec_proc[i]);
    thrs[i].detach();
  }

  while (1)
  {
    std::vector<std::string> v_str;

    // Nhan mess tu thread recv
    std::unique_lock<std::mutex> mlock(mutex_buffrecv);
    cv.wait(mlock, [&] { return check_data_recv(); });
    mutex_arrbuff.lock();
    while (vec_str.size() > 0)
    {
      v_str.push_back(vec_str.back());
      vec_str.pop_back();
    }
    mutex_arrbuff.unlock();

    // phan giai du lieu va thuc thi ses
    for (int i = 0; i < v_str.size(); i++)
    {
      // Tach chuoi
      std::string mes;
      std::string str_vm;

      int tam = v_str[v_str.size() - 1].find('\r');
      str_vm = v_str[v_str.size() - 1].substr(0, tam + 1);
      mes = v_str[v_str.size() - 1].substr(tam + 2);

      V_M vm = DeConvertData(str_vm);
      v_str.pop_back();
      i--;

      //printVM(vm);

      int save = -1;
      for (int j = 0; j < vm.v_p.size(); j++)
      {
        if (vm.v_p[j].procID == procinfo.procID)
          save = j;
      }

      // Truong hop vm empty
      int dkloop = 0;
      if (save == -1)
      {
        dkloop = 1;
        mutex_log.lock();
        log_recv(procinfo.procID, vm, "DELIVERY", mes);
        mutex_log.unlock();
        mutex_VTP.lock();
        mergeVT(vm.VTm);
        Vtime[procinfo.procID - 1] += 1;

        mergeVP(vm.v_p);
        mutex_VTP.unlock();
        std::cout << "delivery mess(1)\n";
      }
      else // Truong hop vm da co vector vp trung voi procID
      {
        mutex_VTP.lock();
        int ketquatrave = vec_compare_greater(Vtime, vm.v_p[save].VT);
        mutex_VTP.unlock();
        if (ketquatrave == 0 || ketquatrave == 3) // vector time cua proc hien tai be hon vector cua message dc gui toi
        {
          // Luu buffer
          std::pair<V_M, std::string> pa;
          pa.first = vm;
          pa.second = mes;
          array_buffer.push_back(pa);
          mutex_log.lock();
          log_recv(procinfo.procID, vm, "BUFFER", mes);
          mutex_log.unlock();
          std::cout << "buffered\n";
        }
        else if (ketquatrave == 1 || ketquatrave == 2) // vector time cua proc lon hon hoac bang
        {
          dkloop = 1;
          mutex_log.lock();
          log_recv(procinfo.procID, vm, "DELIVERY", mes);
          mutex_log.unlock();
          mutex_VTP.lock();
          mergeVT(vm.VTm);
          Vtime[procinfo.procID - 1] += 1;

          mergeVP(vm.v_p);
          mutex_VTP.unlock();

          std::cout << "Delivery message(2)\n";
        }
      }

      if (dkloop == 1)
      {
        for (int k = 0; k < array_buffer.size(); k++)
        {
          int indexl;
          for (int l = 0; l < array_buffer[k].first.v_p.size(); l++)
          {
            if (array_buffer[k].first.v_p[l].procID == procinfo.procID)
            {
              indexl = l;
              break;
            }
          }
          mutex_VTP.lock();
          int kqua = vec_compare_greater(Vtime, array_buffer[k].first.v_p[indexl].VT);
          mutex_VTP.unlock();
          if (kqua == 1 || kqua == 2)
          {
            mutex_log.lock();
            log_recv(procinfo.procID, array_buffer[k].first, "DELIVERY (B)", array_buffer[k].second);
            mutex_log.unlock();
            mutex_VTP.lock();
            mergeVT(array_buffer[k].first.VTm);
            Vtime[procinfo.procID - 1] += 1;

            mergeVP(array_buffer[k].first.v_p);
            mutex_VTP.unlock();

            array_buffer.erase(array_buffer.begin() + k);
            std::cout << "Delivery message(3)\n";

            k = -1;
          }
        }
      }
    }
  }
  return;
}

int main(int argc, char *argv[])
{
  if (argc != 2)
  {
    std::cout << "./program <procID>";
    exit(1);
  }

  // Doc file config
  std::ifstream f;
  f.open(FILE);
  if (!f.is_open())
  {
    std::cout << "Open file error\n";
  }

  std::vector<Proc_Info> vec_proc;
  while (!f.eof())
  {
    std::string term1, term2;

    Proc_Info info;
    f >> term1;
    info.procID = std::stoi(term1);
    f >> info.IP;
    f >> term2;
    info.port = std::stoi(term2);
    vec_proc.push_back(info);
  }
  f.close();

  for (int i = 0; i < vec_proc.size(); i++)
    Vtime.push_back(0);

  int cur_procID = std::stoi(argv[1]);
  Proc_Info cur_proc;
  for (int i = 0; i < vec_proc.size(); i++)
  {
    if (vec_proc[i].procID == cur_procID)
    {
      cur_proc = vec_proc[i];
      vec_proc.erase(vec_proc.begin() + i);
      break;
    }
  }

  S_E_S(cur_proc, vec_proc);

  return 0;
}