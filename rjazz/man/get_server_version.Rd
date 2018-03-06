\name{get_server_version}
\alias{get_server_version}

\title{
Get the version of the Jazz server
}

\description{
Returns the version of the Jazz server and throws a warning if it does not match the (major version, minor version) of the R client.
}

\usage{
get_server_version(host = .host., full = FALSE)
}

\arguments{
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
  \item{full}{
Returns a list of server properties, including: version, build (DEBUG or RELEASE), artifact (name of the OS where it was built), myname
(node name in Jazz), sysname (Linux), hostname (name of the running host), kernel (linux kernel), sysvers (detailed build of the OS),
machine (processor type and size of the pointers).
}
}

\value{
Returns the version of the Jazz server when full = FALSE or a list of server properties if TRUE.
Throws an error on failure and a warning if it does not match the (major version, minor version) of the R client.
}

\examples{
\dontrun{
get_server_version()
get_server_version(full=T)
}
}
