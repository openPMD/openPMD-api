from . import openpmd_api_cxx as cxx
from .openpmd_api_cxx import *  # noqa
from .DataFrame import particles_to_dataframe
from .DaskDataFrame import particles_to_daskdataframe


__version__ = cxx.__version__
__doc__ = cxx.__doc__
__license__ = cxx.__license__
# __author__ = cxx.__author__

# extend CXX classes with extra methods
ParticleSpecies.to_df = particles_to_dataframe  # noqa
ParticleSpecies.to_dask = particles_to_daskdataframe  # noqa

# TODO remove in future versions (deprecated)
Access_Type = Access  # noqa
