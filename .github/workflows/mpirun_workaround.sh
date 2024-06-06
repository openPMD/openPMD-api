#!/usr/bin/env bash

# mpiexec currently seems to have a bug where it tries to parse parameters
# of the launched application when they start with a dash, e.g.
# `mpiexec python ./openpmd-pipe --infile in.bp --outfile out.bp`
# leads to:
# >  An unrecognized option was included on the mpiexec command line:
# >
# >    Option: --infile
# >
# >  Please use the "mpiexec --help" command to obtain a list of all
# >  supported options.
#
# This script provides a workaround by putting the called sub-command into
# a script in a temporary file.

ls="$(which ls)"
mpiexec "$ls" -m \
    && echo "MPIRUN WORKING AGAIN, PLEASE REMOVE WORKAROUND" >&2 \
    && exit 1 \
    || true

mpirun_args=()

script_file="$(mktemp)"

cleanup() {
    rm "$script_file"
}
trap cleanup EXIT

while true; do
    case "$1" in
        -c | -np | --np | -n | --n )
            mpirun_args+=("$1" "$2")
            shift
            shift
            ;;
        *)
            break
            ;;
    esac
done

echo -e '#!/usr/bin/env bash\n' > "$script_file"
for item in "$@"; do
    echo -n "'$item' " >> "$script_file"
done

chmod +x "$script_file"

mpirun "${mpirun_args[@]}" "$script_file"
