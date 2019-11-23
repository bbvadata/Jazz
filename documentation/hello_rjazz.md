## rjazz, an R package to control Jazz, the lightweight data processing framework.

The talk introduces the package rjazz, (in cran).

Jazz (the server https://github.com/kaalam/jazz-server) and rjazz (the client https://cran.r-project.org/web/packages/rjazz/index.html) were released
as OSS by BBVA end of 2017. This corresponds to the current version actually available at cran.

During 2018, a community of volunteers started refactoring Jazz into the product showed in this talk. https://github.com/kaalam/Jazz
It will be available before Nov '18 including:

## From 20,000 feet:

Before a detailed introduction, in case you feel the need to put a label on Jazz or want to pitch it in 10 seconds, you may ask…

### Is it a distributed no-sql database, a language, an http server, a deep learning framework?
Jazz is a new kind of data product. It combines all these things and some others (an automated creator of APIs, an abstraction over data sources, …) while being just one extremely lightweight process. It is free software created on top of third party free software. It is written in C/C++, available a REST API. The framework is Linux only, but, since it is an http back-end, Jazz consumers include all devices and operating systems. Jazz is designed for millions of users interacting concurrently with large clusters.

## What makes it special?

### Note this description of the 0.3.x architecture is from the time of the presentation but has changed in 0.4.1 !!

1. Blocks are volatile, persisted, distributed or ubiquitous as defined. Abstraction from extremely small to extremely large is
automatic. The same function works on a small vector or a distributed data-frame.
2. Extremely efficient persistence (a memory mapped file) in the same process allows sampling without copying. (No need of a DB.)
3. Extremely efficient REST API in the same process with a thread pool. (No need of an http server.)
4. A full language that is close to perfect performance (assuming enough tensor size), thread safe and auto distributing instead of
just some optimized data mangling primitives. (No need of an interpreter.)
5. Distributed data-frames where cells are tensors (E.g., video or sound tracks, not links to files)
6. An abstraction over the file system that allows reading/writing binary and compressed files on demand.
7. Tree Search as a first class citizen
8. Reinforcement learning “out of the box”
9. Inference on small samples done efficiently
10. Support for automatic programming at bytecode level
11. Deep Learning as a first class citizen

## What will the R API look like?

The R package rjazz includes built-in http client support to use the REST API from R.
