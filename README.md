# Oasis

>[!NOTE]
> This project is heavily WIP and is not ready for use and is not even in a state where it can be built or run, it's now just a repository for me to experiment with.

A media and music player written in C, using [clay](https://github.com/nicbarker/clay), [raylib](https://github.com/raysan5/raylib) and [ffmpeg](https://github.com/ffmpeg/ffmpeg), with support for multiple renderers, including web.

## Where does the name Oasis come from?

The name Oasis is a reference to the movie [Ready Player One](https://en.wikipedia.org/wiki/Ready_Player_One), which is a game about a virtual reality game.

It has no real connection to the project, other than the name, and it's purely just cause I like the movie, and it's a cool name.

## Building

To build the project, run `just build`.

## Why not use CMake, Meson, etc.?

I particularly dislike CMake and Meson and similar build systems, simply because they're a pain to set up and use.

I've tried using CMake and Meson before, but I found them to be either too annoying to use or too complicated to set up.

## Running

To run the project with the default renderer, run `just run`.

To run the project with a different renderer, pass the renderer name as an argument to `just run`, e.g. `just run raylib`.

## Contributing

Contributions are welcome!

## License

This project is licensed under the BSD 3-Clause License - see the [LICENSE](LICENSE) file for details.
