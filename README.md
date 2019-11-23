## Jazz

Jazz is a lightweight data processing framework, including a web server. It provides data persistence and computation capabilities accessible from R and Python and also through a REST API.


## Jazz modules diagram

![Jazz modules](https://kaalam.github.io/develop_jazz02/diagrams/jazz_modules.png)


## History and Objectives

Jazz started as a research project on fast data processing servers led by Santiago Basaldúa at [BBVA Data & Analytics](https://www.bbvadata.com/) that evolved into an analytical web server. In December 2017 the source code of the server and the R client was released under an Apache 2.0 license.


## Current development

Jazz version 0.1.7 is the project released by BBVA, Jazz 0.2.x is the transition to a new architecture while keeping the server functionally equivalent to
the original. The architecture expected as Jazz 0.3.x was never completed and has fundamentally changed. The new architecture is Jazz 0.4.1 includes

  * Native support for resources other than blocks
  * Deep simplification of the type hierarchy
  * Removal of the intermediate scripting language APIs, only c++ or http
  * Deep simplification of the persistence model
  * Introduction of **fields** as the fundamental building block
  * **Bebop** grounded on the field definition
  * All releases are end-to-end. Meaning any new feature is immediately available via API.


## Acknowledgments

Special thanks to Roberto Maestre and Israel Herraiz who have been discussing the idea giving valuable feedback since the beginning, the members of the BBVA Software Engineering Core, especially Manuel Valdés and César de Pablo, for their feedback, the BBVA Data and Open Innovation Platforms team, especially Óscar Tomás and Ángel Puerto, for the hardware and their feedback, Jose Antonio Rodriguez who organized the initial presentation to the UI team, as well as BBVA labs for their feedback and help with the OSS release.


## Next Steps

  - Jazz version 0.4.1 is the next major milestone, completing all MVP functionality, expected January 2020.

  - The repositories where it will happen are:

    - [develop](https://github.com/kaalam/Jazz)
    - [stable](https://github.com/bbvadata/Jazz)

  Stay Tuned.

## Version 0.1.7 (The released sources)

  - [server](https://github.com/bbvadata/jazz-server)
  - [R client](https://github.com/bbvadata/jazz-client)
