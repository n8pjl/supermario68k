// Linked into the TAS build only (--js-library, see the Makefile): what the
// page needs from inside Emscripten's runtime to snapshot a suspended game and
// put one back. See src/tas.h for why that is all a snapshot is.
//
// Only meaningful while the game is suspended in tas_suspend(): that is when
// the whole call stack is in linear memory, and Asyncify.currData points at the
// block it was unwound into. Restoring then means writing the memory back, the
// stack pointer that unwinding left deep, and that pointer; the rewind that
// follows reads the rest - which export to re-enter, how far down the stack
// goes - out of the restored memory.
addToLibrary({
  $tasRuntime__deps: ["$Asyncify", "$stackSave", "$stackRestore"],
  $tasRuntime__postset: "Module.tasRuntime = tasRuntime;",
  $tasRuntime: {
    // A view of the whole of linear memory. The memory never grows (the link
    // does not allow it), so the view stays good for the life of the runtime.
    memory() {
      return HEAPU8;
    },

    // Where the snapshot ends: the break, below which everything allocated
    // and everything static lies.
    heapEnd() {
      return _tas_heap_end();
    },

    registers() {
      if (!Asyncify.currData) throw new Error("the game is not suspended");
      return { sp: stackSave(), data: Asyncify.currData };
    },

    setRegisters({ sp, data }) {
      stackRestore(sp);
      Asyncify.currData = data;
    },

    refresh() {
      _tas_refresh();
    },

    // src/tas.cpp's tas_player(), copied out of memory.
    player() {
      const at = _tas_player() >> 2;
      return Array.from(HEAP32.subarray(at, at + 13));
    },
  },
});
