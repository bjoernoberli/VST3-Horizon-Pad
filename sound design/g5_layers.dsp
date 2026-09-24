//-----------------------------------------------------------------------
// G5 null-test harness - the four prototype layers, DRY.
//
// NOT the product and not a second source of truth: every definition here
// is pulled out of four_pads.dsp with Faust's library() primitive, so this
// file holds no copy of the DSP and cannot drift away from it.
//
// Why it exists: four_pads.dsp's `process` is the four layers summed,
// through the shared freeverb, the master Haas split and a x1.6 trim. The
// C++ port restructured all three of those (per-layer WIDTH instead of one
// master Haas, an equal-power reverb crossfade instead of a fixed wet add,
// a limiter instead of a trim - see docs/g1-algorithm.md). Comparing the
// two `process` outputs therefore measures the master stage, not the port.
//
// What the null test actually wants to compare is the part that WAS ported
// structurally: oscillators -> filter -> envelope, per layer. That is what
// this exposes - one layer per output channel, dry, no master stage.
//
//   channel 0: Warm Foundation   channel 2: Airy Choir
//   channel 1: Analog Ensemble   channel 3: Motion Pad
//-----------------------------------------------------------------------

fp = library("four_pads.dsp");

process = fp.warmFoundation, fp.analogEnsemble, fp.airyChoir, fp.motionPad;
