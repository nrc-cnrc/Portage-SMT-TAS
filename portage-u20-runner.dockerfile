## Dockerfile for building and running Portage on Ubuntu 20.04
# Preliminary steps, in the Portage-SMT-TAS root directory:
#    git clone https://github.com/nrc-cnrc/PortageTMXPrepro.git tmx-prepro
#    git clone https://github.com/nrc-cnrc/PortageTrainingFramework.git framework
#    download PortageII-4.0-test-suite-systems.tgz from the GitHub release assets here
# Step 1: build the builder image, which can also be used for running Portage:
#    docker build --tag portage-u20-builder -f portage-u20-builder.dockerfile .
# Step 2: build the runner image, a leaner image with only the runtime software:
#    docker build --tag portage-u20-runner -f portage-u20-runner.dockerfile .

## TODO: configure a user-mountable volume for doing experimental work.

FROM ubuntu:20.04 as runner

## Create a portage user
RUN useradd -ms /bin/bash portage

## Create portage working directory
WORKDIR /usr/local/PortageII
ENV PORTAGE=/usr/local/PortageII
RUN chown portage:portage /usr/local/PortageII
RUN mkdir $PORTAGE/third-party && chown portage:portage $PORTAGE/third-party

## Update the OS and install required Linux utilities and compilers
RUN apt-get update && \
    DEBIAN_FRONTEND=noninteractive apt-get install -y tzdata && \
    apt-get install -y less wget make git time jq vim bsdmainutils bc && \
    apt-get install -y libbz2-1.0 libzc4 libzstd1 libboost-all-dev libunwind8 && \
    # We work with multi-lingual language data!
    apt-get install -y locales-all && \
    # Portage is not compatible wish the default /bin/sh=dash on Ubuntu - we need bash
    ln -sf bash /bin/sh

## Perl and required modules; python3; Java 1.8
RUN apt-get install -y \
    perl-doc \
    libjson-perl \
    libsoap-lite-perl \
    libxml-twig-perl \
    libxml-xpath-perl \
    libxml-writer-perl \
    libyaml-perl \
    libxml2-utils \
    xml-twig-tools \
    python3 \
    openjdk-8-jre-headless

## Copy all the PortageII files and the installed third party software
## into the $PORTAGE working directory
COPY --from=portage-u20-builder:latest /usr/local/PortageII /usr/local/PortageII

## Most steps above clean after themselves, but do a final clean up of /tmp
RUN rm -rf /tmp/*

USER portage

## Add Portage/SETUP.bash and portage's .bashrc and make sure other users can use it too
RUN echo "source $PORTAGE/SETUP.bash" >> /home/portage/.bashrc && \
    chmod 755 /home/portage /home/portage/.bashrc

CMD ["/bin/bash"]
