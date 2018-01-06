// ConsoleApplication1.cpp : 콘솔 응용 프로그램에 대한 진입점을 정의합니다.
//

#include <iostream>
#include <string>
using namespace std;

class person
{
	string name;
	int id;
public:
	person(int id, string name);
	person(person &person);
	void changename(string name);
	void show() { cout << id << "," << name << endl; }
};

person::person(int id, string name)
{
	this->id = id;
	string p = string(name);
	this->name = p;
}

void person::changename(string name)
{
	string p = string(name);
	this->name = p;
}

person::person(person &person)
{
	this->id = id;
	string p = string(person.name);
	this->name = p;
}
int main()
{
	person father(1, "kitae");
	person daugher(father);
	cout << "daughter 객체 생성 직후 -----" << endl;
	father.show();
	daugher.show();

	daugher.changename("grace");
	father.show();
	daugher.show();
}