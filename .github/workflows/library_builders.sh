function install_buildessentials {
    # python -m pip install -U cmake

    # static libc
    # yum check-update -y
    # yum -y install \
    #     glibc-static \
    #     tar

    # TODO see Dockerfile
}

function build_adios1 {
    # avoid picking up a static libpthread in adios (also: those libs lack -fPIC)
    # rm /usr/lib64/libpthread.a /usr/lib64/libm.a /usr/lib64/librt.a

    # static libs need relocatable symbols for linking to shared python lib
    # export CFLAGS="-fPIC ${CFLAGS}"
    # export CXXFLAGS="-fPIC ${CXXFLAGS}"

    # TODO see Dockerfile
}

function build_adios2 {
    # TODO see Dockerfile
}

function build_blosc {
    # TODO see Dockerfile
}

function build_hdf5 {
    # TODO see Dockerfile
    #      see https://github.com/matthew-brett/multibuild/blob/devel/library_builders.sh
}
