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
	emscripten::value_object<LevelEntered>("LevelEntered")
		.field("world", &LevelEntered::world)
		.field("level", &LevelEntered::level);
	emscripten::value_object<LevelCompleted>("LevelCompleted")
		.field("world", &LevelCompleted::world)
		.field("level", &LevelCompleted::level);
}

namespace
{

struct Playing {
	int world;
	int level;
	bool reported;
};

// The level being played, if one is. Let go of when the level returns, so that
// a bonus room or a pipe passage - neither of which is a level, and neither of
// which reports - cannot be taken for the level before it.
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
	hook(std::visit(
		[](const auto &e) {
			emscripten::val payload = emscripten::val(e);

			payload.set("kind", std::string(e.kind));
			return payload;
		},
		event));
}

void entered_level(int world, int level)
{
	playing = Playing{ .world = world, .level = level, .reported = false };

	report(LevelEntered{ .world = world, .level = level });
}

void cleared_level()
{
	if (!playing || playing->reported) {
		return;
	}

	playing->reported = true;

	report(LevelCompleted{ .world = playing->world,
			       .level = playing->level });
}

void left_level(bool completed)
{
	if (completed) {
		cleared_level();
	}

	playing.reset();
}

}
