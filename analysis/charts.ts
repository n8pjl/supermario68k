// The page's three charts, drawn as SVG at the width they are given.
//
// Each takes a host element, empties it and draws into it at the host's current
// width, so redrawing on a resize is calling it again. Values are milliseconds
// or counts; every chart reads the colours it uses off the page's custom
// properties (data.css), so light and dark are the stylesheet's business.
// Every mark carries its own tooltip text, shown on hover and on focus.

const SVG = "http://www.w3.org/2000/svg";

type Attrs = Record<string, string | number>;

function svg<K extends keyof SVGElementTagNameMap>(
  tag: K,
  attrs: Attrs = {},
  parent?: Element,
): SVGElementTagNameMap[K] {
  const el = document.createElementNS(SVG, tag);

  for (const [name, value] of Object.entries(attrs)) {
    el.setAttribute(name, String(value));
  }
  parent?.append(el);
  return el;
}

function text(parent: Element, x: number, y: number, content: string, attrs: Attrs = {}) {
  svg("text", { x, y, ...attrs }, parent).textContent = content;
}

/** One tooltip for the page, placed beside whichever mark asked for it. */
const tip = document.createElement("div");
tip.className = "tip";
tip.hidden = true;
document.body.append(tip);

function showTip(lines: readonly string[], x: number, y: number): void {
  tip.replaceChildren(
    ...lines.map((line, i) => {
      const el = document.createElement(i === 0 ? "strong" : "span");
      el.textContent = line;
      return el;
    }),
  );
  tip.hidden = false;

  const box = tip.getBoundingClientRect();
  const left = Math.min(x + 14, window.innerWidth - box.width - 8);
  const top = y - box.height - 10 < 0 ? y + 14 : y - box.height - 10;

  tip.style.left = `${Math.max(8, left) + window.scrollX}px`;
  tip.style.top = `${top + window.scrollY}px`;
}

function hideTip(): void {
  tip.hidden = true;
}

/** Hover and focus on one mark, with a hit area of its own. */
function hoverable(el: SVGElement, lines: readonly string[]): void {
  el.setAttribute("tabindex", "0");
  el.setAttribute("aria-label", lines.join(", "));
  el.classList.add("hit");
  el.addEventListener("pointermove", (e) => showTip(lines, e.clientX, e.clientY));
  el.addEventListener("pointerleave", hideTip);
  el.addEventListener("focus", () => {
    const box = el.getBoundingClientRect();
    showTip(lines, box.left + box.width / 2, box.top);
  });
  el.addEventListener("blur", hideTip);
}

/**
 * Round, readable ticks covering lo..hi: 1, 2 or 5 times a power of ten, in the
 * unit given, from a tick at or under lo to one at or over hi.
 */
function ticks(lo: number, hi: number, count: number, unit = 1): number[] {
  const raw = Math.max(hi - lo, unit) / unit / count;
  const pow = 10 ** Math.floor(Math.log10(raw));
  const step = ([1, 2, 5, 10].find((m) => m * pow >= raw) ?? 10) * pow * unit;
  const out: number[] = [];

  for (let v = Math.floor(lo / step) * step; v < hi + step; v += step) out.push(v);
  return out;
}

/** Ticks for a span of milliseconds, on whole seconds or minutes. */
function timeTicks(lo: number, hi: number, count: number): number[] {
  return ticks(lo, hi, count, hi - lo > 3 * 60_000 ? 60_000 : 1000);
}

export function clock(ms: number): string {
  const total = Math.max(0, Math.trunc(ms / 10));
  const cs = String(total % 100).padStart(2, "0");
  const s = Math.trunc(total / 100);
  const ss = String(s % 60).padStart(2, "0");
  const m = Math.trunc(s / 60);

  return m >= 60
    ? `${Math.trunc(m / 60)}:${String(m % 60).padStart(2, "0")}:${ss}.${cs}`
    : `${m}:${ss}.${cs}`;
}

/** An axis label: no hundredths, which a tick never needs. */
function shortClock(ms: number): string {
  return clock(ms).replace(/\.\d\d$/, "");
}

function frame(host: HTMLElement, height: number, label: string) {
  const width = Math.max(280, host.clientWidth);
  const root = svg("svg", {
    width,
    height,
    viewBox: `0 0 ${width} ${height}`,
    role: "img",
    "aria-label": label,
  });

  host.replaceChildren(root);
  return { root, width };
}

export interface TimePoint {
  /** Epoch milliseconds. */
  readonly when: number;
  readonly value: number;
  readonly lines: readonly string[];
}

/**
 * Finished run times against the day they were set, with the best time so far
 * stepped along underneath: the one line anyone asks of a run history.
 */
export function progressChart(host: HTMLElement, points: readonly TimePoint[]): void {
  const height = 260;
  const { root, width } = frame(host, height, "Finished run times over time");
  const m = { top: 12, right: 16, bottom: 28, left: 56 };
  const w = width - m.left - m.right;
  const h = height - m.top - m.bottom;

  const lo = Math.min(...points.map((p) => p.value));
  const hi = Math.max(...points.map((p) => p.value));
  const pad = Math.max((hi - lo) * 0.1, 1000);
  const yTicks = timeTicks(Math.max(0, lo - pad), hi + pad, 5);
  const y0 = yTicks[0]!;
  const y1 = yTicks.at(-1)!;
  const t0 = points[0]!.when;
  const t1 = points.at(-1)!.when;
  const x = (t: number) => m.left + (t1 === t0 ? w / 2 : ((t - t0) / (t1 - t0)) * w);
  const y = (v: number) => m.top + h - ((v - y0) / (y1 - y0)) * h;

  const grid = svg("g", { class: "grid" }, root);
  for (const t of yTicks) {
    svg("line", { x1: m.left, x2: m.left + w, y1: y(t), y2: y(t) }, grid);
    text(grid, m.left - 8, y(t) + 4, shortClock(t), { "text-anchor": "end" });
  }

  const days = [t0, t1].filter((t, i, all) => all.indexOf(t) === i);
  for (const t of days) {
    text(grid, x(t), height - 8, new Date(t).toLocaleDateString(), {
      "text-anchor": t === t0 && days.length > 1 ? "start" : "end",
    });
  }

  let best = Infinity;
  let path = "";
  for (const p of points) {
    if (p.value < best) {
      path += path === "" ? `M${x(p.when)},${y(p.value)}` : `H${x(p.when)}V${y(p.value)}`;
      best = p.value;
    }
  }
  path += `H${x(t1)}`;
  svg("path", { d: path, class: "line series-2" }, root);

  for (const p of points) {
    const g = svg("g", {}, root);
    svg("circle", { cx: x(p.when), cy: y(p.value), r: 12, class: "hit-area" }, g);
    svg("circle", { cx: x(p.when), cy: y(p.value), r: 4, class: "dot series-1" }, g);
    hoverable(g, p.lines);
  }
}

export interface Bar {
  readonly label: string;
  readonly value: number;
  readonly lines: readonly string[];
}

/** Counts, one bar each, largest first, labelled at both ends. */
export function barChart(host: HTMLElement, bars: readonly Bar[], label: string): void {
  const row = 26;
  const height = bars.length * row + 8;
  const { root, width } = frame(host, height, label);
  const labelWidth = Math.min(180, width * 0.4);
  const w = width - labelWidth - 48;
  const max = Math.max(1, ...bars.map((b) => b.value));

  bars.forEach((bar, i) => {
    const top = i * row + 4;
    const g = svg("g", {}, root);

    svg("rect", { x: 0, y: top, width, height: row, class: "hit-area" }, g);
    text(g, labelWidth - 10, top + row / 2 + 4, bar.label, {
      "text-anchor": "end",
      class: "label",
    });
    svg(
      "rect",
      {
        x: labelWidth,
        y: top + 5,
        width: Math.max(2, (bar.value / max) * w),
        height: row - 10,
        rx: 3,
        class: "bar series-1",
      },
      g,
    );
    text(g, labelWidth + (bar.value / max) * w + 6, top + row / 2 + 4, String(bar.value), {
      class: "value",
    });
    hoverable(g, bar.lines);
  });
}

export interface StripPoint {
  readonly value: number;
  /** A clear is drawn filled; anything else as a ring in the second colour. */
  readonly cleared: boolean;
  readonly lines: readonly string[];
}

export interface Strip {
  readonly label: string;
  readonly points: readonly StripPoint[];
}

/**
 * Every visit to one place, a row for each thing the player walked in as, laid
 * out by how long it lasted: clears where they finished, deaths where they
 * happened.
 */
export function stripChart(host: HTMLElement, strips: readonly Strip[]): void {
  const row = 34;
  const m = { top: 8, right: 16, bottom: 28, left: 120 };
  const height = m.top + strips.length * row + m.bottom;
  const { root, width } = frame(host, height, "Time in each visit, by loadout");
  const w = width - m.left - m.right;
  const max = Math.max(1000, ...strips.flatMap((s) => s.points.map((p) => p.value)));
  const xTicks = timeTicks(0, max, 6);
  const x1 = xTicks.at(-1)!;
  const x = (v: number) => m.left + (v / x1) * w;

  const grid = svg("g", { class: "grid" }, root);
  for (const t of xTicks) {
    svg("line", { x1: x(t), x2: x(t), y1: m.top, y2: height - m.bottom }, grid);
    text(grid, x(t), height - 8, shortClock(t), { "text-anchor": "middle" });
  }

  strips.forEach((strip, i) => {
    const mid = m.top + i * row + row / 2;

    text(root, m.left - 10, mid + 4, strip.label, { "text-anchor": "end", class: "label" });

    // A little vertical spread, deterministic, so repeats of one time stay
    // countable rather than stacking into one dot.
    strip.points.forEach((p, j) => {
      const jitter = ((j * 7) % 5 - 2) * 3;
      const g = svg("g", {}, root);

      svg("circle", { cx: x(p.value), cy: mid + jitter, r: 10, class: "hit-area" }, g);
      svg(
        "circle",
        {
          cx: x(p.value),
          cy: mid + jitter,
          r: 4,
          class: p.cleared ? "dot series-1" : "ring series-2",
        },
        g,
      );
      hoverable(g, p.lines);
    });
  });
}
