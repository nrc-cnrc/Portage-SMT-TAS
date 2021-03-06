## Dockerfile for building and running Portage on CentOS7
# Optional preliminary steps, in the Portage-SMT-TAS root directory (allows
# choosing a particular branch or version - otherwise, these are automatically
# downloaded from GitHub when needed):
#    git clone https://github.com/nrc-cnrc/PortageTMXPrepro.git tmx-prepro
#    git clone https://github.com/nrc-cnrc/PortageTrainingFramework.git framework
#    git clone https://github.com/nrc-cnrc/PortageTextProcessing.git third-party/PortageTextProcessing
#    git clone https://github.com/nrc-cnrc/PortageClusterUtils.git third-party/PortageClusterUtils
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
# to save its yum cache for reuse here.
COPY saved-yum-cache? /var/cache/yum

## Create a portage user
RUN useradd -ms /bin/bash portage

## Create portage working directory
WORKDIR /usr/local/PortageII
ENV PORTAGE=/usr/local/PortageII
ENV PORTAGE_GIT_ROOT=https://github.com/nrc-cnrc
RUN chown portage:portage /usr/local/PortageII
RUN mkdir $PORTAGE/third-party && chown portage:portage $PORTAGE/third-party

## epel-release is required for perl-SOAP-lite and libsvm
RUN yum -y update && yum -y install epel-release

## Install required Linux applications
RUN yum -y install gcc-c++ wget bzip2 which make git time jq vim file \
    autoconf autoconf-archive autogen automake libtool libicu libicu-devel

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
    libxml2 \
    python3 python3-devel python36-regex \
    java-1.8.0-openjdk-headless && \
    pip3 install click

## Install MITLM (requires fortran)
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

## We need pdflatex and other tools to compile the tutorial and other documentation
RUN yum -y install texlive texlive-latex texlive-upquote transfig \
                   rubygem-asciidoctor doxygen graphviz

## Most steps above clean after themselves, but do a final clean up of /tmp
RUN rm -rf /tmp/*

## Install portage source code -- defer this after dependencies for better docker layer caching
COPY --chown=portage:portage . /usr/local/PortageII

## The rest of the build is done by the portage user
USER portage

## Install all the related Portage Git repos, and configure portage
RUN cd $PORTAGE/third-party && \
    test -d PortageTextProcessing || git clone $PORTAGE_GIT_ROOT/PortageTextProcessing.git && \
    ln -s $PORTAGE/third-party/PortageTextProcessing/SETUP.bash $PORTAGE/third-party/conf.d/PortageTextProcessing.bash && \
    test -d PortageClusterUtils || git clone $PORTAGE_GIT_ROOT/PortageClusterUtils && \
    ln -s $PORTAGE/third-party/PortageClusterUtils/SETUP.bash $PORTAGE/third-party/conf.d/PortageClusterUtils.bash && \
    cd $PORTAGE && \
    test -d framework -o -d framework.git || git clone $PORTAGE_GIT_ROOT/PortageTrainingFramework.git framework && \
    test -d tmx-prepro -o -d tmx-prepro-git || git clone $PORTAGE_GIT_ROOT/PortageTMXPrepro.git tmx-prepro && \
    echo "source $PORTAGE/SETUP.bash" >> /home/portage/.bashrc && \
    echo "export LANG=en_CA.utf8" >> /home/portage/.bashrc && \
    chmod 755 /home/portage /home/portage/.bashrc

## Install the systems used for unit testing
RUN cd $PORTAGE && \
    test -d PortageII-4.0-test-suite-systems.tgz || wget https://github.com/nrc-cnrc/Portage-SMT-TAS/releases/download/oss-pre-release/PortageII-4.0-test-suite-systems.tgz && \
    cd test-suite && \
    tar --strip-components=2 -xzf $PORTAGE/PortageII-4.0-test-suite-systems.tgz && \
    chmod -R o+rX systems && \
    rm $PORTAGE/PortageII-4.0-test-suite-systems.tgz

## Compile and install Portage itself
RUN source $PORTAGE/SETUP.bash && \
    cd $PORTAGE/src && \
    make -j 5 && \
    make -j 5 install && \
    chmod -R o+rX $PORTAGE/bin $PORTAGE/lib

## Compile all documentation
RUN source $PORTAGE/SETUP.bash && \
    # Compile tutorial.pdf in framework
    cd /tmp && \
    git clone $PORTAGE/framework framework-for-doc && \
    cd framework-for-doc && \
    make doc && \
    cp tutorial.pdf $PORTAGE/doc/. && \
    cd /tmp && \
    rm -rf framework-for-doc && \
    # Compile toy.pdf in the toy test suite
    cd $PORTAGE/test-suite/unit-testing/toy && \
    make doc && \
    make clean && \
    # Compile and install the PDFs for extra documentation
    cd $PORTAGE/src && \
    mkdir -p ../doc/extras/confidence && \
    make -j 5 docs && \
    cp */*.pdf ../doc/extras && \
    cp -p adaptation/README ../doc/extras/adaptation.README && \
    cp -p confidence/README confidence/ce*.ini ../doc/extras/confidence/ && \
    cp -p rescoring/README ../doc/extras/rescoring.README && \
    cp -p canoe/sparse-features.txt ../doc/extras/sparse-features.README && \
    # Compile the user manual
    cd $PORTAGE/src/user_manual && \
    make -j 5 docs >& .log.make-docs && \
    cp -ar html ../../doc/user-manual && \
    # Compile the source-code documentation
    cd $PORTAGE/src && \
    make -j 5 doxy >& .log.make-doxy && \
    # Compile the command line usage documentation
    cd $PORTAGE/src && \
    make -j 5 usage >& .log.make-usage

CMD ["/bin/bash"]
