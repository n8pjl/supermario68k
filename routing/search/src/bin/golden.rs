//! Checks the search against plans kept from an earlier one, and times it.
//!
//!     cargo run --release --bin golden -- <dir> [case...]
//!     cargo run --release --bin golden -- --write <dir> [case...]
//!
//! With --write, each case's plan is replaced with what the search makes of
//! it now, to check a change to the search against afterwards. The checks
//! run either way.
//!
//! <dir> holds maps.json, stats-<name>.json, and case-NN.json files, each
//! {stats: <name>, settings, ...plan or error}. Every field of every step is
//! compared, figures to within a millisecond's thousandth; `costed` is
//! reported apart, as which stages a search happens to cost depends on how it
//! searched.

use std::time::Instant;

use routing_search::input::Input;
use routing_search::{Reply, route};
use serde_json::Value;

fn diff(path: &str, a: &Value, b: &Value, out: &mut Vec<String>) {
    match (a, b) {
        (Value::Number(x), Value::Number(y)) => {
            let (x, y) = (x.as_f64().unwrap(), y.as_f64().unwrap());
            if (x - y).abs() > 1e-3 {
                out.push(format!("{path}: {x} vs {y}"));
            }
        }
        (Value::Array(x), Value::Array(y)) => {
            if x.len() != y.len() {
                out.push(format!("{path}: length {} vs {}", x.len(), y.len()));
            }
            for (i, (p, q)) in x.iter().zip(y).enumerate() {
                diff(&format!("{path}[{i}]"), p, q, out);
            }
        }
        (Value::Object(x), Value::Object(y)) => {
            for (k, v) in x {
                match y.get(k) {
                    Some(w) => diff(&format!("{path}.{k}"), v, w, out),
                    None => out.push(format!("{path}.{k}: missing")),
                }
            }
            for k in y.keys() {
                if !x.contains_key(k) {
                    out.push(format!("{path}.{k}: extra"));
                }
            }
        }
        _ if a == b => {}
        _ => out.push(format!("{path}: {a} vs {b}")),
    }
}

fn main() {
    let mut args: Vec<String> = std::env::args().skip(1).collect();
    let write = args.first().is_some_and(|a| a == "--write");
    if write {
        args.remove(0);
    }
    let dir = &args[0];
    let only: Vec<&String> = args[1..].iter().collect();
    let read = |name: &str| std::fs::read_to_string(format!("{dir}/{name}")).unwrap();
    let maps: Value = serde_json::from_str(&read("maps.json")).unwrap();

    let mut cases: Vec<String> = std::fs::read_dir(dir)
        .unwrap()
        .map(|e| e.unwrap().file_name().into_string().unwrap())
        .filter(|n| n.starts_with("case-"))
        .collect();
    cases.sort();

    let mut failed = 0;
    let mut total_ms = 0.0;
    for name in cases {
        let id = &name[5..7];
        if !only.is_empty() && !only.iter().any(|o| o.as_str() == id) {
            continue;
        }
        let want: Value = serde_json::from_str(&read(&name)).unwrap();
        let stats: Value = serde_json::from_str(&read(&format!(
            "stats-{}.json",
            want["stats"].as_str().unwrap()
        )))
        .unwrap();
        let input: Input = serde_json::from_value(serde_json::json!({
            "maps": maps, "stats": stats, "settings": want["settings"],
        }))
        .unwrap();

        let began = Instant::now();
        let reply = route(&input);
        let ms = began.elapsed().as_secs_f64() * 1000.0;
        total_ms += ms;

        // The floor must never be above the best: check it wherever the best
        // is known.
        let mut problems = Vec::new();
        {
            let mut search = routing_search::search::Search::new(&input, routing_search::BUDGET);
            let _ = routing_search::plan::plan(&mut search, None);
            for (s, v) in search.known() {
                let f = search.floor(s);
                if f > v + 1e-6 {
                    problems.push(format!("floor {f} above best {v} at {s:?}"));
                }
            }
        }

        let got = serde_json::to_value(&reply).unwrap();
        let mut costed = String::new();
        let mut states = 0;
        match (&reply, want.get("error")) {
            (Reply::Error { error }, Some(e)) => {
                if Value::String(format!("Error: {error}")) != *e {
                    problems.push(format!("error: {error} vs {e}"));
                }
            }
            (Reply::Error { error }, None) => problems.push(format!("error: {error}")),
            (Reply::Plan { .. }, Some(e)) => problems.push(format!("expected error {e}")),
            (Reply::Plan { .. }, None) => {
                let plan = &got["plan"];
                states = plan["states"].as_u64().unwrap();
                diff("total", &plan["total"], &want["total"], &mut problems);
                diff("steps", &plan["steps"], &want["steps"], &mut problems);
                let key = |c: &Value| {
                    format!(
                        "{}|{}",
                        c["place"].as_str().unwrap(),
                        c["entry"].as_str().unwrap()
                    )
                };
                let mine: Vec<String> =
                    plan["costed"].as_array().unwrap().iter().map(key).collect();
                let theirs: Vec<String> =
                    want["costed"].as_array().unwrap().iter().map(key).collect();
                if mine != theirs {
                    let extra: Vec<_> = mine.iter().filter(|k| !theirs.contains(k)).collect();
                    let missing: Vec<_> = theirs.iter().filter(|k| !mine.contains(k)).collect();
                    costed = format!(
                        " costed: +{} -{} {:?} {:?}",
                        extra.len(),
                        missing.len(),
                        &extra[..extra.len().min(3)],
                        &missing[..missing.len().min(3)]
                    );
                    if extra.is_empty() && missing.is_empty() {
                        costed = " costed: reordered".into();
                    }
                }
            }
        }
        if write {
            let mut case = want.clone();
            let obj = case.as_object_mut().unwrap();
            for key in ["total", "steps", "costed", "states", "ms", "error"] {
                obj.remove(key);
            }
            match &got {
                Value::Object(reply) if reply.contains_key("plan") => {
                    for (k, v) in reply["plan"].as_object().unwrap() {
                        obj.insert(k.clone(), v.clone());
                    }
                    obj.insert("ms".into(), ms.into());
                }
                Value::Object(reply) => {
                    obj.insert(
                        "error".into(),
                        format!("Error: {}", reply["error"].as_str().unwrap()).into(),
                    );
                }
                _ => unreachable!(),
            }
            std::fs::write(
                format!("{dir}/{name}"),
                serde_json::to_string(&case).unwrap(),
            )
            .unwrap();
            problems.clear();
            costed.clear();
        }
        let was = want.get("ms").and_then(Value::as_f64).unwrap_or(f64::NAN);
        let status = if problems.is_empty() { "ok  " } else { "FAIL" };
        let s = &want["settings"];
        println!(
            "{status} {id} {:>12} {:<7} {:<6} items={:<5} detours={:<5} up={:<5} {:<6} {:<10} {:>9.1}ms (ts {:>7.0}ms) {:>8} states{costed}",
            s["category"].as_str().unwrap(),
            s["objective"].as_str().unwrap(),
            s["unknown"].as_str().unwrap(),
            s["items"],
            s["detours"],
            s["borrowUp"],
            "",
            want["stats"].as_str().unwrap(),
            ms,
            was,
            states,
        );
        if !problems.is_empty() {
            failed += 1;
            for p in problems.iter().take(8) {
                println!("       {p}");
            }
        }
    }
    println!("{failed} failed, {total_ms:.0}ms in all");
    std::process::exit(if failed > 0 { 1 } else { 0 });
}
