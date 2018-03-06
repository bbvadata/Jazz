\name{list_web_sources}
\alias{list_web_sources}

\title{
List all the web sources on the Jazz server
}

\description{
Lists all the web sources on the Jazz server. All www resource links and urls are grouped under a "web source" which is just a name to allow removing them with a single call.
}

\usage{
list_web_sources(host = .host.)
}

\arguments{
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
}

\value{
Returns the web sources as a vector of string.
}

\examples{
\dontrun{
set_jazz_host('127.0.0.1:8888')
page <- '<html>\n<body>\n<br/>Hello world!\n</body>\n</html>'
create_web_resource('my_test',
                    '/my_test/hello.html',
                    type_const[['BLOCKTYPE_RAW_MIME_HTML']],
                    page)
# See http://127.0.0.1:8888/my_test/hello.html with a browser.

list_web_sources()
delete_web_source('my_test')
}
}
