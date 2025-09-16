#include <iostream>
#include <format>

#include "StateMachine.hpp"

int main()
{
	std::cout << std::format("Hello, {}", 123) << std::endl;
	return 0;
}