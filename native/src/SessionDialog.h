#pragma once
#include <string>
namespace ra {class Engine;}
struct SessionOptions {std::string name{"Deidad"},profile{"default"},ip;unsigned short port{32145};bool host{},fresh{};unsigned seed{};};
bool chooseSession(ra::Engine& engine,SessionOptions& options,const std::string& capture="");
bool chooseConnection(ra::Engine& engine,std::string& ip,unsigned short& port);
