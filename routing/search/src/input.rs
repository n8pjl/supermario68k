//! What the worker hands over: the maps, the history's figures and the
//! settings, as the page keeps them (see routing/maps.ts, routing/model.ts and
//! Settings in routing/worker.ts). Only the fields the search reads are named;
//! serde passes over the rest.

use rustc_hash::FxHashMap;
use serde::Deserialize;

#[derive(Deserialize)]
pub struct Input {
    pub maps: Vec<WorldMap>,
    pub stats: Stats,
    pub settings: Settings,
    /// Where a run in progress stands, to route the rest of it from; absent,
    /// the route is from a new game.
    #[serde(default)]
    pub from: Option<Start>,
}

/// A run in progress, as the page read it off the history's events (see
/// routing/live.ts).
#[derive(Deserialize)]
pub struct Start {
    pub world: usize,
    /// What was beaten in this world, as place keys: "L1.3", "M1.0".
    pub done: Vec<String>,
    /// The last of them, which is where the player stands; none, the start
    /// of the map.
    pub at: Option<String>,
    pub power: String,
    pub items: Vec<String>,
    /// The stage or fight being played right now, and what it was walked
    /// into as: the route begins by finishing it.
    pub inside: Option<Inside>,
    /// Places walked into with a hammer fewer than before, once per hammer:
    /// a rock was broken on the way to each.
    #[serde(default)]
    pub rocks: Vec<String>,
}

#[derive(Deserialize)]
pub struct Inside {
    pub place: String,
    pub entry: EntryIn,
}

#[derive(Deserialize)]
pub struct EntryIn {
    pub power: String,
    pub star: bool,
    pub pwing: bool,
}

#[derive(Deserialize)]
#[serde(rename_all = "camelCase")]
pub struct Settings {
    pub category: String,
    pub objective: String,
    pub items: bool,
    pub detours: bool,
    pub unknown: String,
    pub unknown_ms: f64,
    pub borrow_up: bool,
    pub ms_per_tile: f64,
    pub overhead_ms: f64,
    pub pipe_ms: f64,
    pub warp_ms: f64,
}

/// Summaries by place key, then by entry key.
pub type Stats = FxHashMap<String, FxHashMap<String, Summary>>;

#[derive(Deserialize)]
pub struct Summary {
    pub clears: f64,
    pub variants: Vec<Variant>,
}

#[derive(Deserialize)]
pub struct Variant {
    pub exit: String,
    pub gains: Vec<String>,
    pub clears: f64,
    pub best: f64,
    pub median: f64,
}

#[derive(Deserialize)]
pub struct WorldMap {
    pub world: usize,
    pub start: usize,
    pub nodes: Vec<MapNode>,
    pub edges: Vec<MapEdge>,
    pub rocks: Vec<Rock>,
    pub bros: Vec<MapBros>,
    pub reward: Option<String>,
    pub warps: Vec<Warp>,
}

#[derive(Deserialize)]
pub struct MapNode {
    pub id: usize,
    pub kind: String,
    pub level: Option<u32>,
    pub exit: Option<usize>,
    pub opens: Option<usize>,
    #[serde(default)]
    pub chests: Vec<String>,
}

#[derive(Deserialize)]
pub struct MapEdge {
    pub a: usize,
    pub b: usize,
    pub by: String,
    pub tiles: f64,
    pub door: Option<usize>,
    pub rock: Option<usize>,
}

#[derive(Deserialize)]
pub struct Rock {
    pub from: usize,
}

#[derive(Deserialize)]
pub struct MapBros {
    pub monster: u32,
    pub node: usize,
    pub treasure: String,
}

#[derive(Deserialize)]
pub struct Warp {
    pub world: usize,
    pub tiles: f64,
}
