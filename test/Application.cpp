#include <iostream>
#include <format>

#include <string>
#include <vector>

#include "StateMachine.hpp"

#define println(fmt, ...) std::cout << std::format(fmt, __VA_ARGS__) <<std::endl

enum class States
{
	ALIVE, INJURED, DEAD, IMMORTAL
};

template <>
struct std::formatter<States> : std::formatter<std::string> {
	auto format(States st, std::format_context& ctx) const {
		std::string s;
		switch (st) {
		case States::ALIVE:		s = "<Alive>";	break;
		case States::INJURED:	s = "<Injured>";	break;
		case States::DEAD:		s = "<Dead>";		break;
		case States::IMMORTAL:	s = "<Immortal>"; break;
		default:				s = "<Unknown>";	break;
		}
		return std::formatter<std::string>::format(s, ctx);
	}
};

int main()
{
	anx::StateMachine<States> sm{States::ALIVE};
	int health{ 100 };
	bool immortality{ false };

	sm.AddTranstionRule(States::ALIVE, States::INJURED,
		[&]() { return health < 50; }
	);
	sm.AddTranstionRule(States::ALIVE, States::IMMORTAL,
		[&]() { return immortality; }
	);

	sm.AddTranstionRule(States::INJURED, States::ALIVE,
		[&]() { return health >= 50; }
	);
	sm.AddTranstionRule(States::INJURED, States::IMMORTAL,
		[&]() { return immortality; }
	);
	sm.AddTranstionRule(States::INJURED, States::DEAD,
		[&]() { return health <= 0; }
	);

	sm.AddTranstionRule(States::IMMORTAL, States::ALIVE,
		[&]() { return health >= 50 && !immortality; }
	);

	sm.AddTranstionRule(States::IMMORTAL, States::INJURED,
		[&]() { return health < 50 && !immortality; }
	);

	sm.AddTranstionRule(States::IMMORTAL, States::DEAD,
		[&]() { return health <= 0 && !immortality; }
	);

	println("Starting with state: {}", sm.GetCurrentState());
	println("Iterating 9 times to deduct health by 10 and then stepping");
	for (int i = 0; i < 9; i++)
	{
		health -= 10;
		println("Current health: {}", health);
		if (i == 2)
		{
			println("Activating immortality");
			immortality = true;
		}
		else if (i == 4)
		{
			println("Deactivating immortality");
			immortality = false;
		}
		sm.Step();
		println("Current state: {}\n", sm.GetCurrentState());
	}
	println("Iterating 10 times to increase health by 10 and then stepping");
	for (int i = 0; i < 10; i++)
	{
		health += 10;
		println("Current health: {}", health);
		if (i == 1)
		{
			println("Activating immortality");
			immortality = true;
		}
		else if (i == 2)
		{
			println("Deactivating immortality");
			immortality = false;
		}
		sm.Step();
		println("Current state: {}\n", sm.GetCurrentState());
	}

	return 0;
}