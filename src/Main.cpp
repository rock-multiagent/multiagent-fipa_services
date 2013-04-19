#include <iostream>
#include <fipa_services/Dummy.hpp>

int main(int argc, char** argv)
{
	dummy_project::DummyClass dummyClass;
	dummyClass.welcome();

	return 0;
}
