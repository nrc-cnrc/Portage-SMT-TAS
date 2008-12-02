#!/usr/bin/perl -w

# @file portage_truecaselibconstantes.pm
# @brief core functionality of the truecasing module
# @author Akakpo Agbago supervised by George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004, 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, 2005, Her Majesty in Right of Canada

package portage_truecaselibconstantes;
use strict;
use warnings;


# -----------------------------------------------------------------------#
# ---- This module defines somes important constants definition for -----#
# ---- the TrueCasing library portage_truecaselib.pm                -----#
# -----------------------------------------------------------------------#

=head1 NAME

B<portage_truecaselibconstantes.pm>

=head1 DESCRIPTION

=over 4

=item B<Programmer>

 Akakpo AGBAGO

=item B<Supervisor>

 George Foster

=item B<Requirements>

  To run this TrueCasing package, the Perl libraries should be in your Perl lib
  path.  In addition, you should have a variable PORTAGE in your environment
  that points to Portage project location (for default models).

  Example in bash:
     export PORTAGE=$HOME/PORTAGEshared

=back


=head1 B<USAGE>

I<This module contains all the setup information to run successfully
TrueCaseLibrary module>.
 1- You are encouraged to backup this file before you edit it.
 2- You should modify only the values assigned but not the constants
    name.
 3- Important constants to setup:
    3.2 DEFAULT_V1V2MAP_MODEL, REMOVE_MARKUPS_FLAG, MARKUP_PATTERNS,
        DEFAULT_NGRAM_ORDER.
        DEFAULT_V1V2MAP_MODEL => path to the default V1 to V2 vocabulary
                            mapping model.
        REMOVE_MARKUPS_FLAG => true if markup tags should be removed from
                            input or output files.
        MARKUP_PATTERNS => a regular expression matching the markup tags.
        DEFAULT_NGRAM_ORDER => the order of the NGrams to generate.

=head1 B<COPYRIGHT>

 Technologies langagieres interactives / Interactive Language Technologies
 Institut de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright (c) 2004, 2005, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2004, 2005, Her Majesty in Right of Canada

=cut


#=========== Text Samples Preparation ============
# MARKUP_PATTERNS is a regular expression that represent the pattern of markup tags in a parsed file.
# It's used to remove these markup blocs from the file prior to processing if REMOVE_MARKUPS_FLAG is set to true.
use constant REMOVE_MARKUPS_FLAG => 0;
use constant MARKUP_PATTERNS => '<[^>]+>';
#junk cleaning regular expressions: Regular expression to detect all more than 3 nexted ALL UPPERCASE words lines
use constant JUNKS_REG => '([A-Z]{3,}\W?)+(\s([A-Z]{3,}\W?)+)+';

#=========== Output Results ======================
#Make sure Every sentence begins with capital letter
use constant ENSURE_UPPERCASE_SENTENCE_BEGIN => 0;

#==================== Language Models path ====================
# A default V1 to V2 case mapping model file
use constant DEFAULT_V1V2MAP_MODEL => $ENV{'PORTAGE'} . '/models/srilm/truecase/default.map';
# A default NGram model file
use constant DEFAULT_NGRAM_MODEL => $ENV{'PORTAGE'} . '/models/srilm/truecase/default.lm';
# A default NGram model file
use constant DEFAULT_UNKNOWN_V1V2MAP_MODEL => $ENV{'PORTAGE'} . '/models/srilm/truecase/default-unknownclasses.map';

# The name of the vocabulary (words) statistiques
use constant VOCABULARY_COUNT_FILENAME => 'vocabulary.count';
# The name of the V1 to V2 words forms mapping
use constant VOCABULARY_MAPPING_FILENAME => 'vocabmap.map';

# Default suffix for the truecased files. If a explicit name is not given to
# the truecase output, this is appended to the input filename
use constant DEFAULT_TRUECASED_FILENAME_SUFFIX => '.truecased';
# Suffix for the NGram Language Model for NIST TITLES MODEL
use constant TITLE_LM_FILENAME_SUFFIX => '-title.lm';
# Suffix for the vocabulary (words) statistiques for NIST TITLES MODEL
use constant TITLE_VOCABULARY_COUNT_FILENAME_SUFFIX => '-title.voc';
# Suffix for the V1 to V2 words forms mapping for NIST TITLES MODEL
use constant TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX => '-title.map';

# The order of the Language Model to train
use constant DEFAULT_NGRAM_ORDER => 3;

#================== NIST TITLES PATTERN REGULAR EXPRESSION =================
# In NIST newswire copora, title lines (Line1) lead lines (Line2) having the pattern matched by this regular expression
# Example:
#   title => Line1: US Federal Reserve May Cut Rates Again 
#            Line2: ( Reuters report from Washington ) Vice Chairman of the US Federal Reserve Rivlin warned on Monday that the global financial crisis will slow down US economic growth , and hinted that the United States may need to further cut interest rates to avoid economic recession . 
#
use constant NIST_TITLE_REG => '(reuters reports? (from )?)'
                   . '|(\( reuters \))'
                   . '|^(\( AFP)'
                   . '|(AFP ?\))'
                   . '|(\(.{1,12}report.{5,50}\))'
                   . '|(correspond\w* report\w* from)'
                   . '|(\d{1,4}.*xinhua.*\(.+\))'
                   . '|^(xinhua news agency.{3,30} \d{1,4})'
                   #. '|(.+\d{1,4} .*xinhua\(.+\))'
                   #. '|(xinhua news agency (report(er)?s?.*(from)?)?)'
                   . '|(\d{1,4}.+\(.*report.*\))'
                   . '|(\(.+\) ?-)'
                   . '|(\(\w*from\w*\))'
                   . '|^(\(.+\))'
                   . '|(AFP.*--)'
                   . '|(\( xinhua \))';      # detection of NIST format "AFTER title" lines
# Used to detect function words such as the, our, in... from regular words.
use constant FUNCTION_WORD_AVG_LENGTH => 4;


1; # End of module
