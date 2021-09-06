
set -e

PQRS_TEST="$(realpath "$1")"
DSDIR="$(realpath "$2")"

ORGDIR="$(pwd)"

cd "$DSDIR"
find -name '*.png' | parallel -k "convert {} PPM:- | '$PQRS_TEST' /dev/stdin /dev/stdout > /dev/null && echo '[GOOD] {}' || echo '[BAD] {}'"

 
