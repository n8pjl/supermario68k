//! The route search: the fastest way through a category, as the history says
//! the player plays it. The routing page runs it on a worker, as wasm; see
//! routing/worker.ts for the other side of the boundary.
//!
//! A run is a walk across eight maps, choosing as it goes: which stage to play
//! next, which to leave, whether to walk to a Bros. for an item, and what to
//! spend from the item list before going in. What each choice costs comes
//! from the history (see routing/model.ts), by stage and by what the stage is
//! walked into as; what the map allows comes from routing/maps.ts.
//!
//! How a stage comes out is the player's to choose, as far as the history has
//! seen it done: 1-1 left as raccoon, 2-Pyramid left with its cloud. Each way
//! out the history holds is an option, at the time the clears that came out
//! that way took.
//!
//! What is left to chance is never planned on, so that no route waits on it:
//! a chest's pick of three, in a mushroom house or a stage, and a Bros.'
//! random drop. Nor is a death; a route is the clears it strings together.
//!
//! The category is what says where the run ends and what it may do on the
//! way: World 1 is over at world 1's castle; Any% may spend a whistle to warp
//! ahead from anywhere on a map; 100% has to have beaten every stage and every
//! Bros. in a world before its castle. See rules_for().
//!
//! A state is: the world, what has been done in it (stages beaten, Bros.
//! fought, rocks broken), where the player stands, their power, and the item
//! list. Everything a state can do next either does something new or spends
//! an item, so nothing leads back to where it started. Every choice leads to
//! one state, so the run is a shortest path. A* finds it, under a floor that
//! keeps what opens each map up - clouds, whistles, a hammer, locked doors -
//! and so passes by most of the stage orders and item lists no good route
//! goes through; see search.rs.
//!
//! Where the history has nothing for a stage walked into as something, the
//! figure is borrowed - from the same stage entered as something close, or,
//! failing that, assumed - and every figure that was not simply read off the
//! history is marked, so the page can say which parts of a route rest on it.

mod cost;
pub mod input;
mod items;
pub mod plan;
pub mod search;

use input::Input;
use plan::{Failure, Plan};
use search::Search;

/// How many states a search may look at before it gives up.
pub const BUDGET: usize = 4_000_000;

#[derive(serde::Serialize)]
#[serde(untagged)]
pub enum Reply {
    Plan { plan: Plan },
    Error { error: String },
}

pub fn route(input: &Input) -> Reply {
    let mut search = Search::new(input, BUDGET);
    match plan::plan(&mut search) {
        Ok(plan) => Reply::Plan { plan },
        Err(Failure::TooBig) => Reply::Error {
            error:
                "The search grew too big to finish. Turn off detours, or item use, and try again."
                    .into(),
        },
        Err(Failure::NoWay(world)) => Reply::Error {
            error: format!(
                "No way through world {} under these rules: every way through it goes by a stage with no history, \
                 and those are set to be avoided.",
                world + 1
            ),
        },
    }
}

/// The whole of it as the worker sees it: JSON in, JSON out. Numbers that are
/// not finite go out as null, as JSON has no other way to say them.
pub fn route_json(json: &[u8]) -> Vec<u8> {
    let reply = match serde_json::from_slice::<Input>(json) {
        Ok(input) => route(&input),
        Err(e) => Reply::Error {
            error: format!("The search could not read what it was given: {e}"),
        },
    };
    serde_json::to_vec(&reply).unwrap()
}

// ---------------------------------------------------------------------------
// The wasm boundary: the worker writes the request into memory it asks for
// here, calls route(), and reads the reply back out of the buffer it names.

#[cfg(target_arch = "wasm32")]
mod wasm {
    use std::cell::RefCell;

    thread_local! {
        static REPLY: RefCell<Vec<u8>> = const { RefCell::new(Vec::new()) };
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn alloc(len: usize) -> *mut u8 {
        let mut buf = Vec::<u8>::with_capacity(len);
        let ptr = buf.as_mut_ptr();
        std::mem::forget(buf);
        ptr
    }

    /// Takes the request back, reads it, and returns where the reply is.
    #[unsafe(no_mangle)]
    pub extern "C" fn route(ptr: *mut u8, len: usize) -> *const u8 {
        let request = unsafe { Vec::from_raw_parts(ptr, len, len) };
        let reply = super::route_json(&request);
        drop(request);
        REPLY.with(|r| {
            *r.borrow_mut() = reply;
            r.borrow().as_ptr()
        })
    }

    #[unsafe(no_mangle)]
    pub extern "C" fn reply_len() -> usize {
        REPLY.with(|r| r.borrow().len())
    }
}
