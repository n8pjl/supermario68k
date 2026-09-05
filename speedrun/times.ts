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

/**
 * When something happened, in the zone it happened in.
 *
 * Wall clock rather than the monotonic one a run is measured with: this says
 * which evening a time was set on, which is a different question from how long
 * it took and is the only one a calendar can answer.
 */
export function stamp(): Temporal.ZonedDateTime {
  return Temporal.Now.zonedDateTimeISO();
}

/**
 * The stamp a time carries when nothing recorded one.
 *
 * A function rather than a constant, and this is not a style choice: shell.js
 * imports the timer on every browser and only then asks whether Temporal is
 * there, so a module here that reaches for Temporal while it is being loaded
 * would take the whole page down on the browsers the timer is meant to be
 * quietly missing from.
 */
export function epoch(): Temporal.ZonedDateTime {
  return new Temporal.ZonedDateTime(0n, "UTC");
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

/** A stamp read back from a file, or null if it is not one. */
export function parseStamp(value: unknown): Temporal.ZonedDateTime | null {
  if (typeof value !== "string") return null;

  try {
    return Temporal.ZonedDateTime.from(value);
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
