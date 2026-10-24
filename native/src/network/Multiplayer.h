#pragma once
#include <memory>
#include <string>
#include <vector>
namespace isla {
std::string localIPv4();
// Nonblocking TCP link between two independently simulated islands.
class PeerLink {
public:
    PeerLink();~PeerLink();PeerLink(const PeerLink&)=delete;PeerLink& operator=(const PeerLink&)=delete;
    bool host(unsigned short port);bool join(const std::string& ip,unsigned short port);
    void poll();void send(const std::string& message);std::vector<std::string> receive();
    void stop();bool hosting() const;bool connected() const;std::string status() const;
private:struct Impl;std::unique_ptr<Impl> impl_;
};
}


