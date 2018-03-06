\name{new_key}
\alias{new_key}

\title{
Create a key for a new block using a RNG
}

\description{
Creates a key for a new block using a RNG. The function uses runif() and does not perform any seed initialization.	See .Random.seed for more information on R's random number generation algorithms.
}

\usage{
new_key(host = .host.)
}

\arguments{
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
}

\value{
This function returns the newly created key value for further use.
}

\examples{
\dontrun{
new_key()
new_key()
new_key()
}
}
