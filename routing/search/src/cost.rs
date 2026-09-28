//! What a stage costs, walked into as something: the ways the history has seen
//! it come out, and where there are none of its own, figures borrowed from
//! the same stage walked into as something close.

use crate::input::{Settings, Stats, Summary};
use crate::items::{Entry, POWERS, power_index, rank, slot_of};

#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub enum Source {
    Data,
    Borrowed,
    Assumed,
}

impl Source {
    pub fn name(self) -> &'static str {
        match self {
            Source::Data => "data",
            Source::Borrowed => "borrowed",
            Source::Assumed => "assumed",
        }
    }
}

/// One way the search can take a stage walked into as something: going for
/// one of the ways the history has seen it come out.
#[derive(Clone, Debug)]
pub struct Costing {
    pub ms: f64,
    /// What is aimed for: leaving as this, holding these.
    pub power: u8,
    pub gains: Vec<String>,
    /// The gains the item list keeps, as slots.
    pub slots: Vec<usize>,
    pub source: Source,
    /// Whose figures were used, where they were borrowed.
    pub from: Option<String>,
    /// Clears of the variant aimed for.
    pub clears: f64,
}

/// The figures a stage walked in as `entry` is costed from: its own, else the
/// same stage walked in as the nearest thing to it - without the star or the
/// P-wing first, then the other powers, nearest first and the weaker side of a
/// tie first, since a borrowed time is better too slow than too fast. With
/// `up` false, only the weaker ones. Figures walked in with a star or a P-wing
/// are never borrowed, only ever used for that same entry: what either one
/// buys says nothing about a stage played without it. Nor do fire and raccoon
/// borrow from each other: they are as many hits from small, but a flower and
/// a leaf take a stage differently, so neither is near the other.
struct Found<'a> {
    summary: &'a Summary,
    key: String,
    exact: bool,
    same_power: bool,
}

fn lookup<'a>(stats: &'a Stats, place: &str, entry: Entry, up: bool) -> Option<Found<'a>> {
    let here = stats.get(place)?;
    let usable = |key: &str| here.get(key).is_some_and(|s| s.clears > 0.0);

    let key = entry.key();
    if usable(&key) {
        return Some(Found {
            summary: &here[&key],
            key,
            exact: true,
            same_power: true,
        });
    }
    let plain = Entry::plain(entry.power).key();
    if usable(&plain) {
        return Some(Found {
            summary: &here[&plain],
            key: plain,
            exact: false,
            same_power: true,
        });
    }

    let own = rank(entry.power);
    let mut order: Vec<(u8, u8, bool)> = (0..4u8)
        .map(|p| (p, rank(p).abs_diff(own), rank(p) < own))
        .filter(|&(_, d, weaker)| d > 0 && (up || weaker))
        .collect();
    // Stable, as the page's was: a tie keeps the order of POWERS.
    order.sort_by_key(|&(_, d, weaker)| (d, !weaker));

    for (p, _, _) in order {
        let other = Entry::plain(p).key();
        if usable(&other) {
            return Some(Found {
                summary: &here[&other],
                key: other,
                exact: false,
                same_power: false,
            });
        }
    }
    None
}

pub struct Coster<'a> {
    stats: &'a Stats,
    settings: &'a Settings,
    /// By thing index and entry code; see Entry::code.
    table: Vec<Option<Box<[Costing]>>>,
    /// Everything costed, in the order it was first asked for.
    pub order: Vec<(usize, usize)>,
}

impl<'a> Coster<'a> {
    pub fn new(stats: &'a Stats, settings: &'a Settings, things: usize) -> Self {
        Coster {
            stats,
            settings,
            table: vec![None; things * 16],
            order: Vec::new(),
        }
    }

    /// Every way the thing can be taken walked in as `entry`, fastest first.
    pub fn cost(&mut self, thing: usize, place: &str, entry: Entry) -> &[Costing] {
        let slot = thing * 16 + entry.code();
        if self.table[slot].is_none() {
            let mut found = self.work(place, entry);
            found.sort_by(|a, b| a.ms.total_cmp(&b.ms));
            self.table[slot] = Some(found.into_boxed_slice());
            self.order.push((thing, entry.code()));
        }
        self.table[slot].as_deref().unwrap()
    }

    /// What is already known, without working anything out.
    pub fn known(&self, thing: usize, entry: Entry) -> Option<&[Costing]> {
        self.table[thing * 16 + entry.code()].as_deref()
    }

    fn work(&self, place: &str, entry: Entry) -> Vec<Costing> {
        let settings = self.settings;
        let found = lookup(self.stats, place, entry, settings.borrow_up);
        let best = settings.objective == "best";
        let pick = |v: &crate::input::Variant| if best { v.best } else { v.median };

        let mut aims: Vec<Costing> = Vec::new();
        // Two ways out that come to the same one here are one: the faster,
        // and on a tie the one first found, which is the entry's own.
        let add = |aims: &mut Vec<Costing>, aim: Costing| match aims
            .iter()
            .position(|a| a.power == aim.power && a.gains == aim.gains)
        {
            None => aims.push(aim),
            Some(i) => {
                if aim.ms < aims[i].ms {
                    aims[i] = aim;
                }
            }
        };

        // Another power's clears, as ways out walked in as this one: the
        // player comes out as well as those clears did, and no better than
        // they went in. What those clears came away with is theirs, and not
        // counted on here.
        let borrow = |aims: &mut Vec<Costing>, key: &str| {
            for v in &self.stats[place][key].variants {
                let seen = power_index(&v.exit);
                let exit = if rank(seen) < rank(entry.power) {
                    seen
                } else {
                    entry.power
                };
                add(
                    aims,
                    Costing {
                        ms: pick(v),
                        power: exit,
                        gains: Vec::new(),
                        slots: Vec::new(),
                        source: Source::Borrowed,
                        from: Some(key.to_string()),
                        clears: v.clears,
                    },
                );
            }
        };

        match found {
            None => {
                // Never cleared as anything, there is nothing to say the
                // player keeps any of what they went in as.
                add(
                    &mut aims,
                    Costing {
                        ms: if settings.unknown == "avoid" {
                            f64::INFINITY
                        } else {
                            settings.unknown_ms
                        },
                        power: 0,
                        gains: Vec::new(),
                        slots: Vec::new(),
                        source: Source::Assumed,
                        from: None,
                        clears: 0.0,
                    },
                );
            }
            Some(found) => {
                if found.same_power {
                    let from = if found.exact {
                        None
                    } else {
                        Some(found.key.clone())
                    };
                    for v in &found.summary.variants {
                        add(
                            &mut aims,
                            Costing {
                                ms: pick(v),
                                power: power_index(&v.exit),
                                gains: v.gains.clone(),
                                slots: v.gains.iter().filter_map(|g| slot_of(g)).collect(),
                                source: if from.is_none() {
                                    Source::Data
                                } else {
                                    Source::Borrowed
                                },
                                from: from.clone(),
                                clears: v.clears,
                            },
                        );
                    }
                } else {
                    borrow(&mut aims, &found.key);
                }

                // Whatever a weaker power has been seen to do, a stronger one
                // can do too, and come out as well: so every way out of a
                // plain weaker entry is one out of this.
                for p in 0..POWERS.len() as u8 {
                    let key = Entry::plain(p).key();
                    if rank(p) < rank(entry.power)
                        && key != found.key
                        && self.stats[place].get(&key).is_some_and(|s| s.clears > 0.0)
                    {
                        borrow(&mut aims, &key);
                    }
                }
            }
        }
        aims
    }
}
