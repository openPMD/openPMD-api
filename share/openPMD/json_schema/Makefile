convert := openpmd-convert-toml-json

json_files = attribute_defs.json attributes.json dataset_defs.json iteration.json mesh.json mesh_record_component.json particle_patches.json particle_species.json patch_record.json record.json record_component.json series.json

.PHONY: all
all: $(json_files)

# The target file should only be created if the conversion succeeded
$(json_files): %.json: %.toml
	$(convert) @$^ > $@.tmp
	mv $@.tmp $@

.PHONY: clean
clean:
	for file in $(json_files); do rm -f "$$file" "$$file.tmp"; done
