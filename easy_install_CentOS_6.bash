#!/usr/bin/bash
# PortageII_cur easy install (CentOS 7.x)
# Run all the commands in this file, in order, to install PortageII_cur and all
# its dependencies on a CentOS 7.x machine (recommended) or execute this file.
# You will need internet access for the downloads.
#
# Marc Tessier
#
# Traitement multilingue de textes / Multilingual Text Processing
# Technologies de l'information et des communications /
# Information and Communications Technologies
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2016, Sa Majeste la Reine du Chef du Canada /
# Copyright 2016, Her Majesty in Right of Canada


#Before you can start, you will need to Copy and/or Extract  PortageII_cur  into your $HOME.
[ ! -f "$HOME/PortageII_cur/SETUP.bash" ] &&  echo "Error: $HOME/PortageII_cur/SETUP.bash does not exists. Copy or Extract PortageII_cur into $HOME" && exit

# Below will define the variables required to run PortageII_cur when you login automaticaly.
##Add  "source $HOME/PortageII_cur/SETUP.bash" somewhere in your $HOME/.bashrc.
	
	echo "source $HOME/PortageII_cur/SETUP.bash" >> $HOME/.bashrc


#login / logout or  source $HOME/PortageII_cur/SETUP.bash for this session.
#If you get the error below, it is expected till everything is installed properly.
##PortageII_cur, NRC-CNRC, (c) 2004 - 2016, Her Majesty in Right of Canada
##Error: PortageII requires Java version 1.6 or more recent
##Error: PortageII requires Python version 2.7
	
	source $HOME/PortageII_cur/SETUP.bash


#Below will install the basic dependencies required to run PortageII_cur and the third-party build tools.
##sudo access will be required for the next few steps if you are not root!
###epel-release is needed for libsvm
	
	sudo yum -y install epel-release
	sudo yum -y groupinstall 'Development Tools'

	sudo yum -y install  zlib-devel bzip2-devel openssl-devel ncurses-devel sqlite-devel  \
readline-devel tk-devel libicu-devel vim-common dl time libffi wget bc java-1.6.0-openjdk icu libicu cmake libsvm \
perl-CPAN perl-JSON perl-XML-Twig perl-XML-XPath perl-YAML perl-Time-HiRes perl-Time-Piece


#Build all third-party tools inside $PORTAGE/build_third-party.
#This folder can be deleted once everything is completed succesfully.

	mkdir $PORTAGE/build_third-party


#Install third-party tools inside $PORTAGE/third-party/
#Build all third-party tools inside $PORTAGE/build_third-party. This folder can be deleted once everything is completed succesfully.
	mkdir $PORTAGE/build_third-party

## The order is important for the first 3 steps.

#Step 1 Install MiniConda to get Python
	cd $PORTAGE/build_third-party
	wget 'https://repo.continuum.io/miniconda/Miniconda2-latest-Linux-x86_64.sh'
	sh Miniconda2-latest-Linux-x86_64.sh -b -p $PORTAGE/third-party/miniconda2
	conda install numpy mock theano

	pip install suds


#Step 2 Install boost 1.57 with Python installed above ( This step might take a while... )

	cd $PORTAGE/build_third-party
	wget https://sourceforge.net/projects/boost/files/boost/1.57.0/boost_1_57_0.tar.gz
	tar -xzf boost_1_57_0.tar.gz
	cd boost_1_57_0
	./bootstrap.sh --prefix=$PORTAGE/third-party/boost --with-python=$(readlink -f $(dirname $(which python)))
	./b2 -j4 -d0 -a
	./b2 -j4 install


#Step 3 Install MGIZA using Boost above

	cd $PORTAGE/build_third-party
	git clone https://github.com/moses-smt/mgiza.git
	cd mgiza/mgizapp
	export BOOST_ROOT=$PORTAGE/third-party/boost
	cmake -DCMAKE_INSTALL_PREFIX=$PORTAGE/third-party -DBoost_NO_BOOST_CMAKE=ON
	make -j 4
	make install


#Step 4 Install MITLM

	cd $PORTAGE/build_third-party
	git clone https://github.com/mit-nlp/mitlm.git
	cd mitlm
	./autogen.sh --prefix=$PORTAGE/third-party/mitlm
	make -j 4
	make  install


#Step 5 Install word2vec

	cd $PORTAGE/build_third-party
	git clone https://github.com/dav/word2vec
	cd word2vec
	make -C src
	cp bin/word2vec $PORTAGE/third-party/bin/


#Quick PortageII_cur installation check

	source $HOME/PortageII_cur/SETUP.bash
	make -C $PORTAGE/test-suite/unit-testing/check-installation

	echo "Go try out the Tutorial @--> $PORTAGE/docs/tutorial.pdf"




##OPTIONAL SRILM ( install manually )
#Needed for running the full test suite or to be used as a replacement for MITLM
#
#Get SRILM 1.71 http://www.speech.sri.com/projects/srilm/download.html
#Filling  out the form will activate the download
#For Noncommercial use only! See link on page for commercial licensing if you fall under that category.
#
#copy srilm-1.7.1.tar.gz  into  /tmp
#	
#	mkdir -p /tmp/srilm
#	cd /tmp/srilm
#	tar -xzf ../srilm-1.7.1.tar.gz
#	sed -i -e 's/\# SRILM = \/home\/speech\/stolcke\/project\/srilm\/devel/SRILM = \/tmp\/srilm/' Makefile	
#	make -j 4 World
#	make -j 4 test 	#optional All test should pass (it takes a while to run...)
#	make -j 4 cleanest
#	cp bin/i686-m64/* $PORTAGE/third-party/bin/
#	cp lib/i686-m64/* $PORTAGE/third-party/lib/
#
#login/logout or re-source your PortageII_cur setup.
#
#
#
#
#If you wish, you can run the PortageII_cur unit test suite.
#You should get the massage once completed: PASSED all test suites
#
#	cd $PORTAGE/test-suite/unit-testing/
#	./run-all-tests.sh
#

#Have fun!


