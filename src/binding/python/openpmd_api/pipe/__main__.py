"""
This file is part of the openPMD-api.

This module provides functions that are wrapped into sys.exit(...()) calls by
the setuptools (setup.py) "entry_points" -> "console_scripts" generator.

Copyright 2021 openPMD contributors
Authors: Franz Poeschel
License: LGPLv3+
"""
import argparse
import os  # os.path.basename
import sys  # sys.stderr.write

from .. import openpmd_api_cxx as io

# MPI is an optional dependency
if io.variants['mpi']:
    try:
        from mpi4py import MPI
        HAVE_MPI = True
    except (ImportError, ModuleNotFoundError):
        print("""
openPMD-api was built with support for MPI,
but mpi4py Python package was not found.
Will continue in serial mode.""",
              file=sys.stderr)
        HAVE_MPI = False
else:
    HAVE_MPI = False

debug = False


class FallbackMPICommunicator:
    def __init__(self):
        self.size = 1
        self.rank = 0


def parse_args(program_name):
    parser = argparse.ArgumentParser(
        # we need this for line breaks
        formatter_class=argparse.RawDescriptionHelpFormatter,
        description="""
openPMD Pipe.

This tool connects an openPMD-based data source with an openPMD-based data sink
and forwards all data from source to sink.
Possible uses include conversion of data from one backend to another one
or multiplexing the data path in streaming setups.
Parallelization with MPI is optionally possible and is done automatically
as soon as the mpi4py package is found and this tool is called in an MPI
context. In that case, each dataset will be equally sliced along the dimension
with the largest extent.

Examples:
    {0} --infile simData.h5 --outfile simData_%T.bp
    {0} --infile simData.sst --inconfig @streamConfig.json \\
        --outfile simData_%T.bp
    {0} --infile uncompressed.bp \\
        --outfile compressed.bp --outconfig @compressionConfig.json
""".format(os.path.basename(program_name)))

    parser.add_argument('--infile', type=str, help='In file')
    parser.add_argument('--outfile', type=str, help='Out file')
    parser.add_argument('--inconfig',
                        type=str,
                        default='{}',
                        help='JSON config for the in file')
    parser.add_argument('--outconfig',
                        type=str,
                        default='{}',
                        help='JSON config for the out file')

    return parser.parse_args()


class Chunk:
    """
    A Chunk is an n-dimensional hypercube, defined by an offset and an extent.
    Offset and extent must be of the same dimensionality (Chunk.__len__).
    """
    def __init__(self, offset, extent):
        assert (len(offset) == len(extent))
        self.offset = offset
        self.extent = extent

    def __len__(self):
        return len(self.offset)

    def slice1D(self, mpi_rank, mpi_size, dimension=None):
        """
        Slice this chunk into mpi_size hypercubes along one of its
        n dimensions. The dimension is given through the 'dimension'
        parameter. If None, the dimension with the largest extent on
        this hypercube is automatically picked.
        Returns the mpi_rank'th of the sliced chunks.
        """
        if dimension is None:
            # pick that dimension which has the highest count of items
            dimension = 0
            maximum = self.extent[0]
            for k, v in enumerate(self.extent):
                if v > maximum:
                    dimension = k
        assert (dimension < len(self))
        # no offset
        assert (self.offset == [0 for _ in range(len(self))])
        offset = [0 for _ in range(len(self))]
        stride = self.extent[dimension] // mpi_size
        rest = self.extent[dimension] % mpi_size

        # local function f computes the offset of a rank
        # for more equal balancing, we want the start index
        # at the upper gaussian bracket of (N/n*rank)
        # where N the size of the dataset in dimension dim
        # and n the MPI size
        # for avoiding integer overflow, this is the same as:
        # (N div n)*rank + round((N%n)/n*rank)
        def f(rank):
            res = stride * rank
            padDivident = rest * rank
            pad = padDivident // mpi_size
            if pad * mpi_size < padDivident:
                pad += 1
            return res + pad

        offset[dimension] = f(mpi_rank)
        extent = self.extent.copy()
        if mpi_rank >= mpi_size - 1:
            extent[dimension] -= offset[dimension]
        else:
            extent[dimension] = f(mpi_rank + 1) - offset[dimension]
        return Chunk(offset, extent)


class deferred_load:
    def __init__(self, source, dynamicView, offset, extent):
        self.source = source
        self.dynamicView = dynamicView
        self.offset = offset
        self.extent = extent


class particle_patch_load:
    """
    A deferred load/store operation for a particle patch.
    Our particle-patch API requires that users pass a concrete value for
    storing, even if the actual write operation occurs much later at
    series.flush().
    So, unlike other record components, we cannot call .store_chunk() with
    a buffer that has not yet been filled, but must wait until the point where
    we actual have the data at hand already.
    In short: calling .store() must be deferred, until the data has been fully
    read from the sink.
    This class stores the needed parameters to .store().
    """
    def __init__(self, data, dest):
        self.data = data
        self.dest = dest

    def run(self):
        for index, item in enumerate(self.data):
            self.dest.store(index, item)


class pipe:
    """
    Represents the configuration of one "pipe" pass.
    """
    def __init__(self, infile, outfile, inconfig, outconfig, comm):
        self.infile = infile
        self.outfile = outfile
        self.inconfig = inconfig
        self.outconfig = outconfig
        self.loads = []
        self.comm = comm

    def run(self):
        if self.comm.size == 1:
            print("Opening data source")
            sys.stdout.flush()
            inseries = io.Series(self.infile, io.Access.read_only,
                                 self.inconfig)
            print("Opening data sink")
            sys.stdout.flush()
            outseries = io.Series(self.outfile, io.Access.create,
                                  self.outconfig)
            print("Opened input and output")
            sys.stdout.flush()
        else:
            print("Opening data source on rank {}.".format(self.comm.rank))
            sys.stdout.flush()
            inseries = io.Series(self.infile, io.Access.read_only, self.comm,
                                 self.inconfig)
            print("Opening data sink on rank {}.".format(self.comm.rank))
            sys.stdout.flush()
            outseries = io.Series(self.outfile, io.Access.create, self.comm,
                                  self.outconfig)
            print("Opened input and output on rank {}.".format(self.comm.rank))
            sys.stdout.flush()
        self.__copy(inseries, outseries)

    def __copy(self, src, dest, current_path="/data/"):
        """
        Worker method.
        Copies data from src to dest. May represent any point in the openPMD
        hierarchy, but src and dest must both represent the same layer.
        """
        if (type(src) != type(dest)
                and not isinstance(src, io.IndexedIteration)
                and not isinstance(dest, io.Iteration)):
            raise RuntimeError(
                "Internal error: Trying to copy mismatching types")
        attribute_dtypes = src.attribute_dtypes
        # The following attributes are written automatically by openPMD-api
        # and should not be manually overwritten here
        ignored_attributes = {
            io.Series:
            ["basePath", "iterationEncoding", "iterationFormat", "openPMD"],
            io.Iteration: ["snapshot"]
        }
        for key in src.attributes:
            ignore_this_attribute = False
            for openpmd_group, to_ignore_list in ignored_attributes.items():
                if isinstance(src, openpmd_group):
                    for to_ignore in to_ignore_list:
                        if key == to_ignore:
                            ignore_this_attribute = True
            if not ignore_this_attribute:
                attr = src.get_attribute(key)
                attr_type = attribute_dtypes[key]
                dest.set_attribute(key, attr, attr_type)
        container_types = [
            io.Mesh_Container, io.Particle_Container, io.ParticleSpecies,
            io.Record, io.Mesh, io.Particle_Patches, io.Patch_Record
        ]
        if isinstance(src, io.Series):
            # main loop: read iterations of src, write to dest
            write_iterations = dest.write_iterations()
            for in_iteration in src.read_iterations():
                if self.comm.rank == 0:
                    print("Iteration {0} contains {1} meshes:".format(
                        in_iteration.iteration_index,
                        len(in_iteration.meshes)))
                    for m in in_iteration.meshes:
                        print("\t {0}".format(m))
                    print("")
                    print(
                        "Iteration {0} contains {1} particle species:".format(
                            in_iteration.iteration_index,
                            len(in_iteration.particles)))
                    for ps in in_iteration.particles:
                        print("\t {0}".format(ps))
                        print("With records:")
                        for r in in_iteration.particles[ps]:
                            print("\t {0}".format(r))
                out_iteration = write_iterations[in_iteration.iteration_index]
                sys.stdout.flush()
                self.__particle_patches = []
                self.__copy(
                    in_iteration, out_iteration,
                    current_path + str(in_iteration.iteration_index) + "/")
                for deferred in self.loads:
                    deferred.source.load_chunk(
                        deferred.dynamicView.current_buffer(), deferred.offset,
                        deferred.extent)
                in_iteration.close()
                for patch_load in self.__particle_patches:
                    patch_load.run()
                out_iteration.close()
                self.__particle_patches.clear()
                self.loads.clear()
                sys.stdout.flush()
        elif isinstance(src, io.Record_Component):
            shape = src.shape
            offset = [0 for _ in shape]
            dtype = src.dtype
            dest.reset_dataset(io.Dataset(dtype, shape))
            if src.empty:
                # empty record component automatically created by
                # dest.reset_dataset()
                pass
            elif src.constant:
                dest.make_constant(src.get_attribute("value"))
            else:
                chunk = Chunk(offset, shape)
                local_chunk = chunk.slice1D(self.comm.rank, self.comm.size)
                if debug:
                    end = local_chunk.offset.copy()
                    for i in range(len(end)):
                        end[i] += local_chunk.extent[i]
                    print("{}\t{}/{}:\t{} -- {}".format(
                        current_path, self.comm.rank, self.comm.size,
                        local_chunk.offset, end))
                span = dest.store_chunk(local_chunk.offset, local_chunk.extent)
                self.loads.append(
                    deferred_load(src, span, local_chunk.offset,
                                  local_chunk.extent))
        elif isinstance(src, io.Patch_Record_Component):
            dest.reset_dataset(io.Dataset(src.dtype, src.shape))
            if self.comm.rank == 0:
                self.__particle_patches.append(
                    particle_patch_load(src.load(), dest))
        elif isinstance(src, io.Iteration):
            self.__copy(src.meshes, dest.meshes, current_path + "meshes/")
            self.__copy(src.particles, dest.particles,
                        current_path + "particles/")
        elif any([
                isinstance(src, container_type)
                for container_type in container_types
        ]):
            for key in src:
                self.__copy(src[key], dest[key], current_path + key + "/")
            if isinstance(src, io.ParticleSpecies):
                self.__copy(src.particle_patches, dest.particle_patches)
        else:
            raise RuntimeError("Unknown openPMD class: " + str(src))


def main():
    args = parse_args(sys.argv[0])
    if not args.infile or not args.outfile:
        print("Please specify parameters --infile and --outfile.")
        sys.exit(1)
    if (HAVE_MPI):
        run_pipe = pipe(args.infile, args.outfile, args.inconfig,
                        args.outconfig, MPI.COMM_WORLD)
    else:
        run_pipe = pipe(args.infile, args.outfile, args.inconfig,
                        args.outconfig, FallbackMPICommunicator())

    run_pipe.run()


if __name__ == "__main__":
    main()
    sys.exit()
