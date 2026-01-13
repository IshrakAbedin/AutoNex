#include <iostream>
#include <format>
#include <string>
#include <vector>
#include <chrono>
#include <thread>

#include "autonex/StateMachine.hpp"
#include "autonex/ObjectPool.hpp"
#include "autonex/Timer.hpp"

#define println(fmt, ...) std::cout << std::format(fmt, __VA_ARGS__) <<std::endl

static void StateMachineExample();
static void ObjectPoolExample();
static void TimerExample();

int main()
{
	//StateMachineExample();
	//ObjectPoolExample();
	TimerExample();
	return 0;
}

enum class States
{
	ALIVE, INJURED, DEAD, IMMORTAL
};

template <>
struct std::formatter<States> : std::formatter<std::string> {
	auto format(States st, std::format_context& ctx) const {
		std::string s;
		switch (st) {
		case States::ALIVE:		s = "<Alive>";		break;
		case States::INJURED:	s = "<Injured>";	break;
		case States::DEAD:		s = "<Dead>";		break;
		case States::IMMORTAL:	s = "<Immortal>";	break;
		default:				s = "<Unknown>";	break;
		}
		return std::formatter<std::string>::format(s, ctx);
	}
};

void StateMachineExample()
{
	anx::StateMachine<States> sm{ States::ALIVE };
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

	sm.BindOnStateLeave(States::ALIVE,
		[&](States into) { println("(EVENT) Left {} and entered {}", States::ALIVE, into); }
	);
	/*sm.BindOnStateLeave(States::ALIVE,
		[&](States into) { println("(EVENT) Just to be sure, Left {} and entered {}", States::ALIVE, into); }
	);*/
	sm.BindOnStateLeave(States::INJURED,
		[&](States into) { println("(EVENT) Left {} and entered {}", States::INJURED, into); }
	);
	sm.BindOnStateLeave(States::DEAD,
		[&](States into) { println("(EVENT) Left {} and entered {}", States::DEAD, into); }
	);
	sm.BindOnStateLeave(States::IMMORTAL,
		[&](States into) { println("(EVENT) Left {} and entered {}", States::IMMORTAL, into); }
	);

	sm.BindOnStateEntry(States::ALIVE,
		[&](States from) { println("(EVENT) Entered {} from {}", States::ALIVE, from); }
	);
	sm.BindOnStateEntry(States::INJURED,
		[&](States from) { println("(EVENT) Entered {} from {}", States::INJURED, from); }
	);
	sm.BindOnStateEntry(States::DEAD,
		[&](States from) { println("(EVENT) Entered {} from {}", States::DEAD, from); }
	);
	sm.BindOnStateEntry(States::IMMORTAL,
		[&](States from) { println("(EVENT) Entered {} from {}", States::IMMORTAL, from); }
	);

	println("Starting with state: {}", sm.GetCurrentState());
	println("{}", "Iterating 9 times to deduct health by 10 and then stepping\n");
	for (int i = 0; i < 9; i++)
	{
		health -= 10;
		println("Current health: {}", health);
		if (i == 2)
		{
			println("{}", "Activating immortality");
			immortality = true;
		}
		else if (i == 4)
		{
			println("{}", "Deactivating immortality");
			immortality = false;
		}
		sm.Step();
		println("Current state: {}\n", sm.GetCurrentState());
	}
	println("{}", "Iterating 10 times to increase health by 10 and then stepping");
	for (int i = 0; i < 10; i++)
	{
		health += 10;
		println("Current health: {}", health);
		if (i == 1)
		{
			println("{}", "Activating immortality");
			immortality = true;
		}
		else if (i == 2)
		{
			println("{}", "Deactivating immortality");
			immortality = false;
		}
		sm.Step();
		println("Current state: {}\n", sm.GetCurrentState());
	}
}


// Custom struct with non-trivial destructor to demonstrate proper cleanup
struct Entity {
	int id;
	std::string name;
	float* data;  // Dynamically allocated data

	Entity(int id, const std::string& name)
		: id(id), name(name), data(new float[100]) {
		println("Entity '{}' (ID: {}) constructed", name, id);
	}

	~Entity() {
		println("Entity '{}' (ID: {}) destroyed", name, id);
		delete[] data;  // Clean up dynamic memory
	}

	void Print() const {
		println("Entity ID: {}, Name: {}", id, name);
	}
};

void ObjectPoolExample()
{
	println("{}", "=== ObjectPool Demo ===\n");

	// ============================================
	// Example 1: Primitive Type (int)
	// ============================================
	println("{}", "--- Example 1: Primitive Type (int) ---");
	anx::ObjectPool<int> intPool(2);  // Reserve space for 2 ints, should grow afterwards

	// Create some integers
	auto h1 = intPool.Create(42);
	auto h2 = intPool.Create(100);
	auto h3 = intPool.Create(256);

	println("Pool size: {}", intPool.Size());

	// Access and modify values
	if (auto opt = intPool.Get(h1)) {
		int& value = opt->get();
		println("h1 value: {}", value);
		value = 99;  // Modify the value
		println("h1 modified to: {}", value);
	}

	// Destroy one object
	println("{}", "Destroying h2...");
	intPool.Destroy(h2);
	println("Pool size after destroy: {}", intPool.Size());

	// Try to access destroyed handle (should fail)
	if (auto opt = intPool.Get(h2)) {
		println("h2 value: {}", opt->get());
	}
	else {
		println("{}", "h2 is invalid (as expected)");
	}

	// Reuse the slot
	auto h4 = intPool.Create(777);  // Reuses h2's slot
	println("{}", "Created h4 in reused slot");
	println("Pool size: {}", intPool.Size());

	// h2 is still invalid due to generation mismatch
	if (intPool.Validate(h2)) {
		println("{}", "h2 is valid");
	}
	else {
		println("{}", "h2 is still invalid (generation mismatch protects us!)");
	}

	auto h5 = intPool.Create(512); // This should use a new slot
	println("Pool size: {}", intPool.Size());

	println("{}", "");

	// ============================================
	// Example 2: Custom Struct with Destructor
	// ============================================
	println("{}", "--- Example 2: Custom Struct (Entity) ---");
	// If using static allocation here, make sure to pre-reserve enough storage beforehand to prevent reallocation
	anx::ObjectPool<Entity, anx::PoolAllocationStrategy::DYNAMIC> entityPool;

	// Create entities
	auto e1 = entityPool.Create(1, "Player");
	auto e2 = entityPool.Create(2, "Enemy");
	auto e3 = entityPool.Create(3, "NPC");

	println("{}", "\nAccessing entities:");
	if (auto opt = entityPool.Get(e1)) {
		opt->get().Print();
	}
	if (auto opt = entityPool.Get(e2)) {
		opt->get().Print();
	}

	// Destroy an entity (watch for destructor call)
	println("{}", "\nDestroying Enemy...");
	entityPool.Destroy(e2);

	// Create a new entity (reuses the slot)
	println("{}", "\nCreating new entity (reuses slot):");
	auto e4 = entityPool.Create(4, "Merchant");

	// Old handle e2 is invalid
	println("{}", "\nTrying to access old e2 handle:");
	if (auto opt = entityPool.Get(e2)) {
		opt->get().Print();
	}
	else {
		println("{}", "e2 handle is invalid (protected by generation!)");
	}

	// New handle e4 works fine
	println("{}", "\nAccessing new e4 handle:");
	if (auto opt = entityPool.Get(e4)) {
		opt->get().Print();
	}

	println("Final pool size: {}", entityPool.Size());

	// ============================================
	// Example 3: Const Access
	// ============================================
	println("{}", "\n--- Example 3: Const Access ---");
	const auto& constPool = entityPool;

	if (auto opt = constPool.Get(e1)) {
		const Entity& entity = opt->get();
		entity.Print();  // Can only call const methods
	}

	println("{}", "\n--- Cleanup (destructors called automatically) ---");
	// When pools go out of scope, all remaining objects are properly destroyed
}

void TimerExample()
{
	using namespace anx;
	using namespace std::chrono_literals;

	println("{}", "=== Timer Demo ===\n");

	// ============================================
	// Setup: Create timer manager and counters
	// ============================================
	println("{}", "--- Creating Timer Manager ---");
	TimerManager<PoolAllocationStrategy::STATIC> timerManager(10);

	int oneSecondCounter = 0;
	int fastCounter = 0;
	int slowCounter = 0;
	int singleShotCounter = 0;
	int disablableCounter = 0;
	bool recursiveTimerCreated = false;

	// ============================================
	// Example 1: Basic Repeating Timer (1s)
	// ============================================
	println("{}", "\n--- Example 1: Repeating Timer (1s interval) ---");
	auto timer1s = timerManager.CreateTimer(
		1s,
		true,  // repeating
		[&oneSecondCounter]() {
			println("[1s Timer] Fired! Count: {}", ++oneSecondCounter);
		},
		true   // enabled
	);
	println("{}", "Created 1-second repeating timer");

	// ============================================
	// Example 2: Fast Repeating Timer (100ms)
	// ============================================
	println("{}", "\n--- Example 2: Fast Repeating Timer (100ms interval) ---");
	auto timerFast = timerManager.CreateTimer(
		100ms,
		true,
		[&fastCounter]() {
			fastCounter++;
			if (fastCounter % 10 == 0) {
				println("[100ms Timer] Fired 10 times! Total: {}", fastCounter);
			}
		}
	);
	println("{}", "Created 100ms repeating timer (reports every 10 fires)");

	// ============================================
	// Example 3: Slow Repeating Timer (2.5s)
	// ============================================
	println("{}", "\n--- Example 3: Slow Repeating Timer (2.5s interval) ---");
	auto timerSlow = timerManager.CreateTimer(
		2500ms,
		true,
		[&slowCounter]() {
			println("[2.5s Timer] Fired! Count: {}", ++slowCounter);
		}
	);
	println("{}", "Created 2.5-second repeating timer");

	// ============================================
	// Example 4: Single-Shot Timer (3s)
	// ============================================
	println("{}", "\n--- Example 4: Single-Shot Timer (fires once at 3s) ---");
	auto timerSingleShot = timerManager.CreateTimer(
		3s,
		false,  // NOT repeating
		[&singleShotCounter]() {
			println("{}", "[Single-Shot Timer] Fired once at 3 seconds!");
			singleShotCounter++;
		}
	);
	println("{}", "Created single-shot timer (auto-destroys after firing)");

	// ============================================
	// Example 5: Enable/Disable Control (500ms)
	// ============================================
	println("{}", "\n--- Example 5: Timer with Enable/Disable Control ---");
	auto timerDisablable = timerManager.CreateTimer(
		500ms,
		true,
		[&disablableCounter]() {
			println("[Disablable Timer] Fired! Count: {}", ++disablableCounter);
		}
	);
	println("{}", "Created 500ms timer (will be disabled at 5s, re-enabled at 7s)");

	// ============================================
	// Example 6: Recursive Timer Creation
	// ============================================
	println("{}", "\n--- Example 6: Timer Creating Another Timer ---");
	auto timerRecursive = timerManager.CreateTimer(
		4s,
		false,
		[&timerManager, &recursiveTimerCreated]() {
			println("{}", "[Recursive Timer] Creating child timer from callback!");
			timerManager.CreateTimer(
				500ms,
				false,
				[]() {
					println("{}", "  [Child Timer] Fired from parent callback!");
				}
			);
			recursiveTimerCreated = true;
		}
	);
	println("{}", "Created recursive timer (spawns child at 4s)");

	// ============================================
	// Main Loop: 1000 TPS for 10 seconds
	// ============================================
	println("{}", "\n--- Starting Main Loop ---");
	println("{}", "Running at 1000 TPS (1ms per tick) for 10 seconds...\n");

	const int TICKS_PER_SECOND = 1000;
	const auto TICK_DURATION = std::chrono::microseconds(1000000 / TICKS_PER_SECOND);
	const int TOTAL_SECONDS = 10;
	const int TOTAL_TICKS = TICKS_PER_SECOND * TOTAL_SECONDS;

	auto startTime = std::chrono::steady_clock::now();
	auto nextTickTime = startTime;

	for (int tick = 0; tick < TOTAL_TICKS; ++tick)
	{
		nextTickTime += TICK_DURATION;
		timerManager.Tick();

		auto elapsed = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::steady_clock::now() - startTime
		).count();

		// Disable timer at 5 seconds
		if (elapsed >= 5000 && elapsed < 5100 && timerDisablable.IsEnabled())
		{
			println("{}", "\n(ACTION) Disabling 500ms timer at 5 seconds\n");
			timerDisablable.Disable();
		}

		// Re-enable timer at 7 seconds
		if (elapsed >= 7000 && elapsed < 7100 && !timerDisablable.IsEnabled())
		{
			println("{}", "\n(ACTION) Re-enabling 500ms timer at 7 seconds\n");
			timerDisablable.Enable();
		}

		std::this_thread::sleep_until(nextTickTime);
	}

	auto endTime = std::chrono::steady_clock::now();
	auto actualDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
		endTime - startTime
	).count();

	// ============================================
	// Results Summary
	// ============================================
	println("{}", "\n--- Test Results ---");
	println("Actual runtime: {}ms", actualDuration);
	println("Target runtime: {}ms", TOTAL_SECONDS * 1000);
	println("Timing accuracy: {}ms drift", actualDuration - TOTAL_SECONDS * 1000);

	println("{}", "\nTimer Fire Counts:");
	println("  1s Timer: {} times (expected: ~10)", oneSecondCounter);
	println("  100ms Timer: {} times (expected: ~100)", fastCounter);
	println("  2.5s Timer: {} times (expected: ~4)", slowCounter);
	println("  Single-Shot Timer: {} time (expected: 1)", singleShotCounter);
	println("  Disablable Timer: {} times (expected: ~15)", disablableCounter);
	println("  Recursive timer created child: {}", recursiveTimerCreated ? "Yes" : "No");

	println("{}", "\nTimer Handler States:");
	println("  timer1s - valid: {}, enabled: {}", timer1s.IsValid(), timer1s.IsEnabled());
	println("  timerFast - valid: {}, enabled: {}", timerFast.IsValid(), timerFast.IsEnabled());
	println("  timerSingleShot - valid: {} (should be false - auto-destroyed)", timerSingleShot.IsValid());
	println("  timerDisablable - valid: {}, enabled: {}", timerDisablable.IsValid(), timerDisablable.IsEnabled());

	println("{}", "\n--- Cleanup (timers destroyed automatically) ---");
}