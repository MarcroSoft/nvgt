# pan_sliding
Determine if a sound is currently sliding its pan (see `slide_pan`).

`bool sound::pan_sliding;`

## Remarks:
This property is read-only. It becomes true when you start a pan slide and returns to false once the slide reaches its target or is cancelled by directly setting the `pan` property.
