\name{create_web_resource}
\alias{create_web_resource}

\title{
High level create a new association of an url from an object
}

\description{
This completely adds a resource to the web interface. The resource has to be created inside a web source. A block key is created, the resource is uploaded and the link with its url is created.
}

\usage{
create_web_resource(web_source, url, type, raw_object, lang = NULL, host = .host.)
}

\arguments{
  \item{web_source}{
The name of the web source where the url and object will be included. All www resource links and urls are grouped under a "web source" which is just a name to allow removing them with a single call.
}
  \item{url}{
The url that will be used by the server to return the resource with a GET call. These urls cannot start with the name of an existing source or the API will try to execute them as API calls.
}
  \item{type}{
The mime type of the resource. A constant in type_const should be used rather than the corresponding integer value.
}
  \item{raw_object}{
The object to be uploaded via an http PUT call. Interpret "raw" as in "as is". Most cases will be strings, some R raw objects for binary PUT are also possible.
}
  \item{lang}{
The http language definition for the resource in case it has to be defined. Default value is not defined. Valid strings are: "en-US", "es-ES", .. or just two letters as in "jp".
}
  \item{host}{
(Optional) the name of the jazz server host (including the port). Usually set just once via set_jazz_host().
}
}

\value{
This function returns the newly created key value for further use or throws an error on failure.
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
