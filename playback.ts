// A TAS movie played back on the game page: the shipped build, with its own
// frame pacing, rather than the TAS build the movie was made on. The point is a
// recording - the page fullscreen, the timer beside it - that looks and times
// exactly as a player pressing those buttons would have.
//
// The game is the same from power-on given the same input on every frame, and
// nothing about the shipped build changes that: what it adds is the pace,
// which only decides when a frame is played, never what it does. So all there
// is to playing a movie is counting the frames as the game lets them go -
// src/compat/gray.cpp and src/scankeys.cpp tell Module.onFrame, at the same
// places the TAS build suspends - and answering Module.gameActions() with the
// movie's line for the frame the count is on. A frame the browser is late with
// comes late, and is still the same frame.

import { actions, type Button, type Movie } from "./tas/movie.ts";

// A movie is read from its text, and checked, the way the TAS page reads one.
export { MovieError, parse as readMovie } from "./tas/movie.ts";

/** One playing of a movie, from power-on. */
export class Playback {
  readonly movie: Movie;
  #frame = 0;

  constructor(movie: Movie) {
    this.movie = movie;
  }

  /** The Module.onFrame hook: the game has let a frame go. */
  advance(): void {
    this.#frame++;
  }

  /**
   * This frame's buttons, or null once the movie has run out. The shell hands
   * the game back to the player then, rather than leaving it with nothing
   * held: whatever comes after a movie is nobody's but theirs.
   */
  get actions(): Record<Button, boolean> | null {
    const input = this.movie.inputs[this.#frame];
    return input === undefined ? null : actions(input);
  }
}
