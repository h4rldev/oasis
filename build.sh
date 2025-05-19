## build.sh

# This script is used to build the project.
# It is called by the `just` command. It is not intended to be called directly.
#

set -ueo pipefail

C_FLAGS="-Wall -Wextra -Werror -pedantic -std=c99 -O2"
LD_FLAGS="-lm $(pkg-config --libs-only-l raylib)"

INCLUDE_DIRS="-I./include $(pkg-config --cflags-only-I raylib)"
C_FILES=""

RENDERER="raylib" # default renderer

get_c_files() {
  local renderer_file_list
  local c_file_list

  if [ -z "$RENDERER" ]; then
    echo "Must specify what renderer to use, currently supported renderers are: raylib"
    exit 1
  fi

  c_file_list=$(find ./src -type f -name "*.c" ! -path "./src/renderers/*/*")
  if [ -z "$c_file_list" ]; then
    echo "No C files found in ./src"
    exit 1
  fi

  renderer_file_list=$(find "./src/renderers/$RENDERER" -name "*.c")
  if [ -z "$renderer_file_list" ]; then
    echo "No C files found in ./src/renderers/$RENDERER"
    exit 1
  fi

  for c_file in $c_file_list; do
    C_FILES+="$c_file "
  done

  echo "C_FILES: $C_FILES"

  for renderer_file in $renderer_file_list; do
    C_FILES+="$renderer_file "
  done
}

get_c_files

echo "${C_FILES}"
