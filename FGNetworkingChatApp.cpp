#include <iostream>
#include <WinSock2.h>

int main()
{
    LPWSADATA data;
    WSAStartup(MAKEWORD(2, 2), data);
}