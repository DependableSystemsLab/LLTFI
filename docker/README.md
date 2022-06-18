### Building LLTFI using Docker: ###

`docker/Dockerfile` can be used to build and run LLTFI in a docker container.
- Use the variable `NPROC` to specify the number of threads that should run simultaneously. 
- All dependencies required to build LLTFI and run sample programs and tests will be installed in the docker container built using this docker file. 

Steps to build:
1. **Creating a docker image from the Dockerfile:** Copy the Dockerfile to a directory of your choice outside this repository. To create an image, run the command `docker build --tag imageName .` in the terminal.
2. **Starting a docker container:** Once the above step is completed, a docker container can be started using the command `docker run -it imageName`
