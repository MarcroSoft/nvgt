# pitch_sliding
Determine if a sound is currently sliding its pitch (see `slide_pitch`).

`bool sound::pitch_sliding;`

## Remarks:
This property is read-only. It becomes true when you start a pitch slide and returns to false once the slide reaches its target or is cancelled by directly setting the `pitch` property.
