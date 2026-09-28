// A world map, drawn: the paths, what stands on them, how much history there
// is for each stage, and the route across it.
//
// Drawn from maps.ts rather than from the game's tiles - the graph the search
// walks is the thing worth seeing, and a square is a square either way. The
// route is laid over it by walking it again: the plan says where each step
// goes, and the shortest way there over what is open by then is the way the
// search costed it.

import { levelName, monsterName } from "../speedrun/names.ts";
import { type MapNode, MAPS, type WorldMap } from "./maps.ts";
import { type Step } from "./route.ts";

const SVG = "http://www.w3.org/2000/svg";
const CELL = 20;

type Attrs = Record<string, string | number>;

function svg<K extends keyof SVGElementTagNameMap>(
  tag: K,
  attrs: Attrs = {},
  parent?: Element,
): SVGElementTagNameMap[K] {
  const el = document.createElementNS(SVG, tag);
  for (const [name, value] of Object.entries(attrs)) el.setAttribute(name, String(value));
  parent?.append(el);
  return el;
}

const mid = (n: number) => n * CELL + CELL / 2;

/** How much history a stage has, for colouring it. */
export type Strength = "none" | "thin" | "ok";

/** What the map is told about each place, by place key. */
export interface PlaceInfo {
  readonly strength: Strength;
  readonly lines: readonly string[];
}

/** The short label a stage carries on the map: "3", "F", "F2", "C". */
function shortName(world: number, node: MapNode): string {
  if (node.kind === "castle") return "C";
  if (node.kind === "bowser") return "K";

  const name = levelName(world, node.level!).replace(/^\d+-/, "");
  const short: Record<string, string> = {
    Fortress: "F",
    "Fortress 1": "F1",
    "Fortress 2": "F2",
    Pyramid: "Py",
    Quicksand: "Qs",
    Bonus: "Bo",
    Pipe: "P",
  };
  return short[name] ?? name;
}

export function placeKey(world: number, node: MapNode): string | null {
  return node.level === undefined ? null : `L${world}.${node.level}`;
}

/** The squares a walk takes, node to node, over what is open by then. */
function route(
  map: WorldMap,
  from: number,
  to: number,
  done: ReadonlySet<number>,
  opened: ReadonlySet<number>,
  broken: ReadonlySet<number>,
): { walks: [number, number][][]; pipes: [number, number][][] } | null {
  const n = map.nodes.length;
  const dist = new Array<number>(n).fill(Infinity);
  const prev = new Array<number>(n).fill(-1);
  const via = new Array<number>(n).fill(-1);
  const settled = new Array<boolean>(n).fill(false);
  dist[from] = 0;

  for (;;) {
    let u = -1;
    for (let i = 0; i < n; i++) {
      if (!settled[i] && dist[i]! < Infinity && (u === -1 || dist[i]! < dist[u]!)) u = i;
    }
    if (u === -1 || u === to) break;
    settled[u] = true;

    // A pipe stage's square is walked past; its pipe is the stage, and is
    // drawn as its step rather than as a way to walk.
    const node = map.nodes[u]!;
    if (node.level !== undefined && node.exit === undefined && !done.has(u) && u !== from) continue;

    map.edges.forEach((e, i) => {
      if (e.a !== u) return;
      if (e.by === "pipe" && node.exit !== undefined) return;
      if (e.door !== undefined && !opened.has(e.door)) return;
      if (e.rock !== undefined && !broken.has(e.rock)) return;
      const d = dist[u]! + (e.by === "pipe" ? 60 : e.tiles);
      if (d < dist[e.b]!) {
        dist[e.b] = d;
        prev[e.b] = u;
        via[e.b] = i;
      }
    });
  }

  if (dist[to] === Infinity) return null;

  const legs: number[] = [];
  for (let at = to; at !== from; at = prev[at]!) legs.unshift(via[at]!);

  // A walk is broken at every pipe, which is drawn as a hop of its own.
  const walks: [number, number][][] = [[]];
  const pipes: [number, number][][] = [];
  for (const i of legs) {
    const e = map.edges[i]!;
    if (e.by === "pipe") {
      pipes.push(e.path.map(([x, y]) => [x, y]));
      walks.push([]);
    } else {
      walks.at(-1)!.push(...e.path.map(([x, y]) => [x, y] as [number, number]));
    }
  }
  return { walks, pipes };
}

/**
 * Draw world `w` into `host`, with the plan's steps in it overlaid if given.
 * `info` colours and labels the stages; `select` is called with a place key
 * when one is chosen.
 */
export function drawWorld(
  host: HTMLElement,
  w: number,
  info: ReadonlyMap<string, PlaceInfo>,
  steps: readonly Step[],
  selected: string | null,
  select: (place: string) => void,
): void {
  const map = MAPS[w]!;
  const root = svg("svg", {
    viewBox: `0 0 ${map.width * CELL} ${map.height * CELL}`,
    class: "world-map",
    role: "img",
    "aria-label": `World ${w + 1} map`,
  });

  // Ground and water.
  const ground = svg("g", { class: "ground" }, root);
  map.cells.forEach((row, y) => {
    [...row].forEach((c, x) => {
      if (c === "~") svg("rect", { x: x * CELL, y: y * CELL, width: CELL, height: CELL, class: "water" }, ground);
    });
  });

  // Roads, boat crossings and pipes.
  const roads = svg("g", { class: "roads" }, root);
  const drawnRoad = new Set<string>();
  for (const e of map.edges) {
    const key = [Math.min(e.a, e.b), Math.max(e.a, e.b), e.by].join();
    if (drawnRoad.has(key)) continue;
    drawnRoad.add(key);

    const points = e.path.map(([x, y]) => `${mid(x)},${mid(y)}`).join(" ");
    if (e.by === "pipe") {
      const [[x1, y1], [x2, y2]] = [e.path[0]!, e.path.at(-1)!];
      svg("path", {
        d: `M${mid(x1)},${mid(y1)} Q${(mid(x1) + mid(x2)) / 2},${Math.min(mid(y1), mid(y2)) - 3 * CELL} ${mid(x2)},${mid(y2)}`,
        class: "pipe-link",
      }, roads);
    } else {
      svg("polyline", {
        points,
        class: e.by === "boat" ? "boat-link" : e.door !== undefined || e.rock !== undefined ? "road locked" : "road",
      }, roads);
    }
  }

  // Doors and rocks.
  map.cells.forEach((row, y) => {
    [...row].forEach((c, x) => {
      if (c === "1" || c === "2") {
        const g = svg("g", { class: "door" }, roads);
        svg("rect", { x: x * CELL + 5, y: y * CELL + 5, width: CELL - 10, height: CELL - 10 }, g);
        svg("title", {}, g).textContent = `Locked door: opened by beating the fortress${c === "2" ? " (the second kind)" : ""}`;
      } else if (c === "r") {
        const g = svg("g", { class: "rock" }, roads);
        const r = CELL / 3;
        svg("path", {
          d: `M${mid(x) - r},${mid(y) + r * 0.7} L${mid(x) - r * 0.5},${mid(y) - r} L${mid(x) + r * 0.6},${mid(y) - r * 0.8} L${mid(x) + r},${mid(y) + r * 0.7} Z`,
        }, g);
        svg("title", {}, g).textContent = "A rock the hammer can break";
      }
    });
  });

  // The route.
  const worldSteps = steps.filter((s) => s.world === w);
  const overlay = svg("g", { class: "route" }, root);
  const done = new Set<number>();
  const opened = new Set<number>();
  const broken = new Set<number>();
  let pos = map.start;
  const order = new Map<number, number[]>();

  worldSteps.forEach((step, i) => {
    const there = route(map, pos, step.node, done, opened, broken);
    for (const walk of there?.walks ?? []) {
      if (walk.length < 2) continue;
      svg("polyline", { points: walk.map(([x, y]) => `${mid(x)},${mid(y)}`).join(" "), class: "route-line" }, overlay);
    }
    for (const pipe of there?.pipes ?? []) {
      const [[x1, y1], [x2, y2]] = [pipe[0]!, pipe.at(-1)!];
      svg("path", {
        d: `M${mid(x1)},${mid(y1)} Q${(mid(x1) + mid(x2)) / 2},${Math.min(mid(y1), mid(y2)) - 3 * CELL} ${mid(x2)},${mid(y2)}`,
        class: "route-line pipe",
      }, overlay);
    }
    if (step.kind === "cloud") {
      const a = map.nodes[step.node]!;
      const b = map.nodes[step.to]!;
      svg("line", { x1: mid(a.x), y1: mid(a.y), x2: mid(b.x), y2: mid(b.y), class: "route-line cloud" }, overlay);
    }
    if (step.kind === "stage" && step.to !== step.node) {
      const a = map.nodes[step.node]!;
      const b = map.nodes[step.to]!;
      svg("path", {
        d: `M${mid(a.x)},${mid(a.y)} Q${(mid(a.x) + mid(b.x)) / 2},${Math.min(mid(a.y), mid(b.y)) - 3 * CELL} ${mid(b.x)},${mid(b.y)}`,
        class: "route-line pipe",
      }, overlay);
    }

    const node = map.nodes[step.node]!;
    if (step.kind === "stage") {
      done.add(step.node);
      if (node.opens !== undefined) opened.add(node.opens);
    }
    if (step.kind === "rock") {
      const rock = map.rocks.findIndex((r, j) => r.from === step.node && !broken.has(j));
      if (rock !== -1) broken.add(rock);
    }
    pos = step.to;
    order.set(step.node, [...(order.get(step.node) ?? []), i + 1]);
  });

  // Stopping squares.
  const nodes = svg("g", { class: "nodes" }, root);
  for (const node of map.nodes) {
    const cx = mid(node.x);
    const cy = mid(node.y);
    const place = placeKey(w, node);
    const g = svg("g", { class: `node ${node.kind}` }, nodes);
    const title = svg("title", {}, g);

    if (place !== null) {
      const about = info.get(place);
      g.classList.add(`data-${about?.strength ?? "none"}`);
      if (place === selected) g.classList.add("selected");
      svg("rect", { x: cx - 9, y: cy - 9, width: 18, height: 18, rx: 4 }, g);
      svg("text", { x: cx, y: cy + 4 }, g).textContent = shortName(w, node);
      title.textContent = [levelName(w, node.level!), ...(about?.lines ?? ["No history yet"])].join("\n");
      g.setAttribute("tabindex", "0");
      g.setAttribute("role", "button");
      g.setAttribute("aria-label", levelName(w, node.level!));
      g.addEventListener("click", () => select(place));
      g.addEventListener("keydown", (e) => {
        if (e.key === "Enter" || e.key === " ") {
          e.preventDefault();
          select(place);
        }
      });
    } else if (node.kind === "house" || node.kind === "game-house") {
      svg("circle", { cx, cy, r: 7 }, g);
      svg("text", { x: cx, y: cy + 3.5 }, g).textContent = node.kind === "house" ? "H" : "G";
      title.textContent =
        node.kind === "house"
          ? "Mushroom house: a mushroom (1 in 4), a fire flower (1 in 4) or a leaf (1 in 2)"
          : "Game house: the slot machine, which pays lives";
    } else if (node.kind === "pipe") {
      svg("rect", { x: cx - 5, y: cy - 6, width: 10, height: 12, rx: 2 }, g);
      title.textContent = "Pipe";
    } else if (node.kind === "dock") {
      svg("rect", { x: cx - 6, y: cy - 3, width: 12, height: 6, rx: 2 }, g);
      title.textContent = "Dock";
    } else {
      svg("circle", { cx, cy, r: 3 }, g);
    }

    if (node.id === map.start) {
      const s = svg("g", { class: "start" }, nodes);
      svg("circle", { cx: cx - 9, cy: cy - 9, r: 6 }, s);
      svg("text", { x: cx - 9, y: cy - 6.5 }, s).textContent = "S";
      svg("title", {}, s).textContent = "Where the world starts";
    }
  }

  // Bros., where they start.
  for (const bros of map.bros) {
    const node = map.nodes[bros.node]!;
    const place = `M${w}.${bros.monster}`;
    const about = info.get(place);
    const g = svg("g", { class: `bros data-${about?.strength ?? "none"}` }, nodes);
    const x = mid(node.x) + 9;
    const y = mid(node.y) + 9;
    svg("path", { d: `M${x},${y - 7} L${x + 7},${y + 5} L${x - 7},${y + 5} Z` }, g);
    svg("title", {}, g).textContent = [
      `${monsterName(w, bros.monster)} starts here and wanders`,
      `Drops: ${bros.treasure === "random" ? "a mushroom, fire flower or leaf" : bros.treasure}`,
      ...(about?.lines ?? ["No history yet"]),
    ].join("\n");
    g.setAttribute("tabindex", "0");
    g.addEventListener("click", () => select(place));
  }

  // Step numbers, on top of everything.
  for (const [node, numbers] of order) {
    const n = map.nodes[node]!;
    const g = svg("g", { class: "step-badge" }, root);
    const label = numbers.join(",");
    const width = Math.max(12, label.length * 6 + 6);
    svg("rect", { x: mid(n.x) + 4, y: mid(n.y) - 18, width, height: 11, rx: 5.5 }, g);
    svg("text", { x: mid(n.x) + 4 + width / 2, y: mid(n.y) - 9.5 }, g).textContent = label;
  }

  host.replaceChildren(root);
}
