#pragma once

struct Employee {
	int num;
	char name[10];
	double hours;

	friend std::ostream& operator << (std::ostream& os, const Employee& employee)
	{
		return os << employee.num << " " << employee.name << " " << employee.hours;
	}

	friend std::istream& operator >> (std::istream& os, Employee& employee)
	{
		os >> employee.num >> employee.name >> employee.hours;
		return os;
	}
};
