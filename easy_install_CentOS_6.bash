# PortageII 3.0 easy install (CentOS 6.x)
# Run all the commands in this file, in order, to install PortageII 3.0 and all
# its dependencies on a CentOS 6.x machine.
# You will need internet access for the downloads.
#
# Marc Tessier
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
#   Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada


#Before you start, Copy PortageII-3.0  into  your $HOME.  Once that is done you can copy/paste the commands below or execute this file.
echo "Starting easy Install!"


#Edit SETUP.bash and uncomment line 30 to enable PYTHON_HOME_OVERRIDE.
sed -i -e 's/^#PYTHON_HOME_OVERRIDE/PYTHON_HOME_OVERRIDE/' $HOME/PortageII-3.0/SETUP.bash


# This will define the variables required to run PortageII-3.0
##Add  "source $HOME/PortageII-3.0/SETUP.bash" somewhere in your $HOME/.bashrc.
echo "source $HOME/PortageII-3.0/SETUP.bash" >> $HOME/.bashrc

#login / logout or  source SETUP.bash. If you get the error below, it is expected till everything is installed properly.
##PortageII 3.0, NRC-CNRC, (c) 2004 - 2016, Her Majesty in Right of Canada
##Error: PortageII requires Java version 1.6 or more recent
##Error: PortageII requires Python version 2.7
source $HOME/PortageII-3.0/SETUP.bash

#Below will install the basic dependencies required to run PortageII-3.0 and the third-party build tools.
##sudo access will be required for the next few steps if you are not root!
###epel-release is needed for libsvm
sudo yum -y install epel-release
sudo yum -y groupinstall 'Development Tools'

sudo yum -y install zlib-devel bzip2-devel openssl-devel ncurses-devel sqlite-devel readline-devel tk-devel \
java-1.6.0-openjdk.x86_64 icu.x86_64 libicu.x86_64 libicu-devel.x86_64 cmake.x86_64 libsvm.x86_64 \
perl-CPAN.x86_64 perl-JSON.noarch  perl-XML-Twig.noarch perl-XML-XPath.noarch perl-YAML.noarch perl-Time-HiRes.x86_64




# Installing third-party tool inside $PORTAGE/third-party/
## The order is important for the first 3 steps.

#Step 1 Install Python 2.7.5
cd /tmp
wget http://python.org/ftp/python/2.7.5/Python-2.7.5.tgz
tar xfz Python-2.7.5.tgz
cd Python-2.7.5
./configure --with-threads --enable-shared --prefix=$PORTAGE/third-party/python-2.7
make -j 4
make install


#Step 2 Install boost 1.44 with Python installed above ( This step might take a while... )
cd /tmp
wget https://sourceforge.net/projects/boost/files/boost/1.44.0/boost_1_44_0.tar.gz
tar -xzf boost_1_44_0.tar.gz
cd boost_1_44_0
./bootstrap.sh --prefix=$PORTAGE/third-party/boost --with-python=$PORTAGE/third-party/python-2.7/bin
./bjam
./bjam install


#Step 3 Install MGIZA using Boost above
cd /tmp
git clone https://github.com/moses-smt/mgiza.git
cd mgiza/mgizapp
export BOOST_ROOT=$PORTAGE/third-party/boost
cmake -DCMAKE_INSTALL_PREFIX=$PORTAGE/third-party -DBoost_NO_BOOST_CMAKE=ON
make -j 4
make install


#Step 4 Install MITLM
cd /tmp
git clone https://github.com/mit-nlp/mitlm.git
cd mitlm
./autogen.sh --prefix=$PORTAGE/third-party/mitlm
make -j 4
make  install


#Step 5 Install word2vec
cd /tmp
git clone https://github.com/dav/word2vec
cd word2vec
make -C src
cp bin/word2vec $PORTAGE/third-party/bin/


#Checking installation
source $HOME/PortageII-3.0/SETUP.bash
make -C $PORTAGE/test-suite/unit-testing/check-installation

echo "Go try out the Tutorial @--> $PORTAGE/docs/tutorial.pdf"


#Have fun!
