#!/usr/bin/perl -w

# @file truecase.pl
# @brief Truecaser.
#
# @author Akakpo Agbago under supervision of George Foster
#         with updates by Eric Joanis and Darlene Stewart
#
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
    $verbose, $uppercaseSentenceBeginFlag, $ucBOSEncoding, $help);

if(! Getopt::Long::GetOptions(

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

      # Uppercasing encoding
      # NOTE: --uppercaseBOS is no longer valid, but we explicitly check for it
      #       in order to generate an informative error message.
         'ucBOSEncoding=s' => \$ucBOSEncoding,
         'uppercaseBOS!' => \$uppercaseSentenceBeginFlag,
      
      # Flags
         'useLMOnly!' => \$useLMOnlyFlag,
         'cleanTags!'     => \$cleanMarkupFlag,
         'uppercaseTitlesBOW!' => \$forceNISTTitlesFUFlag,
         'useTitleModels!'     => \$useNISTTitleModelsFlag,
         'verbose!'   => \$verbose,

      # Output arguments
         'out=s'      => \$outputFile,
         'outDir=s'   => \$outputDir,

      # Help
         'help!'      => \$help,
         ) )
{
   displayHelp();
   die "\nERROR truecase.pl: aborting because of unknown or bad option(s).\n";
};


###############################################
############ INPUT TASKS PROCESSING ###########
###############################################

# print ("ARGV=" . (join '|', @ARGV) . "\r\n");
# exit;

#Displays the help for the usage of this library
if((defined $help) or (not defined $inputFile and (@ARGV < 1)))
{  displayHelp();
   exit 0;
}else
{
   if((not defined $inputFile) and (@ARGV > 0))
   {  $inputFile = $ARGV[0];
   }

   if(not defined $inputFile)
   {  die "ERROR truecase.pl: a file to truecase is required using the option \"--text=InputFile\"!  aborting...\n";
   }

   if ( defined $uppercaseSentenceBeginFlag) {
      print STDERR "ERROR truecase.pl: \"--uppercaseBOS\" flag is invalid. \n" . 
         "Instead use \"--ucBOSEncoding=<enc>\" to specify the encoding " . 
         "to use for beginning of sentence uppercasing.\n";
      displayHelp();
      die "ERROR truecase.pl: aborting because of bad --uppercaseBOS option.\n";
   }
   
   print STDERR "\ntruecase.pl: TRUECASING PROCESS START\n" unless !$verbose;

   my @tmpFiles = undef;

   if (defined $tplm or defined $tppt) {
      # This pre-empts all other cases.
      print STDERR "truecase.pl: using TPT.\n" unless !$verbose;
   }
   elsif(not defined $vocabCountFile and not defined $vocabMapFile and not defined $LMFile)
   {
      print STDERR "truecase.pl: No model is given! Assessing PORTAGE default truecasing model names...\n" unless !$verbose;
      $LMFile = portage_truecaselibconstantes::DEFAULT_NGRAM_MODEL;
      $vocabMapFile = portage_truecaselibconstantes::DEFAULT_V1V2MAP_MODEL;
      $unknownsMapFile = portage_truecaselibconstantes::DEFAULT_UNKNOWN_V1V2MAP_MODEL unless defined $unknownsMapFile;

      if(((not defined $LMFile) or (not -e $LMFile)) and ((not defined $vocabMapFile) or (not -e $vocabMapFile)))
      {  die "ERROR truecase.pl: a vocabulary mapping and/or NGram model are required " . 
             "using the option \"--map=V1V2MapFile\" or \"--voc=VocabCountFile\" " . 
             "or \"--lm=NGramFile\"! aborting...\n";
      }

      if($useNISTTitleModelsFlag)
      {  $LMFileTitles = $LMFile . portage_truecaselibconstantes::TITLE_LM_FILENAME_SUFFIX;
         $LMFileTitles = undef unless (-e $LMFileTitles);
         $vocabMapFileTitles = $vocabMapFile . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX;
         $vocabMapFileTitles = undef unless (-e $vocabMapFileTitles);
      }

      print STDERR "truecase.pl: Assessing PORTAGE default truecasing model names done...\n" unless !$verbose;

   }elsif(defined $vocabCountFile and defined $vocabMapFile)
   {  die "ERROR truecase.pl: only one vocabulary model can be used. " . 
          "Use only one of \"--voc=vocabCountFile\" or \"--map=vocabMapFile\"!  aborting...\n";
   }elsif($useNISTTitleModelsFlag and defined $vocabMapFile)
   {  $vocabMapFileTitles = $vocabMapFile . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX;
      $vocabMapFileTitles = undef unless (-e $vocabMapFileTitles);
   }elsif(defined $vocabCountFile)
   {  $vocabMapFile = portage_truecaselib::getTemporaryFile();   # get a temporary file
      push @tmpFiles, $vocabMapFile;

      # Generate the temporary V1V2 mapping from the vocabulary count file.
      print STDERR "truecase.pl: Creating a temporary V1 to V2 vocabulary mapping model into \"$vocabMapFile\" ...\n" unless !$verbose;
      portage_truecaselib::writeVocabularyMappingFileForVocabulary($vocabCountFile, $vocabMapFile);
      print STDERR "truecase.pl: Temporary V1 to V2 vocabulary mapping model done...\n" unless !$verbose;

      # Generate the temporary V1V2 mapping for titles
      if($useNISTTitleModelsFlag)
      {  $LMFileTitles = $LMFile . portage_truecaselibconstantes::TITLE_LM_FILENAME_SUFFIX;
         $LMFileTitles = undef unless (-e $LMFileTitles);
         $vocabCountFileTitles = $vocabCountFile . portage_truecaselibconstantes::TITLE_VOCABULARY_COUNT_FILENAME_SUFFIX;

         if(-e $vocabCountFileTitles)
         {  # Generate the V1V2 mapping for NIST titiles.
            $vocabMapFileTitles = $vocabMapFile . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX;
            print STDERR "truecase.pl: Creating a temporary titles V1 to V2 mapping model into \"$vocabMapFileTitles\" ...\n" unless !$verbose;
            portage_truecaselib::writeVocabularyMappingFileForVocabulary($vocabCountFileTitles, $vocabMapFileTitles, undef);
            print STDERR "truecase.pl: Temporary titles V1 to V2 mapping model done ...\n" unless !$verbose;
         }else
         {  $vocabCountFileTitles = undef;
         }
      }
   } # End if

   if(defined $unknownsMapFile and not defined $vocabMapFile)
   {  die "ERROR truecase.pl: a vocabulary count or mapping model must also be " . 
          "given when using \"unkMap\" option (Unknown-Word-Classes)!  aborting...\n";
   }

   if((not defined $outputFile) and defined $outputDir)
   {  $outputDir =~ s/\/$//;  #Remove any ending slash
      my @path = portage_truecaselib::getPathAndFilenameFor($inputFile);
      $outputFile = "$outputDir/$path[0]" . portage_truecaselibconstantes::DEFAULT_TRUECASED_FILENAME_SUFFIX;
   }

   if(defined $unknownsMapFile or $useNISTTitleModelsFlag)
   {  print STDERR "truecase.pl: Truecasing with Unknown words resolution launched on \"$inputFile\"...\n" unless !$verbose;
      portage_truecaselib::advancedTruecaseFile($inputFile, $LMFile, $vocabMapFile, $unknownsMapFile,
         $LMFileTitles, $vocabMapFileTitles, $outputFile, $lmOrder, $forceNISTTitlesFUFlag, 
         $useLMOnlyFlag, $ucBOSEncoding, $cleanMarkupFlag, $verbose);
      print STDERR "truecase.pl: Truecasing with Unknown words resolution done...\n" unless !$verbose;
   }else
   {  print STDERR "truecase.pl: Truecasing launched on \"$inputFile\" ...\n" unless !$verbose;
      portage_truecaselib::truecaseFile($inputFile, $LMFile, $vocabMapFile, $lmOrder, $useLMOnlyFlag, 
         $outputFile, $ucBOSEncoding, $cleanMarkupFlag, $verbose, $tplm, $tppt);
      print STDERR "truecase.pl: Truecasing done...\n" unless !$verbose;
   }

   # delete the temporary files
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system('rm -f ' . $file);
      }
   }

   print STDERR "truecase.pl: TRUECASING PROCESS END\n\n" unless !$verbose;
} # End if


sub displayHelp
{  
   -t STDERR ? system "pod2text -t -o $0 >&2" : system "pod2text $0 >&2";
}

=pod

=head1 NAME

truecase.pl - Truecaser

=head1 SYNOPSIS

truecase.pl [options] [--text] INPUT_FILE

=head1 DESCRIPTION

This script converts an input text file into its truecase form.

=head1 OPTIONS

=head2 Input/Output

=over 12

=item --text=INPUT_FILE

Input text file to convert into truecase.

=item --out=OUTPUT_FILE

Output file for truecase text.

=item --outDir=OUTPUT_DIR

Directory for the truecase output file.
The output file name used is the input file name with a default extension appended.

=back

=head2 Models

=over 12

=item --map=s

V1 to V2 vocabulary mapping model. Cannot be used in conjunction with --voc option.

=item --voc=s

Vocabulary statistics model file. Cannot be used in conjunction with --map option.

=item --unkmap=s

V1 to V2 unknown word classes mapping file.

=item --lm=s

Language Model (NGram file) to use.

=item --lmOrder=n

Effective N-gram order used by the language models.

=item --tplm=s

Language Model (NGram file) in TPLM format to use.

=item --tppt=s

V1 to V2 phrase table in TPPT format. (Use vocabMap2tpt.sh to create the TPPT).

=item --useLMOnly

Requests use of only the given NGram model; any given V1-to-V2 mapping will be ignored.

=back

=head2 Uppercasing Options

=over 12

=item --ucBOSEncoding=ENC

If defined, then uppercase the beginning of each sentence using the specified 
encoding (e.g. C<utf8>). Correctly uppercases accented characters.

=item --uppercaseTitlesBOW

Uppercase the first letter of all words in titles.
May not correctly uppercase accented characters.

=item --useTitleModels

Assume the input file is in NIST04 format and detect and titlecase titles accordingly.
May not correctly uppercase accented characters.

=back

=head2 Other

=over 12

=item --verbose

Print verbose information.

=item --help

Print this help.

=back

=head1 EXAMPLES

 truecase.pl --text=inputfile.txt [--lm=ngram.lm] [--useLMOnly]
        [--map=mapping.map [--voc=words.count]] [--unkmap=unknows.map]
        [--lmOrder=3] [--ucBOSEncoding=utf8] [--out=text-tc[--outDir=outs]]
        [--uppercaseTitlesBOW] [--useTitleModels] [--verbose]
        
 truecase.pl inputfile.txt [--map=mapping.map [--voc=words.count]]
        [--lm=ngram.lm] [--unkmap=unknows.map] [--useLMOnly] [--lmOrder=3]
        [--uppercaseTitlesBOW] [--useTitleModels]
        [--ucBOSEncoding=cp1252] [--out=text-tc[--outDir=outs]] [--verbose]
        
 truecase.pl --text=inputfile.txt --tplm=lm.tplm --tppt=mapping.tppt

=head1 CAVEATS

 If you experience errors related to malformed UTF-8 when processing non-UTF8 
 data, the solution is to adjust the value of your "$LANG" Environment variable. 
 Example: if "LANG==en_CA.UTF-8", set "LANG=en_CA" and export it. 
 If the problem persists, try setting LC_ALL instead.
 
 Uppercasing of accented characters may not work correctly for the 
 --uppercaseTitlesBOW and --useTitleModels options.

=head1 AUTHOR

=over 1

B<Programmer> - Akakpo Agbago; B<Supervisor> - George Foster

=back

=head1 COPYRIGHT

 Technologies langagieres interactives / Interactive Language Technologies
 Inst. de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 Copyright (c) 2004-2010, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2004-2010, Her Majesty in Right of Canada

=cut
