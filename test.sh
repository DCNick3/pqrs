
set -e

PQRS_TEST="$(realpath "$1")"
DSDIR="$(realpath "$2")"

ORGDIR="$(pwd)"
OUTDIR="$ORGDIR/out"

cd "$DSDIR"

find -name '*.png' | parallel "mkdir -p \$(dirname '$OUTDIR/{}') && echo {} && convert {} PPM:- | '$PQRS_TEST' /dev/stdin /dev/stdout | convert PPM:- '$OUTDIR/{}'"

 
