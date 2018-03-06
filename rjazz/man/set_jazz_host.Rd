\name{set_jazz_host}
\alias{set_jazz_host}

\title{
Set the name of the Jazz server to avoid passing it in all function calls
}

\description{
Sets the name of the Jazz server to avoid passing it in all function calls. It simply assigns it to the global variable .host.
}

\usage{
set_jazz_host(host)
}

\arguments{
  \item{host}{
The name of the jazz server host (including the port).
}
}

\value{
Returns TRUE if the argument is a single string. No other checks done.
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
