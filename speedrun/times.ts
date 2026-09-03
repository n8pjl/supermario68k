// Durations, and the way they are shown and written down.
//
// A run is measured with performance.now(), which is monotonic - a wall clock
// can step backwards mid-run and take the elapsed time with it. Everything past
// that one reading is a Temporal.Duration, including what is saved: durations
// go to file in their own ISO 8601 form rather than as a count of milliseconds,
// so an exported file says what its numbers are.

/**
 * A measured stretch of milliseconds as a balanced duration.
 *
 * Truncated rather than rounded: a clock shows the time that has passed, and
 * rounding up would show a hundredth that has not.
 */
export function duration(ms: number): Temporal.Duration {
  return Temporal.Duration.from({ milliseconds: Math.trunc(ms) }).round({
    largestUnit: "hour",
    smallestUnit: "millisecond",
  });
}

/** A duration read back from a file, or null if it is not one. */
export function parseDuration(value: unknown): Temporal.Duration | null {
  if (typeof value !== "string") return null;

  try {
    return Temporal.Duration.from(value);
  } catch {
    return null;
  }
}

export function longer(
  a: Temporal.Duration,
  b: Temporal.Duration,
): Temporal.Duration {
  return Temporal.Duration.compare(a, b) >= 0 ? a : b;
}

export function shorter(
  a: Temporal.Duration,
  b: Temporal.Duration,
): Temporal.Duration {
  return Temporal.Duration.compare(a, b) <= 0 ? a : b;
}

/** A time on the clock: m:ss.cc, and h:mm:ss.cc once there is an hour to show. */
export function formatDuration(d: Temporal.Duration): string {
  const t = d.abs();
  const cs = String(Math.trunc(t.milliseconds / 10)).padStart(2, "0");
  const ss = String(t.seconds).padStart(2, "0");

  return t.hours > 0
    ? `${t.hours}:${String(t.minutes).padStart(2, "0")}:${ss}.${cs}`
    : `${t.minutes}:${ss}.${cs}`;
}

/**
 * A difference against another run, always signed.
 *
 * Read at a glance rather than studied, so it is shown to a tenth and only as
 * precisely as it has to be: seconds while it is under a minute, minutes after.
 */
export function formatDelta(d: Temporal.Duration): string {
  const sign = d.sign < 0 ? "−" : "+";
  const t = d.abs();
  const tenth = Math.trunc(t.milliseconds / 100);
  const ss = String(t.seconds).padStart(2, "0");

  if (t.hours > 0) {
    return `${sign}${t.hours}:${String(t.minutes).padStart(2, "0")}:${ss}`;
  }

  return t.minutes > 0
    ? `${sign}${t.minutes}:${ss}.${tenth}`
    : `${sign}${t.seconds}.${tenth}`;
}
