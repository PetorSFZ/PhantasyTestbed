# Phantasy Testbed
A simple application using Phantasy Engine. The goal is to eventually turn it into a testbed for the continuing development of the engine.

Currently in a pretty rough state, may or may not build properly.

Currently expects to be placed inside the same directory containing PhantasyEngine. I.e. something like this:

~~~
PhantasyEngineDevelopmentDir/
	PhantasyEngine/
	PhantasyTestbed/
		README.md* <-- The readme you are currently reading!
~~~

## Building

### Windows

Activate Windows 10 Developer Mode, then just open the directory with Visual Studio (2019+) directly. For VS 2017 use `build_scripts/gen_visualstudio2017.bat`.

### Web (Emscripten on Windows)

* Install Make for Windows (available in chocolatey)
* Install Emscripten, run:
  * `emsdk update`
  * `emsdk install latest`
  * `emsdk activate latest`
* To use emscripten in a terminal first run `emsdk_env.bat`, then `emcc` is available.
* Set the `EMSCRIPTEN` system variable properly
* `cmake -DCMAKE_TOOLCHAIN_FILE="%EMSCRIPTEN%\cmake\Modules\Platform\Emscripten.cmake" .. -G "Unix Makefiles" `

In order to run locally in a browser you need to host the files. Recommend using python http-here:

* `pip install http-here`
* `python3 -m http.server --bind 127.0.0.1`

### Web (Emscripten on macOS)

1. Install Xcode, Xcode command line, Homebrew.
2. `brew install emscripten`
   1. Run `emcc` once
   2. Modify `~/emscripten`:
      1. LLVM should be found at `/usr/local/opt/emscripten/libexec/llvm/bin`
      2. Comment out `BINARYEN_ROOT`
3. Set `EMSCRIPTEN` path variable
   1. I.e., edit `.bash_profile`
   2. Example `EMSCRIPTEN=/usr/local/Cellar/emscripten/1.37.18/libexec`, but you have to change version to the latest one you have.
4. Generate buiild files with: `cmake -DCMAKE_TOOLCHAIN_FILE="$EMSCRIPTEN/cmake/Modules/Platform/Emscripten.cmake" .. -G "Unix Makefiles"`
5. Use python for web server
   1. `brew install python3`
   2. `pip3 install http-here`
   3. `python3 -m http.server --bind 127.0.0.1`
