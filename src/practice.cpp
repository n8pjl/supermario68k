#include "practice.h"

#include "compat/assets.h"
#include "control.h"
#include "gameloop.h"
#include "items.h"
#include "level.h"
#include "levelset.h"
#include "map.h"
#include "player.h"
#include "render.h"
#include "scankeys.h"
#include "stringcopy.h"

#include <emscripten/val.h>
#include <stdint.h>
#include <string>

namespace practice
{

// Attribs, by the bit. The game writes these as literals at each of the two
// dozen places it changes a powerup; they are named here because this file
// changes all of them at once and a mask of five bits with no name in it is
// the kind of thing that ends up one bit different from the game's.
static constexpr uint8_t Attrib_star = 0b10000000;
static constexpr uint8_t Attrib_fire = 0b01000000;
static constexpr uint8_t Attrib_racoon = 0b00100000;
static constexpr uint8_t Attrib_pwing = 0b00001000;

// The powerups, as the shell names them in `power.level`. The order is the
// game's own Life, less the two suits sharing Life 3.
enum Power {
	Power_small = 0,
	Power_large = 1,
	Power_fire = 2,
	Power_racoon = 3,
};

// Frames between squares while a direction is held down. The first press moves
// at once and this is only the repeat, so it costs nothing on a single step;
// crossing a map a keypress at a time is not free movement, and a square a
// frame is faster than the map scrolls.
static constexpr int16_t Roam_repeat = 3;

// Whether the game's own map handling has been switched off, and for how much
// longer the held direction is waiting. Roaming is left set while the shell
// keeps asking for it and cleared the moment it stops - see poll().
static bool Roaming = false;
static int16_t Roam_delay = 0;

// Whether a value the shell handed over is an object with something in it.
// Null is asked about separately because JavaScript calls it an object, and
// reading a field off it is what would then fail.
static bool present(const emscripten::val &Value)
{
	return !Value.isNull() && !Value.isUndefined() &&
	       (Value.typeOf().as<std::string>() == "object");
}

// Whether the shell is listening. Resolved once: the hook is installed when the
// runtime is created, before main() runs, so an answer taken on the first frame
// of the first map is the answer for the whole session.
static bool hooked()
{
	static const bool answer = [] {
		emscripten::val hook =
			emscripten::val::module_property("onPracticeRequest");

		return hook.typeOf().as<std::string>() == "function";
	}();

	return answer;
}

// The map as its own file has it, tile zero first: what Load_map() copied in
// before the game started writing over it. It is the only record of what a
// square used to be - the loaded map keeps none, because a level that has been
// beaten is a piece of road and a road remembers nothing - and it is what
// unclearing reads. NULL if the file has gone missing, which Load_map() would
// already have failed on.
//
// The world file rather than the save: a savegame keeps its map as a
// compressed difference against these same bytes and never writes them back, so
// what is embedded is the map as it shipped whatever has been saved since.
static const uint8_t *original_map()
{
	// The warp zone is a map of the common file, and reachable from here by
	// the whistle like any other; the game makes this same check wherever it
	// reloads the map it is on.
	const struct asset *File = Asset_find(
		(Levelsetdata.CurrentWorld == Levelsetdata.Commonfile) ?
			Commonfilename :
			Levelfilename);

	if (!File) {
		return NULL;
	}

	return File->data + sizeof(struct levelfiledata) +
	       sizeof(struct map_data);
}

// What the square at these coordinates was on the day the map was made, or 0
// where there is no map file to ask.
static uint8_t original_tile(int16_t X, int16_t Y)
{
	const uint8_t *Tiles = original_map();

	if (!Tiles) {
		return 0;
	}

	return Tiles[(Y >> 4) * Map_data.Width + (X >> 4)];
}

// What the panel draws itself from. Read back every frame, so that walking the
// map keeps the readout honest and the panel can tell whether the square being
// stood on is a level - which is the whole of what decides whether there is
// anything to clear.
static emscripten::val snapshot()
{
	emscripten::val Items = emscripten::val::array();

	for (int16_t C = 0; C < itemlist_length; C++) {
		Items.call<void>("push",
				 static_cast<int>(SavePlayer.Itemlist[C]));
	}

	emscripten::val Status = emscripten::val::object();

	Status.set("world", static_cast<int>(Levelsetdata.CurrentWorld));
	Status.set("worlds", static_cast<int>(Levelsetdata.Nr_of_files));
	Status.set("width", static_cast<int>(Map_data.Width));
	Status.set("height", static_cast<int>(Map_data.Height));
	Status.set("x", static_cast<int>(SavePlayer.MapX / 16));
	Status.set("y", static_cast<int>(SavePlayer.MapY / 16));
	Status.set("tile", static_cast<int>(Get_map_tile(SavePlayer.MapX,
							 SavePlayer.MapY)));

	// What the map file has at that square, which is how the panel tells a
	// level that has been beaten from a piece of road that was always road.
	Status.set("original", static_cast<int>(original_tile(
				       SavePlayer.MapX, SavePlayer.MapY)));
	Status.set("life", static_cast<int>(SavePlayer.Life));
	Status.set("attribs",
		   static_cast<int>(static_cast<uint8_t>(SavePlayer.Attribs)));
	Status.set("items", Items);

	return Status;
}

// Puts the player on a square, and calls off whatever the map was doing with
// them. Everything after the two coordinates is what the game does for itself
// when a map is entered for the first time, said again here because arriving
// this way skips the code that would have said it:
//
//   PrevComp is the square the player is returned to when they come out of a
//   level without beating it, and the one Handle_player_map() walks them back
//   from. Setting it to where they now are is what stops a warp from being
//   followed by a walk back to the world before: the difference it takes as the
//   motion still owed is zero.
//
//   Offset is what makes Handle_player_map() recompute that motion at all. It
//   keeps it in statics this file cannot reach, so this is the only way to call
//   off a step the player was in the middle of.
//
//   L is the direction lock, which remembers which way off this square the
//   player may go; the square they have just landed on has not told them yet,
//   and the game zeroes it on every arrival for that reason.
//
//   The item list is not what a warp wants to be looking at, and the map only
//   recomputes its scrolling while the status bar is showing status.
static void place(int16_t X, int16_t Y)
{
	SavePlayer.MapX = X;
	SavePlayer.MapY = Y;

	SavePlayer.PrevCompX = X;
	SavePlayer.PrevCompY = Y;

	SavePlayer.L = 0;
	Player.Offset = 1;

	Map_statusbar = status;
	Player.Curr_item = 0;

	Map_plane.p.force_update = 1;
}

// To a world, and to the square that world's map file starts the player on.
// Nowhere else: which square of it to be on is what roaming is for, and a
// coordinate typed into a panel was never the way anybody wanted to say it.
static void warp(const emscripten::val &Request)
{
	int16_t World = Request["world"].as<int>();

	if ((World < 0) || (World >= Levelsetdata.Nr_of_files)) {
		return;
	}

	// A map is reloaded when the world changes, and otherwise only if asked
	// for. Both are wanted: leaving the map alone keeps the castles that
	// have been knocked down and the roads that have been opened, which is
	// what practising the rest of a world in progress needs, and reloading
	// it puts them all back, which is what practising the same level from
	// the start needs.
	if ((World != Levelsetdata.CurrentWorld) ||
	    Request["reload"].isTrue()) {
		Levelsetdata.CurrentWorld = World;
		StringCopy(Levelfilename, Filenames + 9 * World);

		if (Load_map(Levelfilename)) {
			return; // Load_map has set ErrorCode and Exit
		}

		// Both are a claim about a map object that has just been
		// replaced by a freshly loaded one: the boat being ridden and
		// the cloud being flown are gone with it, and a player still
		// marked as being on either would be moving by rules nothing on
		// this map plays by.
		SavePlayer.IsOnMapBoat = 0;
		SavePlayer.IsClouded = 0;
	}

	place(Map_data.PlayerX, Map_data.PlayerY);
}

// Moving a square at a time, off the roads and through the locks.
//
// The game's own map input is switched off while this is on, because the two
// cannot share the player: Handle_player_map() moves them a few pixels a frame
// along a road it has checked, and something else picking them up mid-step is
// how they end up inside a mountain. Esc is read before the switch takes
// effect, so the mid-game menu is still there; the jump key is not, so leaving
// a level entrance is a matter of turning roaming off and pressing it.
static void roam()
{
	// Passified is the game's own word for a frame the player does not get
	// to act on - it is what the map already does while walking them back
	// from a level they died in - and Gameloop() zeroes the keystate on one.
	// It does that after its own scan, which is why the keys are read here
	// rather than left to it.
	Player.Passified = 1;

	ScanKeys();

	// The step the map was in the middle of belongs to the road the player
	// is about to leave. Only on the way in: doing it every frame would
	// cost the mid-game menu its input, since a frame with Offset set is one
	// Gameloop() does not scan on.
	if (!Roaming) {
		Roaming = true;
		Roam_delay = 0;

		place(SavePlayer.MapX, SavePlayer.MapY);
	}

	int16_t Dx = (Keystate.right ? 1 : 0) - (Keystate.left ? 1 : 0);
	int16_t Dy = (Keystate.down ? 1 : 0) - (Keystate.up ? 1 : 0);

	if ((Dx == 0) && (Dy == 0)) {
		// Let go of, so the next press moves at once rather than
		// finishing out a repeat the player has stopped asking for.
		Roam_delay = 0;
		return;
	}

	if (Roam_delay > 0) {
		Roam_delay--;
		return;
	}

	Roam_delay = Roam_repeat;

	int16_t X = SavePlayer.MapX + Dx * 16;
	int16_t Y = SavePlayer.MapY + Dy * 16;

	// The edges of the map, and the only thing roaming will not cross. Off
	// the map is not a place: the tile lookups index a flat array with no
	// bounds of their own, so a square outside it reads somebody else's
	// memory as terrain.
	if ((X < 0) || (X >= (Map_data.Width * 16)) || (Y < 0) ||
	    (Y >= (Map_data.Height * 16))) {
		return;
	}

	SavePlayer.MapX = X;
	SavePlayer.MapY = Y;

	// Kept level with the player, so that dying in a level entered after a
	// roam returns them to where they roamed to rather than to the last
	// square the game itself thought they had earned.
	SavePlayer.PrevCompX = X;
	SavePlayer.PrevCompY = Y;

	Map_plane.p.force_update = 1;
}

// Marks the level being stood on as beaten, exactly as the map does when the
// player comes out of one having won - see the Exit == 2 arm of
// Handle_player_map(), which is the code this mirrors: a castle falls down and
// unlocks every door of its kind, and anything else becomes a piece of road.
//
// What it deliberately does not do is the rest of that arm: the coins counted
// on the way out, and the map event they can trigger - a card game, a money
// ship, a hidden mushroom house. Those are the reward for having played the
// level, they are read out of the level's own data, and the level has not been
// loaded. Skipping a level is not the same as having beaten it, and this is
// where the two part company.
static void clear_level()
{
	uint8_t Tile = Get_map_tile(SavePlayer.MapX, SavePlayer.MapY);

	if ((Tile < levels_low) || (Tile > levels_high)) {
		return;
	}

	uint8_t Cleared;

	if ((Tile == small_castle) || (Tile == small_castle_2)) {
		Cleared = (Map_data.Color == 2 ? demolished_castle_dark :
						 demolished_castle);

		// The doors this castle held shut, wherever they are on the
		// map: the game replaces them all with road, and which kind of
		// door it is is which castle this was.
		Replace_map_tile((Tile == small_castle) ? locked_door :
							  locked_door_2);
	} else {
		Cleared = M_tile;
	}

	Put_map_tile(SavePlayer.MapX, SavePlayer.MapY, Cleared);

	SavePlayer.PrevCompX = SavePlayer.MapX;
	SavePlayer.PrevCompY = SavePlayer.MapY;

	// The square is a road now and leads on in every direction it ever did.
	SavePlayer.L = 0;

	Map_plane.p.force_update = 1;
}

// Puts back every square that started out as this tile. The inverse of
// Replace_map_tile(), which is what turns a kind of locked door into road when
// the castle holding it shut falls down.
static void restore_tiles(uint8_t Tile)
{
	const uint8_t *Tiles = original_map();

	if (!Tiles) {
		return;
	}

	for (int16_t C = 0; C < Map_data.Height * Map_data.Width; C++) {
		if (Tiles[C] == Tile) {
			Put_map_tile((C % Map_data.Width) * 16,
				     (C / Map_data.Width) * 16, Tile);
		}
	}
}

// Puts a beaten level back, so that it can be played again: the square becomes
// the entrance the map file says it was, and a castle takes its doors back with
// it. The undo of clear_level() above, and the reason both are here - practising
// a level twice means the map has to be able to forget it was beaten, and
// reloading the whole map to arrange that would forget everything else too.
//
// What it does not do is put back what beating the level gave: the coins, the
// score, the item, the mushroom house that appeared somewhere else on the map.
// Clearing a level never granted those either - see clear_level() - so between
// the two the map returns to where it was, and the player keeps what they have.
static void unclear_level()
{
	// Nothing to put back where the level is still standing.
	uint8_t Live = Get_map_tile(SavePlayer.MapX, SavePlayer.MapY);

	if ((Live >= levels_low) && (Live <= levels_high)) {
		return;
	}

	uint8_t Was = original_tile(SavePlayer.MapX, SavePlayer.MapY);

	if ((Was < levels_low) || (Was > levels_high)) {
		return;
	}

	Put_map_tile(SavePlayer.MapX, SavePlayer.MapY, Was);

	// A castle's fall took every door of its kind with it, wherever they
	// were on the map, so putting the castle back puts those back too.
	// Where a map has two castles of the same kind - none of the shipped
	// worlds do - this re-locks doors the other one had opened as well; the
	// map file cannot tell which castle opened which door, because it was
	// never asked to remember.
	if ((Was == small_castle) || (Was == small_castle_2)) {
		restore_tiles((Was == small_castle) ? locked_door :
						      locked_door_2);
	}

	// The square leads where an unbeaten level leads again, which is the
	// same thing the game says on every arrival.
	SavePlayer.L = 0;

	Map_plane.p.force_update = 1;
}

// The powerup, set the way the map's own item list sets it - see the switch in
// Handle_player_map(), which is the code this mirrors. Sprite and mask bases
// come in pairs the renderer indexes blindly, and fire is the one powerup whose
// two differ, so each case names both rather than deriving one from the other.
static void power(const emscripten::val &Request)
{
	uint8_t Attribs = static_cast<uint8_t>(SavePlayer.Attribs);

	switch (Request["level"].as<int>()) {
	case Power_small:
		SavePlayer.Life = 1;
		Player.Height = Player.Height2 = 16;
		SavePlayer.Maskbase = SavePlayer.Spritebase =
			(Player.Face == 1) ? 1 : 0;
		Attribs &= ~(Attrib_fire | Attrib_racoon | Attrib_pwing);
		break;

	case Power_large:
		SavePlayer.Life = 2;
		Player.Height = Player.Height2 = 27;
		SavePlayer.Maskbase = SavePlayer.Spritebase =
			(Player.Face == 1) ? 3 : 2;
		Attribs &= ~(Attrib_fire | Attrib_racoon | Attrib_pwing);
		break;

	case Power_fire:
		SavePlayer.Life = 3;
		Player.Height = Player.Height2 = 27;
		SavePlayer.Spritebase = (Player.Face == 1) ? 5 : 4;
		SavePlayer.Maskbase = (Player.Face == 1) ? 3 : 2;
		Attribs |= Attrib_fire;
		Attribs &= ~(Attrib_racoon | Attrib_pwing);
		break;

	case Power_racoon:
		SavePlayer.Life = 3;
		Player.Height = Player.Height2 = 27;
		SavePlayer.Maskbase = SavePlayer.Spritebase =
			(Player.Face == 1) ? 3 : 2;
		Attribs |= Attrib_racoon;
		Attribs &= ~Attrib_fire;
		break;

	default:
		return;
	}

	// The P-wing is the racoon suit that never has to run up to fly, which
	// is why the item list's case for it falls through into the leaf's. Here
	// the suit has already been set above, so asking for the P-wing on top
	// of a powerup that is not the suit is a contradiction, and the suit is
	// what wins it.
	if (Request["pwing"].isTrue()) {
		Attribs |= Attrib_racoon | Attrib_pwing;
		Attribs &= ~Attrib_fire;
	} else {
		Attribs &= ~Attrib_pwing;
	}

	// Immortal is a frame counter the star shares with the flicker after
	// being wounded, so taking the star away has to take the count with it -
	// otherwise the player walks off the map's star still invulnerable, on a
	// timer nothing is going to stop.
	if (Request["star"].isTrue()) {
		Attribs |= Attrib_star;
		Player.Immortal = star_immortal_time;
	} else {
		Attribs &= ~Attrib_star;
		Player.Immortal = 0;
	}

	SavePlayer.Attribs = Attribs;
}

static void items(const emscripten::val &Request)
{
	emscripten::val Items = Request["items"];

	int16_t Count = Items["length"].as<int>();

	for (int16_t C = 0; C < itemlist_length; C++) {
		SavePlayer.Itemlist[C] =
			(C < Count) ? static_cast<char>(Items[C].as<int>()) : 0;
	}

	// The list is drawn from wherever the cursor was, and the item it was on
	// is not the item that is there now. The game itself zeroes this every
	// time the list is opened, for the same reason.
	Player.Curr_item = 0;
}

void poll()
{
	if (!hooked()) {
		return;
	}

	emscripten::val Request = emscripten::val::module_property(
		"onPracticeRequest")(snapshot());

	// Roaming is the one thing here that is a state rather than an event, so
	// the shell says it on every frame it is on and this reads it on every
	// frame it is asked: nothing is remembered on this side that the panel
	// could then disagree with, and a game started while the box is ticked
	// picks it up on its first frame of map.
	if (present(Request) && Request["roam"].isTrue()) {
		roam();
	} else {
		Roaming = false;
	}

	// Nothing else queued, which is every frame but the ones the player
	// pressed a button on the panel.
	if (!present(Request)) {
		return;
	}

	// Order matters where a warp reloads a map: the powerup and the item
	// list are the player's and survive it, and setting them after means a
	// panel that asked for both in one go gets both, whichever way round the
	// player filled the form in.
	if (present(Request["warp"])) {
		warp(Request["warp"]);
	}

	if (Request["clear"].isTrue()) {
		clear_level();
	}

	if (Request["unclear"].isTrue()) {
		unclear_level();
	}

	if (present(Request["power"])) {
		power(Request["power"]);
	}

	if (present(Request["items"])) {
		items(Request);
	}
}

bool spare_life()
{
	if (!hooked()) {
		return false;
	}

	emscripten::val Hook =
		emscripten::val::module_property("onPracticeGameOver");

	if (Hook.typeOf().as<std::string>() != "function" || !Hook().isTrue()) {
		return false;
	}

	// One, as though the death that took the last of them had not counted:
	// the game goes on exactly as it would have with a life in hand, and the
	// counter on the map says what it has always said about the next one.
	SavePlayer.Lives = 1;

	return true;
}

}
