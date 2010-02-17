#!/usr/bin/perl -w

# @file portage_truecaselib.pm
# @brief core functionality of the truecasing module
# @author Akakpo Agbago supervised by George Foster
#
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004, 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, 2005, Her Majesty in Right of Canada

package portage_truecaselib;

use strict;
use warnings;
use utf8;
use portage_truecaselibconstantes;
use portage_utils;
use IO::File;
use open IO=>qq{:locale};


# ------------------------------------------------------------------ #
# -- This module contains functions necessary to use a trained    -- #
# -- NGram Language Model and Vocabulary Mapping Model to restore -- #
# -- their presumed true case of files.                           -- #
# ------------------------------------------------------------------ #

my $JUNKS_REG = portage_truecaselibconstantes::JUNKS_REG;            # detection of all more than 3 nexted ALL UPPERCASE words lines
my $NIST_TITLE_REG = portage_truecaselibconstantes::NIST_TITLE_REG;  # detection of NIST format "AFTER title" lines
my $FUNCTION_WORD_AVG_LENGTH = portage_truecaselibconstantes::FUNCTION_WORD_AVG_LENGTH;




#===================== advancedTruecaseFile ====================#

=head1 SUB

B< ========================================
 ========= advancedTruecaseFile =========
 ========================================>

=over 4

=item B<DESCRIPTION>

 Convert a given file into truecase form. If $unknownsMapFile and $vocabMapFile
 are provided, it resolves of all the unknow words into their corresponding classes.

=item B<SYNOPSIS>

 portage_truecaselib::advancedTruecaseFile($inputFile, $LMFile, $vocabMapFile,
          $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $outputFile,
          $lmOrder, $forceNISTTitlesFUFlag, $useLMOnlyFlag,
          $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose);

 @PARAM $inputFile the file to be converted into truecase.
 @PARAM $LMFile the NGram Language Models file. The file should be in Doug
    Paul's ARPA ngram file format.  It can be undefined if not used.
 @PARAM $vocabMapFile the vocabulary V1-to-V2 mapping model.
    It should have the format produced by compile_truecase_map.
 @PARAM $unknownsMapFile the V1 to V2 unknown words classes mapping file.
    It should have the format produced by compile_truecase_map.
 @PARAM $LMFileTitles the NGram Language Models file for titles. The file should
    be in Doug Paul's ARPA ngram file format.  It can be undefined if not used.
 @PARAM $vocabMapFileTitles the vocabulary V1-to-V2 mapping model for titles.
    It should have the format produced by compile_truecase_map.
 @PARAM $lmOrder the effective N-gram order used by the language models.
 @PARAM $forceNISTTitlesFUFlag if this flag is true, the first letter of all
    words in titles are uppercased.
 @PARAM $outputFile the file into which the truecase result should be written.
 @PARAM $uppercaseSentenceBeginFlag if this flag is true, the begining
    of all the sentence is uppercased.
 @PARAM $useLMOnlyFlag if true, only a given NGram model ($LMFile) will be
    use; the V1-to-V2 ($vocabMapFile) will be ignored.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
    the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

 @RETURN Void. However the truecase is generated into $outputFile
    if it's given or on the standard output.

=item B<SEE ALSO>

 truecaseFile()

=back

=cut

sub advancedTruecaseFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::advancedTruecaseFile requires 14 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::advancedTruecaseFile requires 14 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($inputFile, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $outputFile, $lmOrder, $forceNISTTitlesFUFlag, $useLMOnlyFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose) = @_;

   my @tmpFiles = ();     # Temporary files collection

   my $outTmpFile;
   if(defined $outputFile)
   {  $outTmpFile = $outputFile;
   }else
   {  $outTmpFile = getTemporaryFile();
      push @tmpFiles, $outTmpFile;
   }

   my $standardOuputFlag = (defined $outputFile) ? undef : 1;

   ############## CASE OF UNKNOW WORDS CLASSES RESOLUTION ###########
   if(defined $unknownsMapFile and defined $vocabMapFile)
   {  print "portage_truecaselib::advancedTruecaseFile: Truecasing with Unknown Classes resolution...\r\n" unless not $verbose;

      #----1- Create a merged V1 to V2 map model from vocabMapFile and unknownsMapFile (unkown words classes).
      print "portage_truecaselib::advancedTruecaseFile: Creating a merged V1 to V2 map model from \"$vocabMapFile\" and \"$unknownsMapFile\"...\r\n" unless not $verbose;
      my $extendedMapFile = getTemporaryFile();   # get a temporary file
      push @tmpFiles, $extendedMapFile;
      if($useLMOnlyFlag)
      {  $extendedMapFile = $unknownsMapFile;   # ignore V1-to-V2 model if requested but process the titles!!!!
      }else
      {  mergeMapFilesInto($vocabMapFile, $unknownsMapFile, $extendedMapFile);
      } # End if
      print "portage_truecaselib::advancedTruecaseFile: merged V1 to V2 done...\r\n" unless not $verbose;

      #----2- Get a hash table of the list of all the words in the vocabulary resource (without the unkown words)
      print "portage_truecaselib::advancedTruecaseFile: Building a list of all the words in the vocabulary resource...\r\n" unless not $verbose;
      my %knownWordsListHash = extractWordsListHashFrom($vocabMapFile);
      print "portage_truecaselib::advancedTruecaseFile: Listing done...\r\n" unless not $verbose;

      #----3- Prepare the input file by resolving the unkown classes. Unknow word is replaced by its class
      print "portage_truecaselib::advancedTruecaseFile: Preparing \"$inputFile\" by resolving the unkown words into classes. An unknow word is replaced by its class...\r\n" unless not $verbose;
      my $tmpInputFile = getTemporaryFile();   # get a temporary file
      push @tmpFiles, $tmpInputFile;
      my %unknownWordsReplacementTrack;

      $cleanMarkupFlag = portage_truecaselibconstantes::REMOVE_MARKUPS_FLAG unless defined $cleanMarkupFlag;

      if($cleanMarkupFlag)
      {  cleanTextFile($inputFile, $tmpInputFile); # Help avoid encoding of flags, thus their losses
         %unknownWordsReplacementTrack = prepareFileResolvingUnknowWordsToClasses($tmpInputFile, $tmpInputFile, \%knownWordsListHash);
      }else
      {  %unknownWordsReplacementTrack = prepareFileResolvingUnknowWordsToClasses($inputFile, $tmpInputFile, \%knownWordsListHash);
      } # End if
      print "portage_truecaselib::advancedTruecaseFile: Preparing done...\r\n" unless not $verbose;

      #----4- Truecase
      print "portage_truecaselib::advancedTruecaseFile: Truecasing ...\r\n" unless not $verbose;
      truecaseFile($tmpInputFile, $LMFile, $extendedMapFile, $lmOrder, undef, $outTmpFile, $uppercaseSentenceBeginFlag, undef, $verbose);
      print "portage_truecaselib::advancedTruecaseFile: Truecasing done...\r\n" unless not $verbose;

      #----5- Recover the unknown classes
      print "portage_truecaselib::advancedTruecaseFile: Recovering the unknown words from classes...\r\n" unless not $verbose;
      recoverUnkownWordsFromClasses($outTmpFile, \%unknownWordsReplacementTrack, $outTmpFile);
      print "portage_truecaselib::advancedTruecaseFile: Recovering done...\r\n" unless not $verbose;

   ############## CASE OF NO UNKNOW WORDS CLASSES RESOLUTION ###########
   }else
   {  #----1- ------- Don't resolve Unknown classes -----------#
      print "portage_truecaselib::advancedTruecaseFile: Truecasing ...\r\n" unless not $verbose;
      truecaseFile($inputFile, $LMFile, $vocabMapFile, $lmOrder, $useLMOnlyFlag, $outTmpFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose);
      print "portage_truecaselib::advancedTruecaseFile: Truecasing done...\r\n" unless not $verbose;
   } # End if

   ############## CASE OF TITLES PROCESSING ###########
   if($forceNISTTitlesFUFlag or defined $LMFileTitles or defined $vocabMapFileTitles)
   {  print "portage_truecaselib::advancedTruecaseFile: Processing titles...\r\n" unless not $verbose;
      specialNISTTitleProcessing($outTmpFile, $LMFileTitles, $vocabMapFileTitles, $forceNISTTitlesFUFlag, $lmOrder, $useLMOnlyFlag, $outTmpFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose);

      print "portage_truecaselib::advancedTruecaseFile: Titles done...\r\n" unless not $verbose;
   } # End if

   if($standardOuputFlag)
   {  open (INPUTFILE, "$outTmpFile") or warn "\r\nWarning portage_truecaselib::advancedTruecaseFile: could not open \"$outTmpFile\" to print out its content to standard output: $! \tSkipping...\r\n";
      flock INPUTFILE, 2;  # Lock
      my @lines = <INPUTFILE>;
      close (INPUTFILE);
      print @lines;
   } # End if

   print "portage_truecaselib::advancedTruecaseFile: completed successfully!\r\n" unless not $verbose;

   # delete the temporary files
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system("rm -rf $file");
      } # End if
   } # End foreach

} # End advancedTruecaseFile




#============ truecaseFile ==============#

=head1 SUB

B< ===============================
 ========= truecaseFile ========
 ===============================>

=over 4

=item B<DESCRIPTION>

 Convert the given input file into truecase form.

=item B<SYNOPSIS>

 portage_truecaselib::truecaseFile($inputFile, $LMFile, $vocabMapFile,
   $lmOrder, $useLMOnlyFlag, $outputFile, $uppercaseSentenceBeginFlag,
   $cleanMarkupFlag, $verbose);

 @PARAM $inputFile the file to be converted into truecase.
 @PARAM $LMFile the NGram Language Models file. The file should
    be in Doug Paul's ARPA ngram file format.  It can be undefined if not used.
 @PARAM $vocabMapFile the vocabulary V1-to-V2 mapping model.
      It should have the format produced by compile_truecase_map.
      It's required.
 @PARAM $lmOrder the effective N-gram order used by the language models.
 @PARAM $outputFile the file into which the truecase result should be written.
 @PARAM $uppercaseSentenceBeginFlag if this flag is true, the begining
    of all the sentence is uppercased.
 @PARAM $useLMOnlyFlag if true, only a given NGram model ($LMFile) will be
         use; the V1-to-V2 ($vocabMapFile) will be ignored.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

 @RETURN Void. However the truecase is generated into $outputFile
    if it's given or on the standard output.

=back

=cut

# --------------------------------------------------------------------------------#
sub truecaseFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::truecaseFile requires 11 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::truecaseFile requires 11 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($inputFile, $LMFile, $vocabMapFile, $lmOrder, $useLMOnlyFlag, $outputFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $tplm, $tppt) = @_;

   if(not defined $inputFile)
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::truecaseFile: a file to convert into truecase is required! \taborting...\r\n\r\n";
   } # End if

   $uppercaseSentenceBeginFlag = portage_truecaselibconstantes::ENSURE_UPPERCASE_SENTENCE_BEGIN unless defined $uppercaseSentenceBeginFlag;
   $cleanMarkupFlag = portage_truecaselibconstantes::REMOVE_MARKUPS_FLAG unless defined $cleanMarkupFlag;
   $lmOrder = portage_truecaselibconstantes::DEFAULT_NGRAM_ORDER unless defined $lmOrder;

   my @tmpFiles = ();     # Temporary files collection

   print STDERR "portage_truecaselib::truecaseFile: Truecasing \"$inputFile\"...\r\n" unless not $verbose;
   my $cleanedInputTextFile = $inputFile;
   if($cleanMarkupFlag)
   {  $cleanedInputTextFile = getTemporaryFile();   # get a temporary file
      push @tmpFiles, $cleanedInputTextFile;
      cleanTextFile($inputFile, $cleanedInputTextFile);
   } # End if

   my @tmpFile = getPathAndFilenameFor($outputFile); # Get the parent directory and create it if not exist yet;
   if((defined $tmpFile[1]) and ($tmpFile[1] ne '') and not ($tmpFile[1] =~ /(\.\.?\/?$)/))
   {  system 'mkdir --parent ' . $tmpFile[1];
   } # End if


   if (defined $tplm or defined $tppt) {
      print STDERR "portage_truecaselib::truecaseFile: using tpt.\n" unless not $verbose;

      die "Error: You must provide a tplm AND a tppt model.\n" unless (defined $tplm and defined $tppt);

      # Build the command.
      my $command = "set -o pipefail;"
        . "cat $cleanedInputTextFile | canoe-escapes.pl -add"
        . " | canoe -f /dev/null "
        . " -ttable-tppt       $tppt"
        . " -lmodel-file       $tplm"
        . (defined $lmOrder ? " -lmodel-order $lmOrder" : "")
        . " -ttable-limit      100"
        . " -stack             100"
        . " -ftm               1.0"
        . " -lm                2.302585"
        . " -tm                0.0"
        . " -distortion-limit  0"
        . ( $verbose ? "" : " 2> /dev/null" )
        . " | perl -n -e 's/^<s>\\s*//o; s/\\s*<\\/s>[ ]*//o;"
        . ( $uppercaseSentenceBeginFlag ? " print ucfirst;'" : " print;'" )
        . ( defined $outputFile ? " > $outputFile" : '' );

      # Normalize space in the command for prettier printing.
      $command =~ s/\s+/ /g;

      # What is the command?
      print STDERR "\tCOMMAND: $command\n" unless not $verbose;

      # Run the command.
      my $rc = system( $command );

      die "Error while truecasing using canoe" unless ($rc == 0);
   }
   else {
      print STDERR "portage_truecaselib::truecaseFile: using canoe with on the fly phrase table.\n" unless not $verbose;

      die "Error: A map file must be provided\n" unless (defined $vocabMapFile);
      die "Error: An LM file must be provided\n" unless (defined $LMFile);

      # Convert the vocabmap to a phrase table on the fly.
      portage_utils::zin(*MAP, $vocabMapFile);
      my $phrase_table = "canoe_tc_tmp_$$.tm";
      open( TM,  ">", "$phrase_table" );
      # Errors caused by UTF-8 characters cause problems for buffered output on Cygwin.
      if ( `uname -s` =~ 'CYGWIN' ) {
         binmode TM;
      }
      while ( <MAP> ) {
         chomp;
         my @line = split( /\t/, $_ );
         my $from = shift( @line );
         my $to   = shift( @line );
         my $prob = shift( @line );
         while (    defined( $to )   && $to ne ''
                 && defined( $prob ) && $prob ne '' ) {
            print TM "$from ||| $to ||| 1 $prob\n";
            $to   = shift( @line );
            $prob = shift( @line );
         }
      }
      close( MAP );
      close( TM );


      # Build the command.
      my $command = "set -o pipefail;"
        . "cat $cleanedInputTextFile | canoe-escapes.pl -add"
        . " | canoe -f /dev/null "
        . " -ttable-multi-prob $phrase_table"
        . " -lmodel-file       $LMFile"
        . (defined $lmOrder ? " -lmodel-order $lmOrder" : "")
        . " -ttable-limit      100"
        . " -stack             100"
        . " -ftm               1.0"
        . " -lm                2.302585"
        . " -tm                0.0"
        . " -distortion-limit  0"
        . " -load-first"
        . ( $verbose ? "" : " 2> /dev/null" )
        . " | perl -n -e 's/^<s>\\s*//o; s/\\s*<\\/s>[ ]*//o;"
        .   ( $uppercaseSentenceBeginFlag ? " print ucfirst;'" : " print;'" )
        . ( defined $outputFile ? " > $outputFile" : '' );

      # Normalize space in the command for prettier printing.
      $command =~ s/\s+/ /g;

      # What is the command?
      print STDERR "\tCOMMAND: $command\n" unless not $verbose;

      # Run the command.
      my $rc = system( $command );

      die "Error while truecasing using canoe" unless ($rc == 0);

      # Clean up.
      system( "rm -rf $phrase_table" );
   }

   print STDERR "portage_truecaselib::truecaseFile: Truecasing done...\r\n" unless not $verbose;

   # delete the temporary files
   if($cleanMarkupFlag)
   {  system("rm -rf $cleanedInputTextFile");
   } # End foreach

   print STDERR "portage_truecaselib::truecaseFile: completed successfully!\r\n" unless not $verbose;

   # delete the temporary files
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system("rm -rf $file");
      } # End if
   } # End foreach

} # End truecaseFile







########################################################################################
########################### PUBLIC UTILITY FUNCTIONS ###################################
########################################################################################


=head1 SUB

B< =================================
 ====== extractVocabFromFile =====
 =================================>

=over 4

=item B<DESCRIPTION>

 Reads a given vocabulary file into a hash table as {word => frequency}.

=item B<SYNOPSIS>

 %vocabWordsData = portage_truecaselib::extractVocabFromFile($vocabularyFilename)

 @PARAM $vocabularyFilename the vocabulary filename in the format output
        by get_voc -c

    Example:
      barry 25
      Barry 2
      Bas-Richelieu 1
      Base 5
      base 255
      ...

 @RETURN %vocabWordsData a hash table of the vocabulary words as follow:
        keys: words in their found cases (truecase).
        value: frequency associated.
 %vocabWordsData= { "example" => Frequency,
                    "Example" => 1555,
                    "another" => 10,
                  }

=back

=cut

# --------------------------------------------------------------------------------#
sub extractVocabFromFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::extractVocabFromFile requires an input argument! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::extractVocabFromFile requires an input argument! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($vocabularyFilename) = @_;

   my(%vocabWordsData, $line, @data);

   open (INPUTFILE, $vocabularyFilename) or die "\r\n\r\n!!! ERROR portage_truecaselib::extractVocabFromFile: could not open \"$vocabularyFilename\" for input: $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   #my @lines = grep !/^[ \t\r\n\f]+|[ \t\r\n\f]+$/, <INPUTFILE>;
   my @lines = <INPUTFILE>;
   close(INPUTFILE);

   foreach $line (@lines)
   {  @data = split /\s+/, $line;
      chomp @data;

      if(@data && @data==2)
      {  if($data[0] eq '<s>')   # Skip Sentence delimiters
         {  next;
         }
         if($data[0] eq '</s>')
         {  next;
         }

         $vocabWordsData{$data[0]} = $data[1];
         #print "\tINNN--- $data[0] ## $vocabWordsData{$data[0]} #\r\n";
      }elsif(@data != 0)
      {  warn "\r\nWarning portage_truecaselib::computeTrueCasingErrorsBetween: the data has a wrong format in \"$vocabularyFilename\"! \tskipping... ...\r\n";
      } # End if
   } #end foreach

   return %vocabWordsData;

} # End of extractVocabFromFile()



#============ mergeMapFilesInto ==============#

=head1 NAME

B< ========================================
 ========== mergeMapFilesInto ===========
 ========================================>

=head1 DESCRIPTION

 Concatenates two V1 to V2 mapping models.

=head1 SYNOPSIS

portage_truecaselib::mergeMapFilesInto($vocabMapFile1, $vocabMapFile2,
                                       $extendedMapFile)

 @PARAM $vocabMapFile1 the 1st V1 to V2 mapping model.
 @PARAM $vocabMapFile2 the 2nd V1 to V2 mapping model.
 @PARAM $extendedMapFile resulting V1 to V2 mapping model.

=head1 SEE ALSO

=cut

# --------------------------------------------------------------------------------#
sub mergeMapFilesInto
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::mergeMapFilesInto requires 3 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::mergeMapFilesInto requires 3 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($vocabMapFile1, $vocabMapFile2, $extendedMapFile) = @_;

   if(not defined $vocabMapFile1)
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::mergeMapFilesInto: a V1 to V2 mapping model is required for merging! \taborting...\r\n\r\n";
   } # End if

   open (INPUTFILE, "$vocabMapFile1") or die "\r\n\r\n!!! ERROR portage_truecaselib::mergeMapFilesInto: could not open the initial model \"$vocabMapFile1\": $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   my @lines = <INPUTFILE>;
   close (INPUTFILE);

   open (OUTPUTFILE, ">>$extendedMapFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::mergeMapFilesInto: could not create an appended copy for the model \"$extendedMapFile\": $! \taborting...\r\n\r\n";
   flock OUTPUTFILE, 2;  # Lock
   print OUTPUTFILE @lines;
   print OUTPUTFILE "\r\n";

   open (INPUTFILE, "$vocabMapFile2") or die "\r\n\r\n!!! ERROR portage_truecaselib::mergeMapFilesInto: could not open the Unknown-Words-Classes-Model \"$vocabMapFile2\": $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   @lines = <INPUTFILE>;
   close (INPUTFILE);

   print OUTPUTFILE @lines;
   close (OUTPUTFILE);

} # End mergeMapFilesInto




=head1 SUB

B< =================================================
 ==== writeVocabularyMappingFileForVocabulary ====
 =================================================>

=over 4

=item B<DESCRIPTION>

 Create a vocabulary V1-to-V2 mapping model from a given vocabulary statistic
 (count) file. The vocabulary count file should be in the format as produced by
 get_voc -c. The mapping has the format produced by compile_truecase_map.

=item B<SYNOPSIS>

 portage_truecaselib::writeVocabularyMappingFileForVocabularymy($vocabularyCountFilename,
                                 $vocabularyMappingFilename, $onesAllFormProbabilityFlag)

 @PARAM $vocabularyCountFilename the vocabulary filename in the format output
       by get_voc -c
 @PARAM $vocabularyMappingFilename the filename where to write the vocabulary
       V1-to-V2 mapping information
 @PARAM $onesAllFormProbabilityFlag if true, set the form probability for all
        different forms to 1 <==> Prob(lci/tci) = 1 always compared to Prob(tci/lci)

 @RETURN void, but writes out a file.
    Example:
      barry Barry 1
      barthélemy  Barthélemy 1
      bas-richelieu  Bas-Richelieu 1
      base  Base 0.0769230769230769 base 0.923076923076923
      baseball baseball 1
      based Based 0.102040816326531 based 0.897959183673469
      ...

    Example for case $onesAllFormProbabilityFlag is true:
      barry Barry 1
      barthélemy  Barthélemy 1
      bas-richelieu  Bas-Richelieu 1
      base  Base 1 base 1
      baseball baseball 1
      based Based 1 based 1
      ...

=back

=cut

# --------------------------------------------------------------------------------#
sub writeVocabularyMappingFileForVocabulary
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::writeVocabularyMappingFileForVocabulary requires 3 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::writeVocabularyMappingFileForVocabulary requires 3 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($vocabularyCountFilename, $vocabularyMappingFilename, $onesAllFormProbabilityFlag) = @_;

   if((!defined $vocabularyCountFilename) or (! -e $vocabularyCountFilename))
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::writeVocabularyMappingFileForVocabulary: vocabulary Count File is required and should have the format output by get_voc -c.  The file \"$vocabularyCountFilename\" is invalid! \taborting...\r\n\r\n";
   } # End if

   if(!defined $vocabularyMappingFilename)
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::writeVocabularyMappingFileForVocabulary: a valide name must be provided for the vocabulary V1-to-V2 mapping information file. The file \"$vocabularyCountFilename\" is invalid! \taborting...\r\n\r\n";
   }

   my %vocabWordsHash = getVocabWordsHash($vocabularyCountFilename, undef);
   writeMappingFileForVocabWordsHash(\%vocabWordsHash, $vocabularyMappingFilename, $onesAllFormProbabilityFlag);

} # End writeVocabularyMappingFileForVocabulary




=head1 SUB

B< =======================
 ==== cleanTextFile ====
 =======================>

=over 4

=item B<DESCRIPTION>

 Clean the string from markers known as <****>.

=item B<SYNOPSIS>

 portage_truecaselib::cleanTextFile($filename, $cleanedFilename,
        $excludeNISTTitlesFlag, $cleaningResidusFilename,
        $forceToLowerCaseFlag, $markupRegExpress)

 @PARAM $filename the name (path) of the text file to clean.
        The cleaning is the application of $markupRegExpress to the text.
 @PARAM $cleanedFilename the name (path) of the cleaned text file.
 @PARAM $cleanJunkLinesFlag if this flag is true, any line in the input file
        that matches portage_truecaselibconstantes::JUNKS_REG is removed from
        the output cleaned file.
 @PARAM $excludeNISTTitlesFlag if this flag is true, the input file is
        assumed to have the NIST04 format and titles are detected accordingly
        and removed from the output cleaned file.
 @PARAM $cleaningResidusFilename the file where to APPEND the removed NIST titles.
 @PARAM $forceToLowerCaseFlag, if defined, forces all the text into LowerCase.
 @PARAM $markupRegExpress a regular expression that represents the patterns
            to remove from $filename.
 @RETURN void. Write out $cleanedFilename file.

=back

=cut

#-------------------------------------------------------------------------#
sub cleanTextFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::cleanTextFile requires 7 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::cleanTextFile requires 7 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my ($filename, $cleanedFilename, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleaningResidusFilename, $forceToLowerCaseFlag, $markupRegExpress) = @_;

   open(INPUTFILE, $filename) or die "\r\n\r\n!!! ERROR portage_truecaselib::cleanTextFile: could not open \"$filename\" for input: $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   my @lines = <INPUTFILE>;
   close (INPUTFILE);

   my @residuLines = ();

   $markupRegExpress = ('(' . portage_truecaselibconstantes::MARKUP_PATTERNS . ($cleanJunkLinesFlag ? ")|($JUNKS_REG)" : ')')) unless defined $markupRegExpress;

   # Cleaning
   @lines = grep !/$markupRegExpress/o, @lines;

   if($excludeNISTTitlesFlag)
   {  if(defined $cleaningResidusFilename)
      {  for(my $i=0; $i < @lines; $i++)
         {  if($lines[$i] =~ /$NIST_TITLE_REG/io)
            {  if($i > 0)
               {  push @residuLines, $lines[$i-1];
                  $lines[$i-1] = "CRTL_PORTAGE_NIST_TITLE_LIGNE_DETECTED\r\n";
               }
            } # End if
         } # End For
      }else
      {  for(my $i=0; $i < @lines; $i++)
         {  if($lines[$i] =~ /$NIST_TITLE_REG/io)
            {  if($i > 0)
               {  $lines[$i-1] = "CRTL_PORTAGE_NIST_TITLE_LIGNE_DETECTED\r\n";
               }
            } # End if
         } # End For
      } # End if
      @lines = grep !/^CRTL_PORTAGE_NIST_TITLE_LIGNE_DETECTED/, @lines;
      # print "excludeNISTTitlesFlag\r\n";
   } # End if


   # --- Write out the cleaned string to ---#
   if(defined $cleanedFilename)
   {  my @path = getPathAndFilenameFor($cleanedFilename); # Get the parent directory and create it if not exist yet;
      if(($path[1] ne '') and (not -e $path[1]))
      {  system 'mkdir --parent ' . $path[1];
      } # End if
      open (OUTPUTFILE, ">$cleanedFilename") or die "\r\n\r\n!!! ERROR portage_truecaselib::cleanTextFile: could not open \"$cleanedFilename\" for output: $! \taborting...\r\n\r\n";
      flock OUTPUTFILE, 2;  # Lock
      print OUTPUTFILE ($forceToLowerCaseFlag ? lc (join '', @lines) : join '', @lines);
      close (OUTPUTFILE);
   } # End if
   if(defined $cleaningResidusFilename and (@residuLines > 0))
   {  my @path = getPathAndFilenameFor($cleaningResidusFilename); # Get the parent directory and create it if not exist yet;
      if(($path[1] ne '') and (not -e $path[1]))
      {  system 'mkdir --parent ' . $path[1];
      } # End if
      open (OUTPUTFILE, ">>$cleaningResidusFilename") or die "\r\n\r\n!!! ERROR portage_truecaselib::cleanTextFile: could not open \"$cleaningResidusFilename\" for output: $! \taborting...\r\n\r\n";
      flock OUTPUTFILE, 2;  # Lock
      print OUTPUTFILE ($forceToLowerCaseFlag ? lc (join '', @residuLines) : join '', @residuLines);
      close (OUTPUTFILE);
   } # End if
} # End of cleanTextFile



=head1 SUB

B< =================================
 ==== getPathAndFilenameFor ====
 =================================>

=over 4

=item B<DESCRIPTION>

 Gets the filename and the parent path of a given file.

=item B<SYNOPSIS>

 @path = portage_truecaselib::getPathAndFilenameFor($file)

 @PARAM $file the input file
 @RETURN an array of 2 elements: $path[0] <=> filename and $path[1] <=> parent path
         The parent path $result[1] is undefined if $file contains no directory.

=back

=cut

# ----------------------------------------------------------------------------#
sub getPathAndFilenameFor
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::getPathAndFilenameFor requires an input argument! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::getPathAndFilenameFor requires an input argument! \taborting...\r\n\r\n";
      } # End if
   } # End if

   my ($file) = @_;

   #my $file = "agb ago/aka kpo/fif ion.fif    \t ";
   #my $file = "fif ion.fif    \t ";
   #$filename =~ s/.+\///;  #Takes only the name of the file

   #print "getPathAndFilenameFor:\r\n";
   #print "file=$file\r\n";
   #print "end:\r\n";

   my $filename = $file;
   my $parent = undef;
   if($filename)
   {  $filename =~ s/(.+\/)|\s+$//go;  #Takes only the name of the file
      $parent = $file;
      #print "parent= $parent#\n";
      $parent =~ s/$filename//;  #Takes only the name of the file
      #$parent =~ s/(\/\s*$)|(\/?\s+$)//;  #Takes only the name of the file
      $parent =~ s/(\/?\s*$)//;  #Takes only the name of the file
      #print "filename= $filename#\nparent= $parent#\n";
   } # End if

#   if(not defined $parent)
#   {  $parent = undef;
#   }

   #print "getPathAndFilenameFor:\r\n";
   #print "filename=$filename\r\n";
   #print "parent=$parent\r\n";
   #print "end:\r\n";

   my @h = ($filename, $parent);

   return @h;
} # End getPathAndFilenameFor




=head1 SUB

B< ==========================
 ==== getTemporaryFile ====
 ==========================>

=over 4

=item B<DESCRIPTION>

 Gets and return a temporary file for the system.

=item B<SYNOPSIS>

 $name = portage_truecaselib::getTemporaryFile()

 @RETURN the file path.

=back

=cut

# --------------------------------------------------------#
sub getTemporaryFile()
{  use IO::File;
   use POSIX qw(tmpnam);

   # try new temporary filenames until we get one that didn't already exist
   my $filename = undef;
   my $fh;
   my $maxLoop = 0;
   do
   {
      $filename = tmpnam() . '_agbagoa-tmp';
#   my ($handle, $filename) = tempfile("plughXXXXXX", DIR => "/export/home/paulp/dev/perl/tmpnam", SUFFIX => '_agbagoa-tmp');

####Debugging Bypass###
#      my @path = getPathAndFilenameFor(tmpnam()); # Get the parent directory and create it if not exist yet;
#      $filename = "../exp/tmp/$path[0]\_agbagoa-tmp";
####Debugging Bypass###
      $maxLoop++;
      if($maxLoop > 5000)
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::getTemporaryFile: Could not get a temporary file from the OS! \taborting...\r\n\r\n";
      } # End if
   }until $fh = IO::File->new($filename, O_RDWR|O_CREAT|O_EXCL);

   # install atexit-style handler so that when we exit or die,
   # we automatically delete this temporary file
   #END
   #{  system("rm -rf $name") or die "Couldn't unlink $name : $!";
   #}

#### Other way Bypass###
# use File::Temp "tempfile";
# my ($handle, $filename) = tempfile("plughXXXXXX", DIR => "/export/home/paulp/dev/perl/tmpnam", SUFFIX => '_agbagoa-tmp');
# END {  system("rm -rf $filename") or die "Couldn't unlink $filename : $!" }
#### End Other way Bypass###

   return $filename;
} # End getTemporaryFile






########################################################################################
########################## PRIVATE UTILITY FUNCTIONS ###################################
########################################################################################


#================ getVocabWordsHash =================#

# Extract vocabulary words into a hash of hash tables of different cases
# of a word and its frenquency.
# {"example" => {{"example" => 2}, {"Example" => 1}, {"EXAMPLE" => 5}},..}

# @PARAM $vocabularyCountFilename the vocabulary filename in the format output
#        by get_voc -c
# @RETURN %vocabWordsHash a hash of hash of the vocabulary words as follow:
#        hash of lower case sequence entry of words of hash of different
#        surface forms of words with value data as FREQUENCY.
# %vocabWordsHash= { example => {"example" => Frequency, "Example" => 10,
#                                "EXAMPLE" => 5},
#                    try => { ....},
#                    ...
#                  }
# --------------------------------------------------------------------------------#
sub getVocabWordsHash
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::getVocabWordsHash requires 1 input argument! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::getVocabWordsHash requires 1 input argument! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my ($vocabularyCountFilename) = @_;

   #----------- extract the vocabulary words ----------------
   my %vocabWordsData = extractVocabFromFile($vocabularyCountFilename);

   my (%vocabWordsHash, $tmp, $w, $ww);

   foreach $w (keys %vocabWordsData)
   {  $tmp = lc $w;
      if(!exists $vocabWordsHash{$tmp})
      {  #print "\r\nh:$w";
         $vocabWordsHash{$tmp} = {$w => $vocabWordsData{$w}};
      }elsif (!exists $vocabWordsHash{$tmp}{$w})
      {  #print "\r\nhh:$w";
         $vocabWordsHash{$tmp}{$w} = $vocabWordsData{$w};
      }else #not supposed to happen
      {  #warn "\r\nWarning portage_truecaselib::not supposed to happen\r\n";
         $vocabWordsHash{$tmp}{$w} += $vocabWordsData{$w};
      } # End if
   } # End for

   return %vocabWordsHash;

} # End getVocabWordsHash




#============= writeMappingFileForVocabWordsHash ==============#
# Writes out the statistic data in the given hash table of the vocabulary words as
# output by see getVocabWordsHash() sub into the given (filename).
# @PARAM $vocabWordsHashRef a reference to the hash of hashes as output
#                           by getVocabWordsHash(..) sub.
# @PARAM $vocabularyMappingFilename the name or path of the mapping file as produced
#        by compile_truecase_map.
# @PARAM $onesAllFormProbabilityFlag if true, set the form probability for all
#        different forms to 1 <==> Prob(lci/tci) = 1 always compared to Prob(tci/lci)
# @RETURN void. Writes out $vocabularyMappingFilename.
# --------------------------------------------------------------------------------#
sub writeMappingFileForVocabWordsHash
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::writeMappingFileForVocabWordsHash requires 3 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::writeMappingFileForVocabWordsHash requires 3 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($vocabWordsHashRef, $vocabularyMappingFilename, $onesAllFormProbabilityFlag) = @_;

   my ($ww, $w, $totalCount, @keysTmp, $strBuffer);
   $strBuffer = '';

   foreach $w (sort keys %{$vocabWordsHashRef})
   {  @keysTmp = keys %{$vocabWordsHashRef->{$w}};
      $totalCount = 0;
      # get the total freq count for this word
      foreach $ww (@keysTmp)
      {  $totalCount += $vocabWordsHashRef->{$w}{$ww};
      }

      $strBuffer .= "\r\n$w\t";
      if($onesAllFormProbabilityFlag)
      {  foreach $ww (sort @keysTmp)
         {  $strBuffer .= $ww . " " . 1 . " ";
         }
      }else
      {  foreach $ww (sort @keysTmp)
         {  $strBuffer .= $ww . "\t" . ($vocabWordsHashRef->{$w}{$ww} / $totalCount) . "\t";
         } # End foreach
      } # End if
   } # End foreach

   # --- Write out the map string buffer to filename ---#
   my @path = getPathAndFilenameFor($vocabularyMappingFilename); # Get the parent directory and create it if not exist yet;
   if(($path[1] ne '') and (not -e $path[1]))
   {  system 'mkdir --parent ' . $path[1];
   } # End if
   open (OUTPUTFILE, ">$vocabularyMappingFilename") or die "\r\n\r\n!!! ERROR: could not open \"$vocabularyMappingFilename\" for output: $! \taborting...\r\n\r\n";
   flock OUTPUTFILE, 2;  # Lock
   print OUTPUTFILE $strBuffer;
   close (OUTPUTFILE);

} # End writeMappingFileForVocabWordsHash




#============ recoverUnkownWordsFromClasses ==============#

# Recovers unknown word classes into their real words and with casse if indicated.
# Example: if the word "akakpo" is unknown and associated in the hashtable with
#      "crtl_portage_common_regular_uniform_word_unknown_class_FU"
#      it'll be replaced by "Akakpo"
#
# portage_truecaselib::recoverUnkownWordsFromClasses($inputFile,
#                                 $unknownWordsReplacementTrackRef, $outputFile);
#
# @PARAM $inputFile the file to be processed for unknown word classes recovery.
# @PARAM $unknownWordsReplacementTrackRef the reference to a hashtable with a
#    structure as output by prepareFileResolvingUnknowWordsToClasses(..).
# @PARAM $outputFile the file where the result will be written.
#
# @RETURN Void. However the outputFile is generated.
#
#---------------------------------------------------------#
sub recoverUnkownWordsFromClasses
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::recoverUnkownWordsFromClasses requires 3 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::recoverUnkownWordsFromClasses requires 3 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($inputFile, $unknownWordsReplacementTrackRef, $outputFile) = @_;

   if(not defined $inputFile)
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::recoverUnkownWordsFromClasses: a file is required to recover unknown words! \taborting...\r\n\r\n";
   } # End if

   my %unknownWordsReplacementTrack = %{$unknownWordsReplacementTrackRef};

   #----4- Recover the unknown classes
   open(INPUTFILE, "$inputFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::recoverUnkownWordsFromClasses: could not open to map file \"$inputFile\" for input: $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   my @lines = <INPUTFILE>;
   @lines = grep !/^\s+$/, @lines;
   close (INPUTFILE);

   my($class, $word, $tmpWord);

   #---------------------------------------------#
   #---------- Recover Unknown classes ----------#
   #---------------------------------------------#
   foreach my $numLine (keys %unknownWordsReplacementTrack)
   {
#      print "numLine#$numLine#Line=" . (defined $lines[$numLine] ? $lines[$numLine] : 'NUUUUUUUUUUUUL') . "\r\n"; # unless ($numLine != 12);
      $tmpWord = '';
      foreach my $wordClassPairRef (@{$unknownWordsReplacementTrack{$numLine}})
      {  $word = $wordClassPairRef->[0];
         $class = $wordClassPairRef->[1];
#         print "\tWord=" . (defined $word ? $word : 'NUUUUUUUUUUUUL') . "\tClass=" . (defined $class ? $class : 'NUUUUUUUUUUUUL') . "\r\n"; # unless ($numLine != 152);
#         print "\t\t#$numLine:Line=$lines[$numLine]\r\n";
#         print "\t\tMAX#" . (scalar @lines) . "#$numLine:Line=" . (defined $lines[$numLine] ? $lines[$numLine] : 'NUUUUUUUUUUUUL') . "\r\n"; # unless ($numLine != 152);

         $lines[$numLine] =~ s/$class\_AL/$word/i;
#         print "\t\tAL:tmpWord=" . (defined $tmpWord ? $tmpWord : 'NUUUUUUUUUUUUL') . "\r\n";

         $tmpWord = ucfirst $word;
         $lines[$numLine] =~ s/$class\_FU/$tmpWord/i;
#         print "\t\tAL:tmpWord=" . (defined $tmpWord ? $tmpWord : 'NUUUUUUUUUUUUL') . "\r\n";

         $tmpWord = uc $word;
         $lines[$numLine] =~ s/$class\_AU/$tmpWord/i;
#            print "\t\tline:tmpWord=$lines[$numLine]\r\n" unless ($numLine != 12);
#            print "\t\tAU:tmpWord=$tmpWord\r\n" unless ($numLine != 12);

         if($lines[$numLine] =~ /$class\_MC/i)
         {  $word =~ /^\S{2,}(-\S{2,})*$/;    # detect Hiphened words but filtered by quantity tokens detection
            # Uppercase the hiphen parts
            if(defined $& and ($& ne ''))
            {  my @parts = split /-/, $&;
#                  print "\t\tMC:&=$&\t@parts\r\n";
               foreach my $part (@parts)
               {  if($part =~ /^(\W)(.+)$/)  # case ")Reporting-Agancy" resulting from tokenization errors
                  {  $tmpWord .= '-' . $1 . (ucfirst $2);   # Uppercase the letter after the non-word letter
                  }else
                  {  $tmpWord .= '-' . (ucfirst $part);
                  } # End if
               } # End foreach

               $tmpWord =~ s/^-//;
#                  print "\t\tMC:&=$&\r\n";
#                  print "\t\tMC:+=$+\r\n";
#                  print "\t\tMC:1=$1" . (defined $2 ? "\t2=$2" : '') . (defined $3 ? "\t3=$3" : '') . (defined $4 ? "\t4=$4" : '') . "\r\n";
            }else
            {  $tmpWord = $word;
            } # End if
            $lines[$numLine] =~ s/$class\_MC/$tmpWord/i;
            #print "\t\tMC:tmpWord=$tmpWord\r\n";
         } # End if

         #------ Just in case CASSE symbol is omitted -----#
         $lines[$numLine] =~ s/ $class /$word/i;

      } # End foreach
   } # End foreach


   #---------------------------------------------#
   #------------- Write output file -------------#
   #---------------------------------------------#
   if(defined $outputFile)
   {  my @path = getPathAndFilenameFor($outputFile); # Get the parent directory and create it if not exist yet;
      if(($path[1] ne '') and (not -e $path[1]))
      {  system 'mkdir --parent ' . $path[1];
      } # End if
      open (OUTPUTFILE, ">$outputFile") or warn "\r\nWarning portage_truecaselib::recoverUnkownWordsFromClasses: could not append the file \"$outputFile\" for output: $!\r\n";
      flock OUTPUTFILE, 2;  # Lock
      print OUTPUTFILE @lines;
      close (OUTPUTFILE);
   }else
   {  print @lines;
   } # End if


} # End recoverUnkownWordsFromClasses



#===================== specialNISTTitleProcessing ====================#

=head1 SUB

B< ===============================================
 ========== specialNISTTitleProcessing =========
 ===============================================>

=over 4

=item B<DESCRIPTION>

 Detect NIST format titles lines and truecase only those lines leaving untouched
 the other parts of the file.

=item B<SYNOPSIS>

 portage_truecaselib::specialNISTTitleProcessing($inputFile, $LMFileTitles,
            $vocabMapFileTitles, $forceNISTTitlesFUFlag, $lmOrder, $useLMOnlyFlag,
            $outputFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose);

 @PARAM $inputFile the file whose eventual titles to be converted into truecase.
 @PARAM $LMFileTitles the NGram Language Models file for titles. The file should
    be in Doug Paul's ARPA ngram file format.  It can be undefined if not used.
 @PARAM $vocabMapFileTitles the vocabulary V1-to-V2 mapping model for titles.
    It should have the format produced by compile_truecase_map.
 @PARAM $forceNISTTitlesFUFlag if this flag is true, the first letter of all
    words in titles are uppercased.
 @PARAM $lmOrder the effective N-gram order used by the language models.
 @PARAM $useLMOnlyFlag if true, only a given NGram model ($LMFile) will be
    use; the V1-to-V2 ($vocabMapFile) will be ignored.
 @PARAM $outputFile the file into which the truecase result should be written.
 @PARAM $uppercaseSentenceBeginFlag if this flag is true, the begining
    of all the sentence is uppercased.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
    the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

 @RETURN Void. However the truecase is generated into $outputFile
    if it's given or on the standard output.

=item B<SEE ALSO>

 truecaseFile()

=back

=cut

# --------------------------------------------------------------------------------#
sub specialNISTTitleProcessing
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::specialNISTTitleProcessing requires 12 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::specialNISTTitleProcessing requires 12 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($inputFile, $LMFileTitles, $vocabMapFileTitles, $forceNISTTitlesFUFlag, $lmOrder, $useLMOnlyFlag, $outputFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose) = @_;

   # Load input file into a text line buffer
   open (INPUTFILE, $inputFile) or die "\r\n\r\n!!! ERROR portage_truecaselib::specialNISTTitleProcessing: could not open \"$inputFile\" to process titles: $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   my @lines = <INPUTFILE>;
   close(INPUTFILE);

   my @tmpFiles = ();     # Temporary files collection
   my @titleLines = ();
   my @titleLineNums;

   print "portage_truecaselib::specialNISTTitleProcessing: Detecting titles in \"$inputFile\" ...\r\n" unless not $verbose;
   for(my $numLine=0; $numLine < @lines; $numLine++)
   {  # Detect title lignes (that aren't blank lines) and track them
      if(($numLine > 0) and ($lines[$numLine] =~ /$NIST_TITLE_REG/io) and not ($lines[$numLine-1] =~ /^\s+$/))
      {  push @titleLines, $lines[$numLine-1];
         push @titleLineNums, ($numLine - 1);
      } # End if
   } # End for

   print "portage_truecaselib::specialNISTTitleProcessing: Detecting titles done...\r\n" unless not $verbose;

   if(@titleLines > 0)
   {  #---- Truecase titles' buffer ----
      print "portage_truecaselib::specialNISTTitleProcessing: Truecasing titles' buffer...\r\n" unless not $verbose;
      my $inTmpFile = getTemporaryFile();  # get a temporary file to concatenate titles in
      push @tmpFiles, $inTmpFile;
      open (OUTPUTFILE, ">$inTmpFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::specialNISTTitleProcessing: could not open \"$inTmpFile\" to write the concatenated titles in: $! \taborting...\r\n\r\n";
      flock OUTPUTFILE, 2;  # Lock
      print OUTPUTFILE @titleLines;
      close(OUTPUTFILE);

      my $outTmpFile = getTemporaryFile();  # get a temporary file to receive the titles truecasing
      push @tmpFiles, $outTmpFile;

      truecaseFile($inTmpFile, $LMFileTitles, $vocabMapFileTitles, $lmOrder, $useLMOnlyFlag, $outTmpFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose);
      print "portage_truecaselib::specialNISTTitleProcessing: Truecasing titles' buffer done...\r\n" unless not $verbose;

      print "portage_truecaselib::specialNISTTitleProcessing: Updating truecased titles...\r\n" unless not $verbose;
      open (INPUTFILE, $outTmpFile) or die "\r\n\r\n!!! ERROR portage_truecaselib::specialNISTTitleProcessing: could not open \"$outTmpFile\" to process titles: $! \taborting...\r\n\r\n";
      flock INPUTFILE, 2;  # Lock
      @titleLines = <INPUTFILE>;
      close(INPUTFILE);
      @titleLines = grep !/^\s+$/, @titleLines;     # Make sure no blank lines

      # Check the consistency: #TitlesLines form truecaseFile == #TitlesLines before
      if(scalar @titleLines != scalar @titleLineNums)
      {  warn "\r\nWarning portage_truecaselib::specialNISTTitleProcessing: Titles processing is ignored due to line matching inconsistencies...\r\n";
         print "portage_truecaselib::specialNISTTitleProcessing: Titles processing is ignored due to line matching inconsistencies...\r\n" unless not $verbose;
      }else
      {  if($forceNISTTitlesFUFlag)
         {  for(my $i=0; $i < @titleLineNums; $i++)
            {  my @words = split /\s+/, $titleLines[$i];
               # print "words=@words\r\n";
               if(@words > 0)
               {  my $tmpWord = '';
                  foreach my $word (@words)
                  {  if($word =~ /\S{$FUNCTION_WORD_AVG_LENGTH,}/o)  # Detect words less than 4 letters ==> probably function words: the, in, our...
                     {  $tmpWord .= (ucfirst $word) . ' ';   # Uppercase the letter after the word
                     }else
                     {  $tmpWord .= "$word ";   # as is
                     } # End if
                  } # End foreach
                  $tmpWord =~ s/ $/\r\n/;
                  $titleLines[$i] = $tmpWord;
               } # End if
               $lines[$titleLineNums[$i]] = $titleLines[$i];
            } # End for
         }else
         {  for(my $i=0; $i < @titleLineNums; $i++)
            {  $lines[$titleLineNums[$i]] = $titleLines[$i];
            } # End for
         } # End if
      } # End if

      print "portage_truecaselib::specialNISTTitleProcessing: Updating truecased titles done...\r\n" unless not $verbose;
   }else
   {  print "portage_truecaselib::specialNISTTitleProcessing: No titles were detected...\r\n" unless not $verbose;
   } # End if

   if(defined $outputFile)
   {  my @path = getPathAndFilenameFor($outputFile); # Get the parent directory and create it if not exist yet;
      if(($path[1] ne '') and (not -e $path[1]))
      {  system 'mkdir --parent ' . $path[1];
      } # End if
      open (OUTPUTFILE, ">$outputFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::specialNISTTitleProcessing: could not open \"$outputFile\" to write out the title processed file: $! \taborting...\r\n\r\n";
      flock OUTPUTFILE, 2;  # Lock
      print OUTPUTFILE @lines;
      close(OUTPUTFILE);
   }else
   {  print @lines;
   } # End if

   # delete the temporary files
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system("rm -rf $file");
      } # End if
   } # End foreach

} # End specialNISTTitleProcessing






#============ prepareFileResolvingUnknowWordsToClasses ==============#

sub prepareFileResolvingUnknowWordsToClasses
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::prepareFileResolvingUnknowWordsToClasses requires 3 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::prepareFileResolvingUnknowWordsToClasses requires 3 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if

   my($inputFile, $preparedFile, $knownWordsListHashRef) = @_;

   my @lines;
   my @path;

   ########## CASE: NO WORDS IN THE LIST #######
   if(not defined $knownWordsListHashRef)
   {  # Copy inputFile to preparedFile as is and return undef as tracking hash
      open(INPUTFILE, "$inputFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::prepareFileResolvingUnknowWordsToClasses: could not open \"$inputFile\" for reading: $! \taborting...\r\n\r\n";
      flock INPUTFILE, 2;  # Lock
      @lines = <INPUTFILE>;
      close(INPUTFILE);

      @path = getPathAndFilenameFor($preparedFile); # Get the parent directory and create it if not exist yet;
      if(($path[1] ne '') and (not -e $path[1]))
      {  system 'mkdir --parent ' . $path[1];
      } # End if
      open(OUTPUTFILE, ">$preparedFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::prepareFileResolvingUnknowWordsToClasses: could not open \"$preparedFile\" for writing: $! \taborting...\r\n\r\n";
      flock OUTPUTFILE, 2;  # Lock
      print OUTPUTFILE @lines;
      close(OUTPUTFILE);

      return undef;
   } # End if


   ########## CASE: WORDS IN THE LIST #######

   my %knownWordsListHash = %{$knownWordsListHashRef};

   # print join '|', keys %knownWordsListHash;
   my($outLine, $line, $class);
   my @words;
   my %trackingHashTable = ();

   open(INPUTFILE, "$inputFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::prepareFileResolvingUnknowWordsToClasses: could not open \"$inputFile\" for reading: $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   @lines = <INPUTFILE>;
   @lines = grep !/^\s+$/, @lines;
   close(INPUTFILE);

   for(my $numLine=0; $numLine < @lines; $numLine++)
   {
#      print "PREP numLine#$numLine#Line=$lines[$numLine]\r\n"; # unless ($numLine > 9);
      #-----------------------------------------#
      #----- Detection of Unknown classes ------#
      #-----------------------------------------#
      my @wordClassPairs;
      @words = split /\s/, $lines[$numLine];
      $outLine = '';
      for(my $j=0; $j < @words; $j++)
      {  if(exists $knownWordsListHash{lc $words[$j]})
         {  $outLine .= " $words[$j]";
#            print "\texist words[$j]=$words[$j]\r\n"; # unless ($numLine > 9);
         }else   # unknown word found
         {
#            print "\twords[$j]=$words[$j]\r\n"; # unless ($numLine > 9);
            $class = resolveWordToClass(lc $words[$j]);
            if(defined $class)
            {  $outLine .= " $class";
               push @wordClassPairs, [$words[$j], $class];
#               print "in class=" . ($numLine + 1) . ":\twords[$j]=$words[$j]\tclass=$class\r\n"; # unless ($numLine ne 9);
            }else # Put the word back as is
            {  $outLine .= " $words[$j]";
#               print "Can't resolve Line=" . ($numLine + 1) . ":\twords[$j]=$words[$j]\r\n"; # unless ($numLine ne 9);
            } # End if
         } # End if
      } # End for
      $trackingHashTable{$numLine} = \@wordClassPairs unless (@wordClassPairs < 1);
      $outLine =~ s/^ //;
      $lines[$numLine] = "$outLine\r\n";
   } # End For

   @path = getPathAndFilenameFor($preparedFile); # Get the parent directory and create it if not exist yet;
   if(($path[1] ne '') and (not -e $path[1]))
   {  system 'mkdir --parent ' . $path[1];
   } # End if
   open(OUTPUTFILE, ">$preparedFile") or die "\r\n\r\n!!! ERROR portage_truecaselib::prepareFileResolvingUnknowWordsToClasses: could not open \"$preparedFile\" for writing: $! \taborting...\r\n\r\n";
   flock OUTPUTFILE, 2;  # Lock
   print OUTPUTFILE @lines;
   close(OUTPUTFILE);

   return %trackingHashTable;

} # End prepareFileResolvingUnknowWordsToClasses




#============ resolveWordToClass ==============#

sub resolveWordToClass
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::resolveWordToClass requires an input argument! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::resolveWordToClass requires an input argument! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($word) = @_;

   my $class = undef;

   return undef unless ($word !~ /^crtl_portage_.+_class$/);

   #-------- Detection of quantity -----#
   # /^([+-]?)(?=\d|[\.,]\d)\d*([.,]\d*)?([Ee]([+-]?\d+))?$/)
   #if($word =~ /^(\.?,?\d+)\w|\w(\d+\,?.?)$/)             # detects quantity tokens: 'us$0.169' '120us$0.169'
   if($word =~ /^(\d*\.?,?\d+)\w+|\w+(\W?\d+\,?.?\d*)$/)   # detects quantity tokens: 'us$0.169' '120us$0.169'
   {  $class = 'crtl_portage_common_quantity_unknown_class';

   #-------- Detection of acronyms -----#
   #}elsif($word =~ /^[a-z_A-Z](\.[a-z_A-Z]\.?)+$/)   # detect acronym words made of alphabetic letters only
   }elsif($word =~ /^\w(\.\w\.?)+$/)                  # detect acronym words but filtered by quantity tokens detection
   {  $class = 'crtl_portage_common_acronym_unknown_class';

   #-------- Detection of hiphen_word and uniform_word -----#
   #}elsif($word =~ /^\w{2,}(-\w{2,})*$/)   # detect Hiphened words but filtered by quantity and acronym tokens detection
   }elsif($word =~ /^\S{2,}(-\S{2,})*$/)    # detect Hiphened words but filtered by quantity and acronym tokens detection
   {  $class = (defined $1 ? 'crtl_portage_common_regular_hiphen_word_unknown_class' : 'crtl_portage_common_regular_uniform_word_unknown_class');

   #}else return undef;
   } # End if

   return $class;

} # End resolveWordToClass




#============ extractWordsListHashFrom ==============#

sub extractWordsListHashFrom
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::extractWordsListHashFrom requires an input argument! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::extractWordsListHashFrom requires an input argument! \taborting...\r\n\r\n";
      } # End if
   } # End if

   my($vocabFile) = @_;

   open (INPUTFILE, $vocabFile) or die "\r\n\r\n!!! ERROR portage_truecaselib::extractWordsListHashFrom: could not open \"$vocabFile\" to extract the list of words: $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   my @lines = <INPUTFILE>;
   close(INPUTFILE);

   my %wordsListHash;
   my @data;
   foreach my $line (@lines)
   {  @data = split /\s+/, $line;
      chomp @data;
      $wordsListHash{lc $data[0]} = 0 unless (@data < 1);    # just build a hash of words
   } #end foreach

   return %wordsListHash;

} # End extractWordsListHashFrom




=head1 COPYRIGHT INFORMATION

=over 4

=item B<Programmer>

 Akakpo AGBAGO

=item B<Supervisor>

 George Foster

=item B<Institution>

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright (c) 2004, 2005, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2004, 2005, Her Majesty in Right of Canada

=back

=cut

1;  # End of module
