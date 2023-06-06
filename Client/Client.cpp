#include <iostream>
#include <winsock2.h>
#include <stdint.h>
#include <string>
#include <cstring> 
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

using namespace std;

int check()
{
    int n;
    while (true) {
        cin >> n;
        if ((cin.fail() || cin.peek() != '\n') || (n <= 0))
        {
            cin.clear();
            cin.ignore(1000000, '\n');
            cout << "Error, try again: ";
        }
        else return n;
    }
}

int main()
{
    setlocale(LC_ALL, 0);

    WSAData wsaData;
    WORD DLLVersion = MAKEWORD(2, 1);
    if (WSAStartup(DLLVersion, &wsaData) != 0)
    {
        cout << "Error";
        return 1;
    }

    SOCKADDR_IN addr;
    int size_of_addr = sizeof(addr);
    addr.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr.sin_port = htons(1111);
    addr.sin_family = AF_INET;

    SOCKET Connection = socket(AF_INET, SOCK_STREAM, NULL);
    if (connect(Connection, (SOCKADDR*)&addr, sizeof(addr)) != 0)
    {
        cout << "Failed connection\n";
        return 1;
    }

    int k;
    char text[256]; 
    cout << "Enter message: ";
    cin.getline(text, sizeof(text)); 

    cout << "Enter key: ";
    k = check();
    if (send(Connection, text, sizeof(text), NULL) == SOCKET_ERROR)
    {
        cout << "Failed to write text: " << "Failed to send message to server" << endl;
        return 1;
    }
    if (send(Connection, (char*)&k, sizeof(k), NULL) == SOCKET_ERROR)
    {
        cout << "Failed to write text: " << "Failed to send key to server" << endl;
        return 1;
    }

    closesocket(Connection);

    return 0;
}
