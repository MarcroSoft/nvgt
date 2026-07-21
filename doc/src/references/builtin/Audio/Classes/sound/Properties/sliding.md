# sliding
Determine if a sound is currently sliding its pan or pitch (see `slide_pan` and `slide_pitch`).

`bool sound::sliding;`

## Remarks:
This property is read-only. It is true while either a pan slide or a pitch slide is in progress, and false otherwise. To check a specific parameter, use `pan_sliding` or `pitch_sliding` instead.
