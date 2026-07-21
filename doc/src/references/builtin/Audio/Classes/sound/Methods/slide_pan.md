# slide_pan
Smoothly slide the sound's pan from its current value to a target over a period of time.

`void sound::slide_pan(float target, uint64 length);`
`void sound::slide_pan_in_frames(float target, uint64 length_frames);`
`void sound::slide_pan_in_milliseconds(float target, uint64 length_ms);`

## Arguments:
* float target: the pan value to slide to (the same range and units as the `pan` property).
* uint64 length: how long the slide should take. For `slide_pan` this is interpreted as milliseconds unless the parent engine was created with the `AUDIO_ENGINE_DURATIONS_IN_FRAMES` flag, in which case it is in frames.

## Remarks:
The slide starts from whatever the pan currently is and advances on the sound's own playback clock, so it pauses and resumes together with the sound. A length of 0 applies the target immediately.

Directly assigning to the `pan` property cancels a pan slide that is in progress. Use the `pan_sliding` property (or the combined `sliding` property) to tell whether a slide is still running.

This is the smoothly-interpolated counterpart to setting `pan` directly, analogous to how `set_fade` relates to setting `volume`.
