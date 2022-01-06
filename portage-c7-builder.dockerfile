## Dockerfile for building and running Portage on CentOS7
# Preliminary steps, in the Portage-SMT-TAS root directory:
#    cp Portage-CentOS7-Dockerfile Dockerfile
#    git clone git@github.com:nrc-cnrc/PortageTMXPrepro.git tmx-prepro
#    download PortageII-4.0-test-suite-systems.tgz from the GitHub release assets here
# Then build the image:
#    docker build --tag portage-c7-builder -f portage-c7-builder.dockerfile .
# If you want to build a learner image including only the runtime software, you can
# then also build the runner, which copies stuff from the builder image:
#    docker build --tag portage-c7-runner -f portage-c7-runner.dockerfile .

FROM centos:7 as builder

## Reuse an old yum cache
# Potential optimization, especially useful when network is slow.
# Given a previously built image which is running, run
#   docker cp <imageid>:/var/cache/yum ./saved-yum-cache
COPY saved-yum-cache? /var/cache/yum

## Create a portage user
RUN useradd -ms /bin/bash portage

## Create portage working directory
WORKDIR /usr/local/PortageII
ENV PORTAGE=/usr/local/PortageII
RUN chown portage:portage /usr/local/PortageII
run mkdir $PORTAGE/third-party && chown portage:portage $PORTAGE/third-party

## epel-release is required for perl-SOAP-lite and libsvm
RUN yum -y update && yum -y install epel-release

## Install required Linux applications
RUN yum -y install gcc-c++ wget bzip2 which make git time jq vim file \
    autoconf autoconf-archive autogen automake libtool

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
    perl-YAML \
    python3 python3-devel \
    java-1.8.0-openjdk-headless

## Install ICU
RUN yum -y install libicu libicu-devel

## Install MTILM (requires fortran)
RUN yum -y install gcc-gfortran && \
    cd /tmp && \
    git clone https://github.com/mitlm/mitlm.git && \
    cd mitlm && \
    git checkout v0.4.2 && \
    autoreconf -i && \
    ./autogen.sh --prefix $PORTAGE/third-party/mitlm && \
    make -j 4 && \
    make -j 4 install && \
    cd /tmp && \
    rm -rf mitlm

## Install word2vec
RUN cd /tmp && \
    git clone https://github.com/dav/word2vec.git && \
    cd word2vec && \
    make build && \
    mkdir -p $PORTAGE/third-party/bin && \
    cp bin/word2vec $PORTAGE/third-party/bin && \
    cd /tmp && \
    rm -rf word2vec

## Install libsvm
RUN yum -y install libsvm

## Install xmllint (normally it is already installed)
RUN yum -y install libxml2

## Install Conda and required librairies
RUN cd /tmp && \
    wget -q "https://repo.continuum.io/miniconda/Miniconda2-latest-Linux-x86_64.sh" && \
    bash Miniconda2-latest-Linux-x86_64.sh -b -p $PORTAGE/third-party/miniconda2 && \
    $PORTAGE/third-party/miniconda2/bin/conda install -y nomkl numpy mock theano && \
    rm Miniconda2-latest-Linux-x86_64.sh && \
    $PORTAGE/third-party/miniconda2/bin/conda clean -afy

## Install ICTCLAS (Chinese segmenter, optional)
RUN cd /tmp && \
    git clone https://github.com/pierrchen/ictclas_plus.git && \
    cd ictclas_plus/Source && \
    g++ -fpermissive -c *.cpp && \
    ar -r ictclas.a *.o && \
    cd ../ && \
    g++ -o ictclas main.cpp Source/ictclas.a && \
    mkdir -p $PORTAGE/third-party/bin && \
    cp -pr ictclas Data $PORTAGE/third-party/bin && \
    cd /tmp && \
    rm -rf ictclas_plus

## Install compression libraries required by Boost, and the Boost library itself
RUN yum install -y bzip2-devel zlib zlib-devel xz-devel zstd libzstd libzstd-devel boost-devel

## Since Boost is yum installed, set BOOST_ROOT to the empty string
ENV BOOST_ROOT=

## Install MGIZA++ (requires Boost and cmake)
RUN yum install -y cmake && \
    cd /tmp && \
    git clone https://github.com/moses-smt/mgiza.git && \
    cd mgiza/mgizapp && \
    cmake -DCMAKE_INSTALL_PREFIX=$PORTAGE/third-party/mgiza -DBoost_NO_BOOST_CMAKE=ON . && \
    make -j 4 && \
    make install && \
    chmod -R o+rX $PORTAGE/third-party/mgiza && \
    cd /tmp  && \
    rm -rf mgiza

## Install TCMalloc and libunwind
RUN yum -y install libunwind libunwind-devel && \
    cd /tmp && \
    wget -q https://github.com/gperftools/gperftools/archive/gperftools-2.8.tar.gz && \
    tar -xzf gperftools-2.8.tar.gz && \
    cd gperftools-gperftools-2.8 && \
    ./autogen.sh && \
    ./configure --prefix $PORTAGE/third-party/gperftools && \
    make CXXFLAGS=-g -j 5 && \
    ( make CXXFLAGS=-g -j 5 check || true ) && \
    make CXXFLAGS=-g -j 5 install && \
    cd /tmp && \
    rm -rf gperftools-gperftools-2.8 gperftools-2.8.tar.gz

## Install testing and debugging tools (CxxTest, Test::Doctest, log4cxx)
RUN yum -y install \
    log4cxx log4css-devel \
    perl-Data-TreeDumper \
    perl-Time-Piece \
    perl-CPAN  && \
    cd $PORTAGE/third-party && \
    wget -q -P /tmp https://github.com/CxxTest/cxxtest/releases/download/4.4/cxxtest-4.4.tar.gz && \
    tar -xzf /tmp/cxxtest-4.4.tar.gz && \
    (echo yes ; echo sudo ; echo yes) | cpan -i Test::Doctest && \
    cpan -i Tree::Simple && \
    rm /tmp/cxxtest-4.4.tar.gz
ENV CXXTEST_HOME=$PORTAGE/third-party/cxxtest-4.4

## Most steps above clean after themselves, but do a final clean up of /tmp
RUN rm -rf /tmp/*

## Install portage source code -- defer this after dependencies for better docker layer caching
COPY --chown=portage:portage . /usr/local/PortageII

## The rest of the build is done by the portage user
USER portage

## Install PortageTextProcessing and PortageClusterUtils and config portage
RUN cd $PORTAGE/third-party && \
    git clone https://github.com/nrc-cnrc/PortageTextProcessing.git && \
    ln -s $PORTAGE/third-party/PortageTextProcessing/SETUP.bash $PORTAGE/third-party/conf.d/PortageTextProcessing.bash && \
    git clone https://github.com/nrc-cnrc/PortageClusterUtils && \
    ln -s $PORTAGE/third-party/PortageClusterUtils/SETUP.bash $PORTAGE/third-party/conf.d/PortageClusterUtils.bash && \
    echo "source $PORTAGE/SETUP.bash" >> /home/portage/.bashrc && \
    chmod 755 /home/portage /home/portage/.bashrc

## Install the systems used for unit testing
## TODO Download PortageII-4.0-test-suite-systems.tgz from GitHub ahead of time or,
##      even better, wget it during the Docker installation process, once it's public
##      For now, we assume it was downloaded to the root of the sandbox, so it's in $PORTAGE
RUN cd $PORTAGE/test-suite && \
    tar --strip-components=2 -xzf $PORTAGE/PortageII-4.0-test-suite-systems.tgz && \
    chmod -R o+rX systems && \
    rm $PORTAGE/PortageII-4.0-test-suite-systems.tgz

## Compile and install Portage itself
# TODO - get tmx-prepro, somehow!
RUN source $PORTAGE/SETUP.bash && \
    cd $PORTAGE/src && \
    make -j 5 && \
    make -j 5 install && \
    chmod -R o+rX $PORTAGE/bin $PORTAGE/lib

CMD ["/bin/bash"]
