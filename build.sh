## build.sh

### This script is used to build the project.
### It is called by `just`. It is not intended to be called directly.
### Be sure to install just, pkg-config, and dependencies before running this script.

set -ueo pipefail
# set -x

C_FLAGS="-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -pedantic -std=c99 -O2 $(pkg-config --cflags clay)"
LD_FLAGS="-lm"

INCLUDE_DIRS="-I./include"
C_FILES=""
O_FILES=""
UI_DEBUG=false

RENDERER="raylib" # default renderer

usage() {
  echo "Usage: build.sh [-r <renderer>] [-h]"
  echo ""
  echo "Options:"
  echo "  -r,  --renderer <renderer>  Specify the renderer to use. Currently supported renderers are: raylib"
  echo "  -c,  --compile              Compile the project without linking"
  echo "  -l,  --link                 Link the compiled object files to oasis"
  echo "  -b,  --build, (no argument) Compile and link the project"
  echo "  -ui, --ui-debug             Enable ui debug mode"
  echo "  -h,  --help                 Show this help message and exit"
  exit 0
}

config_compile() {
  local renderer_file_list
  local renderer_file

  local c_file_list
  local c_file

  local renderer_ld_flag
  local renderer_c_flag

  local renderer_upper

  if [ -z "$RENDERER" ]; then
    echo "No renderer specified, trying to build without renderer, this will go poorly"
    RENDERER="none"
  fi

  if [ "$RENDERER" != "none" ]; then
    c_file_list=$(find ./src -type f -name "*.c" ! -path "./src/renderers/*/*")
  else
    c_file_list=$(find ./src -type f -name "*.c" ! -path "./src/renderers/*/*" ! -path "./src/renderers/*")
  fi

  if [ -z "$c_file_list" ]; then
    echo "No C files found in ./src"
    exit 1
  fi

  for c_file in $c_file_list; do
    C_FILES+="$c_file "
  done

  if [ "$RENDERER" != "none" ]; then
    renderer_file_list=$(find "./src/renderers/$RENDERER" -name "*.c")
    if [ -z "$renderer_file_list" ]; then
      echo "No C files found in ./src/renderers/$RENDERER"
      exit 1
    fi

    for renderer_file in $renderer_file_list; do
      C_FILES+="$renderer_file "
    done

    renderer_c_flag=$(pkg-config --cflags "$RENDERER")
    renderer_ld_flag=$(pkg-config --libs "$RENDERER")

    LD_FLAGS+=" $renderer_ld_flag"

    renderer_upper=$(echo "$RENDERER" | tr '[:lower:]' '[:upper:]')
    if [ "$UI_DEBUG" = true ]; then
      C_FLAGS+=" -DDEBUG=1"
    fi

    C_FLAGS+=" -DRENDERER=$renderer_upper"
    C_FLAGS+=" $renderer_c_flag"
  fi

  echo "Making build directory"
  mkdir -p ./build/bin
  mkdir -p ./build/out
}

link_() {
  O_FILES=$(find ./build/out -name "*.o")
  if [ -z "$O_FILES" ]; then
    echo "No object files found in ./build/out"
    exit 1
  fi

  if gcc -o "./build/bin/oasis" $O_FILES $LD_FLAGS; then
    echo "Successfully linked object files to oasis"
  else
    echo "Failed to link object files to oasis"
    exit 1
  fi

  exit 0
}

build() {
  compile
  link_
}

compile() {
  local source_file

  config_compile

  for source_file in $C_FILES; do
    echo "Compiling $source_file..."
    if gcc -c $C_FLAGS $INCLUDE_DIRS $source_file -o "./build/out/$(basename "$source_file" .c).o" $LD_FLAGS; then
      echo "Compiled $source_file"
    else
      echo "Failed to compile $source_file"
      exit 1
    fi
  done

  echo "Done compiling!"
}

link() {
  local object_file

}

if [ $# -eq 0 ]; then
  build
fi

i=0
for arg in "$@"; do
  case $arg in
  -r | --renderer)
    RENDERER_IDX=$((i + 1))
    RENDERER="${*:$RENDERER_IDX:1}"
    ;;
  -ui | --ui-debug)
    UI_DEBUG=true
    ;;
  -c | --compile)
    compile
    exit 0
    ;;
  -l | --link)
    link_
    ;;
  -h | --help)
    usage
    ;;
  -b | --build | *)
    build
    ;;
  esac
  _=$((i++))

  if [ $i -eq $# ]; then
    build
  fi
done
