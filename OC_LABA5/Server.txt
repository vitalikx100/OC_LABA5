#include <iostream>
#include <winsock2.h>
#include <stdint.h>
#include <string>
#include <thread>
#include <vector>
#include <mutex>
#include <condition_variable>
#include <cstring> 
#pragma comment(lib, "ws2_32.lib")
#pragma warning(disable: 4996)

using namespace std;

string decrypt(const char cipher[], int key) { 
    string plain = "";
    int length = strlen(cipher); 
    for (int i = 0; i < length; i++) {
        if (isalpha(cipher[i])) {
            char c = tolower(cipher[i]);
            c = (((c - 'a') - key + 26) % 26) + 'a';
            if (isupper(cipher[i])) {
                plain += toupper(c);
            }
            else {
                plain += c;
            }
        }
        else {
            plain += cipher[i];
        }
    }
    return plain;
}

void clientThread(SOCKET clientSocket, HANDLE semaphore, mutex& mtx, condition_variable& cv) {
    bool connected = true;

    while (connected) {
        DWORD dwWaitResult = WaitForSingleObject(semaphore, INFINITE);
        if (dwWaitResult == WAIT_OBJECT_0 || dwWaitResult == WAIT_ABANDONED)
        {
            cout << "Connection successful. Client " << clientSocket << " connected." << endl;

            int k;
            char text[256]; 
            if (recv(clientSocket, text, sizeof(text), NULL) == SOCKET_ERROR) 
            {
                connected = false;
                closesocket(clientSocket);
                ReleaseSemaphore(semaphore, 1, nullptr);
                break;
            }
            if (recv(clientSocket, (char*)&k, sizeof(k), NULL) == SOCKET_ERROR)
            {
                cout << "Failed to read key: " << GetLastError() << endl;
                connected = false;
                closesocket(clientSocket);
                ReleaseSemaphore(semaphore, 1, nullptr);
                break;
            }
            string message = decrypt(text, k);

            unique_lock<mutex> lock(mtx);
            cout << "Decrypted message from client " << clientSocket << ": " << message << endl;
            lock.unlock();

            ReleaseSemaphore(semaphore, 1, nullptr);

            cv.notify_one(); 
            break;
        }
        else if (dwWaitResult == WAIT_TIMEOUT)
        {
            cout << "Maximum number of clients reached. Waiting for available slot..." << endl;
            connected = false;
            closesocket(clientSocket);
            unique_lock<mutex> lock(mtx);
            cv.wait(lock); // Ожидание сигнала об освобождении слота
            continue;
        }
        else
        {
            cout << "Error in waiting for semaphore" << endl;
            connected = false;
            closesocket(clientSocket);
            break;
        }
    }

    cout << "Client " << clientSocket << " disconnected." << endl << endl;
}

int main()
{
    setlocale(LC_ALL, 0);

    LPCWSTR SERVER_NAME = L"myserver";
    HANDLE hServer = CreateSemaphore(nullptr, 3, 3, SERVER_NAME);
    if (hServer == nullptr)
    {
        std::cout << "Error to create semaphore" << std::endl;
        return 1;
    }

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

    SOCKET sListen = socket(AF_INET, SOCK_STREAM, NULL);
    bind(sListen, (SOCKADDR*)&addr, sizeof(addr));
    listen(sListen, SOMAXCONN);

    vector<thread> clientThreads;
    mutex mtx;
    condition_variable cv;

    while (true)
    {
        SOCKET newConnection = accept(sListen, (SOCKADDR*)&addr, &size_of_addr);
        if (newConnection == 0)
        {
            cout << "Error\n";
        }
        else
        {
            clientThreads.emplace_back(clientThread, newConnection, hServer, ref(mtx), ref(cv));
        }
    }

    for (auto& thread : clientThreads)
    {
        thread.join();
    }

    closesocket(sListen);
    CloseHandle(hServer);

    return 0;
}
