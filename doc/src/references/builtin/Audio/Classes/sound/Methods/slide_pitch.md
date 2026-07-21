# slide_pitch
Smoothly slide the sound's pitch from its current value to a target over a period of time.

`void sound::slide_pitch(float target, uint64 length);`
`void sound::slide_pitch_in_frames(float target, uint64 length_frames);`
`void sound::slide_pitch_in_milliseconds(float target, uint64 length_ms);`

## Arguments:
* float target: the pitch value to slide to (the same range and units as the `pitch` property; must be greater than 0).
* uint64 length: how long the slide should take. For `slide_pitch` this is interpreted as milliseconds unless the parent engine was created with the `AUDIO_ENGINE_DURATIONS_IN_FRAMES` flag, in which case it is in frames.

## Remarks:
The slide starts from whatever the pitch currently is and advances on the sound's own playback clock, so it pauses and resumes together with the sound. A length of 0 applies the target immediately.

Directly assigning to the `pitch` property cancels a pitch slide that is in progress. Use the `pitch_sliding` property (or the combined `sliding` property) to tell whether a slide is still running.

Because pitch is realized through resampling, very fast pitch slides may produce minor artifacts at buffer boundaries; typical slide durations are unaffected.
