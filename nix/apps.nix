{ openpmd_api }:
{
  ls = {
    type = "app";
    program = "${openpmd_api}/bin/openpmd-ls";
  };
  pipe = {
    type = "app";
    program = "${openpmd_api}/bin/openpmd-pipe";
  };
  default = {
    type = "app";
    program = "${openpmd_api}/bin/openpmd-ls";
  };
}
