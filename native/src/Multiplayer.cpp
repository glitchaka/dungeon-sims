#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Multiplayer.h"
#include <utility>
namespace isla {
std::string localIPv4(){WSADATA data{};if(WSAStartup(MAKEWORD(2,2),&data))return "127.0.0.1";char hostname[256]{};gethostname(hostname,sizeof(hostname));addrinfo hints{};hints.ai_family=AF_INET;addrinfo* list=nullptr;std::string selected="127.0.0.1";int best=0;
    if(getaddrinfo(hostname,nullptr,&hints,&list)==0){for(auto* at=list;at;at=at->ai_next){char text[INET_ADDRSTRLEN]{};auto* address=reinterpret_cast<sockaddr_in*>(at->ai_addr);inet_ntop(AF_INET,&address->sin_addr,text,sizeof(text));std::string ip=text;int score=ip.rfind("192.168.",0)==0?4:ip.rfind("10.",0)==0?3:ip.rfind("172.",0)==0?2:ip.rfind("127.",0)!=0?1:0;if(score>best){best=score;selected=ip;}}freeaddrinfo(list);}WSACleanup();return selected;
}
struct PeerLink::Impl {
    SOCKET listener{INVALID_SOCKET},socket{INVALID_SOCKET};bool connecting{},ready{},started{};
    std::string input,output,note{"SIN CONEXION"};std::vector<std::string> inbox;
    void disconnect(){if(socket!=INVALID_SOCKET)closesocket(socket);socket=INVALID_SOCKET;connecting=ready=false;input.clear();output.clear();note="DESCONECTADO; VISITAS CONSERVADAS";}
    void close(){disconnect();if(listener!=INVALID_SOCKET)closesocket(listener);listener=INVALID_SOCKET;}
};
PeerLink::PeerLink():impl_(std::make_unique<Impl>()){WSADATA data{};impl_->started=WSAStartup(MAKEWORD(2,2),&data)==0;}
PeerLink::~PeerLink(){impl_->close();if(impl_->started)WSACleanup();}
bool PeerLink::host(unsigned short port){auto& p=*impl_;p.close();if(!p.started)return false;p.listener=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(p.listener==INVALID_SOCKET)return false;sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons(port);if(bind(p.listener,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==SOCKET_ERROR||listen(p.listener,1)==SOCKET_ERROR){p.close();p.note="NO SE PUDO ABRIR EL PUERTO";return false;}u_long on=1;ioctlsocket(p.listener,FIONBIO,&on);p.note="ABIERTO "+localIPv4()+":"+std::to_string(port);return true;}
bool PeerLink::join(const std::string& ip,unsigned short port){auto& p=*impl_;p.close();if(!p.started)return false;sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_port=htons(port);if(inet_pton(AF_INET,ip.c_str(),&addr.sin_addr)!=1){p.note="DIRECCION IPv4 INVALIDA";return false;}p.socket=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);u_long on=1;ioctlsocket(p.socket,FIONBIO,&on);int result=connect(p.socket,reinterpret_cast<sockaddr*>(&addr),sizeof(addr));if(result==SOCKET_ERROR&&WSAGetLastError()!=WSAEWOULDBLOCK){p.disconnect();return false;}p.connecting=result!=0;p.ready=result==0;p.note="CONECTANDO A "+ip;return true;}
void PeerLink::poll(){auto& p=*impl_;
    if(p.socket==INVALID_SOCKET&&p.listener!=INVALID_SOCKET){p.socket=accept(p.listener,nullptr,nullptr);if(p.socket!=INVALID_SOCKET){u_long on=1;ioctlsocket(p.socket,FIONBIO,&on);p.ready=true;p.note="ISLA CONECTADA";}}
    if(p.socket==INVALID_SOCKET)return;
    if(p.connecting){fd_set writes,errors;FD_ZERO(&writes);FD_ZERO(&errors);FD_SET(p.socket,&writes);FD_SET(p.socket,&errors);timeval timeout{};if(select(0,nullptr,&writes,&errors,&timeout)>0){int error=0;int size=sizeof(error);getsockopt(p.socket,SOL_SOCKET,SO_ERROR,reinterpret_cast<char*>(&error),&size);if(error){p.disconnect();return;}p.connecting=false;p.ready=true;p.note="ISLA CONECTADA";}}
    if(!p.ready)return;
    char buffer[4096];for(int n=0;n<16;++n){int count=recv(p.socket,buffer,sizeof(buffer),0);if(count==0){p.disconnect();return;}if(count<0){if(WSAGetLastError()!=WSAEWOULDBLOCK)p.disconnect();break;}p.input.append(buffer,count);if(p.input.size()>65536){p.disconnect();return;}std::size_t end;while((end=p.input.find('\n'))!=std::string::npos){if(p.inbox.size()>=128){p.disconnect();return;}p.inbox.push_back(p.input.substr(0,end));p.input.erase(0,end+1);}}
    if(!p.output.empty()){int count=::send(p.socket,p.output.data(),static_cast<int>(p.output.size()),0);if(count>0)p.output.erase(0,count);else if(count<0&&WSAGetLastError()!=WSAEWOULDBLOCK)p.disconnect();}
}
void PeerLink::send(const std::string& message){auto& p=*impl_;if(p.ready&&message.size()<16384&&p.output.size()+message.size()<65536)p.output+=message+'\n';}
std::vector<std::string> PeerLink::receive(){auto result=std::move(impl_->inbox);impl_->inbox.clear();return result;}
void PeerLink::stop(){impl_->close();impl_->note="ISLA CERRADA";}
bool PeerLink::hosting() const{return impl_->listener!=INVALID_SOCKET;}
bool PeerLink::connected() const{return impl_->ready;}
std::string PeerLink::status() const{return impl_->note;}
}

