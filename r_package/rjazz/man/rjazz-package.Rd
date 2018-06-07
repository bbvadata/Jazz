\name{rjazz-package}
\alias{rjazz}
\docType{package}

\title{
Official Client for Jazz
}

\description{
\strong{How to get a Jazz server}

Currently, in March 2018, Jazz is in the middle of major refactoring. The client is released
mainly for supporting Jazz development, evaluation purposes and keeping a stable package name
for the future. Unless you are interested in Jazz core development, we recommend you to wait
until version 0.3.1 before giving Jazz a serious evaluation.


\strong{Run a docker image from docker hub (hub.docker.com)}

This is fastest way for evaluation purposes. Check dockerhub to find the name of the latest image.

  docker run -p8888:8888 kaalam/jazz_neat:0.2.1.99


\strong{Compile from source}

The most controlable way is to compile the server from source.

  git clone https://github.com/kaalam/Jazz\cr
  cd Jazz\cr
  ./config.sh\cr
  cd server\cr
  make jazz\cr
  ./jazz start\cr


You can point a browser at \code{http://localhost:8888///} to check if the server is running.
}
