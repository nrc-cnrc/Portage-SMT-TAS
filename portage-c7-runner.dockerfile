## Dockerfile for running Portage on CentOS7
# Preliminary steps, in the Portage-SMT-TAS root directory:
#    git clone https://github.com/nrc-cnrc/PortageTMXPrepro.git tmx-prepro
#    git clone https://github.com/nrc-cnrc/PortageTrainingFramework.git framework
#    download PortageII-4.0-test-suite-systems.tgz from the GitHub release assets here
# Step 1: build the builder image, which can also be used for running portage:
#    docker build --tag portage-c7-builder -f portage-c7-builder.dockerfile .
# Step 2: build the runner image, a leaner image with only the runtime software:
#    docker build --tag portage-c7-runner -f portage-c7-runner.dockerfile .

## TODO: configure a user-mountable volume for doing experimental work.

FROM centos:7 as runner

## Reuse an old yum cache
# Potential optimization, especially useful when network is slow.
# Given a previously built image which is running, run
#   docker cp <imageid>:/var/cache/yum ./saved-yum-cache
COPY saved-yum-cache? /var/cache/yum

## epel-release is required for perl-SOAP-lite and libsvm
RUN yum -y update && yum -y install epel-release

## Install required Linux applications for runtime use of Portage
RUN yum -y install wget bzip2 which make git time jq vim file \
                   libicu libsvm libxml2 \
                   gcc-g++ gcc-gfortran \
                   zlib xz zstd libzstd libzstd boost \
                   libunwind log4cxx

## We're working with many languages, we need to install all the locales
## Ref: https://serverfault.com/questions/616790/how-to-add-language-support-on-centos-7-on-docker
RUN sed -i 's/^override_install_langs/#override_install_langs/' /etc/yum.conf && \
    yum -y reinstall glibc-common

## Install Perl, Python and Java 1.8.0
RUN yum -y install perl \
    perl-Env \
    perl-JSON \
    perl-SOAP-Lite \
    perl-Time-HiRes \
    perl-XML-Twig \
    perl-XML-XPath \
    perl-XML-Writer \
    perl-YAML \
    perl-Data-TreeDumper \
    perl-Time-Piece \
    perl-CPAN \
    python3 python3-devel \
    java-1.8.0-openjdk-headless && \
    (echo yes ; echo sudo ; echo yes) | cpan -i Test::Doctest && \
    cpan -i Tree::Simple

## Create a portage user
RUN useradd -ms /bin/bash portage

## Copy all the PortageII files and the installed third party software
## into the $PORTAGE working directory
ENV PORTAGE=/usr/local/PortageII
COPY --from=portage-c7-builder:latest /usr/local/PortageII /usr/local/PortageII

## Add Portage/SETUP.bash and portage's .bashrc and make sure other users can use it too
RUN echo "source $PORTAGE/SETUP.bash" >> /home/portage/.bashrc && \
    chmod 755 /home/portage /home/portage/.bashrc

USER portage

CMD ["/bin/bash"]
