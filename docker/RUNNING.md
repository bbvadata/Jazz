# How to run docker images

## Running a server

Use the proper version or `:latest` searching at https://hub.docker.com/repository/docker/kaalam/jazz_base and replace the version below
as necessary.

```
docker run -p8899:8899 kaalam/jazz_lss:0.5.3

# Other options:

# Run locally built image [DEBUG]:		docker run -p8899:8899 -ti jazz_lss_stable:latest /bin/bash
# Run locally built image [Release]:	docker run -p8899:8899 jazz_lss_stable:latest
# Run Docker Hub image [DEBUG]:			docker run -p8899:8899 -ti kaalam/jazz_lss:0.5.3 /bin/bash
```

See more at: https://kaalam.github.io/jazz_reference/reference_docker_how_to_build.html


## Running The Tangle

```
docker run -p8899:8899 -v /path/local/lmdb:/home/jazz/jazz_dbg_mdb kaalam/jazz_tng:0.5.3
```

Where `/path/local/lmdb` is whatever path on your machine is given to store the persistence.

See more at: https://kaalam.github.io/jazz_reference/reference_docker_tangle_server.html