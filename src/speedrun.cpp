#include "speedrun.h"

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <optional>
#include <string>
#include <variant>

namespace speedrun
{

// Registering each event as a value_object is what lets emscripten::val(event)
// below build the JS object out of the struct on its own, field by field. An
// event with no payload still needs its registration: it crosses as an empty
// object that only the `kind` added below tells from any other.
EMSCRIPTEN_BINDINGS(speedrun)
{
	emscripten::value_object<RunStarted>("RunStarted");
	emscripten::value_object<RunAbandoned>("RunAbandoned");
	emscripten::value_object<RunEnded>("RunEnded");
	emscripten::value_object<WorldEntered>("WorldEntered")
		.field("world", &WorldEntered::world);
	emscripten::value_object<WarpTaken>("WarpTaken")
		.field("world", &WarpTaken::world);
	emscripten::value_object<LevelEntered>("LevelEntered")
		.field("world", &LevelEntered::world)
		.field("level", &LevelEntered::level);
	emscripten::value_object<LevelCompleted>("LevelCompleted")
		.field("world", &LevelCompleted::world)
		.field("level", &LevelCompleted::level);
	emscripten::value_object<MonsterFought>("MonsterFought")
		.field("world", &MonsterFought::world)
		.field("monster", &MonsterFought::monster);
	emscripten::value_object<MonsterDefeated>("MonsterDefeated")
		.field("world", &MonsterDefeated::world)
		.field("monster", &MonsterDefeated::monster);
}

namespace
{

struct Playing {
	int world;
	// The level's index in the world file, or, where `monster` is set, the
	// monster's index in the map's objects. Which of the two it is decides
	// what beating it reports, and nothing else here looks at it.
	int index;
	bool monster;
	bool reported;
};

// What is being played, if anything is: a level, or the fight an overworld
// monster drops the player into. Let go of when it returns, so that a bonus
// room or a pipe passage - neither of which is either, and neither of which
// reports - cannot be taken for the one before it.
std::optional<Playing> playing;

}

void report(const Event &event)
{
	// The shell only installs the hook when the player asked for the timer,
	// so on most runs of the game there is nothing listening here at all.
	emscripten::val hook =
		emscripten::val::module_property("onSpeedrunEvent");

	if (hook.typeOf().as<std::string>() != "function") {
		return;
	}

	// One visitor covers every event: the payload converts itself, and the
	// discriminant the shell switches on is the struct's own name. Adding an
	// event is a struct in the header and a registration above, and nothing
	// here.
	emscripten::val answer = hook(std::visit(
		[](const auto &e) {
			emscripten::val payload = emscripten::val(e);

			payload.set("kind", std::string(e.kind));
			return payload;
		},
		event));

	// The shell's answer is its one way of talking back, and it only ever says
	// one thing: this run is over, stop the game. It says it when a recording
	// has just written the last split its category has, which is a moment the
	// game itself has no opinion about - it would carry on into the next level
	// with the player still holding the keys.
	if (answer.isTrue()) {
		throw Stopped{};
	}
}

void entered_level(int world, int level)
{
	playing = Playing{ .world = world,
			   .index = level,
			   .monster = false,
			   .reported = false };

	report(LevelEntered{ .world = world, .level = level });
}

void entered_monster(int world, int monster)
{
	playing = Playing{ .world = world,
			   .index = monster,
			   .monster = true,
			   .reported = false };

	report(MonsterFought{ .world = world, .monster = monster });
}

void cleared_level()
{
	if (!playing || playing->reported) {
		return;
	}

	playing->reported = true;

	if (playing->monster) {
		report(MonsterDefeated{ .world = playing->world,
					.monster = playing->index });
		return;
	}

	report(LevelCompleted{ .world = playing->world,
			       .level = playing->index });
}

void left_level(bool completed)
{
	if (completed) {
		cleared_level();
	}

	playing.reset();
}

}
