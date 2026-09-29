//! The route search: the fastest way through a category, as the history says
//! the player plays it. See lib.rs for the model it searches.

use std::cmp::Ordering;
use std::collections::BinaryHeap;

use rustc_hash::FxHashMap;

use crate::cost::{Coster, Costing};
use crate::input::{Input, Settings, Start, Stats, WorldMap};
use crate::items::*;

// ---------------------------------------------------------------------------
// The category's rules

/// What a category asks of a route, as the search needs it said.
pub struct Rules {
    /// The world whose end is the end of the run, counted from zero.
    pub last_world: usize,
    /// Whether the whistle may be spent.
    pub warps: bool,
    /// Whether every stage and Bros. of a world has to be beaten before its end.
    pub everything: bool,
}

pub fn rules_for(category: &str, worlds: usize) -> Rules {
    let last = worlds - 1;
    match category {
        "any" => Rules {
            last_world: last,
            warps: true,
            everything: false,
        },
        // A warp would skip a world whose every stage the rules ask for, so
        // there is nothing for one to do in 100%.
        "100" => Rules {
            last_world: last,
            warps: false,
            everything: true,
        },
        "world-1" => Rules {
            last_world: 0,
            warps: false,
            everything: false,
        },
        _ => Rules {
            last_world: last,
            warps: false,
            everything: false,
        },
    }
}

// ---------------------------------------------------------------------------
// The maps, made ready to search

pub struct Link {
    pub to: usize,
    pub ms: f64,
    /// The kind of locked door on the way, or 0.
    pub door: usize,
    /// The rock's bit, or 0.
    pub rock: u32,
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum ThingKind {
    Stage,
    Bros,
    Rock,
}

pub struct Thing {
    pub kind: ThingKind,
    pub bit: u32,
    pub node: usize,
    /// The place key visits are grouped by, for stages and Bros.
    pub place: String,
    /// The slots it hands over, as treasure() gives them.
    pub treasure: Vec<usize>,
}

pub struct World {
    pub links: Vec<Vec<Link>>,
    /// Each node's stage, if it is one - the castle and Bowser's included - by thing index.
    pub stage: Vec<Option<usize>>,
    pub bros: Vec<usize>,
    pub rocks: Vec<usize>,
    /// The fortresses that open each kind of door, as a mask.
    pub opens: [u32; 3],
    pub start: usize,
    /// The castle or Bowser's: the end of the world.
    pub last: Vec<bool>,
    /// A pipe stage: where beating it comes out.
    pub exit: Vec<Option<usize>>,
    /// A chest in it that is a pick of three.
    pub random_chest: Vec<bool>,
    pub castle: Vec<bool>,
    /// What beating the castle gives, as slots.
    pub reward: Vec<usize>,
    /// Where the whistle goes: the world, and the walk in the warp zone.
    pub warps: Vec<(usize, f64)>,
    /// Each node's links in, as (from, index in from's links), and the pipe
    /// stages whose far end it is: the map walked backwards, for the floor.
    pub into: Vec<Vec<(usize, usize)>>,
    pub exits_into: Vec<Vec<usize>>,
}

fn prepare(map: &WorldMap, settings: &Settings, things: &mut Vec<Thing>) -> World {
    let mut bits = 0;
    let mut thing = |kind, node, place: String, treasure: Vec<usize>| {
        things.push(Thing {
            kind,
            bit: 1 << bits,
            node,
            place,
            treasure,
        });
        bits += 1;
        things.len() - 1
    };

    let stage: Vec<Option<usize>> = map
        .nodes
        .iter()
        .map(|n| {
            n.level.map(|level| {
                thing(
                    ThingKind::Stage,
                    n.id,
                    format!("L{}.{}", map.world, level),
                    vec![],
                )
            })
        })
        .collect();
    // A mushroom house's chest is a pick of three, so no house is anywhere a
    // route has reason to go, and a Bros. that drops one drops nothing.
    let bros: Vec<usize> = map
        .bros
        .iter()
        .map(|b| {
            thing(
                ThingKind::Bros,
                b.node,
                format!("M{}.{}", map.world, b.monster),
                treasure(Some(&b.treasure)),
            )
        })
        .collect();
    let rocks: Vec<usize> = map
        .rocks
        .iter()
        .map(|r| thing(ThingKind::Rock, r.from, String::new(), vec![]))
        .collect();

    let mut opens = [0u32; 3];
    for n in &map.nodes {
        if let Some(kind) = n.opens {
            opens[kind] |= things[stage[n.id].unwrap()].bit;
        }
    }

    // The way through a pipe that plays a stage - 7-Pipe - is the stage, and
    // is taken as one (see actions()), so it is no walk.
    let mut links: Vec<Vec<Link>> = map.nodes.iter().map(|_| Vec::new()).collect();
    for e in &map.edges {
        if e.by == "pipe" && map.nodes[e.a].exit.is_some() {
            continue;
        }
        links[e.a].push(Link {
            to: e.b,
            ms: if e.by == "pipe" {
                settings.pipe_ms
            } else {
                e.tiles * settings.ms_per_tile
            },
            door: e.door.unwrap_or(0),
            rock: e.rock.map_or(0, |r| things[rocks[r]].bit),
        });
    }

    World {
        stage,
        bros,
        rocks,
        opens,
        start: map.start,
        last: map
            .nodes
            .iter()
            .map(|n| n.kind == "castle" || n.kind == "bowser")
            .collect(),
        exit: map.nodes.iter().map(|n| n.exit).collect(),
        random_chest: map
            .nodes
            .iter()
            .map(|n| n.chests.iter().any(|c| c == "random"))
            .collect(),
        castle: map.nodes.iter().map(|n| n.kind == "castle").collect(),
        reward: treasure(map.reward.as_deref()),
        warps: map.warps.iter().map(|w| (w.world, w.tiles)).collect(),
        into: {
            let mut into = vec![Vec::new(); map.nodes.len()];
            for (from, list) in links.iter().enumerate() {
                for (i, link) in list.iter().enumerate() {
                    into[link.to].push((from, i));
                }
            }
            into
        },
        exits_into: {
            let mut into = vec![Vec::new(); map.nodes.len()];
            for n in &map.nodes {
                if let Some(exit) = n.exit {
                    into[exit].push(n.id);
                }
            }
            into
        },
        links,
    }
}

// ---------------------------------------------------------------------------
// States

#[derive(Clone, Copy, PartialEq, Eq, Hash, Debug)]
pub struct State {
    pub world: u8,
    pub done: u32,
    pub pos: u8,
    pub power: u8,
    pub inv: u32,
}

impl State {
    /// Everything in one number: 13 bits of items, 2 of power, 6 of where the
    /// player stands, 24 of what is done, and the world.
    pub fn key(self) -> u64 {
        (self.inv as u64)
            | (self.power as u64) << 13
            | (self.pos as u64) << 15
            | (self.done as u64) << 21
            | (self.world as u64) << 45
    }

    pub fn from_key(key: u64) -> State {
        State {
            inv: (key & 0x1fff) as u32,
            power: (key >> 13 & 3) as u8,
            pos: (key >> 15 & 63) as u8,
            done: (key >> 21 & 0xff_ffff) as u32,
            world: (key >> 45) as u8,
        }
    }
}

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Kind {
    Stage,
    Cloud,
    Bros,
    Rock,
    Warp,
}

impl Kind {
    pub fn name(self) -> &'static str {
        match self {
            Kind::Stage => "stage",
            Kind::Cloud => "cloud",
            Kind::Bros => "bros",
            Kind::Rock => "rock",
            Kind::Warp => "warp",
        }
    }
}

/// What an action spends from the list: at most a power item and a star.
#[derive(Clone, Copy, PartialEq, Eq, Debug, Default)]
pub struct Use {
    pub len: u8,
    pub slots: [u8; 2],
}

impl Use {
    pub fn one(slot: usize) -> Use {
        Use {
            len: 1,
            slots: [slot as u8, 0],
        }
    }

    pub fn and(self, slot: usize) -> Use {
        let mut out = self;
        out.slots[out.len as usize] = slot as u8;
        out.len += 1;
        out
    }

    pub fn iter(self) -> impl Iterator<Item = usize> {
        self.slots
            .into_iter()
            .take(self.len as usize)
            .map(|s| s as usize)
    }
}

/// Something to do next, and what it leads to.
#[derive(Clone, Copy, Debug)]
pub struct Action {
    pub kind: Kind,
    /// A warp: the world it goes to.
    pub warp: Option<usize>,
    pub node: usize,
    /// Where the player ends up: the thing itself, past a clouded stage, or
    /// out the far end of a pipe stage.
    pub to: usize,
    pub thing: Option<usize>,
    pub spends: Use,
    pub entry: Option<Entry>,
    pub walk_ms: f64,
    /// Everything else it takes: the stage, the fight.
    pub do_ms: f64,
    /// The costing aimed for, by its index in the coster's list for the thing and entry.
    pub costing: Option<usize>,
    /// The state it leaves.
    pub next: State,
}

/// Thrown when a search has looked at more states than it is allowed.
#[derive(Debug)]
pub struct TooBig;

#[derive(Clone, Copy)]
struct Loadout {
    entry: Entry,
    spends: Use,
    inv: u32,
}

/// No more than there are: walked in as is, or on each of four power items,
/// and each of those with a star or without.
const MOST_LOADOUTS: usize = 10;

/// What each power item makes of the player; see Handle_player_map().
fn spend_to(slot: usize, p: u8) -> Option<u8> {
    match slot {
        MUSHROOM => (p == SMALL).then_some(SUPER),
        FIRE_FLOWER => (p != FIRE).then_some(FIRE),
        LEAF => (p != RACOON).then_some(RACOON),
        PWING => Some(RACOON),
        _ => None,
    }
}

/// What the floor keeps count of: what opens the map up.
const OPENERS: [usize; 3] = [CLOUD, WHISTLE, HAMMER];

const SPENDS: [usize; 4] = [MUSHROOM, FIRE_FLOWER, LEAF, PWING];
const POWER_UPS: [usize; 3] = [MUSHROOM, FIRE_FLOWER, LEAF];

/// Floors are kept by what the floor lets the player hold, one layer each:
/// clouds and whistles, counted; whether they have a hammer; and which kinds
/// of locked door their way has opened, as a mask.
const HELD: usize = ((CAP + 1) * (CAP + 1) * 2) as usize;
const LAYERS: usize = HELD * 4;

fn layer(clouds: u32, whistles: u32, hammer: u32, doors: u32) -> usize {
    (((doors * 2 + hammer) * (CAP + 1) + whistles) * (CAP + 1) + clouds) as usize
}

struct HeapItem {
    f: f64,
    g: f64,
    s: State,
}

impl PartialEq for HeapItem {
    fn eq(&self, other: &Self) -> bool {
        self.f == other.f
    }
}
impl Eq for HeapItem {}
impl PartialOrd for HeapItem {
    fn partial_cmp(&self, other: &Self) -> Option<Ordering> {
        Some(self.cmp(other))
    }
}
impl Ord for HeapItem {
    // Least f first, out of a max-heap.
    fn cmp(&self, other: &Self) -> Ordering {
        other.f.total_cmp(&self.f)
    }
}

pub struct Search<'a> {
    pub settings: &'a Settings,
    pub stats: &'a Stats,
    pub rules: Rules,
    pub worlds: Vec<World>,
    pub things: Vec<Thing>,
    pub coster: Coster<'a>,
    floors: Coster<'a>,
    budget: usize,
    /// Everything the rules want beaten in a world before its end, as a mask.
    required: Vec<u32>,
    /// How many of each item could be worth holding from world `w` on.
    useful: Vec<[u32; 8]>,
    helps_memo: Vec<i8>,
    walked: FxHashMap<u64, Box<[f64]>>,
    memo: FxHashMap<u64, f64>,
    fastest_memo: Vec<f64>,
    /// What each stage has been seen to hand over, of what opens the map up.
    gives: Vec<[u32; 3]>,
    from_start: Vec<[f64; HELD]>,
    /// The floor by world and what is done in it: by what is held, then by square.
    floors_at: FxHashMap<u64, Box<[f64]>>,
    /// How many states have been taken, and in how many searches: for
    /// seeing what a change to the search did.
    pub expanded: usize,
    pub searches: usize,
}

impl<'a> Search<'a> {
    pub fn new(input: &'a Input, budget: usize) -> Search<'a> {
        let settings = &input.settings;
        let stats = &input.stats;
        let rules = rules_for(&settings.category, input.maps.len());
        let mut things = Vec::new();
        let worlds: Vec<World> = input
            .maps
            .iter()
            .map(|m| prepare(m, settings, &mut things))
            .collect();

        let required = worlds
            .iter()
            .map(|w| {
                if !rules.everything {
                    return 0;
                }
                let mut mask = 0;
                for (node, st) in w.stage.iter().enumerate() {
                    if let Some(t) = st {
                        if !w.last[node] {
                            mask |= things[*t].bit;
                        }
                    }
                }
                for &b in &w.bros {
                    mask |= things[b].bit;
                }
                mask
            })
            .collect();

        let n = things.len();
        let mut search = Search {
            settings,
            stats,
            rules,
            worlds,
            coster: Coster::new(stats, settings, n),
            floors: Coster::new(stats, settings, n),
            things,
            budget,
            required,
            useful: Vec::new(),
            helps_memo: vec![-1; n * 256],
            walked: FxHashMap::default(),
            memo: FxHashMap::default(),
            fastest_memo: vec![f64::NAN; n],
            gives: Vec::new(),
            from_start: Vec::new(),
            floors_at: FxHashMap::default(),
            expanded: 0,
            searches: 0,
        };
        search.useful = search.work_out_useful();
        search.gives = (0..n)
            .map(|t| OPENERS.map(|slot| search.gives(t, slot) as u32))
            .collect();
        search.prepare_floor();
        search
    }

    fn costs(&mut self, thing: usize, entry: Entry) -> &[Costing] {
        let place = &self.things[thing].place;
        self.coster.cost(thing, place, entry)
    }

    /// Whether walking into a thing as `to` rather than `from` is worth
    /// anything: a better time, or leaving as something else.
    fn helps(&mut self, thing: usize, from: Entry, to: Entry) -> bool {
        let id = (thing * 16 + from.code()) * 16 + to.code();
        if self.helps_memo[id] < 0 {
            self.costs(thing, from);
            self.costs(thing, to);
            let before = self.coster.known(thing, from).unwrap();
            let after = self.coster.known(thing, to).unwrap();
            let found = after.iter().any(|c| {
                let had = before
                    .iter()
                    .rev()
                    .find(|b| b.power == c.power && b.gains == c.gains)
                    .map_or(f64::INFINITY, |b| b.ms);
                c.ms < had
            });
            self.helps_memo[id] = found as i8;
        }
        self.helps_memo[id] == 1
    }

    fn places_from(&self, w: usize) -> Vec<usize> {
        let mut out = Vec::new();
        for world in self.worlds.iter().take(self.rules.last_world + 1).skip(w) {
            out.extend(world.stage.iter().flatten().copied());
            out.extend(world.bros.iter().copied());
        }
        out
    }

    /// How many of an item could be worth spending from world `w` on: one for
    /// each place left that it could make faster, each rock left for a
    /// hammer, and so on - no more than CAP.
    fn work_out_useful(&mut self) -> Vec<[u32; 8]> {
        let n = self.worlds.len();
        // The longest string of warps the whistle can still make from each
        // world on: a whistle more than that has nowhere to go.
        let mut warps_left = vec![0u32; n];
        for w in (0..n).rev() {
            let mut most = if w < self.rules.last_world {
                warps_left[w + 1]
            } else {
                0
            };
            if self.rules.warps {
                for &(to, _) in &self.worlds[w].warps {
                    if to > w && to <= self.rules.last_world {
                        most = most.max(1 + warps_left[to]);
                    }
                }
            }
            warps_left[w] = most;
        }

        let mut out = Vec::with_capacity(n);
        for w in 0..n {
            let places = self.places_from(w);
            let later = w..(self.rules.last_world + 1).max(w);
            let mut row = [0u32; 8];
            for slot in 0..SLOTS.len() {
                let most: u32 = match slot {
                    STAR => places
                        .iter()
                        .filter(|&&t| {
                            (0..4).any(|p| {
                                self.helps(
                                    t,
                                    Entry::plain(p),
                                    Entry {
                                        power: p,
                                        star: true,
                                        pwing: false,
                                    },
                                )
                            })
                        })
                        .count() as u32,
                    CLOUD => {
                        if self.rules.everything {
                            0
                        } else {
                            later
                                .clone()
                                .map(|x| &self.worlds[x])
                                .map(|world| {
                                    (0..world.stage.len())
                                        .filter(|&i| {
                                            world.stage[i].is_some()
                                                && !world.last[i]
                                                && world.exit[i].is_none()
                                        })
                                        .count() as u32
                                })
                                .sum()
                        }
                    }
                    HAMMER => later
                        .clone()
                        .map(|x| self.worlds[x].rocks.len() as u32)
                        .sum(),
                    WHISTLE => warps_left[w],
                    _ => places
                        .iter()
                        .filter(|&&t| {
                            (0..4).any(|p| match spend_to(slot, p) {
                                Some(to) => self.helps(
                                    t,
                                    Entry::plain(p),
                                    Entry {
                                        power: to,
                                        star: false,
                                        pwing: slot == PWING,
                                    },
                                ),
                                None => false,
                            })
                        })
                        .count() as u32,
                };
                row[slot] = most.min(CAP);
            }
            out.push(row);
        }
        out
    }

    // -----------------------------------------------------------------------
    // Moving on

    /// Shortest walks from the player's square, over what is open now.
    fn walks(&mut self, s: State) -> &[f64] {
        let key = s.key() >> 15;
        if !self.walked.contains_key(&key) {
            let dist = self.walk(s);
            self.walked.insert(key, dist);
        }
        &self.walked[&key]
    }

    fn walk(&self, s: State) -> Box<[f64]> {
        let world = &self.worlds[s.world as usize];
        let n = world.links.len();
        let mut dist = vec![f64::INFINITY; n].into_boxed_slice();
        let mut settled = vec![false; n];
        dist[s.pos as usize] = 0.0;

        loop {
            let mut u = usize::MAX;
            for i in 0..n {
                if !settled[i] && dist[i] < f64::INFINITY && (u == usize::MAX || dist[i] < dist[u])
                {
                    u = i;
                }
            }
            if u == usize::MAX {
                break;
            }
            settled[u] = true;

            // A stage not yet beaten is walked onto and no further - the
            // player's own square included, which is one only when a cloud
            // has just set them down on it.
            if !self.open(s.world as usize, s.done, u) {
                continue;
            }
            for link in &world.links[u] {
                if link.door != 0 && s.done & world.opens[link.door] == 0 {
                    continue;
                }
                if link.rock != 0 && s.done & link.rock == 0 {
                    continue;
                }
                let d = dist[u] + link.ms;
                if d < dist[link.to] {
                    dist[link.to] = d;
                }
            }
        }
        dist
    }

    /// Walkable to and past, as opposed to walkable onto.
    fn open(&self, w: usize, done: u32, node: usize) -> bool {
        let world = &self.worlds[w];
        match world.stage[node] {
            None => true,
            Some(t) => done & self.things[t].bit != 0 || world.exit[node].is_some(),
        }
    }

    /// The item list a clear leaves, given what it was seen to hand over,
    /// less what the maps already give on their own: a castle's reward, and a
    /// Bros.' treasure. A power-up out of a stage with a random chest in it is
    /// that chest, which is luck.
    fn handed(&self, w: usize, thing: usize, gains: &[usize], inv: u32) -> u32 {
        let t = &self.things[thing];
        if t.kind == ThingKind::Bros {
            return inv;
        }
        let world = &self.worlds[w];
        let castle = world.castle[t.node];
        let chest = world.random_chest[t.node];
        let mut out = inv;
        for &slot in gains {
            if (castle && world.reward.contains(&slot)) || (chest && POWER_UPS.contains(&slot)) {
                continue;
            }
            out = give(out, slot);
        }
        out
    }

    /// Items no further use can be made of dropped, and any more than there
    /// is use for.
    fn tidy(&self, world: usize, inv: u32) -> u32 {
        let Some(keep) = self.useful.get(world) else {
            return inv;
        };
        let mut out = inv;
        for slot in 0..SLOTS.len() {
            let c = count(out, slot);
            if c > keep[slot] {
                out -= (c - keep[slot]) * WEIGHT[slot];
            }
        }
        out
    }

    fn after(&self, s: State, done: u32, pos: usize, power: u8, inv: u32) -> State {
        State {
            world: s.world,
            done,
            pos: pos as u8,
            power,
            inv: self.tidy(s.world as usize, inv),
        }
    }

    pub fn end(&self) -> State {
        State {
            world: self.worlds.len() as u8,
            done: 0,
            pos: 0,
            power: 0,
            inv: 0,
        }
    }

    pub fn s0(&self, w: usize) -> State {
        State {
            world: w as u8,
            done: 0,
            pos: self.worlds.get(w).map_or(0, |x| x.start) as u8,
            power: 0,
            inv: 0,
        }
    }

    /// The things of world `w` that go by a place key: a stage has one per
    /// square it is entered from.
    fn things_at<'s>(&'s self, w: usize, place: &'s str) -> impl Iterator<Item = usize> + 's {
        let world = &self.worlds[w];
        world
            .stage
            .iter()
            .flatten()
            .chain(&world.bros)
            .copied()
            .filter(move |&t| self.things[t].place == place)
    }

    /// Where a run in progress stands, as a state; and the thing being played
    /// now, if one is, with what it was walked into as.
    ///
    /// Nothing reports a rock being broken, so one is taken as broken where
    /// the player has been somewhere that nothing else would have let them
    /// reach: see broken_rocks().
    pub fn start(&self, from: &Start) -> (State, Option<(usize, Entry)>) {
        let w = from.world;
        if w >= self.worlds.len() || w > self.rules.last_world {
            return (self.end(), None);
        }
        let power = power_index(&from.power);
        let mut inv = 0;
        for slot in from.items.iter().filter_map(|item| slot_of(item)) {
            inv = give(inv, slot);
        }

        let mut done = 0;
        for place in &from.done {
            for t in self.things_at(w, place) {
                done |= self.things[t].bit;
            }
        }

        let world = &self.worlds[w];
        let mut pos = world.start;
        if let Some(t) = from.at.as_deref().and_then(|at| self.things_at(w, at).next()) {
            let node = self.things[t].node;
            pos = node;
            if self.things[t].kind == ThingKind::Stage {
                // The castle beaten, and the next world not yet said to be
                // entered: its reward is in the list already, as the game
                // reports the list.
                if world.last[node] {
                    if w >= self.rules.last_world {
                        return (self.end(), None);
                    }
                    let next = self.s0(w + 1);
                    return (
                        State {
                            power,
                            inv: self.tidy(w + 1, inv),
                            ..next
                        },
                        None,
                    );
                }
                pos = world.exit[node].unwrap_or(node);
            }
        }
        let inside = from.inside.as_ref().and_then(|i| {
            let t = self.things_at(w, &i.place).next()?;
            let entry = Entry {
                power: power_index(&i.entry.power),
                star: i.entry.star,
                pwing: i.entry.pwing,
            };
            Some((t, entry))
        });
        // The thing being played was walked to as well.
        let been = inside.map(|(t, _)| self.things[t].node);
        done |= self.broken_rocks(w, done, pos, been, &from.rocks);

        let state = State {
            world: w as u8,
            done,
            pos: pos as u8,
            power,
            inv: self.tidy(w, inv),
        };
        (state, inside)
    }

    /// The rocks broken so far: first one on the way to each of `hinted`,
    /// the nearest to it of those the player could have got to; then any
    /// more the player must have broken to have got to `pos`, to `been` if
    /// given, and to everything they have beaten - one at a time, each one on
    /// the edge of where they could otherwise reach, and one that opens the
    /// way to somewhere they have been before any that does not.
    ///
    /// A stage not beaten is taken as no obstacle here: a cloud may have
    /// carried the player over it, and it is no reason to think a rock was
    /// broken to get round it.
    fn broken_rocks(
        &self,
        w: usize,
        done: u32,
        pos: usize,
        been: Option<usize>,
        hinted: &[String],
    ) -> u32 {
        let world = &self.worlds[w];
        let mut needed: Vec<usize> = [pos].into_iter().chain(been).collect();
        for &t in world.stage.iter().flatten().chain(&world.bros) {
            if done & self.things[t].bit != 0 {
                needed.push(self.things[t].node);
            }
        }

        // The ways on from a node with these open, and the rocks in the way:
        // the roads, and out the far end of a pipe stage that has been beaten.
        let ways = |u: usize, open: u32, out: &mut Vec<(usize, f64)>, rocks: &mut Vec<(u32, usize)>| {
            out.clear();
            for link in &world.links[u] {
                if link.door != 0 && open & world.opens[link.door] == 0 {
                    continue;
                }
                if link.rock != 0 && open & link.rock == 0 {
                    rocks.push((link.rock, u));
                    continue;
                }
                out.push((link.to, link.ms));
            }
            if let (Some(exit), Some(t)) = (world.exit[u], world.stage[u])
                && open & self.things[t].bit != 0
            {
                out.push((exit, 0.0));
            }
        };

        // What can be reached from the start with these open, and the rocks
        // at its edge, each with the square it is broken from.
        let reach = |open: u32| -> (Vec<bool>, Vec<(u32, usize)>) {
            let mut reached = vec![false; world.links.len()];
            let mut edge = Vec::new();
            let mut next = Vec::new();
            let mut stack = vec![world.start];
            reached[world.start] = true;
            while let Some(u) = stack.pop() {
                ways(u, open, &mut next, &mut edge);
                for &(to, _) in &next {
                    if !reached[to] {
                        reached[to] = true;
                        stack.push(to);
                    }
                }
            }
            (reached, edge)
        };
        // How far each square is from `from`, with these open.
        let far = |open: u32, from: usize| -> Vec<f64> {
            let n = world.links.len();
            let mut dist = vec![f64::INFINITY; n];
            let mut settled = vec![false; n];
            let mut next = Vec::new();
            let mut ignored = Vec::new();
            dist[from] = 0.0;
            loop {
                let Some(u) = (0..n)
                    .filter(|&i| !settled[i] && dist[i] < f64::INFINITY)
                    .min_by(|&x, &y| dist[x].total_cmp(&dist[y]))
                else {
                    return dist;
                };
                settled[u] = true;
                ways(u, open, &mut next, &mut ignored);
                for &(to, ms) in &next {
                    dist[to] = dist[to].min(dist[u] + ms);
                }
            }
        };
        let missing = |reached: &[bool]| needed.iter().filter(|&&n| !reached[n]).count();

        let mut broken = 0;
        for place in hinted {
            let Some(t) = self.things_at(w, place).next() else {
                continue;
            };
            let target = self.things[t].node;
            let (_, edge) = reach(done | broken);
            let best = edge
                .iter()
                .map(|&(rock, from)| (rock, far(done | broken | rock, target)[from]))
                .filter(|&(_, d)| d < f64::INFINITY)
                .min_by(|x, y| x.1.total_cmp(&y.1));
            if let Some((rock, _)) = best {
                broken |= rock;
            }
        }

        loop {
            let (reached, edge) = reach(done | broken);
            let left = missing(&reached);
            if left == 0 || edge.is_empty() {
                return broken;
            }
            let rock = edge
                .iter()
                .map(|&(rock, _)| rock)
                .find(|&r| missing(&reach(done | broken | r).0) < left)
                .unwrap_or(edge[0].0);
            broken |= rock;
        }
    }

    /// Finishing the thing being played now, walked into as `entry`: every
    /// way out the history has for it, the best `keep` of them ranked as
    /// choices() ranks, by the time each leaves to go.
    pub fn played(
        &mut self,
        s: State,
        thing: usize,
        entry: Entry,
        keep: usize,
    ) -> Result<Vec<(Action, f64)>, TooBig> {
        let w = s.world as usize;
        let (kind, node, bit) = {
            let t = &self.things[thing];
            (t.kind, t.node, t.bit)
        };
        let overhead = self.settings.overhead_ms;
        let mut with_treasure = s.inv;
        for &slot in &self.things[thing].treasure {
            with_treasure = give(with_treasure, slot);
        }

        self.costs(thing, entry);
        let list = self.coster.known(thing, entry).unwrap().to_vec();
        let actions: Vec<Action> = list
            .iter()
            .enumerate()
            .map(|(i, c)| {
                let (kind, to, next) = if kind == ThingKind::Bros {
                    let next = self.after(s, s.done | bit, node, c.power, with_treasure);
                    (Kind::Bros, node, next)
                } else {
                    let lands = self.worlds[w].exit[node].unwrap_or(node);
                    let inv = self.handed(w, thing, &c.slots, s.inv);
                    let next = if self.worlds[w].last[node] {
                        self.next_world(w, c.power, inv)
                    } else {
                        self.after(s, s.done | bit, lands, c.power, inv)
                    };
                    (Kind::Stage, lands, next)
                };
                Action {
                    kind,
                    warp: None,
                    node,
                    to,
                    thing: Some(thing),
                    spends: Use::default(),
                    entry: Some(entry),
                    walk_ms: 0.0,
                    do_ms: c.ms + overhead,
                    costing: Some(i),
                    next,
                }
            })
            .collect();

        let mut out = Vec::with_capacity(actions.len());
        for a in actions {
            let total = self.lookahead(&a, f64::INFINITY)?;
            out.push((a, total));
        }
        out.sort_by(|x, y| x.1.total_cmp(&y.1));
        out.truncate(keep);
        Ok(out)
    }

    /// The start of the next world, with the castle's reward in hand - or the
    /// end, where this world was the last the rules ask for.
    fn next_world(&self, w: usize, power: u8, inv: u32) -> State {
        if w >= self.rules.last_world {
            return self.end();
        }
        let next = self.s0(w + 1);
        let mut with_reward = inv;
        for &slot in &self.worlds[w].reward {
            with_reward = give(with_reward, slot);
        }
        State {
            power,
            inv: self.tidy(w + 1, with_reward),
            ..next
        }
    }

    /// What can be walked into a thing as, and what it spends.
    fn loadouts(&mut self, s: State, thing: usize) -> ([Loadout; MOST_LOADOUTS], usize) {
        let power = s.power;
        let plain = Entry::plain(power);
        let mut base = Vec::with_capacity(5);
        base.push((power, false, Use::default(), s.inv));
        let items = self.settings.items;

        if items {
            for slot in SPENDS {
                let Some(to) = spend_to(slot, power) else {
                    continue;
                };
                if count(s.inv, slot) == 0 {
                    continue;
                }
                if !self.helps(
                    thing,
                    plain,
                    Entry {
                        power: to,
                        star: false,
                        pwing: slot == PWING,
                    },
                ) {
                    continue;
                }
                base.push((to, slot == PWING, Use::one(slot), take(s.inv, slot)));
            }
        }

        let none = Loadout {
            entry: Entry::plain(0),
            spends: Use::default(),
            inv: 0,
        };
        let mut out = [none; MOST_LOADOUTS];
        let mut n = 0;
        for (power, pwing, spends, inv) in base {
            let without = Entry {
                power,
                star: false,
                pwing,
            };
            out[n] = Loadout {
                entry: without,
                spends,
                inv,
            };
            n += 1;
            if items
                && count(inv, STAR) > 0
                && self.helps(
                    thing,
                    without,
                    Entry {
                        star: true,
                        ..without
                    },
                )
            {
                out[n] = Loadout {
                    entry: Entry {
                        star: true,
                        ..without
                    },
                    spends: spends.and(STAR),
                    inv: take(inv, STAR),
                };
                n += 1;
            }
        }
        (out, n)
    }

    pub fn actions(&mut self, s: State) -> Vec<Action> {
        let mut out = Vec::new();
        self.actions_into(s, &mut out);
        out
    }

    /// The same, into a list kept for the purpose: the search asks this of
    /// every state it takes.
    fn actions_into(&mut self, s: State, out: &mut Vec<Action>) {
        out.clear();
        let w = s.world as usize;
        let mut dist = [f64::INFINITY; 64];
        let walks = self.walks(s);
        dist[..walks.len()].copy_from_slice(walks);
        let items = self.settings.items;
        let overhead = self.settings.overhead_ms;
        let n = self.worlds[w].stage.len();

        for node in 0..n {
            let Some(st) = self.worlds[w].stage[node] else {
                continue;
            };
            if s.done & self.things[st].bit != 0 || dist[node] == f64::INFINITY {
                continue;
            }
            let last = self.worlds[w].last[node];
            // Where beating it leaves the player: on its square, or out the
            // far end of a pipe stage.
            let exit = self.worlds[w].exit[node];
            let lands = exit.unwrap_or(node);
            let walk_ms = dist[node];
            let needed = self.required[w];
            let allowed = !last || s.done & needed == needed;

            if allowed {
                let (loadouts, n) = self.loadouts(s, st);
                for &l in &loadouts[..n] {
                    self.costs(st, l.entry);
                    let list = self.coster.known(st, l.entry).unwrap();
                    for (i, costing) in list.iter().enumerate() {
                        let inv = self.handed(w, st, &costing.slots, l.inv);
                        let next = if last {
                            self.next_world(w, costing.power, inv)
                        } else {
                            self.after(s, s.done | self.things[st].bit, lands, costing.power, inv)
                        };
                        out.push(Action {
                            kind: Kind::Stage,
                            warp: None,
                            node,
                            to: lands,
                            thing: Some(st),
                            spends: l.spends,
                            entry: Some(l.entry),
                            walk_ms,
                            do_ms: costing.ms + overhead,
                            costing: Some(i),
                            next,
                        });
                    }
                }
            }

            // A cloud carries the player over a stage without playing it,
            // onto whatever is beyond; the stage is still there to block the
            // way back. A pipe stage is no level tile, and a cloud does
            // nothing for it.
            if items && !last && exit.is_none() && node != s.pos as usize && count(s.inv, CLOUD) > 0
            {
                let world = &self.worlds[w];
                for link in &world.links[node] {
                    if dist[link.to] != f64::INFINITY {
                        continue;
                    }
                    if link.door != 0 && s.done & world.opens[link.door] == 0 {
                        continue;
                    }
                    if link.rock != 0 && s.done & link.rock == 0 {
                        continue;
                    }
                    out.push(Action {
                        kind: Kind::Cloud,
                        warp: None,
                        node,
                        to: link.to,
                        thing: Some(st),
                        spends: Use::one(CLOUD),
                        entry: None,
                        walk_ms: walk_ms + link.ms,
                        do_ms: 0.0,
                        costing: None,
                        next: self.after(s, s.done, link.to, s.power, take(s.inv, CLOUD)),
                    });
                }
            }
        }

        // A Bros. is a detour, unless the rules want it beaten, when it is
        // part of the route like a stage.
        if self.rules.everything || (items && self.settings.detours) {
            for i in 0..self.worlds[w].bros.len() {
                let bros = self.worlds[w].bros[i];
                let node = self.things[bros].node;
                if s.done & self.things[bros].bit != 0
                    || dist[node] == f64::INFINITY
                    || !self.open(w, s.done, node)
                {
                    continue;
                }
                // Fought for its drop alone, so one that drops nothing that
                // could be of use from here on is only time lost.
                if !self.rules.everything
                    && !self.things[bros]
                        .treasure
                        .iter()
                        .any(|&slot| self.useful[w][slot] > 0)
                {
                    continue;
                }
                let (loadouts, n) = self.loadouts(s, bros);
                for &l in &loadouts[..n] {
                    let mut inv = l.inv;
                    for &slot in &self.things[bros].treasure {
                        inv = give(inv, slot);
                    }
                    self.costs(bros, l.entry);
                    let list = self.coster.known(bros, l.entry).unwrap();
                    for (i, costing) in list.iter().enumerate() {
                        out.push(Action {
                            kind: Kind::Bros,
                            warp: None,
                            node,
                            to: node,
                            thing: Some(bros),
                            spends: l.spends,
                            entry: Some(l.entry),
                            walk_ms: dist[node],
                            do_ms: costing.ms + overhead,
                            costing: Some(i),
                            next: self.after(
                                s,
                                s.done | self.things[bros].bit,
                                node,
                                costing.power,
                                inv,
                            ),
                        });
                    }
                }
            }
        }

        // A rock is the one thing an item is spent on that a category can
        // need: 100% has to reach 3-Bonus, which is behind one. So the hammer
        // is used there even with spending turned off.
        if (items || self.rules.everything) && count(s.inv, HAMMER) > 0 {
            for &rock in &self.worlds[w].rocks {
                let node = self.things[rock].node;
                if s.done & self.things[rock].bit != 0
                    || dist[node] == f64::INFINITY
                    || !self.open(w, s.done, node)
                {
                    continue;
                }
                out.push(Action {
                    kind: Kind::Rock,
                    warp: None,
                    node,
                    to: node,
                    thing: Some(rock),
                    spends: Use::one(HAMMER),
                    entry: None,
                    walk_ms: dist[node],
                    do_ms: 0.0,
                    costing: None,
                    next: self.after(
                        s,
                        s.done | self.things[rock].bit,
                        node,
                        s.power,
                        take(s.inv, HAMMER),
                    ),
                });
            }
        }

        // The whistle, from wherever the player stands: into the warp zone,
        // and down whichever of its pipes goes furthest usefully.
        if self.rules.warps && items && count(s.inv, WHISTLE) > 0 {
            for &(to, tiles) in &self.worlds[w].warps {
                if to <= w || to > self.rules.last_world {
                    continue;
                }
                let next = self.s0(to);
                out.push(Action {
                    kind: Kind::Warp,
                    warp: Some(to),
                    node: s.pos as usize,
                    to: next.pos as usize,
                    thing: None,
                    spends: Use::one(WHISTLE),
                    entry: None,
                    walk_ms: tiles * self.settings.ms_per_tile,
                    do_ms: self.settings.warp_ms,
                    costing: None,
                    next: State {
                        power: s.power,
                        inv: self.tidy(to, take(s.inv, WHISTLE)),
                        ..next
                    },
                });
            }
        }
    }

    // -----------------------------------------------------------------------
    // A floor under the time left
    //
    // The map with everything the player is left as taken out of it: every
    // stage in the way at the fastest it has ever been played as anything,
    // and the fastest of every world after. What it keeps is what opens the
    // map up: clouds and whistles held, which let a route leave stages out and
    // are gone once spent; a hammer, which opens every rock; and which kinds of
    // locked door have been opened.
    //
    // None of these is had anywhere in particular. A Bros.' treasure can be
    // had from anywhere, at the fastest it has been fought; an item a stage
    // not yet beaten has been seen to hand over is held already; and a door's
    // fortress is paid for as the door is crossed, at the fastest of its kind,
    // once. That is what keeps this a floor. Going somewhere for something and
    // coming back would cross the stages on the way twice, where a route plays
    // them once; with nothing to go anywhere for, any route with the loops
    // taken out of it is a way across this map, at no more than it takes.

    /// Whether the history has seen the stage hand the item over.
    fn gives(&self, thing: usize, slot: usize) -> bool {
        self.stats.get(&self.things[thing].place).is_some_and(|by| {
            by.values().any(|sum| {
                sum.variants
                    .iter()
                    .any(|v| v.gains.iter().any(|g| slot_of(g) == Some(slot)))
            })
        })
    }

    fn most(&self, w: usize, slot: usize) -> u32 {
        if self.settings.items && w < self.worlds.len() {
            self.useful[w][slot]
        } else {
            0
        }
    }

    /// Whether a hammer is worth keeping count of in world `w`, this much
    /// done: whether one can be spent at all (see the rocks in actions()),
    /// and on a rock still standing, here or in a world after.
    fn hammers(&self, w: usize, done: u32) -> u32 {
        if !(self.settings.items || self.rules.everything) {
            return 0;
        }
        let here = self.worlds[w]
            .rocks
            .iter()
            .any(|&r| done & self.things[r].bit == 0);
        let later = (w + 1..=self.rules.last_world.min(self.worlds.len() - 1))
            .any(|x| !self.worlds[x].rocks.is_empty());
        (here || later) as u32
    }

    /// A stage or a fight at the fastest it has been done as anything at all.
    fn fastest(&mut self, thing: usize) -> f64 {
        if self.fastest_memo[thing].is_nan() {
            let mut best = f64::INFINITY;
            let place = self.things[thing].place.clone();
            for code in 0..16 {
                let list = self.floors.cost(thing, &place, Entry::from_code(code));
                best = best.min(list.first().map_or(f64::INFINITY, |c| c.ms));
            }
            self.fastest_memo[thing] = best;
        }
        self.fastest_memo[thing] + self.settings.overhead_ms
    }

    /// The floor from every square of world `w`, with this much done, by
    /// what is held. What the stages not yet beaten could hand over is counted
    /// as held already; and each Bros. whose treasure is one of these may be
    /// fought, once, at the fastest it has been, for its treasure in hand
    /// from the start.
    fn table(&mut self, w: usize, done: u32) -> &[f64] {
        let key = (w as u64) << 32 | done as u64;
        if !self.floors_at.contains_key(&key) {
            let table = self.work_out_table(w, done);
            self.floors_at.insert(key, table);
        }
        &self.floors_at[&key]
    }

    fn work_out_table(&mut self, w: usize, done: u32) -> Box<[f64]> {
        let nodes = self.worlds[w].stage.len();
        let mut have = [0u32; 3];
        for &t in self.worlds[w].stage.iter().flatten() {
            if done & self.things[t].bit == 0 {
                for k in 0..3 {
                    have[k] += self.gives[t][k];
                }
            }
        }
        let mut bros: Vec<(f64, usize)> = Vec::new();
        if self.rules.everything || (self.settings.items && self.settings.detours) {
            for b in self.worlds[w].bros.clone() {
                if done & self.things[b].bit != 0 {
                    continue;
                }
                if let Some(k) = OPENERS
                    .iter()
                    .position(|slot| self.things[b].treasure.contains(slot))
                {
                    bros.push((self.fastest(b), k));
                }
            }
        }

        // Where every stage and Bros. has to be beaten, the world takes no
        // less than each of them at its fastest, whatever the way between.
        let mut every = 0.0;
        if self.rules.everything {
            let all: Vec<usize> = self.worlds[w]
                .stage
                .iter()
                .flatten()
                .chain(&self.worlds[w].bros)
                .copied()
                .collect();
            for t in all {
                if done & self.things[t].bit == 0 {
                    every += self.fastest(t);
                }
            }
        }

        let ends = self.ends(w, done);
        let most = [
            self.most(w, CLOUD),
            self.most(w, WHISTLE),
            self.hammers(w, done),
        ];
        let after = if w >= self.rules.last_world {
            0.0
        } else {
            // With everything the world could hand on, which is as little as
            // the next can take.
            let mut least = f64::INFINITY;
            for &v in &self.from_start[w + 1] {
                least = least.min(v);
            }
            least
        };
        let mut out = vec![f64::INFINITY; HELD * nodes].into_boxed_slice();
        for k in 0..=most[2] {
            for h in 0..=most[1] {
                for c in 0..=most[0] {
                    let row = &mut out[layer(c, h, k, 0) * nodes..][..nodes];
                    for fought in 0..1usize << bros.len() {
                        let mut ms = 0.0;
                        let mut got = [c + have[0], h + have[1], k + have[2]];
                        for (i, &(fight, kind)) in bros.iter().enumerate() {
                            if fought & 1 << i != 0 {
                                ms += fight;
                                got[kind] += 1;
                            }
                        }
                        let at = layer(
                            got[0].min(most[0]),
                            got[1].min(most[1]),
                            got[2].min(most[2]),
                            0,
                        );
                        for pos in 0..nodes {
                            row[pos] = row[pos].min(ms + ends[at * nodes + pos]);
                        }
                    }
                    if self.rules.everything {
                        for v in row.iter_mut() {
                            *v = v.max(every + after);
                        }
                    }
                }
            }
        }
        out
    }

    /// The floor from the start of world `w`, held what is held.
    fn started(&self, w: usize, clouds: u32, whistles: u32, hammer: u32) -> f64 {
        if w > self.rules.last_world || w >= self.worlds.len() {
            return 0.0;
        }
        self.from_start[w][layer(
            clouds.min(self.most(w, CLOUD)),
            whistles.min(self.most(w, WHISTLE)),
            hammer.min(self.hammers(w, 0)),
            0,
        )]
    }

    fn prepare_floor(&mut self) {
        let n = self.worlds.len();
        self.from_start = vec![[0.0; HELD]; n + 1];
        for w in (0..=self.rules.last_world).rev() {
            let start = self.worlds[w].start;
            let nodes = self.worlds[w].stage.len();
            let mut row = [0.0; HELD];
            let table = self.table(w, 0);
            for (i, v) in row.iter_mut().enumerate() {
                *v = table[i * nodes + start];
            }
            self.from_start[w] = row;
        }
    }

    /// The floor from each square of a world, with this much done in it, by
    /// what is held and which doors are open, as [layer][square]. Each layer
    /// is the map walked back from its end, Dijkstra's way: a stage in the
    /// way costs its fastest on the way onto its square. What changes the
    /// layer - a cloud spent, a door opened - only ever leads to one with
    /// fewer clouds or more doors open, so those are worked out first, and
    /// read here as they are.
    fn ends(&mut self, w: usize, done: u32) -> Vec<f64> {
        let nodes = self.worlds[w].stage.len();
        let (most_c, most_h) = (self.most(w, CLOUD), self.most(w, WHISTLE));
        let most_k = self.hammers(w, done);

        // Every stage the floor might ask for, worked out first.
        let mut fast = vec![0.0; nodes];
        for i in 0..nodes {
            if let Some(t) = self.worlds[w].stage[i] {
                fast[i] = self.fastest(t);
            }
        }

        let world = &self.worlds[w];
        let undone: Vec<bool> = (0..nodes)
            .map(|i| world.stage[i].is_some_and(|t| done & self.things[t].bit == 0))
            .collect();
        let cloudable: Vec<bool> = (0..nodes)
            .map(|i| undone[i] && !world.last[i] && world.exit[i].is_none())
            .collect();
        let reward = |slot: usize| world.reward.iter().filter(|&&got| got == slot).count() as u32;

        // The kinds of door still shut, each with the fastest fortress that
        // would open it, and the kinds each fortress not yet beaten opens.
        let mut shut = 0u32;
        let mut key_ms = [f64::INFINITY; 3];
        let mut opens = vec![0u32; nodes];
        for kind in 1..3 {
            if done & world.opens[kind] != 0
                || !world.links.iter().flatten().any(|l| l.door == kind)
            {
                continue;
            }
            shut |= 1 << (kind - 1);
            for i in 0..nodes {
                if undone[i] && world.opens[kind] & self.things[world.stage[i].unwrap()].bit != 0 {
                    key_ms[kind] = key_ms[kind].min(fast[i]);
                    opens[i] |= 1 << (kind - 1);
                }
            }
        }
        // A stage in the way, played: a fortress whose kind of door is paid
        // for already is paid for with it.
        let play = |x: usize, d: u32| -> (f64, u32) {
            if !undone[x] || (opens[x] != 0 && d & opens[x] == opens[x]) {
                (0.0, d)
            } else {
                (fast[x], d | opens[x])
            }
        };
        // A link walked: what it costs besides the walk, and the doors open after.
        let cross = |link: &Link, k: u32, d: u32| -> Option<(f64, u32)> {
            if link.rock != 0 && done & link.rock == 0 && k == 0 {
                return None;
            }
            if link.door == 0 || done & world.opens[link.door] != 0 {
                return Some((0.0, d));
            }
            let bit = 1 << (link.door - 1);
            if d & bit != 0 {
                Some((0.0, d))
            } else {
                Some((key_ms[link.door], d | bit))
            }
        };

        let mut found = vec![f64::INFINITY; LAYERS * nodes];
        let mut val = vec![0.0; nodes];
        // What a square adds to a way on through it in the same layer: its
        // stage, where it stands in the way; or nothing on through it at all.
        let mut adds = vec![0.0; nodes];
        let mut settled = vec![false; nodes];

        for d in (0..4u32).rev().filter(|d| d & !shut == 0) {
            for k in 0..=most_k {
                for h in 0..=most_h {
                    // The whistle, from anywhere: to whichever world it
                    // reaches is soonest done with.
                    let mut warps = [f64::INFINITY; (CAP + 1) as usize];
                    if h > 0 {
                        for (c, warp) in warps.iter_mut().enumerate() {
                            for &(to, tiles) in &world.warps {
                                if to <= w || to > self.rules.last_world {
                                    continue;
                                }
                                let ms = tiles * self.settings.ms_per_tile
                                    + self.settings.warp_ms
                                    + self.started(to, c as u32, h - 1, k);
                                *warp = warp.min(ms);
                            }
                        }
                    }
                    let beyond = if w >= self.rules.last_world {
                        [0.0; (CAP + 1) as usize]
                    } else {
                        let mut out = [0.0; (CAP + 1) as usize];
                        for (c, v) in out.iter_mut().enumerate() {
                            *v = self.started(
                                w + 1,
                                c as u32 + reward(CLOUD),
                                h + reward(WHISTLE),
                                k + reward(HAMMER),
                            );
                        }
                        out
                    };

                    for c in 0..=most_c {
                        let at = |c: u32, d: u32| layer(c, h, k, d) * nodes;
                        // What each square comes to by way of another layer,
                        // or by nothing but itself.
                        for x in 0..nodes {
                            let mut best = warps[c as usize];
                            adds[x] = f64::INFINITY;
                            if world.last[x] {
                                best = best.min(fast[x] + beyond[c as usize]);
                            } else {
                                // A stage stood on and not yet beaten - set
                                // down on by a cloud - is played before
                                // anything else.
                                let standing = undone[x] && world.exit[x].is_none();
                                let (ms, dd) = if standing { play(x, d) } else { (0.0, d) };
                                if dd == d {
                                    adds[x] = ms;
                                }
                                let mut other = f64::INFINITY;
                                for link in &world.links[x] {
                                    if let Some((extra, d2)) = cross(link, k, dd) {
                                        if d2 != d {
                                            other = other
                                                .min(link.ms + extra + found[at(c, d2) + link.to]);
                                        }
                                    }
                                }
                                if let Some(exit) = world.exit[x] {
                                    let (pm, d2) = play(x, d);
                                    if d2 != d {
                                        other = other.min(pm + found[at(c, d2) + exit]);
                                    }
                                }
                                if c > 0 {
                                    for link in &world.links[x] {
                                        if !cloudable[link.to] {
                                            continue;
                                        }
                                        let Some((extra, d2)) = cross(link, k, dd) else {
                                            continue;
                                        };
                                        for over in &world.links[link.to] {
                                            if let Some((more, d3)) = cross(over, k, d2) {
                                                let v = link.ms
                                                    + extra
                                                    + over.ms
                                                    + more
                                                    + found[at(c - 1, d3) + over.to];
                                                other = other.min(v);
                                            }
                                        }
                                    }
                                }
                                best = best.min(ms + other);
                            }
                            val[x] = best;
                            settled[x] = false;
                        }

                        // And by way of the squares around it, least first.
                        loop {
                            let mut u = usize::MAX;
                            for i in 0..nodes {
                                if !settled[i]
                                    && val[i] < f64::INFINITY
                                    && (u == usize::MAX || val[i] < val[u])
                                {
                                    u = i;
                                }
                            }
                            if u == usize::MAX {
                                break;
                            }
                            settled[u] = true;
                            for &(x, i) in &world.into[u] {
                                if settled[x] || adds[x] == f64::INFINITY {
                                    continue;
                                }
                                let link = &world.links[x][i];
                                if let Some((_, d2)) = cross(link, k, d) {
                                    if d2 == d {
                                        val[x] = val[x].min(adds[x] + link.ms + val[u]);
                                    }
                                }
                            }
                            for &x in &world.exits_into[u] {
                                let (pm, d2) = play(x, d);
                                if !settled[x] && d2 == d {
                                    val[x] = val[x].min(pm + val[u]);
                                }
                            }
                        }
                        found[at(c, d)..][..nodes].copy_from_slice(&val);
                    }
                }
            }
        }
        found
    }

    pub fn floor(&mut self, s: State) -> f64 {
        let w = s.world as usize;
        if w >= self.worlds.len() {
            return 0.0;
        }
        let hammer = (count(s.inv, HAMMER) > 0) as u32 & self.hammers(w, s.done);
        let held = layer(
            count(s.inv, CLOUD).min(self.most(w, CLOUD)),
            count(s.inv, WHISTLE).min(self.most(w, WHISTLE)),
            hammer,
            0,
        );
        let nodes = self.worlds[w].stage.len();
        self.table(w, s.done)[held * nodes + s.pos as usize]
    }

    // -----------------------------------------------------------------------
    // The search

    /// The most that is known about the best from here without working it
    /// out. What memo holds for a state is its best, or where the search has
    /// only shown that its best is no less than some figure, that figure less
    /// one, negated.
    fn least(&mut self, s: State) -> f64 {
        if s.world as usize >= self.worlds.len() {
            return 0.0;
        }
        match self.memo.get(&s.key()).copied() {
            None => self.floor(s),
            Some(known) if known >= 0.0 => known,
            Some(known) => self.floor(s).max(-1.0 - known),
        }
    }

    /// The best one could do from here.
    pub fn value(&mut self, s: State) -> Result<f64, TooBig> {
        self.value_below(s, f64::INFINITY)
    }

    /// The best one could do from here, where that is less than `cutoff`;
    /// otherwise no more than a figure no less than `cutoff`, which is all
    /// that is asked where a choice is only being ruled out.
    fn value_below(&mut self, s: State, cutoff: f64) -> Result<f64, TooBig> {
        if s.world as usize >= self.worlds.len() {
            return Ok(0.0);
        }
        match self.memo.get(&s.key()).copied() {
            Some(known) if known >= 0.0 => return Ok(known),
            Some(known) if -1.0 - known >= cutoff => return Ok(-1.0 - known),
            _ => {}
        }
        let floor = self.floor(s);
        if floor >= cutoff {
            return Ok(floor);
        }
        self.shortest(s, cutoff)
    }

    /// The fastest way from here to the end, found by A*, or where that is no
    /// less than `cutoff`, a floor under it that is. States are taken from
    /// the least time so far plus the least that could follow, so none is
    /// looked at that could only come to more.
    ///
    /// What it finds is kept. Along the route, the best from each state is
    /// the route's time less the time to get there. Off it, the best from a
    /// state is no less than what the search stopped at less the time to get
    /// there - were it less, the way through it would have been found -
    /// which is a floor that the next search from nearby can start from.
    fn shortest(&mut self, from: State, cutoff: f64) -> Result<f64, TooBig> {
        let n = self.worlds.len();
        self.searches += 1;
        let mut heap = BinaryHeap::new();
        let mut reached: FxHashMap<u64, (f64, State)> = FxHashMap::default();
        let mut seen: Vec<State> = vec![from];
        reached.insert(from.key(), (0.0, from));
        let f0 = self.least(from);
        heap.push(HeapItem {
            f: f0,
            g: 0.0,
            s: from,
        });

        let mut buffer = Vec::new();
        let mut found = f64::INFINITY;
        let mut last: Option<State> = None;
        // The least that anything not yet looked at could come to.
        let mut stop = f64::INFINITY;

        while let Some(HeapItem { f, g, s }) = heap.pop() {
            if f >= found || f >= cutoff {
                stop = f;
                break;
            }
            if g > reached[&s.key()].0 {
                continue;
            }
            let known = if s.world as usize >= n {
                Some(0.0)
            } else {
                self.memo.get(&s.key()).copied()
            };
            if let Some(known) = known {
                if known >= 0.0 {
                    if g + known < found {
                        found = g + known;
                        last = Some(s);
                    }
                    continue;
                }
            }

            self.expanded += 1;
            self.actions_into(s, &mut buffer);
            for &a in &buffer {
                let to = g + a.walk_ms + a.do_ms;
                if !(to < f64::INFINITY) {
                    continue;
                }
                let next = a.next;
                match reached.get(&next.key()) {
                    Some(&(had, _)) if had <= to => continue,
                    Some(_) => {}
                    None => {
                        if self.memo.len() + seen.len() >= self.budget {
                            return Err(TooBig);
                        }
                        seen.push(next);
                    }
                }
                reached.insert(next.key(), (to, s));
                let f = to + self.least(next);
                heap.push(HeapItem { f, g: to, s: next });
            }
        }

        let bound = found.min(stop);
        if found <= stop {
            let mut at = last;
            while let Some(s) = at {
                let (g, from_state) = reached[&s.key()];
                if (s.world as usize) < n {
                    self.memo.insert(s.key(), found - g);
                }
                at = if from_state == s {
                    None
                } else {
                    Some(from_state)
                };
            }
        }
        for s in seen {
            if s.world as usize >= n {
                continue;
            }
            let known = self.memo.get(&s.key()).copied();
            if known.is_some_and(|k| k >= 0.0) {
                continue;
            }
            // Where there is no way to the end from here, there is none from
            // anywhere that was reached from here either.
            if bound == f64::INFINITY {
                self.memo.insert(s.key(), f64::INFINITY);
                continue;
            }
            let lo = bound - reached[&s.key()].0;
            if lo > 0.0 && known.is_none_or(|k| lo > -1.0 - k) {
                self.memo.insert(s.key(), -1.0 - lo);
            }
        }
        Ok(bound)
    }

    /// An action's time, and the best that can be done after it - or, where
    /// that is no less than `cutoff`, a figure that is.
    fn lookahead(&mut self, a: &Action, cutoff: f64) -> Result<f64, TooBig> {
        let total = a.walk_ms + a.do_ms;
        if total == f64::INFINITY {
            return Ok(f64::INFINITY);
        }
        Ok(total + self.value_below(a.next, cutoff - total)?)
    }

    /// What to do here, every option ranked by the time it leaves to go: its
    /// own, and then the best that can be done after it. Only the best `keep`.
    pub fn choices(&mut self, s: State, keep: usize) -> Result<Vec<(Action, f64)>, TooBig> {
        let all = self.actions(s);
        let mut by_least: Vec<(usize, f64)> = Vec::with_capacity(all.len());
        for (i, a) in all.iter().enumerate() {
            let least = a.walk_ms + a.do_ms + self.least(a.next);
            by_least.push((i, least));
        }
        by_least.sort_by(|x, y| x.1.total_cmp(&y.1));

        // Between two that come out the same, the one that spends less: an
        // item kept costs nothing, and may yet be wanted. Past that, in the
        // order they were found in. The same is the same to a microsecond.
        let rank = |x: &(usize, f64), y: &(usize, f64)| -> Ordering {
            let d = x.1 - y.1;
            let by_total = if d.abs() < 1e-3 || x.1 == y.1 {
                Ordering::Equal
            } else {
                x.1.total_cmp(&y.1)
            };
            by_total
                .then(all[x.0].spends.len.cmp(&all[y.0].spends.len))
                .then(x.0.cmp(&y.0))
        };

        // Once there are `keep`, one that cannot come within a microsecond
        // of the last of them is no rival for any, and is worked out only as
        // far as showing that.
        let mut out: Vec<(usize, f64)> = Vec::new();
        for (i, least) in by_least {
            let cutoff = if out.len() >= keep {
                out[keep - 1].1 + 1e-3
            } else {
                f64::INFINITY
            };
            if least > cutoff {
                break;
            }
            let total = self.lookahead(&all[i], cutoff)?;
            // Against no way at all, there is nothing to rule out: two that
            // cannot be done tie, and are ranked by what they spend.
            if out.len() >= keep && cutoff < f64::INFINITY && total >= cutoff {
                continue;
            }
            out.push((i, total));
            out.sort_by(rank);
        }
        out.truncate(keep);
        Ok(out.into_iter().map(|(i, total)| (all[i], total)).collect())
    }

    /// Every state whose best is known exactly, with that best: for checking
    /// the floor against.
    pub fn known(&self) -> Vec<(State, f64)> {
        self.memo
            .iter()
            .filter(|(_, v)| **v >= 0.0)
            .map(|(k, v)| (State::from_key(*k), *v))
            .collect()
    }

    pub fn tables(&self) -> usize {
        self.floors_at.len()
    }

    pub fn states(&self) -> usize {
        self.memo.len()
    }
}
