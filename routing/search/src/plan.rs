//! A search, read out as a route: what the page shows.
//!
//! The search answers "what is best from here" for any state; a route is the
//! answers strung together from a new game. Each step keeps the options it
//! beat and by how much, which is what says whether a choice is a clear one
//! or a coin toss the data could turn either way.

use serde::Serialize;

use crate::cost::Costing;
use crate::items::{Entry, POWERS, SLOTS, inventory, spent};
use crate::search::{Action, Search, State};

#[derive(Serialize)]
pub struct EntryOut {
    power: &'static str,
    star: bool,
    pwing: bool,
}

impl From<Entry> for EntryOut {
    fn from(e: Entry) -> Self {
        EntryOut {
            power: POWERS[e.power as usize],
            star: e.star,
            pwing: e.pwing,
        }
    }
}

#[derive(Serialize)]
pub struct CostingOut {
    ms: f64,
    exit: &'static str,
    gains: Vec<String>,
    power: u8,
    slots: Vec<usize>,
    source: &'static str,
    from: Option<String>,
    clears: f64,
}

impl From<&Costing> for CostingOut {
    fn from(c: &Costing) -> Self {
        CostingOut {
            ms: c.ms,
            exit: POWERS[c.power as usize],
            gains: c.gains.clone(),
            power: c.power,
            slots: c.slots.clone(),
            source: c.source.name(),
            from: c.from.clone(),
            clears: c.clears,
        }
    }
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Alternative {
    kind: &'static str,
    warp: Option<usize>,
    node: usize,
    place: Option<String>,
    #[serde(rename = "use")]
    spends: Vec<&'static str>,
    entry: Option<EntryOut>,
    /// How much longer the whole run takes for choosing it.
    delta_ms: f64,
    source: Option<&'static str>,
    /// The way out it goes for, where it is a stage or a fight.
    exit: Option<&'static str>,
    gains: Vec<String>,
}

#[derive(Serialize)]
#[serde(rename_all = "camelCase")]
pub struct Step {
    world: usize,
    kind: &'static str,
    warp: Option<usize>,
    node: usize,
    to: usize,
    place: Option<String>,
    #[serde(rename = "use")]
    spends: Vec<&'static str>,
    entry: Option<EntryOut>,
    walk_ms: f64,
    do_ms: f64,
    costing: Option<CostingOut>,
    /// The item list on the way in, before anything is spent.
    holding: Vec<&'static str>,
    /// The time left for the run as this step begins.
    left: f64,
    /// What the player is after it.
    power: &'static str,
    /// What it put in the item list: a chest, a Bros.' treasure, a castle's.
    got: Vec<&'static str>,
    /// The next best things to have done instead, best first.
    alternatives: Vec<Alternative>,
}

/// One stage walked in as one thing, as the search costed it.
#[derive(Serialize)]
pub struct Costed {
    place: String,
    entry: String,
    ms: f64,
    source: &'static str,
    from: Option<String>,
    clears: f64,
}

#[derive(Serialize)]
pub struct Plan {
    /// Time for the whole run.
    total: f64,
    steps: Vec<Step>,
    /// Every stage and entry the search found a use for, and what it made of it.
    costed: Vec<Costed>,
    states: usize,
}

/// How many options each step keeps besides the one taken.
const ALTERNATIVES: usize = 3;

pub enum Failure {
    TooBig,
    NoWay(usize),
}

impl Search<'_> {
    fn place(&self, a: &Action) -> Option<String> {
        a.thing
            .map(|t| self.things[t].place.clone())
            .filter(|p| !p.is_empty())
    }

    fn costing(&self, a: &Action) -> Option<&Costing> {
        Some(&self.coster.known(a.thing?, a.entry?)?[a.costing?])
    }
}

fn names(slots: impl Iterator<Item = usize>) -> Vec<&'static str> {
    slots.map(|s| SLOTS[s]).collect()
}

pub fn plan(search: &mut Search) -> Result<Plan, Failure> {
    let mut steps = Vec::new();
    let mut state: State = search.s0(0);
    let end = search.worlds.len() as u8;

    while state.world < end {
        let ranked = search
            .choices(state, 1 + ALTERNATIVES)
            .map_err(|_| Failure::TooBig)?;
        let Some(&(a, best)) = ranked.first().filter(|(_, total)| *total < f64::INFINITY) else {
            return Err(Failure::NoWay(state.world as usize));
        };

        let holding = inventory(state.inv);
        let mut kept = holding.clone();
        for slot in a.spends.iter() {
            let i = kept.iter().position(|x| *x == SLOTS[slot]).unwrap();
            kept.remove(i);
        }

        let alternatives = ranked[1..]
            .iter()
            .map(|&(alt, total)| {
                let c = search.costing(&alt);
                Alternative {
                    kind: alt.kind.name(),
                    warp: alt.warp,
                    node: alt.node,
                    place: search.place(&alt),
                    spends: names(alt.spends.iter()),
                    entry: alt.entry.map(Into::into),
                    // Never below nothing: a tie can add up a hair either way.
                    delta_ms: (total - best).max(0.0),
                    source: c.map(|c| c.source.name()),
                    exit: c.map(|c| POWERS[c.power as usize]),
                    gains: c.map_or(Vec::new(), |c| c.gains.clone()),
                }
            })
            .collect();

        steps.push(Step {
            world: state.world as usize,
            kind: a.kind.name(),
            warp: a.warp,
            node: a.node,
            to: a.to,
            place: search.place(&a),
            spends: names(a.spends.iter()),
            entry: a.entry.map(Into::into),
            walk_ms: a.walk_ms,
            do_ms: a.do_ms,
            costing: search.costing(&a).map(Into::into),
            holding,
            left: best,
            power: POWERS[a.next.power as usize],
            got: spent(&inventory(a.next.inv), &kept),
            alternatives,
        });
        state = a.next;
    }

    let costed = search
        .coster
        .order
        .iter()
        .filter_map(|&(thing, code)| {
            let entry = Entry::from_code(code);
            let list = search.coster.known(thing, entry)?;
            let c = list.first()?;
            Some(Costed {
                place: search.things[thing].place.clone(),
                entry: entry.key(),
                ms: c.ms,
                source: c.source.name(),
                from: c.from.clone(),
                clears: list.iter().map(|v| v.clears).sum(),
            })
        })
        .collect();

    Ok(Plan {
        total: steps.first().map_or(0.0, |s| s.left),
        steps,
        costed,
        states: search.states(),
    })
}
