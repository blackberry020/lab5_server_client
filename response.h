#pragma once
#include "lab5_server_client/employee.h"

struct Response {
	Employee employee;
	bool status;
};

Response getEmployee(Employee* employees, int employeesCnt, int num) {
	for (int i = 0; i < employeesCnt; i++)
		if (num == employees[i].num)
			return { employees[i], true };
	return { NULL, false };
}