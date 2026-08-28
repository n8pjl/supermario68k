const text = {
  main_menu: ["Main menu:", "New game", "Load game", "Options", "Quit"],
  load_menu: ["Load game:", "Save 1", "<empty>"],
  options_menu: ["Options:", "Statusbar", "Controls", "Back"],

  mid_game_map: ["Mid-game menu:", "Continue", "Save game", "Quit"],

  save_menu: ["Save game:", "Save 1", "<empty>"],
  game_over: ["Game over:", "Continue", "Quit"],
  overwrite: ["Overwrite?", "No", "Yes"],
};

const processedText = [
  "main_menu",
  "load_menu",
  "options_menu",
  "mid_game_map",
  "save_menu",
  "game_over",
  "overwrite",
].map((id) => text[id]);

const encodeTextInto = TextEncoder.prototype.encodeInto.bind(new TextEncoder());

let prevFrameTime = 0;
