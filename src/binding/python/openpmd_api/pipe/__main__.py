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
import re
import sys  # sys.stderr.write

from .. import openpmd_api_cxx as io


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
context.
Parallelization with MPI is optionally possible and can be switched on with
the --mpi switch, resp. switched off with the --no-mpi switch.
By default, openpmd-pipe will use MPI if all of the following conditions
are fulfilled:
1) The mpi4py package can be imported.
2) The openPMD-api has been built with support for MPI.
3) The MPI size is greater than 1.
   By default, the openPMD-api will be initialized without an MPI communicator
   if the MPI size is 1. This is to simplify the use of the JSON backend
   which is only available in serial openPMD.
With parallelization enabled, each dataset will be equally sliced according to
a chunk distribution strategy which may be selected via the environment
variable OPENPMD_CHUNK_DISTRIBUTION. Options include "roundrobin",
"binpacking", "slicedataset" and "hostname_<1>_<2>", where <1> should be
replaced with a strategy to be applied within a compute node and <2> with a
secondary strategy in case the hostname strategy does not distribute
all chunks.
The default is `hostname_binpacking_slicedataset`.

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
    # MPI, default: Import mpi4py if available and openPMD is parallel,
    # but don't use if MPI size is 1 (this makes it easier to interact with
    # JSON, since that backend is unavailable in parallel)
    if io.variants['mpi']:
        parser.add_argument('--mpi', action='store_true')
        parser.add_argument('--no-mpi', dest='mpi', action='store_false')
        parser.set_defaults(mpi=None)

    return parser.parse_args()


args = parse_args(sys.argv[0])
# MPI is an optional dependency
if io.variants['mpi'] and (args.mpi is None or args.mpi):
    try:
        from mpi4py import MPI
        HAVE_MPI = True
    except (ImportError, ModuleNotFoundError):
        if args.mpi:
            raise
        else:
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


class deferred_load:
    def __init__(self, source, dynamicView, offset, extent):
        self.source = source
        self.dynamicView = dynamicView
        self.offset = offset
        self.extent = extent

# Find below a couple of examples on how to define chunk distribution
# strategies in Python by extending classes PartialStrategy or Strategy.
# These strategies may then be used inside composing strategies
# such as ByHostname. They may also call other strategies, as in
# IncreaseGranularity defined below.


# Example how to implement a simple partial strategy in Python
class LoadOne(io.PartialStrategy):
    def __init__(self):
        super().__init__()

    def assign(self, assignment, ranks_in, ranks_out, my_rank, num_ranks):
        element = assignment.not_assigned.pop()
        if my_rank not in assignment.assigned:
            assignment.assigned[my_rank] = [element]
        else:
            assignment.assigned[my_rank].append(element)
        return assignment


# Example how to implement a simple strategy in Python
class LoadAll(io.Strategy):

    def __init__(self):
        super().__init__()

    def assign(self, assignment, ranks_in, ranks_out, my_rank, num_ranks):
        res = assignment.assigned
        if my_rank not in res:
            res[my_rank] = assignment.not_assigned
        else:
            res[my_rank].extend(assignment.not_assigned)
        return res


# A more complex distribution strategy. This creates supergroups of hostnames,
# separately for the writer and reader ranks.
# Every `granularity_in` writer hostnames are merged into one new hostname
# each, same for every `granularity_out` reader hostnames.
# An example usage is defining granularity_in=32, granularity_out=1 for a
# 32-to-1 fan-in pattern.
class IncreaseGranularity(io.PartialStrategy):
    def __init__(
        self,
        granularity_in,
        granularity_out,
        inner_distribution,
    ):
        super().__init__()
        self.inner_distribution = inner_distribution
        self.granularity_in = granularity_in
        self.granularity_out = granularity_out

    def assign(self, assignment, in_ranks, out_ranks, my_rank, num_ranks):
        if "in_ranks_inner" in dir(self):
            return self.inner_distribution.assign(
                assignment, self.in_ranks_inner, self.out_ranks_inner
            )

        def hosts_in_order(rank_assignment):
            already_seen = set()
            res = []
            for (_, hostname) in rank_assignment.items():
                if hostname not in already_seen:
                    already_seen.add(hostname)
                    res.append(hostname)
            return res

        in_hosts_in_order = hosts_in_order(in_ranks)
        out_hosts_in_order = hosts_in_order(out_ranks)

        # Creates names "0", "1", "2", ... for the meta hosts and maps the real
        # host names to the meta host names
        def hostname_to_hostgroup(ordered_hosts, granularity):
            res = {}  # real host -> host group
            current_meta_host = 0
            granularity_counter = 0
            for host in ordered_hosts:
                res[host] = str(current_meta_host)
                granularity_counter += 1
                if granularity_counter >= granularity:
                    granularity_counter = 0
                    current_meta_host += 1
            return res

        in_hostname_to_hostgroup = hostname_to_hostgroup(
            in_hosts_in_order, self.granularity_in
        )
        out_hostname_to_hostgroup = hostname_to_hostgroup(
            out_hosts_in_order, self.granularity_out
        )

        # Creates `in_ranks` and `out_ranks` for the inner call, based on the
        # meta hosts created above
        def inner_rank_assignment(
                outer_rank_assignment, hostname_to_hostgroup):
            res = {}
            for (rank, hostname) in outer_rank_assignment.items():
                res[rank] = hostname_to_hostgroup[hostname]
            return res

        self.in_ranks_inner = \
            inner_rank_assignment(in_ranks, in_hostname_to_hostgroup)
        self.out_ranks_inner = inner_rank_assignment(
            out_ranks, out_hostname_to_hostgroup
        )

        return self.inner_distribution.assign(
            assignment,
            self.in_ranks_inner, self.out_ranks_inner,
            my_rank, num_ranks
        )


# Merge chunks into larger chunks as much as possible within
# each source process for reducing the number of load requests.
class MergingStrategy(io.Strategy):
    def __init__(self, inner_strategy):
        super().__init__()
        self.inner_strategy = inner_strategy

    def assign(self, assignment, in_ranks, out_ranks, my_rank, num_ranks):
        res = self.inner_strategy.assign(
            assignment, in_ranks, out_ranks, my_rank, num_ranks)
        for out_rank, assignment in res.items():
            merged = assignment.merge_chunks_from_same_sourceID()
            assignment.clear()
            for in_rank, chunks in merged.items():
                for chunk in chunks:
                    assignment.append(
                        io.WrittenChunkInfo(
                            chunk.offset, chunk.extent, in_rank)
                    )
        return res


def distribution_strategy(dataset_extent,
                          strategy_identifier=None):
    if strategy_identifier is None or not strategy_identifier:
        if 'OPENPMD_CHUNK_DISTRIBUTION' in os.environ:
            strategy_identifier = os.environ[
                'OPENPMD_CHUNK_DISTRIBUTION'].lower()
        else:
            strategy_identifier = 'hostname_binpacking_slicedataset'  # default
    match = re.search('hostname_(.*)_(.*)', strategy_identifier)
    if match is not None:
        inside_node = distribution_strategy(
            dataset_extent, strategy_identifier=match.group(1))
        second_phase = distribution_strategy(
            dataset_extent,
            strategy_identifier=match.group(2))
        return io.FromPartialStrategy(io.ByHostname(inside_node), second_phase)
    elif strategy_identifier == 'fan_in':
        granularity = os.environ['OPENPMD_FAN_IN']
        granularity = int(granularity)
        return IncreaseGranularity(
            granularity, 1,
            io.FromPartialStrategy(io.ByHostname(io.RoundRobin()),
                                   io.DiscardingStrategy()))
    elif strategy_identifier == 'all':
        return io.FromPartialStrategy(IncreaseGranularity(5), LoadAll())
    elif strategy_identifier == 'roundrobin':
        return io.RoundRobin()
    elif strategy_identifier == 'binpacking':
        return io.BinPacking()
    elif strategy_identifier == 'slicedataset':
        return io.ByCuboidSlice(io.OneDimensionalBlockSlicer(), dataset_extent)
    elif strategy_identifier == 'fail':
        return io.FailingStrategy()
    elif strategy_identifier == 'discard':
        return io.DiscardingStrategy()
    elif strategy_identifier == 'blocksofsourceranks':
        return io.BlocksOfSourceRanks()
    else:
        raise RuntimeError("Unknown distribution strategy: " +
                           strategy_identifier)


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
        if HAVE_MPI:
            hostinfo = io.HostInfo.MPI_PROCESSOR_NAME
            self.outranks = hostinfo.get_collective(self.comm)
        else:
            self.outranks = {i: str(i) for i in range(self.comm.size)}

    def run(self):
        if not HAVE_MPI or (args.mpi is None and self.comm.size == 1):
            print("Opening data source")
            sys.stdout.flush()
            inseries = io.Series(self.infile, io.Access.read_linear,
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
            inseries = io.Series(self.infile, io.Access.read_linear, self.comm,
                                 self.inconfig)
            print("Opening data sink on rank {}.".format(self.comm.rank))
            sys.stdout.flush()
            outseries = io.Series(self.outfile, io.Access.create, self.comm,
                                  self.outconfig)
            print("Opened input and output on rank {}.".format(self.comm.rank))
            sys.stdout.flush()
        # In Linear read mode, global attributes are only present after calling
        # this method to access the first iteration
        inseries.parse_base()
        self.__copy(inseries, outseries)

    def __copy(self, src, dest, current_path="/data/"):
        """
        Worker method.
        Copies data from src to dest. May represent any point in the openPMD
        hierarchy, but src and dest must both represent the same layer.
        """
        if (type(src) is not type(dest)
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
            io.Iteration: ["snapshot"],
            io.Record_Component: ["value", "shape"] if isinstance(
                src, io.Record_Component) and src.constant else []
        }
        # filter the map for relevant openpmd object model types
        from itertools import chain
        ignored_attributes = set(chain.from_iterable(value for (
            key, value) in ignored_attributes.items() if isinstance(src, key)))

        for key in src.attributes:
            ignore_this_attribute = key in ignored_attributes
            if not ignore_this_attribute:
                attr = src.get_attribute(key)
                attr_type = attribute_dtypes[key]
                dest.set_attribute(key, attr, attr_type)

        container_types = [
            io.Mesh_Container, io.Particle_Container, io.ParticleSpecies,
            io.Record, io.Mesh, io.Particle_Patches, io.Patch_Record
        ]
        is_container = any([
            isinstance(src, container_type)
            for container_type in container_types
        ])

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
                # With linear read mode, we can only load the source rank table
                # inside `read_iterations()` since it's a dataset.
                if src.has_rank_table_read:
                    self.inranks = src.get_rank_table(collective=True)
                else:
                    self.inranks = {}
                out_iteration = write_iterations[in_iteration.iteration_index]
                sys.stdout.flush()
                self.__copy(
                    in_iteration, out_iteration,
                    current_path + str(in_iteration.iteration_index) + "/")
                for deferred in self.loads:
                    deferred.source.load_chunk(
                        deferred.dynamicView.current_buffer(), deferred.offset,
                        deferred.extent)
                in_iteration.close()
                out_iteration.close()
                self.loads.clear()
                sys.stdout.flush()
        elif isinstance(src, io.Record_Component) and (not is_container
                                                       or src.scalar):
            shape = src.shape
            dtype = src.dtype
            dest.reset_dataset(io.Dataset(dtype, shape))
            if src.empty:
                # empty record component automatically created by
                # dest.reset_dataset()
                pass
            elif src.constant:
                dest.make_constant(src.get_attribute("value"))
            else:
                chunk_table = src.available_chunks()
                # todo buffer the strategy
                strategy = distribution_strategy(shape)
                my_chunks = strategy.assign(chunk_table, self.inranks,
                                            self.outranks,
                                            self.comm.rank, self.comm.size)
                for chunk in my_chunks[
                        self.comm.rank] if self.comm.rank in my_chunks else []:
                    if debug:
                        end = chunk.offset.copy()
                        for i in range(len(end)):
                            end[i] += chunk.extent[i]
                        print("{}\t{}/{}:\t{} -- {}".format(
                            current_path, self.comm.rank, self.comm.size,
                            chunk.offset, end))
                    span = dest.store_chunk(chunk.offset, chunk.extent)
                    self.loads.append(
                        deferred_load(src, span, chunk.offset, chunk.extent))
        elif isinstance(src, io.Iteration):
            self.__copy(src.meshes, dest.meshes, current_path + "meshes/")
            self.__copy(src.particles, dest.particles,
                        current_path + "particles/")
        elif is_container:
            for key in src:
                self.__copy(src[key], dest[key], current_path + key + "/")
            if isinstance(src, io.ParticleSpecies):
                self.__copy(src.particle_patches, dest.particle_patches)
        else:
            raise RuntimeError("Unknown openPMD class: " + str(src))


def main():
    if not args.infile or not args.outfile:
        print("Please specify parameters --infile and --outfile.")
        sys.exit(1)
    if HAVE_MPI:
        communicator = MPI.COMM_WORLD
    else:
        communicator = FallbackMPICommunicator()
    run_pipe = pipe(args.infile, args.outfile, args.inconfig, args.outconfig,
                    communicator)

    run_pipe.run()


if __name__ == "__main__":
    main()
    sys.exit()
