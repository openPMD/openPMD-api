# Build locally (just to test)
(
    cd ~/src/openPMD-api
    cmake --build config --verbose && cmake --install config --verbose
)

# Commit
(
    cd ~/src/openPMD-api
    git add src && git commit -m 'Make changes' && git push
    commit=$(git log | head -n 1 | awk '{ print $2; }')
    sha256=$(echo $(wget https://github.com/eschnett/openPMD-api/archive/$commit.tar.gz && sha256sum $commit.tar.gz | awk '{ print $1; }' && rm $commit.tar.gz))
    echo $commit
    echo $sha256
)

# Build in Yggdrasil
(
    cd ~/src/Yggdrasil/O/openPMD_api
    julia --color=yes build_tarballs.jl --debug --verbose --deploy=local x86_64-apple-darwin-libgfortran5
)

# Commit in local repo
(
    cd ~/.julia/dev/openPMD_api_jll
    git add . && git commit -m 'Update generated package'
)

# Test in Julia
# julia +1.6 --eval 'using openPMD'
julia +1.7 --eval 'using openPMD'
julia +1.8 --eval 'using openPMD'
julia +1.9 --eval 'using openPMD'
julia +1.10 --eval 'using openPMD'
julia +dev --eval 'using openPMD'
