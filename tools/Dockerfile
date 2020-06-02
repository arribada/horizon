FROM ubuntu

RUN apt-get update && apt-get install -y git python3 python3.7 python3-pip libglib2.0-dev libyaml-dev usbutils
RUN pip3 install setuptools

# This argument can be modified by building with --build-arg CACHEBUST=$(date +%s)
# By doing so the image will be rebuilt from this point onwards
ARG CACHEBUST=1

# Copy the root directory into the docker image
ADD . /arribada_python_tools
WORKDIR /arribada_python_tools
RUN pip3 install python-dateutil==2.8.0
RUN python3 setup.py install