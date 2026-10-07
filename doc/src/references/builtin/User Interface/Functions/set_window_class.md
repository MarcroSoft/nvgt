# set_window_class
Sets the window class name of your game's windows on Windows, in place of SDL's default "SDL_app". Screen reader app modules and scripts, as well as other automation tools, often recognise programs by their window class, so a class of its own lets them tell your game apart from every other NVGT game.

`bool set_window_class(const string&in name);`

## Arguments:
* const string&in name: the window class name to use.

## Returns:
bool: true if the class was registered, false otherwise.

## Remarks:
The window class is fixed when NVGT starts its windowing system, which happens the first time a window is shown or input is checked. Call this function before that, for example at the very start of your main function; once the windowing system is running it returns false and the class stays as it was.

This function only has an effect on Windows. On other platforms it does nothing and returns false.
