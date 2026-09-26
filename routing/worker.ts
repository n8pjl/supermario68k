// The route search, off the page's thread: it can take a few seconds, and the
// page is meant to stay usable - and to keep following a game being played in
// another tab - while it does.
//
// One message in, the history's figures and the settings; one message out,
// the plan or why there is none.

import { type Stats } from "./model.ts";
import { TooBig, type Settings } from "./search.ts";
import { type Plan, plan } from "./plan.ts";

export interface Request {
  readonly id: number;
  readonly stats: Stats;
  readonly settings: Settings;
}

export type Reply =
  | { readonly id: number; readonly plan: Plan }
  | { readonly id: number; readonly error: string };

addEventListener("message", (e: MessageEvent<Request>) => {
  const { id, stats, settings } = e.data;
  let reply: Reply;

  try {
    reply = { id, plan: plan(stats, settings) };
  } catch (error) {
    reply = {
      id,
      error:
        error instanceof TooBig
          ? "The search grew too big to finish. Turn off detours, or item use, and try again."
          : error instanceof Error
            ? error.message
            : String(error),
    };
  }
  postMessage(reply);
});
