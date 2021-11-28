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

The docker image from The Tangle requires the datasets to be stored in the server's LMDB persistence. The LMDB file is stored outside the
docker image in some path of your file system that you give to the `docker run` command. This way, you don't have to download everything
each time you run the image. Since the data is not stored in the docker image, the first time you use it, you will have to download and
store all the datasets.


### To download and store the data for the first time

```
docker run -ti -v /path/local/lmdb:/home/jazz/jazz_dbg_mdb kaalam/jazz_tng:0.5.3 /bin/bash
```

Where `/path/local/lmdb` is whatever path on your machine you choose to store the downloaded datasets.

This will download the docker image and take you to a linux prompt **inside the docker container**.

From that prompt, you have to run:

```
./download_and_store.sh
```

When the process has completed (after several minutes depending on your connection and hardware settings), remember the path to which you
saved the LMDB files and exit by executing `exit`.


### To run the image containing the data you already downloaded

```
docker run -p8899:8899 -v /path/local/lmdb:/home/jazz/jazz_dbg_mdb kaalam/jazz_tng:0.5.3
```

Where `/path/local/lmdb` is the path you already used in a successful `download_and_store` call.


See more at: https://kaalam.github.io/jazz_reference/reference_docker_tangle_server.html
