#include <iostream>
#include <fstream>
#include <windows.h>
#include "employee.h"
#include "../query.h"
#include "../response.h"


struct ThreadArgs {
    HANDLE hNamedPipe;
    HANDLE fileMutex;
    std::string fileName;
    Employee* employees;
    int empolyeesCnt;
    int num;
};

DWORD WINAPI threadFunc(LPVOID sId) {
	ThreadArgs args = *static_cast<ThreadArgs*>(sId);

	DWORD dwBytesRead;
	
	Query query;

	while (true) {
		bool read = ReadFile(
			args.hNamedPipe,
			&query,
			sizeof(Query),
			&dwBytesRead,
			(LPOVERLAPPED)NULL
		);

		if (!read)
			break;

		if (query.command == 'r') {
			Response outEmpl = getEmployee(args.employees, args.empolyeesCnt, query.number);
			DWORD dwBytesWrite;

			WriteFile(
				args.hNamedPipe,
				(char*)&outEmpl,
				sizeof(Response),
				&dwBytesWrite,
				(LPOVERLAPPED)NULL
			);

		}
		else if (query.command == 'm') {

			Response outEmpl = getEmployee(args.employees, args.empolyeesCnt, query.number);
			DWORD dwBytesWrite;

			WriteFile(
				args.hNamedPipe,
				(char*)&outEmpl,
				sizeof(Response),
				&dwBytesWrite,
				(LPOVERLAPPED)NULL
			);

			if (outEmpl.status) {
				Employee modifiedEmployee;

				bool read = ReadFile(
					args.hNamedPipe,
					(char*)&modifiedEmployee,
					sizeof(Employee),
					&dwBytesRead,
					(LPOVERLAPPED)NULL
				);
				
				{
					for (int i = 0; i < args.empolyeesCnt; i++) {
						if (modifiedEmployee.num == args.employees[i].num)
							args.employees[i] = modifiedEmployee;
					}

					std::ofstream file(args.fileName, std::ios::binary | std::ios::trunc);

					for (int i = 0; i < args.empolyeesCnt; i++) {
						file.write((char*)&args.employees[i], sizeof(Employee));
					}

					file.close();
				}
			}
			else {
				std::cout << "Employee is not found" << std::endl;
			}
		}
		else {
			std::cout << "Wrong command" << std::endl;
		}
	}

	return 0;
}

int main()
{
    const char* connectionPipeName = "\\\\.\\pipe\\connection_pipe";

    std::string fileName; 
    std::cout << "enter file name" << std::endl;
    std::cin >> fileName;

    int employeesCnt; 
    std::cout << "enter employees amount" << std::endl;
    std::cin >> employeesCnt;

    Employee* employees = new Employee[employeesCnt];
    std::ofstream file(fileName, std::ios::binary);

    std::cout << "enter employees info" << std::endl;

    for (int i = 0; i < employeesCnt; i++) {
        std::cin >> employees[i];
        file.write((char*)&employees[i], sizeof(Employee));
    }

    file.close();

    std::cout << "enter the amount of clients" << std::endl;
    int clientsCnt; std::cin >> clientsCnt;

    HANDLE* hClients = new HANDLE[clientsCnt];
    STARTUPINFOA* si = new STARTUPINFOA[clientsCnt];
    PROCESS_INFORMATION* piCom = new PROCESS_INFORMATION;

    ThreadArgs* threadArgs = new ThreadArgs[clientsCnt];
    HANDLE* threads = new HANDLE[clientsCnt];
    DWORD* threadsID = new DWORD[clientsCnt];

    HANDLE hNamedPipe;
    HANDLE fileMutex = CreateMutexA(NULL, FALSE, "fileMutex");

    bool status;

    for (int i = 0; i < clientsCnt; i++) {

        hNamedPipe = CreateNamedPipeA(
            "\\\\.\\pipe\\connection_pipe",
            PIPE_ACCESS_DUPLEX,
            PIPE_TYPE_MESSAGE | PIPE_READMODE_MESSAGE | PIPE_WAIT,
            PIPE_UNLIMITED_INSTANCES,
            0, 
            0,
            INFINITE,
            (LPSECURITY_ATTRIBUTES)NULL
        );

        //std::cout << "pipe: " << hNamedPipe << std::endl;

        ZeroMemory(&si[i], sizeof(STARTUPINFO));
        si[i].cb = sizeof(STARTUPINFO);

        char* command = new char[20];
        strcpy_s(command, 11, "client.exe");

        status = CreateProcessA(NULL, command, NULL, NULL, FALSE,
            CREATE_NEW_CONSOLE, NULL, NULL, &si[i], &piCom[i]);

        if (!status) {
            std::cout << "couldn't create process" << std::endl;
            continue;
        }

        hClients[i] = piCom[i].hProcess;
        
        threadArgs[i] = ThreadArgs{ hNamedPipe, fileMutex, fileName, employees, employeesCnt, i + 1 };

        //std::cout << "Waiting\n";
        status = ConnectNamedPipe(hNamedPipe, (LPOVERLAPPED)NULL);
        //std::cout << "Connected\n";

        if (!status) {
            std::cout << "couldn't connect a thread";
            continue;
        }

        threads[i] = CreateThread(NULL, 0, threadFunc, (LPVOID)&threadArgs[i], 0, &threadsID[i]);

        if (threads[i] == NULL) {
            std::cout << "couldn't create a thread " + GetLastError();
        }
    }

    for (int i = 0; i < clientsCnt; i++)
        WaitForSingleObject(threads[i], INFINITE);

    //CloseHandle(hNamedPipe);
}