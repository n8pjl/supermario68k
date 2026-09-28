//! Powers, entries and the item list, packed the way states carry them.

pub const POWERS: [&str; 4] = ["small", "super", "fire", "racoon"];
pub const SMALL: u8 = 0;
pub const SUPER: u8 = 1;
pub const FIRE: u8 = 2;
pub const RACOON: u8 = 3;

pub fn power_index(name: &str) -> u8 {
    POWERS.iter().position(|p| *p == name).unwrap_or(0) as u8
}

/// How many hits a power is from small: fire and raccoon alike.
pub fn rank(power: u8) -> u8 {
    power.min(2)
}

/// What a stage is walked into as, packed into 4 bits: the power, then a star,
/// then a P-wing.
#[derive(Clone, Copy, PartialEq, Eq, Debug)]
pub struct Entry {
    pub power: u8,
    pub star: bool,
    pub pwing: bool,
}

impl Entry {
    pub const fn plain(power: u8) -> Entry {
        Entry {
            power,
            star: false,
            pwing: false,
        }
    }

    pub fn code(self) -> usize {
        self.power as usize * 4 + if self.star { 2 } else { 0 } + if self.pwing { 1 } else { 0 }
    }

    pub fn from_code(code: usize) -> Entry {
        Entry {
            power: (code / 4) as u8,
            star: code & 2 != 0,
            pwing: code & 1 != 0,
        }
    }

    /// "fire", "racoon+pwing", "small+star": what the history's entries are keyed by.
    pub fn key(self) -> String {
        let mut out = POWERS[self.power as usize].to_string();
        if self.star {
            out.push_str("+star");
        }
        if self.pwing {
            out.push_str("+pwing");
        }
        out
    }
}

/// The items a route spends, in the order the item list is kept in a state.
pub const SLOTS: [&str; 8] = [
    "mushroom",
    "fire-flower",
    "leaf",
    "star",
    "p-wing",
    "cloud",
    "hammer",
    "whistle",
];
pub const MUSHROOM: usize = 0;
pub const FIRE_FLOWER: usize = 1;
pub const LEAF: usize = 2;
pub const STAR: usize = 3;
pub const PWING: usize = 4;
pub const CLOUD: usize = 5;
pub const HAMMER: usize = 6;
pub const WHISTLE: usize = 7;

pub fn slot_of(item: &str) -> Option<usize> {
    SLOTS.iter().position(|s| *s == item)
}

/// How many of one item a state keeps count of. More are dropped: a third leaf
/// in hand changes nothing a second does not, and the count is what the
/// number of states grows with.
pub const CAP: u32 = 2;
pub const BASE: u32 = CAP + 1;
pub const WEIGHT: [u32; 8] = {
    let mut w = [1u32; 8];
    let mut i = 1;
    while i < 8 {
        w[i] = w[i - 1] * BASE;
        i += 1;
    }
    w
};

pub fn count(inv: u32, slot: usize) -> u32 {
    inv / WEIGHT[slot] % BASE
}

pub fn give(inv: u32, slot: usize) -> u32 {
    if count(inv, slot) >= CAP {
        inv
    } else {
        inv + WEIGHT[slot]
    }
}

pub fn take(inv: u32, slot: usize) -> u32 {
    inv - WEIGHT[slot]
}

/// The list itself, a name an item.
pub fn inventory(inv: u32) -> Vec<&'static str> {
    let mut out = Vec::new();
    for (slot, name) in SLOTS.iter().enumerate() {
        for _ in 0..count(inv, slot) {
            out.push(*name);
        }
    }
    out
}

/// The slot a treasure fills, if it is one a route can spend - and none for a
/// random pick, which is luck.
pub fn treasure(item: Option<&str>) -> Vec<usize> {
    item.and_then(slot_of).into_iter().collect()
}

/// What is in `before` and not in `after`, as a multiset.
pub fn spent<'a>(before: &[&'a str], after: &[&'a str]) -> Vec<&'a str> {
    let mut left: Vec<&str> = after.to_vec();
    let mut out = Vec::new();
    for item in before {
        match left.iter().position(|x| x == item) {
            Some(i) => {
                left.remove(i);
            }
            None => out.push(*item),
        }
    }
    out
}
