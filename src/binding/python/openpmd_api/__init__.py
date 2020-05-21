from . import openpmd_api_cxx as cxx
from .openpmd_api_cxx import *  # noqa


__version__ = cxx.__version__
__doc__ = cxx.__doc__
__license__ = cxx.__license__
# __author__ = cxx.__author__

# TODO remove in future versions (deprecated)
Access_Type = Access  # noqa
