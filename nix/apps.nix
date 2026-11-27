{ openpmd_api }:
{
  ls = {
    type = "app";
    program = "${openpmd_api}/bin/openpmd-ls";
    meta = {
      description = "List the contents of an openPMD series";
    };
  };
  pipe = {
    type = "app";
    program = "${openpmd_api}/bin/openpmd-pipe";
    meta = {
      description = "Convert between openPMD file formats";
    };
  };
  default = {
    type = "app";
    program = "${openpmd_api}/bin/openpmd-ls";
    meta = {
      description = "List the contents of an openPMD series";
    };
  };
}
