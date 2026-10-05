#include "speedrun.h"

#include "player.h"
#ifdef SM68K_TAS
#include "tas.h"
#endif

#include <emscripten/bind.h>
#include <emscripten/val.h>
#include <optional>
#include <string>
#include <type_traits>
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
	emscripten::value_object<Loadout>("Loadout")
		.field("power", &Loadout::power)
		.field("star", &Loadout::star)
		.field("pwing", &Loadout::pwing)
		.field("items", &Loadout::items);
	emscripten::value_object<LevelEntered>("LevelEntered")
		.field("world", &LevelEntered::world)
		.field("level", &LevelEntered::level)
		.field("player", &LevelEntered::player);
	emscripten::value_object<LevelCompleted>("LevelCompleted")
		.field("world", &LevelCompleted::world)
		.field("level", &LevelCompleted::level)
		.field("player", &LevelCompleted::player);
	emscripten::value_object<MonsterFought>("MonsterFought")
		.field("world", &MonsterFought::world)
		.field("monster", &MonsterFought::monster)
		.field("player", &MonsterFought::player);
	emscripten::value_object<MonsterDefeated>("MonsterDefeated")
		.field("world", &MonsterDefeated::world)
		.field("monster", &MonsterDefeated::monster)
		.field("player", &MonsterDefeated::player);
	// Whichever of the two was not being played crosses as undefined, which
	// is a field the JSON the history is exported as leaves out altogether.
	emscripten::register_optional<int>();
	emscripten::value_object<PlayerDied>("PlayerDied")
		.field("world", &PlayerDied::world)
		.field("level", &PlayerDied::level)
		.field("monster", &PlayerDied::monster)
		.field("player", &PlayerDied::player);
	emscripten::value_object<PlayerHit>("PlayerHit")
		.field("world", &PlayerHit::world)
		.field("level", &PlayerHit::level)
		.field("monster", &PlayerHit::monster)
		.field("player", &PlayerHit::player);
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

// SavePlayer.Attribs, by the bit; see struct saveplayer in player.h.
constexpr uint8_t Attrib_star = 0b10000000;
constexpr uint8_t Attrib_fire = 0b01000000;
constexpr uint8_t Attrib_racoon = 0b00100000;
constexpr uint8_t Attrib_pwing = 0b00001000;

// An item list entry by name, numbered as the switch in Handle_player_map()
// that spends them numbers them. A number past the end is named rather than
// dropped: the list is what the player was holding, and leaving a slot out
// would say they held less than they did.
std::string item_name(int item)
{
	switch (item) {
	case 1:
		return "mushroom";
	case 2:
		return "fire-flower";
	case 3:
		return "leaf";
	case 4:
		return "star";
	case 5:
		return "whistle";
	case 6:
		return "hammer";
	case 7:
		return "p-wing";
	case 8:
		return "cloud";
	case 9:
		return "anchor";
	default:
		return "item-" + std::to_string(item);
	}
}

// The player as they are now. Life is 1 small, 2 super and 3 either suit, and
// which suit is the Attribs bit - the same reading practice.ts makes of it.
Loadout loadout()
{
	const uint8_t Attribs = static_cast<uint8_t>(SavePlayer.Attribs);

	std::string power = SavePlayer.Life >= 2 ? "super" : "small";

	if (SavePlayer.Life >= 3 && (Attribs & Attrib_fire)) {
		power = "fire";
	}
	if (SavePlayer.Life >= 3 && (Attribs & Attrib_racoon)) {
		power = "racoon";
	}

	emscripten::val items = emscripten::val::array();

	for (int C = 0; C < itemlist_length; C++) {
		if (SavePlayer.Itemlist[C]) {
			items.call<void>("push",
					 item_name(static_cast<uint8_t>(
						 SavePlayer.Itemlist[C])));
		}
	}

	return Loadout{ .power = power,
			.star = (Attribs & Attrib_star) != 0,
			.pwing = (Attribs & Attrib_pwing) != 0,
			.items = items };
}

}

void report(const Event &event)
{
	// The TAS build has no timer to report to, and asking whether it does is
	// not free there: Embind hands out ids from its JS side that the C side
	// caches in statics, so a restored snapshot would ask again and cache a
	// different one - harmless to the game, but no longer the same memory
	// the movie made the first time. See src/tas.h. The page is told the
	// kind, and where for the events that start and end what is played,
	// which is all its clock and its segments need and crosses without
	// Embind.
#ifdef SM68K_TAS
	std::visit(
		[](const auto &e) {
			using E = std::decay_t<decltype(e)>;
			if constexpr (std::is_same_v<E, LevelEntered> ||
				      std::is_same_v<E, LevelCompleted>) {
				tas::event(e.kind, e.world, e.level);
			} else if constexpr (std::is_same_v<E, MonsterFought> ||
					     std::is_same_v<E, MonsterDefeated>) {
				tas::event(e.kind, e.world, e.monster);
			} else {
				tas::event(e.kind, -1, -1);
			}
		},
		event);
	return;
#endif

	// The shell only installs the hook when the player asked for the timer
	// or for practice mode - the second so that practice is kept in the run
	// history too - so on most runs of the game there is nothing listening
	// here at all.
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

	report(LevelEntered{
		.world = world, .level = level, .player = loadout() });
}

void entered_monster(int world, int monster)
{
	playing = Playing{ .world = world,
			   .index = monster,
			   .monster = true,
			   .reported = false };

	report(MonsterFought{
		.world = world, .monster = monster, .player = loadout() });
}

void cleared_level()
{
	if (!playing || playing->reported) {
		return;
	}

	playing->reported = true;

	if (playing->monster) {
		report(MonsterDefeated{ .world = playing->world,
					.monster = playing->index,
					.player = loadout() });
		return;
	}

	report(LevelCompleted{ .world = playing->world,
			       .level = playing->index,
			       .player = loadout() });
}

namespace
{

// Something that happened to the player in whatever is being played, said with
// where it happened: the level, or the monster fight. Nothing where nothing is
// being played - see PlayerDied.
template <typename Event> void report_in_play()
{
	if (!playing) {
		return;
	}

	Event event{ .world = playing->world, .player = loadout() };

	if (playing->monster) {
		event.monster = playing->index;
	} else {
		event.level = playing->index;
	}

	report(event);
}

}

void died()
{
	report_in_play<PlayerDied>();
}

void hit()
{
	report_in_play<PlayerHit>();
}

void left_level(bool completed)
{
	if (completed) {
		cleared_level();
	}

	playing.reset();
}

bool in_play()
{
	return playing.has_value();
}

}
