#!/usr/bin/env bash
#

set -eu -o pipefail

# create a temporary build directory
tmp_dir=$(mktemp --help >/dev/null 2>&1 && mktemp -d -t ci-XXXXXXXXXX || mktemp -d "${TMPDIR:-/tmp}"/ci-XXXXXXXXXX)
if [ $? -ne 0 ]; then
    echo "Cannot create a temporary directory"
    exit 2
fi
mkdir -p ${tmp_dir}

# Call the cleanup function
function cleanvenv {
    echo
    echo "Cleaning ... ${tmp_dir}"
    set +u
    deactivate
    set -u
    rm -rf ${tmp_dir}/venv-validator
    echo "Done"
}
trap cleanvenv EXIT

# Install openPMD-validator
python3 -m venv ${tmp_dir}/venv-validator
set +u
source ${tmp_dir}/venv-validator/bin/activate
set -u
python3 -m pip install git+https://github.com/openPMD/openPMD-validator.git@1.1.X

# Check HDF5 Files
#   skip intentionally invalid sample files
fignore=("issue-sample/no_particles/" "issue-sample/no_fields/")
h5files=$(find . -name "*.h5")
for f in ${h5files}
do
   echo
   ok=0
   for fi in ${fignore[@]}
   do
       if [[ "${f}" == *"${fi}"* ]]; then
           ok=1
           break
       fi
   done
   if [[ ${ok} -ne 0 ]]; then
       echo "Skipping \"${f}\" ..."
       continue
   fi

   echo "Checking \"${f}\" ..."
   openPMD_check_h5 -i ${f}
done
