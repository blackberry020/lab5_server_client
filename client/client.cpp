#include <iostream>
#include <windows.h>
#include "../query.h"
#include "../response.h"

const int nameSize = 10;

int main()
{
	HANDLE hNamedPipe;
	HANDLE fileMutex = OpenMutexA(MUTEX_ALL_ACCESS, TRUE, "fileMutex");

	const char* connectionPipeName = "\\\\.\\pipe\\connection_pipe";

	if (!WaitNamedPipeA("\\\\.\\pipe\\connection_pipe", NMPWAIT_WAIT_FOREVER)) {
		std::cout << "Error waiting for named pipe" << std::endl;
		return 1;
	}

	hNamedPipe = CreateFileA(
		connectionPipeName,
		GENERIC_READ | GENERIC_WRITE,
		FILE_SHARE_READ,
		(LPSECURITY_ATTRIBUTES)NULL,
		OPEN_EXISTING,
		0,
		(HANDLE)NULL
	);
	
	if (hNamedPipe == INVALID_HANDLE_VALUE)
	{
		std::cout << "Connection with the named pipe failed" << std::endl;
		system("pause");
		return 0;
	}

	DWORD dwBytesWritten;
	DWORD dwBytesRead;
	Query query;
	Response response;
	bool read;

	while (true) {
		std::cin >> query.command >> query.number;
		if (query.command == 'r' || query.command == 'm') {
			WriteFile(
				hNamedPipe,
				(char*)&query,
				sizeof(Query),
				&dwBytesWritten,
				(LPOVERLAPPED)NULL
			);

			read = ReadFile(
				hNamedPipe,
				&response,
				sizeof(Response),
				&dwBytesRead,
				(LPOVERLAPPED)NULL
			);

			if (!read)
				break;

			if (response.status) {
				std::cout << response.employee.num << " " << response.employee.name
					<< " " << response.employee.hours << std::endl;

				if (query.command == 'm') {
					WaitForSingleObject(fileMutex, INFINITE);

					Employee modifiedEmloyee;
					bool validated = false;

					while (!validated) {
						std::cout << "Enter name and hours" << std::endl;
						modifiedEmloyee.num = query.number;
						std::string name;
						std::cin >> name >> modifiedEmloyee.hours;

						if (name.length() < nameSize) {
							validated = true;
							strcpy_s(modifiedEmloyee.name, name.c_str());
						}
						else
							std::cout << "Name size must be less than " << nameSize;
					}

					WriteFile(
						hNamedPipe,
						(char*)&modifiedEmloyee,
						sizeof(Employee),
						&dwBytesWritten,
						(LPOVERLAPPED)NULL
					);

					ReleaseMutex(fileMutex);
				}
			}
			else {
				std::cout << "Empolyee with number " << query.number << " is not found" << std::endl;
			}
		}
		else {
			std::cout << "Wrong command" << std::endl;
		}
	}

	CloseHandle(hNamedPipe);

    system("pause");
}