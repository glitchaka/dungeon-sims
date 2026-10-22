#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <winsock2.h>
#include <ws2tcpip.h>
#include "Multiplayer.h"
#include <utility>
namespace isla {
struct PeerLink::Impl {
    SOCKET listener{INVALID_SOCKET},socket{INVALID_SOCKET};bool connecting{},ready{},started{};
    std::string input,output,note{"SIN CONEXION"};std::vector<std::string> inbox;
    void disconnect(){if(socket!=INVALID_SOCKET)closesocket(socket);socket=INVALID_SOCKET;connecting=ready=false;input.clear();output.clear();note="DESCONECTADO; VISITAS CONSERVADAS";}
    void close(){disconnect();if(listener!=INVALID_SOCKET)closesocket(listener);listener=INVALID_SOCKET;}
};
PeerLink::PeerLink():impl_(std::make_unique<Impl>()){WSADATA data{};impl_->started=WSAStartup(MAKEWORD(2,2),&data)==0;}
PeerLink::~PeerLink(){impl_->close();if(impl_->started)WSACleanup();}
bool PeerLink::host(unsigned short port){auto& p=*impl_;p.close();if(!p.started)return false;p.listener=::socket(AF_INET,SOCK_STREAM,IPPROTO_TCP);if(p.listener==INVALID_SOCKET)return false;sockaddr_in addr{};addr.sin_family=AF_INET;addr.sin_addr.s_addr=htonl(INADDR_ANY);addr.sin_port=htons(port);if(bind(p.listener,reinterpret_cast<sockaddr*>(&addr),sizeof(addr))==SOCKET_ERROR||listen(p.listener,1)==SOCKET_ERROR){p.close();p.note="NO SE PUDO ABRIR EL PUERTO";return false;}u_long on=1;ioctlsocket(p.listener,FIONBIO,&on);p.note="ESPERANDO ISLA EN PUERTO "+std::to_string(port);return true;}
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
bool PeerLink::connected() const{return impl_->ready;}
std::string PeerLink::status() const{return impl_->note;}
}
