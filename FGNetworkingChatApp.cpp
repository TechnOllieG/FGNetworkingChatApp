#include <iostream>
#include <WinSock2.h>
#include <ws2tcpip.h>
#include <vector>
#include <mutex>

struct USER
{
    SOCKET out_socket;
    char name[15];
};

std::vector<USER> users;
std::mutex user_mutex;

DWORD server_handle_user(LPVOID socket)
{
    SOCKET sock = reinterpret_cast<SOCKET>(socket);

    USER user;
    int nameSize = recv(sock, user.name, 15, 0);
    if (nameSize > 14)
        nameSize = 14;

    user.name[nameSize] = 0;
    user.out_socket = sock;

    user_mutex.lock();
    users.push_back(user);
    user_mutex.unlock();

    printf("%s connected\n", user.name);
    
    while (true)
    {
        char buffer[1024];
        int recieve_size = recv(sock, buffer, 1024, 0);

        if (recieve_size < 0)
        {
            printf("%s disconnected\n", user.name);
            return 0;
        }

        char message[1050];
        sprintf_s(message, "<%s>: %.*s\n", user.name, recieve_size, buffer);

        printf(message);

        user_mutex.lock();
        for (USER temp_user : users)
        {
            if (temp_user.out_socket == sock)
                continue;

            send(temp_user.out_socket, message, strlen(message), 0);
        }
        user_mutex.unlock();
    }
}

void server()
{
    printf("Input server port: ");
    unsigned short port;
    scanf_s("%hu", &port);

    SOCKET listen_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (listen_socket == INVALID_SOCKET)
    {
        printf("listen socket failed to be created\n");
        return;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    int bind_result = bind(listen_socket, (sockaddr*)&address, sizeof(address));
    if (bind_result)
    {
        printf("Bind failed, reason: %d\n", WSAGetLastError());
        return;
    }

    listen(listen_socket, 5);

    while (true)
    {
        SOCKET sock = accept(listen_socket, nullptr, nullptr);
        CreateThread(nullptr, 0, server_handle_user, (void*) sock, 0, 0);
    }
}

char name[15];

DWORD client_handle_message_history(LPVOID socket)
{
    SOCKET sock = reinterpret_cast<SOCKET>(socket);
    char buffer[1024];

    while (true)
    {
        int length = recv(sock, buffer, 1024, 0);
        if (length < 0)
            return 0;

        printf("\r                                                                                                    ");
        printf("\r%.*s", length, buffer);
        char current_console_text[1024] = {0};
        // Somehow read from the console so the user can see what is already in cin
        //std::cin.get(current_console_text, 1024);
        printf("<%s>: %s", name, current_console_text);
    }
}

void client()
{
    printf("Input ip to connect to (without port) [0 = 127.0.0.1]: ");
    char input_ip[20];
    scanf_s("%s", &input_ip, 20);
    if (input_ip[0] == '0')
        sprintf_s(input_ip, "127.0.0.1");

    printf("Input port to connect to: ");
    unsigned short port;
    scanf_s("%hu", &port);

    SOCKET connection_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (connection_socket == INVALID_SOCKET)
    {
        printf("Connection socket failed to be created\n");
        return;
    }

    sockaddr_in address;
    address.sin_family = AF_INET;
    address.sin_port = htons(port);
    
    inet_pton(AF_INET, input_ip, &(address.sin_addr));

    int connect_result = connect(connection_socket, (sockaddr*) &address, sizeof(address));
    if (connect_result)
    {
        printf("Connection failed, reason: %d\n", WSAGetLastError());
        return;
    }

    printf("Input username: ");
    scanf_s("%s", name, 15);

    CreateThread(nullptr, 0, client_handle_message_history, (void*)connection_socket, 0, 0);

    send(connection_socket, name, 15, 0);

    while (true)
    {
        printf("<%s>: ", name);
        char buffer[1024];
        gets_s(buffer, 1024);

        int message_len = strlen(buffer);
        if (buffer[0] == '\0' || message_len == 0)
            gets_s(buffer, 1024);

        send(connection_socket, buffer, 1024, 0);
    }
}

int main()
{
    WSADATA data;
    WSAStartup(MAKEWORD(2, 2), &data);

    printf("Would you like to initialize a server [s] or client [c]?: ");
    while (true)
    {
        char response[5];
        scanf_s("%s", response, 5);

        if (response[0] == 'c')
        {
            // Client
            client();
            return 0;
        }
        else if (response[0] == 's')
        {
            // Server
            server();
            return 0;
        }
        printf("That input is not valid, please try again, input either [s] or [c]: ");
    }
}