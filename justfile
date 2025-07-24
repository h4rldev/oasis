set shell := ["bash", "-uc"]

default:
  @just --list

# Available arguments:
#   -r, --renderer <renderer>
#   -h, --help (run this command for more information)
@compile *args:
  ./build.sh -c {{args}}

@link *args:
  ./build.sh -l {{args}}

@build *args:
  ./build.sh {{args}}

@run:
  bash -c 'if [ ! -f ./build/bin/oasis ]; then just build; fi'
  ./build/bin/oasis

# bear generates a compilation database for clang-tools
# Available arguments:

@bear *args:
  bear -- just build {{args}}

@test *args:
  ./build.sh -d -t {{args}}
