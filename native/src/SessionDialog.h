#pragma once
#include <string>
struct SessionOptions {std::string name{"Deidad"},profile{"default"},ip;unsigned short port{32145};bool host{},fresh{};unsigned seed{};};
bool chooseSession(SessionOptions& options);

bool chooseConnection(std::string& ip,unsigned short& port);

