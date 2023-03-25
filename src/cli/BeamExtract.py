#!/usr/bin/env python3
# coding: utf-8

# before you start:
#!conda install -c conda-forge -y mamba
#!mamba install -c conda-forge -y openpmd-api openpmd-viewer ipympl fast-histogram

########################################################################################################################################

import json
import sys

import numpy as np
import openpmd_api as io

########################################################################################################################################

if len(sys.argv) == 1:
    print("ABORTED: Please pass a run as command line argument.")
    sys.exit(1)
Runs = sys.argv[1:]
print("Working on runs: ", Runs)

########################################################################################################################################
for Run in Runs:

    sim = '../' + Run + '/'

    input_series = io.Series(sim + "diags/openPMD/openpmd_%T.bp", io.Access.read_only)
    input_series.backend

    #!rm -rf particle_extract.*

    output_series = io.Series(sim + "particle_extract_9000.bp", io.Access.create)
    output_series.backend

    ######################################################################################################################################

    # species record: filter
    def filter_species(in_p, in_slice):
        """
        example: filter by z position
        """
        # prepare reading of records of the current slice
        # note: you can read more than one record to use for filtering
        pos_x = in_p["position"]["x"][in_slice]
        pos_y = in_p["position"]["y"][in_slice]
        pos_z = in_p["position"]["z"][in_slice]

        # trigger read operations
        in_p.series_flush()

        # example here: simple position threshold filter
        #   assumption: unitSI is 1.0 and positionOffset are zero
        filter_data = (pos_z > 16e-06) & (np.abs(pos_x) < 5.0e-6) & (np.abs(pos_y) < 5.0e-6)

        #  return binary filter array for the current slice
        return filter_data

    ######################################################################################################################################

    k_it = 9000  # time step for beam extraction

    ######################################################################################################################################

    # avoid running out-of-memory: maximum number of particles to copy at once
    slice_size = 250_000_000  # 250 million particles at a time

    ######################################################################################################################################

    # compress extracted datasets, unless they are chunked (most of them are)
    dataset_config = {
        'adios2': {
            'dataset': {
            }
        }
    }
    dataset_config['adios2']['dataset'] = {
        'operators': [{
            'type': 'blosc',
            'parameters': {
                'compressor': '1',
                'clevel': '1',
                'threshold': '2048',
                'nthreads': '6',
                'doshuffle': 'BLOSC_BITSHUFFLE'
            }
        }]
    }

    # resizable data sets w/ compression seem to crash in write as of 0.14.5
    dataset_config_resizable = {}  # dataset_config
    dataset_config_resizable['resizable'] = True

    ######################################################################################################################################

    # single iteration
    in_it = input_series.iterations[k_it]
    for _ in [k_it]:

    # OR all iterations:
    #for k_it, in_it in input_series.iterations.items():
        print(f"Iteration: {k_it}")
        out_it = output_series.iterations[k_it]

        # particle species
        for k_p, in_p in in_it.particles.items():
            print(f"  Particle species '{k_p}'")
            out_p = out_it.particles[k_p]

            num_particles = in_p["momentum"]["x"].shape[0]
            print(f"   Number of particles: {num_particles}")

            N_pass = int(np.ceil(num_particles/slice_size))
            print(f"   number of passes: {N_pass}")

            # bookkeeping in the global output arrays of the species
            out_slice_start = 0
            out_slice_end = 0

            # stepping through particle in slices
            for slice_start in range(0, num_particles, slice_size):
                slice_end = slice_start + slice_size
                if slice_end > num_particles:
                    slice_end = num_particles
                #print('    {:,} {:,}'.format(slice_start, slice_end))
                in_slice = np.s_[slice_start:slice_end]

                # species record: filter
                accepted = filter_species(in_p, in_slice)
                out_slice_end = out_slice_start + np.sum(accepted)

                if out_slice_end == out_slice_start:
                    continue

                # species records
                for k_p_r, in_p_r in in_p.items():
                    print(f"    {k_p_r}")

                    # species record components: data
                    for k_p_rc, in_p_rc in in_p_r.items():
                        print(f"      {k_p_rc} {in_p_rc.shape}->[{slice_start}:{slice_end}]")

                        # copy data
                        if in_p_rc.empty:
                            out_p_r = out_p[k_p_r]
                            out_p_rc = out_p_r[k_p_rc]
                            if not out_p_rc.empty:
                                dataset = io.Dataset(in_p_rc.dtype, (0, ))
                                dataset.options = json.dumps(dataset_config)
                                out_p_rc.reset_dataset(dataset)
                                # out_p_rc.make_empty(dtype, 1)  # done by reset_datatype w/ zero shape already
                        elif in_p_rc.constant:
                            # later, once we know the final shape
                            pass
                        else:
                            data = in_p_rc[in_slice]
                            input_series.flush()

                            # write accepted particles back
                            out_slice = np.s_[out_slice_start:out_slice_end]
                            print(f"        out_slice={out_slice}, {out_slice_start}")

                            out_p_r = out_p[k_p_r]
                            out_p_rc = out_p_r[k_p_rc]
                            dataset = io.Dataset(in_p_rc.dtype, (out_slice_end,))
                            dataset.options = json.dumps(dataset_config_resizable)
                            out_p_rc.reset_dataset(dataset)
                            out_p_rc[out_slice] = data[accepted]
                            output_series.flush()

                out_slice_start = out_slice_end
                # next species record

            # filter results are empty?
            if out_slice_start == 0:
                print("    Filter results for this iteration and species are empty!")
                for k_p_r, in_p_r in in_p.items():
                    out_p_r = out_p[k_p_r]
                    for k_p_rc, in_p_rc in in_p_r.items():
                        out_p_rc = out_p_r[k_p_rc]

                        if not out_p_rc.empty:
                            print(f"      {k_p_r} {k_p_rc}")
                            dataset = io.Dataset(in_p_rc.dtype, (0,))
                            dataset.options = json.dumps(dataset_config)
                            out_p_rc.reset_dataset(dataset)
                            # out_p_rc.make_empty(dtype, 1)  # done by reset_datatype w/ zero shape already
            else:
                # write constant record components with final shape
                print("    Writing constant record components")
                for k_p_r, in_p_r in in_p.items():
                    out_p_r = out_p[k_p_r]
                    for k_p_rc, in_p_rc in in_p_r.items():
                        out_p_rc = out_p_r[k_p_rc]

                        if in_p_rc.constant:
                            print(f"      {k_p_r} {k_p_rc} {in_p_rc.shape}->[{out_slice_end}]")
                            dataset = io.Dataset(in_p_rc.dtype, (out_slice_end,))
                            dataset.options = json.dumps(dataset_config)
                            out_p_rc.reset_dataset(dataset)
                            out_p_rc.make_constant(in_p_rc.get_attribute("value"))

            output_series.flush()
            # next particle species
        # next iteration

    output_series.flush()

    ######################################################################################################################################

    # series attributes
    for a in input_series.attributes:
        print(f"{a}")

        # meshesPath: if this attribute is missing, the file is interpreted as if it contains no mesh records! If the attribute is set, the group behind it must exist!
        if a in ["meshesPath"]:
            print(" - skipped")
            continue

        output_series.set_attribute(a, input_series.get_attribute(a))

    # iteration attributes
    # single iteration
    in_it = input_series.iterations[k_it]
    for _ in [k_it]:

    # OR all iterations:
    #for k_it, in_it in input_series.iterations.items():
        print(k_it)
        out_it = output_series.iterations[k_it]
        for a in in_it.attributes:
            print(f" {a}")
            out_it.set_attribute(a, in_it.get_attribute(a))

        # species attributes
        for k_p, in_p in in_it.particles.items():
            print(k_p)
            out_p = out_it.particles[k_p]
            for a in in_p.attributes:
                print(f"  {a}")
                out_p.set_attribute(a, in_p.get_attribute(a))

            # species record attributes
            for k_p_r, in_p_r in in_p.items():
                print(k_p, k_p_r)
                out_p_r = out_p[k_p_r]
                for a in in_p_r.attributes:
                    print(f"   {a}")
                    if a in ["shape", "value"]:
                        print("    - skipped")
                        continue
                    out_p_r.set_attribute(a, in_p_r.get_attribute(a))

                # species record component attributes
                for k_p_rc, in_p_rc in in_p_r.items():
                    print(f"  {k_p_rc}")
                    out_p_rc = out_p_r[k_p_rc]
                    for a in in_p_rc.attributes:
                        print(f"    {a}")
                        if a in ["shape", "value"]:
                            print("     - skipped")
                            continue
                        out_p_rc.set_attribute(a, in_p_rc.get_attribute(a))

    output_series.flush()

    ######################################################################################################################################

    del input_series
    del output_series

    ######################################################################################################################################
    print('This job is finished!')
