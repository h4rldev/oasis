## build.sh

### This script is used to build the project.
### It is called by `just`. It is not intended to be called directly.
### Be sure to install just, pkg-config, and dependencies before running this script.

set -ueo pipefail
# set -x

COLORS=true # set to false to disable colors

C_FLAGS="-Wall -Wextra -Werror -Wno-unused-parameter -Wno-unused-but-set-variable -Wno-unused-but-set-parameter -pedantic -std=c99 -O2 $(pkg-config --cflags clay)"
LD_FLAGS="-lm -lbsd $(pkg-config --libs libavcodec libavformat libavutil)"

INCLUDE_DIRS="-I./include"
C_FILES=""
O_FILES=""
DEBUG=false

# Planned renderers are: raylib, sdl2, sdl3, sokol, vulkan, cairo, wasm
SUPPORTED_RENDERERS=("raylib")
RENDERER="raylib" # default renderer

if ${COLORS}; then
  ESCAPE=$(printf "\e")
	RED="${ESCAPE}[0;31m"
	GREEN="${ESCAPE}[0;32m"
	YELLOW="${ESCAPE}[1;33m"
	BLUE="${ESCAPE}[0;34m"
	CYAN="${ESCAPE}[0;36m"
	CLEAR="${ESCAPE}[0m"
fi

print_supported_renderers() {
  for ((i = 0; i < ${#SUPPORTED_RENDERERS[@]}; i++)); do
    printf "%b%s%b" "$CYAN" "${SUPPORTED_RENDERERS[$i]}" "$CLEAR"
    [[ $i < $((${#SUPPORTED_RENDERERS[@]} - 1)) ]] && printf ", "
  done
}

print_line() {
  level=$1
  message=$2

  if [ -z "$message" ]; then
    message="No message provided"
  fi

  if [ -z "$level" ]; then
    level="info"
  fi

  case $level in
  "info")
    echo -e "${GREEN}>${CLEAR} $message"
    ;;
  "warn")
    echo -e "${YELLOW}?${CLEAR} $message"
    ;;
  "error")
    echo -e "${RED}! $message${CLEAR}"
    exit 1
    ;;
  "success")
    echo -e "${GREEN}✓${CLEAR} $message"
    ;;
  *)
    echo -e "${GREEN}>${CLEAR} $message"
    ;;
  esac
}

usage() {
cat << EOF
${BLUE}Usage${CLEAR}: build.sh [-r <renderer>] [-h]

Currently supported renderers are: $(print_supported_renderers) 

${GREEN}Options${CLEAR}:
${RED}  -r --renderer ${CYAN}<renderer>${CLEAR}         Specify the renderer to use. 
${RED}  -c --compile                   ${CLEAR}  Compile the project without linking
${RED}  -l --link                      ${CLEAR}  Link the compiled object files to oasis
${RED}  -b --build (no argument)       ${CLEAR}  Compile and link the project
${RED}  -d --debug                     ${CLEAR}  Build with debug and address sanitizer enabled
${RED}  -h --help                      ${CLEAR}  Show this help message and exit
EOF
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
    print_line "warn" "No renderer specified, trying to build without renderer, this will go poorly"
    RENDERER="none"
  fi

  if [ "$RENDERER" != "none" ]; then
    c_file_list=$(find ./src -type f -name "*.c" ! -path "./src/renderers/*/*")
  else
    c_file_list=$(find ./src -type f -name "*.c" ! -path "./src/renderers/*/*" ! -path "./src/renderers/*")
  fi

  if [ -z "$c_file_list" ]; then
    print_line "error" "No C files found in ./src"
  fi

  for c_file in $c_file_list; do
    C_FILES+="$c_file "
  done

  if [ "$RENDERER" != "none" ]; then
    renderer_file_list=$(find "./src/renderers/$RENDERER" -name "*.c")
    if [ -z "$renderer_file_list" ]; then
      print_line "error" "No C files found in ./src/renderers/$RENDERER"
    fi

    for renderer_file in $renderer_file_list; do
      C_FILES+="$renderer_file "
    done

    renderer_c_flag=$(pkg-config --cflags "$RENDERER")
    renderer_ld_flag=$(pkg-config --libs "$RENDERER")

    LD_FLAGS+=" $renderer_ld_flag"

    renderer_upper=$(echo "$RENDERER" | tr '[:lower:]' '[:upper:]')
    if ${DEBUG}; then
      print_line "info" "Building with debug enabled"
      C_FLAGS+=" -DDEBUG=1 -fsanitize=address -g"
      LD_FLAGS+=" -fsanitize=address -static-libasan"
    fi

    C_FLAGS+=" -DRENDERER=$renderer_upper"
    C_FLAGS+=" $renderer_c_flag"
  fi

  print_line "info" "Making build directory"
  mkdir -p ./build/bin
  mkdir -p ./build/out
}

link_() {
  O_FILES=$(find ./build/out -name "*.o")
  if [ -z "$O_FILES" ]; then
    print_line "error" "No object files found in ./build/out"
  fi

  if gcc -o "./build/bin/oasis" $O_FILES $LD_FLAGS; then
    print_line "success" "Successfully linked object files to ./build/bin/oasis"
  else
    print_line "error" "Failed to link object files to oasis"
  fi

  exit 0
}

build() {
  compile
  link_
}

compile() {
  local source_file
  local index
  local total

  config_compile

  total=$(echo "$C_FILES" | wc -w)
  index=1

  for source_file in $C_FILES; do
    print_line "info" "[${index}/${total}] - Compiling $source_file..."
    if gcc -c $C_FLAGS $INCLUDE_DIRS $source_file -o "./build/out/$(basename "$source_file" .c).o" $LD_FLAGS; then
      print_line "success" "[${index}/${total}] - Compiled  $source_file"
      index=$((index + 1))
    else
      print_line "error" "Failed to compile $source_file"
    fi
  done

  print_line "success" "Done compiling!"
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
  -d | --debug)
    DEBUG=true
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
