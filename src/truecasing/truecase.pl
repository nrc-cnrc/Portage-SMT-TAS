#!/usr/bin/perl -w

# @file
# @brief Truecaser.
#
#
# This software is distributed to the GALE project participants under the terms
# and conditions specified in GALE project agreements, and remains the sole
# property of the National Research Council of Canada.
#
# For further information, please contact :
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# Copyright 2004, Sa Majeste la Reine du Chef du Canada /
# Copyright 2004, Her Majesty in Right of Canada

use strict;
use warnings;

BEGIN {
   # If this script is run from within src/ rather than being properly
   # installed, we need to add utils/ to the Perl library include path (@INC).
   if ( $0 !~ m#/bin/[^/]*$# ) {
      my $bin_path = $0;
      $bin_path =~ s#/[^/]*$##;
      unshift @INC, "$bin_path/../utils", $bin_path;
   }
}
use portage_utils;
printCopyright("truecase.pl", 2004);
$ENV{PORTAGE_INTERNAL_CALL} = 1;


use utf8;
#use Getopt::Long::Configure('ignore_case');
use Getopt::Long;
use portage_truecaselib;

###############################################
###### READING OF COMMAND LINE ARGUMENTS ######
###############################################

my ($inputFile, $LMFile, $vocabCountFile, $vocabMapFile, $LMFileTitles,
    $vocabMapFileTitles, $vocabCountFileTitles,
    $unknownsMapFile, $lmOrder, $outputFile, $outputDir, $useLMOnlyFlag,
    $cleanMarkupFlag, $forceNISTTitlesFUFlag, $useNISTTitleModelsFlag,
    $tplm, $tppt,
    $verbose, $uppercaseSentenceBeginFlag, $help);

Getopt::Long::GetOptions(

      # Input argument
         'text=s'    => \$inputFile,

      # Models arguments
         'map=s'     => \$vocabMapFile,
         'voc=s'     => \$vocabCountFile,
         'lm=s'      => \$LMFile,
         'unkmap=s'  => \$unknownsMapFile,
         'lmorder=s' => \$lmOrder,
         'tplm=s'    => \$tplm,
         'tppt=s'    => \$tppt,

      # Flags
         'useLMOnly!' => \$useLMOnlyFlag,
         'cleanTags!'     => \$cleanMarkupFlag,
         'uppercaseBOS!' => \$uppercaseSentenceBeginFlag,
         'uppercaseTitlesBOW!' => \$forceNISTTitlesFUFlag,
         'useTitleModels!'     => \$useNISTTitleModelsFlag,
         'verbose!'   => \$verbose,

      # Output arguments
         'out=s'      => \$outputFile,
         'outDir=s'   => \$outputDir,

      # Help
         'help!'      => \$help,
         );


###############################################
############ INPUT TASKS PROCESSING ###########
###############################################

# print ("ARGV=" . (join '|', @ARGV) . "\r\n");
# exit;

#Displays the help for the usage of this library
if((defined $help) or (not defined $inputFile and (@ARGV < 1)))
{  displayHelp();

# Trains a Language Model (trigram et vocab mapping) from a sample training file given with the option "--text"
# and save the models in their respective files given with the options "--lm" and "--map".
}else
{
   if((not defined $inputFile) and (@ARGV > 0))
   {  $inputFile = $ARGV[0];
   } # End if

   if(not defined $inputFile)
   {  die "\r\n!!! ERROR truecase.pl: a file to convert into truecase is required using the option \"--text=InputFile\"! \taborting...\r\n\r\n";
   } # End if

   print STDERR "\r\ntruecase.pl: TRUECASING PROCESS START\r\n\r\n" unless !$verbose;

   my @tmpFiles = undef;

   if (defined $tplm or defined $tppt) {
      # This pre-empts all other cases.
      print STDERR "\ntruecase.pl: using TPT.\n" unless !$verbose;
   }
   elsif(not defined $vocabCountFile and not defined $vocabMapFile and not defined $LMFile)
   {
      print STDERR "truecase.pl: No model is given! Assessing PORTAGE default truecasing model names...\r\n" unless !$verbose;
      $LMFile = portage_truecaselibconstantes::DEFAULT_NGRAM_MODEL;
      $vocabMapFile = portage_truecaselibconstantes::DEFAULT_V1V2MAP_MODEL;
      $unknownsMapFile = portage_truecaselibconstantes::DEFAULT_UNKNOWN_V1V2MAP_MODEL unless defined $unknownsMapFile;

      if(((not defined $LMFile) or (not -e $LMFile)) and ((not defined $vocabMapFile) or (not -e $vocabMapFile)))
      {  die "\r\n!!! ERROR truecase.pl: a vocabulary mapping and/or NGram model are required using either the option \"--map=V1V2MapFile\" or \"--voc=VocabCountFile\" or \"--lm=NGramFile\"! \taborting...\r\n\r\n";
      } # End if

      if($useNISTTitleModelsFlag)
      {  $LMFileTitles = $LMFile . portage_truecaselibconstantes::TITLE_LM_FILENAME_SUFFIX;
         $LMFileTitles = undef unless (-e $LMFileTitles);
         $vocabMapFileTitles = $vocabMapFile . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX;
         $vocabMapFileTitles = undef unless (-e $vocabMapFileTitles);
      } # End if

      print STDERR "truecase.pl: Assessing PORTAGE default truecasing model names done...\r\n" unless !$verbose;

   }elsif(defined $vocabCountFile and defined $vocabMapFile)
   {  die "\r\n!!! ERROR truecase.pl: only one vocabulary model can be used. It's either the option \"--voc=vocabCountFile\" or the option \"--map=vocabMapFile\"! \taborting...\r\n\r\n";
   }elsif($useNISTTitleModelsFlag and defined $vocabMapFile)
   {  $vocabMapFileTitles = $vocabMapFile . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX;
      $vocabMapFileTitles = undef unless (-e $vocabMapFileTitles);
   }elsif(defined $vocabCountFile)
   {  $vocabMapFile = portage_truecaselib::getTemporaryFile();   # get a temporary file
      push @tmpFiles, $vocabMapFile;

      # Generate the temporary V1V2 mapping from the vocabulary count file.
      print STDERR "truecase.pl: Creating a temporary V1 to V2 vocabulary mapping model into \"$vocabMapFile\" ...\r\n" unless !$verbose;
      portage_truecaselib::writeVocabularyMappingFileForVocabulary($vocabCountFile, $vocabMapFile);
      print STDERR "truecase.pl: Temporary V1 to V2 vocabulary mapping model done...\r\n" unless !$verbose;

      # Generate the temporary V1V2 mapping for titles
      if($useNISTTitleModelsFlag)
      {  $LMFileTitles = $LMFile . portage_truecaselibconstantes::TITLE_LM_FILENAME_SUFFIX;
         $LMFileTitles = undef unless (-e $LMFileTitles);
         $vocabCountFileTitles = $vocabCountFile . portage_truecaselibconstantes::TITLE_VOCABULARY_COUNT_FILENAME_SUFFIX;

         if(-e $vocabCountFileTitles)
         {  # Generate the V1V2 mapping for NIST titiles.
            $vocabMapFileTitles = $vocabMapFile . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX;
            print STDERR "truecase.pl: Creating a temporary titles V1 to V2 mapping model into \"$vocabMapFileTitles\" ...\r\n" unless !$verbose;
            portage_truecaselib::writeVocabularyMappingFileForVocabulary($vocabCountFileTitles, $vocabMapFileTitles, undef);
            print STDERR "truecase.pl: Temporary titles V1 to V2 mapping model done ...\r\n" unless !$verbose;
         }else
         {  $vocabCountFileTitles = undef;
         } # End if
      } # End if
   } # End if

   if(defined $unknownsMapFile and not defined $vocabMapFile)
   {  die "\r\n!!! ERROR truecase.pl: a vocabulary count or mapping model must also be given when using \"unkMap\" option (Unknown-Word-Classes)! \taborting...\r\n\r\n";
   } # End if

   if((not defined $outputFile) and defined $outputDir)
   {  $outputDir =~ s/\/$//;  #Remove any ending slash
      my @path = portage_truecaselib::getPathAndFilenameFor($inputFile);
      $outputFile = "$outputDir/$path[0]" . portage_truecaselibconstantes::DEFAULT_TRUECASED_FILENAME_SUFFIX;
   } # End if


   if(defined $unknownsMapFile or $useNISTTitleModelsFlag)
   {  print STDERR "truecase.pl: Truecasing with Unknown words resolution launched on \"$inputFile\"...\r\n" unless !$verbose;
      portage_truecaselib::advancedTruecaseFile($inputFile, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $outputFile, $lmOrder, $forceNISTTitlesFUFlag, $useLMOnlyFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose);
      print STDERR "truecase.pl: Truecasing with Unknown words resolution done...\r\n" unless !$verbose;
   }else
   {  print STDERR "truecase.pl: Truecasing launched on \"$inputFile\" ...\r\n" unless !$verbose;
      portage_truecaselib::truecaseFile($inputFile, $LMFile, $vocabMapFile, $lmOrder, $useLMOnlyFlag, $outputFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $tplm, $tppt);
      print STDERR "truecaseengine.pl: Truecasing done...\r\n" unless !$verbose;
   } # End if


   # delete the temporary files
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system('rm -f ' . $file);
      } # End if
   } # End foreach

   print STDERR "\r\ntruecase.pl: TRUECASING PROCESS END\r\n" unless !$verbose;

} # End if



sub displayHelp
{  print "\r\n\r\n\
 =====================================================\r\
 ==================== truecase.pl ====================\r\
 =====================================================\r\n\
 This script is used to convert an input text file into\r\
 its truecase form. It can be run from command line as follows:\r\n\
   # Input argument\r\
      --text=s => input text file to convert into truecase\r\
                  (Use truecaseengine.pl to process multiple files at once.)\r\n\
   # Models arguments\r\
      --map=s  => the V1 to V2 vocabulary mapping model. Cannot\r\
                  be used in conjunction with --voc option.\r\
      --voc=s  => the vocabulary statistics model file. Cannot\r\
                  be used in conjunction with --map option.\r\
      --lm=s   => the Language Model (NGram file) to be used.\r\
      --unkmap=s  => the V1 to V2 of unknown words classes mapping file.\r\
      --lmOrder=s => the effective N-gram order used by the language models.
      --tplm=s => the Language Model (NGram file) in TPLM format to be used.
      --tppt=s => the V1 to V2 phrase table in TPPT format
                  (use vocabMap2tpt.sh to create the TPPT).

   # Flags\r\
      --useLMOnly => Requests that only a given NGram model be used;\r\
                  any given V1-to-V2 will be ignored.\r\
      --uppercaseBOS => make sur that every sentence begins with uppercase.\r\
                  It overrides the models' judgment.\r\
      --uppercaseTitlesBOW => request that the first letter of all words in\r\
                  titles be uppercased.\r\
      --useTitleModels => If this flag is true, the input file is assumed to\r\
                  have the NIST04 format and titles are detected accordingly\r\
                  and uppercased.\r\
      --verbose => print out some logs\r\n
   # Output argument\r\
      --out    => the file where to output the truecase results.\r\
                  If not given, the result is output on the standard output.\r\
      --outDir => a directory where to output the truecase result file.\r\
                  The file is given a name that is the input file name\r\
                  appended with a default extension.\r\n\
   # Help\r\
      --help   => print this help.\r\n\
 Examples:\r\n\
   truecase.pl --text=inputfile.txt [--lm=ngram.lm] [--useLMOnly]\r\
      [--map=mapping.map [--voc=words.count]] [--unkmap=unknows.map]\r\
      [--lmOrder=3] [--uppercaseBOS] [--out=text-tc[--outDir=outs]]\r\
      [--uppercaseTitlesBOW] [--useTitleModels] [--verbose]\r\
   truecase.pl inputfile.txt [--map=mapping.map [--voc=words.count]]\r\
      [--lm=ngram.lm] [--unkmap=unknows.map] [--lmOrder=3]\r\
      [--uppercaseTitlesBOW] [--useTitleModels] [--useLMOnly]\r\
      [--uppercaseBOS] [--out=text-tc[--outDir=outs]] [--verbose]\r\n\
   truecase.pl --text=inputfile.txt --tplm=lm.tplm --tppt=mapping.tppt\n\
 WARNING:\r\
    - Default options are set in the portage_truecaselibconstantes.pm module.\r\n\
    - You need the Perl modules File and IO::File.  They should be in your\r\
      Perl lib path.\r\n\
    - You should have a variable PORTAGE in your environment that\r\
      points to Portage project location (for default options).\r\n\
    - You might have some errors related to malformed UTF-8 if you're\r\
      processing non-UTF-8 data.  The solution is to remove it from your\r\
      Environment variable \$LANG.  Example: if LANG==en_CA.UTF-8, set it to\r\
      LANG=en_CA and export it.  If the problem remains, set LC_ALL instead.\r\n\
 LICENSE:\r\
  Copyright (c) 2004-2010, Sa Majeste la Reine du Chef du Canada /\r\
  Copyright (c) 2004-2010, Her Majesty in Right of Canada\r\n\
  This software is distributed to the GALE project participants under the terms\r\
  and conditions specified in GALE project agreements, and remains the sole\r\
  property of the National Research Council of Canada.\r\n\
  For further information, please contact :\r\
  Technologies langagieres interactives / Interactive Language Technologies\r\
  Inst. de technologie de l'information / Institute for Information Technology\r\
  Conseil national de recherches Canada / National Research Council Canada\r\
  See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html\r\n";
} # displayHelp


=head1

B< =======================
 ===== truecase.pl =====
 =======================>

=head1 DESCRIPTION

 This script is used to convert an input text file into
 its truecase form. It can be run from command line as follows:

   # Input argument
     --text=s => input text file to convert into truecase.
                 (Use truecaseengine.pl to process multiple files at once.)

   # Models arguments
     --map=s => the V1 to V2 vocabulary mapping model. Cannot
                be used in conjunction with --voc option.
     --voc=s => the vocabulary statistics model file. Cannot
                be used in conjunction with --map option.
     --lm=s  => the Language Model (NGram file) to be used.
     --unkmap=s => the V1 to V2 of unknown words classes mapping file.
     --lmOrder=s => the effective N-gram order used by the language models.
     --tplm=s => the Language Model (NGram file) in TPLM format to be used.
     --tppt=s => the V1 to V2 phrase table in TPPT format
                 (use vocabMap2tpt.sh to create the TPPT).

   # Flags
     --useLMOnly => Requests that only a given NGram model be used;
                any given V1-to-V2 will be ignored.
     --uppercaseBOS => make sur that every sentence begins with uppercase.
                It overrides the models' judgment.
     --uppercaseTitlesBOW => request that the first letter of all words in
                titles be uppercased.
     --useTitleModels => If this flag is true, the input file is assumed
                to have the NIST04 format and titles are detected accordingly
                and uppercased.
     --verbose => print out some logs

   # Output argument
     --out   => the file where to output the truecase results.
                If not given, the result is output on the standard output.
     --outDir => a directory where to output the truecase result file.
                The file is given a name that is the input file name
                appended with a default extension.
   # Help
     --help  => print this help.

=head1 WARNING:

   - You should set the values in portage_truecaselibconstantes.pm module
     for default options.

   - Default options are set in the portage_truecaselibconstantes.pm module.\r\n\

   - You need the Perl modules File and IO::File.  They should be in your\r\
     Perl lib path.\r\n\

   - You should have a variable PORTAGE in your environment that\r\
     points to Portage project location (for default options).\r\n\

   - You might have some errors related to malformed UTF-8 if you're
     processing non-UTF-8 data.  The solution is to remove it from your
     Environment variable \$LANG.  Example: if LANG==en_CA.UTF-8, set it to
     LANG=en_CA and export it.  If the problem remains, set LC_ALL instead.

=head1 SYNOPSIS

    truecase.pl --text=inputfile.txt [--lm=ngram.lm] [--useLMOnly]
           [--map=mapping.map [--voc=words.count]] [--unkmap=unknows.map]
           [--lmOrder=3] [--uppercaseBOS] [--out=text-tc[--outDir=outs]]
           [--uppercaseTitlesBOW] [--useTitleModels] [--verbose]
    truecase.pl inputfile.txt [--map=mapping.map [--voc=words.count]]
           [--lm=ngram.lm] [--unkmap=unknows.map] [--useLMOnly] [--lmOrder=3]
           [--uppercaseTitlesBOW] [--useTitleModels]
           [--uppercaseBOS] [--out=text-tc[--outDir=outs]] [--verbose]
    truecase.pl --text=inputfile.txt --tplm=lm.tplm --tppt=mapping.tppt

=head1 COPYRIGHT INFORMATION

=over 4

=item B<Programmer>

 Akakpo AGBAGO

=item B<Supervisor>

 George Foster

=item B<Institution>

 Copyright (c) 2004, 2005, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2004, 2005, Her Majesty in Right of Canada

 This software is distributed to the GALE project participants under the terms
 and conditions specified in GALE project agreements, and remains the sole
 property of the National Research Council of Canada.

 For further information, please contact :
 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html

=back

=cut
