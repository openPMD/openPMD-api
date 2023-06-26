convert := openpmd-convert-json-toml

json_files = attribute_defs.json attributes.json dataset_defs.json iteration.json mesh.json mesh_record_component.json particle_patches.json particle_species.json patch_record.json record.json record_component.json series.json

.PHONY: all
all: $(json_files)

$(json_files): %.json: %.toml
	$(convert) @$^ > $@

.PHONY: clean
clean:
	-rm $(json_files)
