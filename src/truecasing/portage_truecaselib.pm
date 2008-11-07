#!/usr/bin/perl -w

# Copyright (c) 2004, 2005, Sa Majeste la Reine du Chef du Canada /
# Copyright (c) 2004, 2005, Her Majesty in Right of Canada
#
# This software is distributed to the GALE project participants under the terms
# and conditions specified in GALE project agreements, and remains the sole
# property of the National Research Council of Canada.
#
# For further information, please contact :
# Technologies langagieres interactives / Interactive Language Technologies
# Inst. de technologie de l'information / Institute for Information Technology
# Conseil national de recherches Canada / National Research Council Canada
# See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html

package portage_truecaselib;

use strict;
use warnings;
use utf8;
use portage_truecaselibconstantes;
use IO::File;
use open IO=>qq{:locale};


# -----------------------------------------------------------------------#
# -- This module contains functions necessary to train NGram Language ---#
# --- Model and Vocabulary Mapping Model for truecasing. It also have ---#
# - functions to convert (recover) files into their presumed true case. -#
# -----------------------------------------------------------------------#

my $JUNKS_REG = portage_truecaselibconstantes::JUNKS_REG;            # detection of all more than 3 nexted ALL UPPERCASE words lines
my $NIST_TITLE_REG = portage_truecaselibconstantes::NIST_TITLE_REG;  # detection of NIST format "AFTER title" lines
my $FUNCTION_WORD_AVG_LENGTH = portage_truecaselibconstantes::FUNCTION_WORD_AVG_LENGTH;


#print "NIST_TITLE_REG=$NIST_TITLE_REG#\r\n";

=head1 MODULE NAME

B<portage_truecaselib.pm>

=head1 DESCRIPTION

 This module contains functions necessary to train NGram Language
 Model and Vocabulary Mapping Model for truecasing. It also have
 functions to convert (recover) files into their presumed true case.

=head1 Requirements

 You should set the values in portage_truecaselibconstantes.pm module
 before you use this portage_truecaselib.pm module.

 You need SRILM libraries and Perl module File to run this TrueCasing package.
 The SRILM libraries should be seen in your search paths and the Perl libraries
 in Perl lib path. In addition to setting your search paths, you should have a
 variable PORTAGE in your environment that points to Portage project location
 (for default options).

 You might have some errors related to malformed UTF.8.
 The solution is to remove it from your Environment variable $LANG.
 Example: if LANG==en_CA.UTF.8, set it to LANG=en_CA
 and rehash or export it.

 This module can be used in 2 ways:
 1- As an object instantiated from its new() method.
 2- As a class object portage_truecaselib->XXXX()

=head1 SYNOPSIS

 $obj = portage_truecaselib->new();
 $obj->trainFileDirectoryToModelsDirectory(...)
 portage_truecaselib->trainFileDirectoryToModelsDirectory(...)
 portage_truecaselib::trainFileDirectoryToModelsDirectory(...)
 portage_truecaselib::truecaseFile(...)
 portage_truecaselib::truecaseDirectoryToDirectory(...)

=cut

# ---------------------------------------------------
sub new
{  #print '#'; print @_; print "#\n";
   my $proto = shift;
   my $self  = {};
   if($proto)   # if instantiated as a class object
   {  my $class = ref($proto) || $proto;
      bless ($self, $class);
   }else        # just as a module call
   {  bless ($self);
   } # End if
   return $self;
} # End new




#########################################################################
###################### TRAINING LANGUAGE MODELS #########################
#########################################################################



# ================= trainDirectoryToModelsDirectory ====================#

=head1 SUB

B< =============================================
 ====== trainDirectoryToModelsDirectory ======
 =============================================>

=over 4

=item B<DESCRIPTION>

 Trains a Language Model (LM) with the files in the provided directory
 argument. The Language Model is saved into the given output directory
 with names (LM_FILENAME, VOCABULARY_COUNT_FILENAME) as defined in
 portage_truecaselibconstantes.

=item B<SYNOPSIS>

 portage_truecaselib::trainDirectoryToModelsDirectory($corpusDir, $resultModelDir,
      $useModelMergingFlag, $lmOrder, $onesAllFormProbabilityFlag,
      $excludeNISTTitlesFlag, $cleanMarkupFlag, $cleanJunkLinesFlag, $verbose)

 @PARAM $corpusDir the directory where the sample files (full paths) can be found.
 @PARAM $useModelMergingFlag indicates the training algorithm to be used
        for the training of the samples.
        If $useModelMergingFlag is true then LM merging technique is used.
           See trainModelIncrementally() function.
           This algorithm is slow but uses less memory resources as it
           uses relatively small files.
        else if useModelMergingFlag is false or undefined, the pool
           technique is used. See trainModelWithPoolOfFiles() function.
           This algorithm is slow but uses less memory resources as it uses
           relatively small files.
 @PARAM $resultModelDir the directory where to put the resulting NGram and
         vocabulary statistics models.
 @PARAM $lmOrder the order of the NGram to generate.
 @PARAM $onesAllFormProbabilityFlag if true, set the form probability for all
        different forms to 1 in the V1 to V2 vocabulary mapping file. This
        means Prob(lci/tci) = 1 always compared to Prob(tci/lci)
 @PARAM $cleanJunkLinesFlag if this flag is true, any line in the input file
        that matches portage_truecaselibconstantes::JUNKS_REG is removed from
        the files before training.
 @PARAM $excludeNISTTitlesFlag if this flag is true, the training input
        files are assumed to have the NIST04 format and titles are detected
        accordingly and removed before training.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

 @RETURN Void. However language models (NGram and Vocabulary statistics) are generated.

=item B<SEE ALSO>

 trainModelIncrementally(), trainModelWithPoolOfFiles(), trainFile()

=back

=cut

# ----------------------------------------------------------------------------#
sub trainDirectoryToModelsDirectory
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainDirectoryToModelsDirectory requires 9 input arguments! \taborting...\r\n\r\n"
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainDirectoryToModelsDirectory requires 9 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($corpusDir, $resultModelDir, $useModelMergingFlag, $lmOrder, $onesAllFormProbabilityFlag, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleanMarkupFlag, $verbose) = @_;

   $useModelMergingFlag = portage_truecaselibconstantes::USE_MODELS_MERGING_ALGORITHM unless defined $useModelMergingFlag;

   #------------- Generate filenames for LM and Vocabulary models
   # Get the filnames and paths.
   my @LMFilePath = getPathAndFilenameFor(portage_truecaselibconstantes::LM_FILENAME); # Get the parent directory and create it if not exist yet;
   my @vocabCountFilePath = getPathAndFilenameFor(portage_truecaselibconstantes::VOCABULARY_COUNT_FILENAME);# Get the parent directory and create it if not exist yet;
   my @vocabMapFilePath = getPathAndFilenameFor(portage_truecaselibconstantes::VOCABULARY_MAPPING_FILENAME);# Get the parent directory and create it if not exist yet;

   # Make $resultModelDir the root parent of the model files if given
   if(defined $resultModelDir)
   {  # Get the parent directory and create it if not exist yet;
      $LMFilePath[1] = (defined $LMFilePath[1] and ($LMFilePath[1] ne '')) ? "$resultModelDir/$LMFilePath[1]" : $resultModelDir;
      $vocabCountFilePath[1] = (defined $vocabCountFilePath[1] and ($vocabCountFilePath[1] ne '')) ? "$resultModelDir/$vocabCountFilePath[1]" : $resultModelDir;
      $vocabMapFilePath[1] = (defined $vocabMapFilePath[1] and ($vocabMapFilePath[1] ne '')) ? "$resultModelDir/$vocabMapFilePath[1]" : $resultModelDir;
   } # End if

   # Create the path so that all the directories exist
   if((defined $LMFilePath[1]) and ($LMFilePath[1] ne '') and not ($LMFilePath[1] =~ /(\.\.?\/?$)/))
   {  system 'mkdir --parent ' . $LMFilePath[1];
   } # End if
   if((defined $vocabCountFilePath[1]) and ($vocabCountFilePath[1] ne '') and not ($vocabCountFilePath[1] =~ /(\.\.?\/?$)/))
   {  system 'mkdir --parent ' . $vocabCountFilePath[1];
   } # End if
   if((defined $vocabMapFilePath[1]) and ($vocabMapFilePath[1] ne '') and not ($vocabMapFilePath[1] =~ /(\.\.?\/?$)/))
   {  system 'mkdir --parent ' . $vocabMapFilePath[1];
   } # End if

   # Form the absolute paths
   $LMFilePath[0] = (defined $LMFilePath[1] and ($LMFilePath[1] ne '')) ? $LMFilePath[1] . '/' . $LMFilePath[0] : $LMFilePath[0];
   $vocabCountFilePath[0] = (defined $vocabCountFilePath[1] and ($vocabCountFilePath[1] ne '')) ? $vocabCountFilePath[1] . '/' . $vocabCountFilePath[0] : $vocabCountFilePath[0];
   $vocabMapFilePath[0] = (defined $vocabMapFilePath[1] and ($vocabMapFilePath[1] ne '')) ? $vocabMapFilePath[1] . '/' . $vocabMapFilePath[0] : $vocabMapFilePath[0];

   print "portage_truecaselib::trainDirectoryToModelsDirectory: Training with files in \"$corpusDir\"...\r\n" unless not $verbose;
   if($useModelMergingFlag)
   {  trainModelIncrementally($corpusDir, $LMFilePath[0], $vocabCountFilePath[0], $lmOrder, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleanMarkupFlag, $verbose);
   }else
   {  trainModelWithPoolOfFiles($corpusDir, $LMFilePath[0], $vocabCountFilePath[0], $lmOrder, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleanMarkupFlag, $verbose);
   } # End if
   print "portage_truecaselib::trainDirectoryToModelsDirectory: Directory training done...\r\n" unless not $verbose;

   # Generate the V1V2 mapping model of requested
   print "portage_truecaselib::trainDirectoryToModelsDirectory: Creating V1 to V2 vocabulary mapping model into \"$vocabMapFilePath[0]\" ...\r\n" unless !$verbose;
   writeVocabularyMappingFileForVocabulary($vocabCountFilePath[0], $vocabMapFilePath[0], $onesAllFormProbabilityFlag);
   print "portage_truecaselib::trainDirectoryToModelsDirectory: V1 to V2 mapping model done ...\r\n" unless !$verbose;

   if($excludeNISTTitlesFlag)
   {  # Generate the V1V2 mapping for NIST titiles.
      my $vocabFile = $vocabCountFilePath[0] . portage_truecaselibconstantes::TITLE_VOCABULARY_COUNT_FILENAME_SUFFIX;
      if(-e $vocabFile)
      { print "portage_truecaselib::trainDirectoryToModelsDirectory: Creating V1 to V2 titles vocabulary mapping model into \"$vocabMapFilePath[0]" . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX . "\" ...\r\n" unless !$verbose;
        portage_truecaselib::writeVocabularyMappingFileForVocabulary($vocabFile,
                                                                     $vocabMapFilePath[0] . portage_truecaselibconstantes::TITLE_VOCABULARY_MAPPING_FILENAME_SUFFIX,
                                                                     $onesAllFormProbabilityFlag);

        print "portage_truecaselib::trainDirectoryToModelsDirectory: V1 to V2 mapping model done ...\r\n" unless !$verbose;
      }else
      {  print "portage_truecaselib::trainDirectoryToModelsDirectory: No title information to modelize...\r\n" unless not $verbose;
      } # End if
   } # End if

   print "portage_truecaselib::trainDirectoryToModelsDirectory: completed successfully!\r\n" unless not $verbose;

} # End trainDirectoryToModelsDirectory




# ======================= trainModelIncrementally =========================#

=head1 SUB

B< =================================
 ==== trainModelIncrementally ====
 =================================>

=over 4

=item B<DESCRIPTION>

 Run the training process using merging techniques of the resulting
 models from each training file individually from the list of files
 found on the given directory as follow:
 1- take one file, trains a LM model with it;
 2- then take the next file, train another LM model with it;
 3- then merge this latter model with the former one;
 4- discard the former LM model and consider the merged LM model for
    the next iteration from point 2-4.

 Note: this algorithm is slow but uses less memory resources as it
       uses relatively small files.

=item B<SYNOPSIS>

 portage_truecaselib::trainModelIncrementally($corpusDir, $LMFile,
         $vocabCountFile, $lmOrder, $excludeNISTTitlesFlag,
         $cleanMarkupFlag, $verbose)

 @PARAM $corpusDir the training sample file. Required argument.
 @PARAM $LMFile the NGram LM file.
 @PARAM $vocabCountFile the vocabulary statistics file.
 @PARAM $lmOrder the order of the NGram to generate.
 @PARAM $cleanJunkLinesFlag if this flag is true, any line in the input file
        that matches portage_truecaselibconstantes::JUNKS_REG is removed from
        the files before training.
 @PARAM $excludeNISTTitlesFlag if this flag is true, the training input
        files are assumed to have the NIST04 format and titles are detected
        accordingly and removed before training.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

@RETURN Void. However language models (NGram and Vocabulary statistics) are generated.

=item B<SEE ALSO>

 trainDirectoryToModelsDirectory(), trainModelWithPoolOfFiles(), trainFile()

=back

=cut

# ----------------------------------------------------------------------------#
sub trainModelIncrementally
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainModelIncrementally requires 8 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainModelIncrementally requires 8 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($corpusDir, $LMFile, $vocabCountFile, $lmOrder, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleanMarkupFlag, $verbose) = @_;

   if((not defined $corpusDir) or (! -e $corpusDir))
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::trainModelIncrementally: the provided samples' directory \"$corpusDir\" does not exist! \taborting...\r\n\r\n";
   } # End if

   $lmOrder = portage_truecaselibconstantes::DEFAULT_NGRAM_ORDER unless defined $lmOrder;
   $cleanMarkupFlag = portage_truecaselibconstantes::REMOVE_MARKUPS_FLAG unless defined $cleanMarkupFlag;


   #---- Reads all the sample files URL from the considered dir. -----#
   print "portage_truecaselib::trainModelIncrementally: Inventory of all the sample files from \"$corpusDir\"...\r\n" unless not $verbose;
   opendir(INPUTFILE, $corpusDir) or die ("\r\n\r\n!!! ERROR portage_truecaselib::trainModelIncrementally: could not open \"$corpusDir\": $! \taborting...\r\n\r\n");
   my @dirFiles = grep !/(^\.\.?$)/, readdir(INPUTFILE);
   closedir(INPUTFILE);
   #build full filepaths
   my @tmpFiles = ();
   for(my $i=0; $i<@dirFiles; $i++)
   {  $dirFiles[$i] = "$corpusDir/$dirFiles[$i]";
      if(not -d $dirFiles[$i])
      {  push @tmpFiles, $dirFiles[$i];
      }else
      {  warn "\r\nWarning portage_truecaselib::trainModelIncrementally: \"$dirFiles[$i]\" in the corpus is not a file! \tskipping...\r\n";
      } # End if
   } # End for
   @dirFiles = @tmpFiles;
   #print "Files: " . (join "\r\n", sort @dirFiles) . "\r\n";
   print "portage_truecaselib::trainModelIncrementally: Inventory done...\r\n" unless not $verbose;

   @tmpFiles = ();     # Serve for Temporary files collection

   my $file = pop @dirFiles;
   if(defined $file)
   {  my ($NGramCountFilePrevious, $NGramCountFileCurrent, $resultNGramCountFile, $filePath,
          $vocabCountFilePrevious, $vocabCountFileCurrent, $resultVocabCountFile,
          $cleanedInputTextFile, $cleaningResidusLMFile, $cleaningResidusVocFile,
          $cleaningResidusFile);

      if($excludeNISTTitlesFlag)
      {  $cleaningResidusLMFile = defined $LMFile ? $LMFile . portage_truecaselibconstantes::TITLE_LM_FILENAME_SUFFIX : undef;
         $cleaningResidusVocFile = defined $vocabCountFile ? $vocabCountFile . portage_truecaselibconstantes::TITLE_VOCABULARY_COUNT_FILENAME_SUFFIX : undef;
      } # End if


      if(defined $vocabCountFile)
      {  print "portage_truecaselib::trainModelIncrementally: training with each file incrememtally...\r\n" unless not $verbose;
         $NGramCountFilePrevious = getTemporaryFile();   # get a temporary file
         push @tmpFiles, $NGramCountFilePrevious;  # to be automatically deleted when we exit or die
         $resultNGramCountFile = getTemporaryFile();   # get a temporary file
         push @tmpFiles, $resultNGramCountFile;  # to be automatically deleted when we exit or die

         $vocabCountFilePrevious = getTemporaryFile();   # get a temporary file
         push @tmpFiles, $vocabCountFilePrevious;  # to be automatically deleted when we exit or die
         $resultVocabCountFile = getTemporaryFile();   # get a temporary file
         push @tmpFiles, $resultVocabCountFile;  # to be automatically deleted when we exit or die

         my $noMessFlag = undef;    # This determines if a temporary cleanedInputTextFile should be deleted inline to help free disk space

         # ----- Load and clean the input corpus text sample by removing markers known as <****>
         print "portage_truecaselib::trainModelIncrementally: training with \"$file\"...\r\n" unless not $verbose;
         if($excludeNISTTitlesFlag or $cleanMarkupFlag)
         {  $cleanedInputTextFile = getTemporaryFile();  # get a temporary file
            $cleaningResidusFile = getTemporaryFile();   # get a temporary file
            push @tmpFiles, $cleaningResidusFile;        # to be automatically deleted when we exit or die
            cleanTextFile($file, $cleanedInputTextFile, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleaningResidusFile);
            $noMessFlag = 1;  # delete cleanedInputTextFile inline
         }else
         {  $cleanedInputTextFile = $file;
         } # End if

         # Use the SRILM toolkit to produce NGram counts as well as 1Gram count that would be the vocabulary
         system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile -write $NGramCountFilePrevious -write1 $vocabCountFilePrevious");
         system("rm -rf $cleanedInputTextFile") unless not $noMessFlag;
         print "portage_truecaselib::trainModelIncrementally: \"$file\" done...\r\n" unless not $verbose;

         foreach $file (@dirFiles)
         {  print "portage_truecaselib::trainModelIncrementally: training with \"$file\"...\r\n" unless not $verbose;
            $NGramCountFileCurrent = getTemporaryFile();   # get a temporary file
            push @tmpFiles, $NGramCountFileCurrent;  # to be automatically deleted when we exit or die

            $vocabCountFileCurrent = getTemporaryFile();   # get a temporary file
            push @tmpFiles, $vocabCountFileCurrent;        # to be automatically deleted when we exit or die

            $cleanedInputTextFile = $file;
            if($excludeNISTTitlesFlag or $cleanMarkupFlag)
            {  $cleanedInputTextFile = getTemporaryFile();   # get a temporary file
               cleanTextFile($file, $cleanedInputTextFile, $excludeNISTTitlesFlag, $cleaningResidusFile);
               $noMessFlag = 1;  # delete cleanedInputTextFile inline
            } # End if
            system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile -write $NGramCountFileCurrent -write1 $vocabCountFileCurrent");
            system("ngram-merge -write $resultNGramCountFile -float-counts -- $NGramCountFilePrevious $NGramCountFileCurrent");
            system("ngram-merge -write $resultVocabCountFile -float-counts -- $vocabCountFilePrevious $vocabCountFileCurrent");
            system ("mv -f $resultNGramCountFile $NGramCountFilePrevious");
            system ("mv -f $resultVocabCountFile $vocabCountFilePrevious");
            system ("rm -f $NGramCountFileCurrent $vocabCountFileCurrent");
            system("rm -rf $cleanedInputTextFile") unless not $noMessFlag;

            print "portage_truecaselib::trainModelIncrementally: \"$file\" done...\r\n" unless not $verbose;
         } # End foreach
         print "portage_truecaselib::trainModelIncrementally: training on all files done...\r\n" unless not $verbose;

      }else
      {
         print "portage_truecaselib::trainModelIncrementally: training with each file incrememtally...\r\n" unless not $verbose;
         $NGramCountFilePrevious = getTemporaryFile();   # get a temporary file
         push @tmpFiles, $NGramCountFilePrevious;  # to be automatically deleted when we exit or die
         $resultNGramCountFile = getTemporaryFile();   # get a temporary file
         push @tmpFiles, $resultNGramCountFile;  # to be automatically deleted when we exit or die

         my $noMessFlag = undef;    # This determines if a temporary cleanedInputTextFile should be deleted inline to help free disk space

         # ----- Load and clean the input corpus text sample by removing markers known as <****>
         print "portage_truecaselib::trainModelIncrementally: training with \"$file\"...\r\n" unless not $verbose;
         if($excludeNISTTitlesFlag or $cleanMarkupFlag)
         {  $cleanedInputTextFile = getTemporaryFile();  # get a temporary file
            $cleaningResidusFile = getTemporaryFile();   # get a temporary file
            push @tmpFiles, $cleaningResidusFile;        # to be automatically deleted when we exit or die
            cleanTextFile($file, $cleanedInputTextFile, $excludeNISTTitlesFlag, $cleaningResidusFile);
            $noMessFlag = 1;  # delete cleanedInputTextFile inline
         }else
         {  $cleanedInputTextFile = $file;
         } # End if

         # Use the SRILM toolkit to produce NGram counts as well as 1Gram count that would be the vocabulary
         system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile -write $NGramCountFilePrevious");
         system("rm -rf $cleanedInputTextFile") unless not $noMessFlag;

         print "portage_truecaselib::trainModelIncrementally: \"$file\" done...\r\n" unless not $verbose;
         
         foreach $file (@dirFiles)
         {  print "portage_truecaselib::trainModelIncrementally: training with \"$file\"...\r\n" unless not $verbose;
            $NGramCountFileCurrent = getTemporaryFile();   # get a temporary file
            push @tmpFiles, $NGramCountFileCurrent;  # to be automatically deleted when we exit or die

            $cleanedInputTextFile = $file;
            if($excludeNISTTitlesFlag or $cleanMarkupFlag)
            {  $cleanedInputTextFile = getTemporaryFile();   # get a temporary file
               cleanTextFile($file, $cleanedInputTextFile, $excludeNISTTitlesFlag, $cleaningResidusFile);
               $noMessFlag = 1;  # delete cleanedInputTextFile inline
            } # End if
            system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile -write $NGramCountFileCurrent");
            system("ngram-merge -write $resultNGramCountFile -float-counts -- $NGramCountFilePrevious $NGramCountFileCurrent");
            system("mv -rf $resultNGramCountFile $NGramCountFilePrevious");
            system("rm -rf $NGramCountFileCurrent");
            system("rm -rf $cleanedInputTextFile") unless not $noMessFlag;
            
            print "portage_truecaselib::trainModelIncrementally: \"$file\" done...\r\n" unless not $verbose;
         } # End foreach
         print "portage_truecaselib::trainModelIncrementally: training on all files done...\r\n" unless not $verbose;

      } # End if


      # Generate the model with the final merged models
      print "portage_truecaselib::trainModelIncrementally: creating models...\r\n" unless not $verbose;
      if(defined $LMFile)
      {  system("ngram-count -sort -order $lmOrder -read $NGramCountFilePrevious -lm $LMFile");
         if(defined $vocabCountFile)
         {  #rename $vocabCountFilePrevious, $vocabCountFile;
            system ("mv -f $vocabCountFilePrevious $vocabCountFile");
         } # End if
      }else
      {  if(defined $vocabCountFile)
         {  #rename $vocabCountFilePrevious, $vocabCountFile;
            system ("mv -f $vocabCountFilePrevious $vocabCountFile");
         }else
         {  system("ngram-count -sort -order $lmOrder -read $NGramCountFilePrevious");
         } # End if
      } # End if
      print "portage_truecaselib::trainModelIncrementally: models done...\r\n" unless not $verbose;

      # Generate the model for the NIST titles all concatenated together

      if(defined $cleaningResidusFile)
      {  open (INPUTFILE, $cleaningResidusFile) or warn "\r\nWarning portage_truecaselib::trainModelIncrementally: could not open \"$cleaningResidusFile\" for input: $!\r\n";
         flock INPUTFILE, 2;  # Lock
         my @lines = <INPUTFILE>;
         close (INPUTFILE);

         if(@lines > 0)    # File not empty
         {  print "portage_truecaselib::trainModelIncrementally: creating title models...\r\n" unless not $verbose;
            if(defined $cleaningResidusLMFile)
            {  if(defined $vocabCountFile)
               {  system("ngram-count -sort -order $lmOrder -text $cleaningResidusFile -lm $cleaningResidusLMFile -write1 $cleaningResidusVocFile");
               }else
               {  system("ngram-count -sort -order $lmOrder -text $cleaningResidusFile -lm $cleaningResidusLMFile");
               } # End if
            }else
            {  if(defined $cleaningResidusVocFile)
               {  system("ngram-count -sort -order $lmOrder -text $cleaningResidusFile -write1 $cleaningResidusVocFile");
               } # End if
            } # End if
            print "portage_truecaselib::trainModelIncrementally: title models done...\r\n" unless not $verbose;
         }else
         {  print "portage_truecaselib::trainModelIncrementally: No title information to modelize...\r\n" unless not $verbose;
         } # End if
      } # End if

   } #End if

   # delete the temporary files
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system("rm -rf $file");
      } # End if
   } # End foreach

   print "portage_truecaselib::trainModelIncrementally: completed successfully!\r\n" unless not $verbose;

} #End trainModelIncrementally()



# ==================== trainModelWithPoolOfFiles =====================#

=head1 SUB

B< =====================================
 ===== trainModelWithPoolOfFiles =====
 =====================================>

=over 4

=item B<DESCRIPTION>

 It builds a large temporary file that is the buffer of the concatenation
 of all the sample text files found in the given directory and train LM
 models with it.

item B<SYNOPSIS>

 portage_truecaselib::trainModelWithPoolOfFiles($corpusDir, $LMFile,
         $vocabCountFile, $lmOrder, excludeNISTTitlesFlag,
         $cleanMarkupFlag, $verbose)

 @PARAM $corpusDir the training sample file. Required argument.
 @PARAM $LMFile the NGram LM file.
 @PARAM $vocabCountFile the vocabulary statistics file.
 @PARAM $lmOrder the order of the NGram to generate.
 @PARAM $cleanJunkLinesFlag if this flag is true, any line in the input file
        that matches portage_truecaselibconstantes::JUNKS_REG is removed from
        the files before training.
 @PARAM $excludeNISTTitlesFlag if this flag is true, the training input
        files are assumed to have the NIST04 format and titles are detected
        accordingly and removed before training.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

@RETURN Void. However language models (NGram and Vocabulary statistics) are generated.

=item B<SEE ALSO>

 trainDirectoryToModelsDirectory(), trainFile(), trainModelIncrementally

=back

=cut

# -----------------------------------------------------------------------#
sub trainModelWithPoolOfFiles
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainModelWithPoolOfFiles requires 8 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainModelWithPoolOfFiles requires 8 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my ($corpusDir, $LMFile, $vocabCountFile, $lmOrder, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleanMarkupFlag, $verbose) = @_;

   if((not defined $corpusDir) or (! -e $corpusDir))
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::trainModelWithPoolOfFiles: the provided samples' directory \"$corpusDir\" does not exist! \taborting...\r\n\r\n";
   } # End if


   #---- Reads all the sample files URL from the considered dir. -----#
   print "portage_truecaselib::trainModelWithPoolOfFiles: Inventory of all the sample files from \"$corpusDir\"...\r\n" unless not $verbose;
   opendir(INPUTFILE, $corpusDir) or die ("\r\n\r\n!!! ERROR portage_truecaselib::trainModelWithPoolOfFiles: could not open \"$corpusDir\": $! \taborting...\r\n\r\n");
   my @dirFiles = grep !/(^\.\.?$)/, readdir(INPUTFILE);
   closedir(INPUTFILE);
   #build full filepaths
   my @tmpFiles = ();
   for(my $i=0; $i<@dirFiles; $i++)
   {  $dirFiles[$i] = "$corpusDir/$dirFiles[$i]";
      if(not -d $dirFiles[$i])
      {  push @tmpFiles, $dirFiles[$i];
      }else
      {  warn "\r\nWarning portage_truecaselib::trainModelWithPoolOfFiles: \"$dirFiles[$i]\" in the corpus is not a file! \tskipping...\r\n";
      } # End if
   } # End for
   @dirFiles = @tmpFiles;
   #print "Files: " . (join "\r\n", sort @dirFiles) . "\r\n";
   print "portage_truecaselib::trainModelWithPoolOfFiles: Inventory done...\r\n" unless not $verbose;

   if(@dirFiles > 0)
   {  my $inputFilename = getTemporaryFile();   # get a temporary file

      open (OUTPUTFILE, ">$inputFilename") or die ("\r\n\r\n!!! ERROR portage_truecaselib::trainModelWithPoolOfFiles: \"$inputFilename\" cannot be opened for output: $! \taborting...\r\n\r\n");
      flock OUTPUTFILE, 2;  # Lock

      my $strBuffer;
      my @lines;

      print "portage_truecaselib::trainModelWithPoolOfFiles: creating text pool of from all files...\r\n" unless not $verbose;
      foreach my $file (@dirFiles)
      {  open (INPUTFILE, $file) or die ("\r\n\r\n!!! ERROR portage_truecaselib::trainModelWithPoolOfFiles: could not open \"$file\": $! \taborting...\r\n\r\n");
         flock INPUTFILE, 2;  # Lock
         @lines = <INPUTFILE>;
         close (INPUTFILE);

         print OUTPUTFILE @lines;
      } # End foreach
      close (OUTPUTFILE);
      print "portage_truecaselib::trainModelWithPoolOfFiles: text pool done...\r\n" unless not $verbose;

#### debug
#   open (INPUTFILE, $inputFilename);
#   @lines = <INPUTFILE>;
#   close (INPUTFILE);
#   open (OUTPUTFILE, '>outTmpFile') or warn "\r\nWarning portage_truecaselib::cleanTextFile: could not open to map file \"outTmpFile\" for output: $!\r\n";
#   print OUTPUTFILE @lines;
#   print OUTPUTFILE "\r\n";
#   close (OUTPUTFILE);
#### End debug

      print "portage_truecaselib::trainModelWithPoolOfFiles: Training with pool...\r\n" unless not $verbose;
      trainFile($inputFilename, $LMFile, $vocabCountFile, $lmOrder, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleanMarkupFlag, $verbose);
      print "portage_truecaselib::trainModelWithPoolOfFiles: Training done...\r\n" unless not $verbose;

      # deleted the temp file
      system("rm -rf $inputFilename");

   } #End if

   print "portage_truecaselib::trainModelWithPoolOfFiles: completed successfully!\r\n" unless not $verbose;

} #End trainModelWithPoolOfFiles()




# ==================== trainFile =====================#

=head1 SUB

B< =====================================
 ============= trainFile =============
 =====================================>

=over 4

=item B<DESCRIPTION>

 It builds a NGram Language Model (LM) from the given training file.
 The LM result is output into the given LM_File. If the 3rd
 argument $vocabCountFile is defined, a vocabulary statistics
 (word counts) is generated into that file.

=item B<SYNOPSIS>

 portage_truecaselib::trainFile($inputFilename, $LMFile, $vocabCountFile,
                   $lmOrder, $cleanJunkLinesFlag, $excludeNISTTitlesFlag,
                   $cleanMarkupFlag, $verbose)

 @PARAM $inputFilename the training sample file. Required argument.
 @PARAM $LMFile the NGram LM file. Defined and valide argument is required.
 @PARAM $vocabCountFile the vocabulary statistics file. This argument
        can be undefined.
 @PARAM $lmOrder the order of the NGram to generate.
 @PARAM $cleanJunkLinesFlag if this flag is true, any line in the input file
        that matches portage_truecaselibconstantes::JUNKS_REG is removed from
        the files before training.
 @PARAM $excludeNISTTitlesFlag if this flag is true, the training input
        files are assumed to have the NIST04 format and titles are detected
        accordingly and removed before training.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

 @RETURN Void. However language models (NGram and Vocabulary statistics) are generated.

=item B<SEE ALSO>

 trainModelIncrementally(), trainModelWithPoolOfFiles(), trainFile(),
 trainDirectoryToModelsDirectory()

=back

=cut

# ----------------------------------------------------------------------------#
sub trainFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainFile requires 8 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::trainFile requires 8 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my ($inputFilename, $LMFile, $vocabCountFile, $lmOrder, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleanMarkupFlag, $verbose) = @_;

   if(not defined $inputFilename)
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::trainFile: a file is required for the train! \taborting...\r\n\r\n";
   } # End if

   my @tmpFile;
   if(defined $LMFile)
   {  # Get the parent directory and create it if not exist yet;
      @tmpFile = getPathAndFilenameFor($LMFile); # Get the parent directory and create it if not exist yet;
      if((defined $tmpFile[1]) and ($tmpFile[1] ne '') and not ($tmpFile[1] =~ /(\.\.?\/?$)/))
      {  system 'mkdir --parent ' . $tmpFile[1];
      } # End if
   } # End if

   if(defined $vocabCountFile)
   {  # Get the parent directory and create it if not exist yet;
      @tmpFile = getPathAndFilenameFor($vocabCountFile); # Get the parent directory and create it if not exist yet;
      if((defined $tmpFile[1]) and ($tmpFile[1] ne '') and not ($tmpFile[1] =~ /(\.\.?\/?$)/))
      {  system 'mkdir --parent ' . $tmpFile[1];
      } # End if
   } # End if

   @tmpFile = getPathAndFilenameFor($inputFilename); # Get the parent directory and create it if not exist yet;
   if((defined $tmpFile[1]) and ($tmpFile[1] ne '') and not ($tmpFile[1] =~ /(\.\.?\/?$)/))
   {  system 'mkdir --parent ' . $tmpFile[1];
   } # End if

   $lmOrder = portage_truecaselibconstantes::DEFAULT_NGRAM_ORDER unless defined $lmOrder;
   $cleanMarkupFlag = portage_truecaselibconstantes::REMOVE_MARKUPS_FLAG unless defined $cleanMarkupFlag;

   print "portage_truecaselib::trainFile: Training with \"$inputFilename\"...\r\n" unless not $verbose;

   my $cleanedInputTextFile = $inputFilename;
   my $cleaningResidusFile = undef;
   # ----- Load and clean the input corpus text sample by removing markers known as <****>
   if($excludeNISTTitlesFlag or $cleanMarkupFlag or $cleanJunkLinesFlag)
   {  $cleanedInputTextFile = getTemporaryFile();  # get a temporary file
      $cleaningResidusFile = getTemporaryFile();   # get a temporary file
      cleanTextFile($inputFilename, $cleanedInputTextFile, $cleanJunkLinesFlag, $excludeNISTTitlesFlag, $cleaningResidusFile);
   } # End if

#### debug
#   open (INPUTFILE, $inputFilename);
#   my @lines = <INPUTFILE>;
#   close (INPUTFILE);
#   open (OUTPUTFILE, '>outTmpFile') or warn "\r\nWarning portage_truecaselib::cleanTextFile: could not open to map file \"outTmpFile\" for output: $!\r\n";
#   print OUTPUTFILE @lines;
#   print OUTPUTFILE "\r\n";
#   close (OUTPUTFILE);
#
#   open (INPUTFILE, $cleanedInputTextFile);
#   @lines = <INPUTFILE>;
#   close (INPUTFILE);
#   open (OUTPUTFILE, '>outTmpFile1') or warn "\r\nWarning portage_truecaselib::cleanTextFile: could not open to map file \"outTmpFile\" for output: $!\r\n";
#   print OUTPUTFILE @lines;
#   print OUTPUTFILE "\r\n";
#   close (OUTPUTFILE);
#### End debug

   # Generate the models
   if(defined $LMFile)
   {  if(defined $vocabCountFile)
      {  system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile -lm $LMFile -write1 $vocabCountFile");
      }else
      {  system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile -lm $LMFile");
      } # End if
   }else
   {  if(defined $vocabCountFile)
      {  system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile -write1 $vocabCountFile");
      }else
      {  system("ngram-count -sort -order $lmOrder -text $cleanedInputTextFile");
      } # End if
   } # End if
   print "portage_truecaselib::trainFile: Training done...\r\n" unless not $verbose;


   # Generate the model for the NIST titles all concatenated together
   if($excludeNISTTitlesFlag)
   {  print "portage_truecaselib::trainFile: Training from NIST titles...\r\n" unless not $verbose;
      my $cleaningResidusLMFile = defined $LMFile ? $LMFile . portage_truecaselibconstantes::TITLE_LM_FILENAME_SUFFIX : undef;
      my $cleaningResidusVocFile = defined $vocabCountFile ? $vocabCountFile . portage_truecaselibconstantes::TITLE_VOCABULARY_COUNT_FILENAME_SUFFIX : undef;

      if(defined $cleaningResidusLMFile)
      {  if(defined $vocabCountFile)
         {  system("ngram-count -sort -order $lmOrder -text $cleaningResidusFile -lm $cleaningResidusLMFile -write1 $cleaningResidusVocFile");
         }else
         {  system("ngram-count -sort -order $lmOrder -text $cleaningResidusFile -lm $cleaningResidusLMFile");
         } # End if
      }else
      {  if(defined $cleaningResidusVocFile)
         {  system("ngram-count -sort -order $lmOrder -text $cleaningResidusFile -write1 $cleaningResidusVocFile");
         } # End if
      } # End if
      print "portage_truecaselib::trainFile: Titles training done...\r\n" unless not $verbose;
   } # End if

   system("rm -rf $cleanedInputTextFile") unless not $cleanMarkupFlag;   # delete temporary file
   system("rm -rf $cleaningResidusFile") unless not defined $cleaningResidusFile;   # delete temporary file

   print "portage_truecaselib::trainFile: completed successfully!\r\n" unless not $verbose;

} #End trainFile






###############################################################
############## TRUECASING (OR FILES CONVERSION) ###############
###############################################################


#============ truecaseDirectoryToDirectory ==============#

=head1 SUB

B< ========================================
 ===== truecaseDirectoryToDirectory =====
 ========================================>

=over 4

=item B<DESCRIPTION>

 Convert the files found in the given input directory into truecase form
 stored into the given output directory.

=item B<SYNOPSIS>

 portage_truecaselib::truecaseDirectoryToDirectory($inputDir, $outputDir, $LMFile,
          $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles,
          $lmOrder, $useLMOnlyFlag, $forceNISTTitlesFUFlag,
          $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM,
          $useViterbi);

 @PARAM $inputDir the input files directory.
 @PARAM $outputDir the output files directory.
 @PARAM $LMFile the Language Models filename or path. The file should
         have the ARPA ngram-format(5) format as output by SRILM
         "ngram-count -lm"
 @PARAM $vocabMapFile the V1 to V2 vocabulary mapping file. It should
         have the format required by SRILM "disambig -map XXX"
 @PARAM $unknownsMapFile the V1 to V2 unknown words classes mapping file. It should
         have the format required by SRILM "disambig -map XXX"
 @PARAM $LMFileTitles the NGram Language Models file for titles. The file should
         have the ARPA ngram-format(5) format as output by SRILM
         "ngram-count -lm". It can be undefined if not used.
 @PARAM $vocabMapFileTitles the vocabulary V1-to-V2 mapping model for titles.
         It should have the format required by SRILM "disambig -map XXX".
 @PARAM $lmOrder the effective N-gram order used by the language models.
 @PARAM $forceNISTTitlesFUFlag if this flag is true, the first letter of all
         words in titles are uppercased.
 @PARAM $uppercaseSentenceBeginFlag if this flag is true, the begining
         of all the sentence is uppercased.
 @PARAM $useLMOnlyFlag if true, only a given NGram model ($LMFile) will be
         use; the V1-to-V2 ($vocabMapFile) will be ignored.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.
 @PARAM $useSRILM if true, SRILM disambig will be used rather than canoe.
 @PARAM $useViterbi if true and using SRILM, use viterbi rather than forward-backward.

 @RETURN Void. However $truecaseOutputFile file is created.

=item B<SEE ALSO>

 truecaseFile(), truecaseDirectoryToFile()

=back

=cut

# --------------------------------------------------------------------------------#
sub truecaseDirectoryToDirectory
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::truecaseDirectoryToDirectory requires 14 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::truecaseDirectoryToDirectory requires 14 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($inputDir, $outputDir, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $lmOrder, $useLMOnlyFlag, $forceNISTTitlesFUFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi) = @_;

   if((not defined $inputDir) or (! -e $inputDir))
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::truecaseDirectoryToDirectory: the provided samples' directory \"$inputDir\" does not exist! \taborting...\r\n\r\n";
   } # End if

   print "portage_truecaselib::truecaseDirectoryToDirectory: Inventory of all the sample files from \"$inputDir\"...\r\n" unless not $verbose;

   #---- Reads all the sample files URL from the considered dir. -----#
   opendir(INPUTFILE, $inputDir) or die ("\r\n\r\n!!! ERROR portage_truecaselib::truecaseDirectoryToDirectory: could not open \"$inputDir\": $! \taborting...\r\n\r\n");
   my @dirFiles = grep !/(^\.\.?$)/, readdir(INPUTFILE);
   closedir(INPUTFILE);
   #build full filepaths
   my @tmpFiles = ();
   for(my $i=0; $i<@dirFiles; $i++)
   {  $dirFiles[$i] = "$inputDir/$dirFiles[$i]";
      if(not -d $dirFiles[$i])
      {  push @tmpFiles, $dirFiles[$i];
      }else
      {  warn "\r\nWarning portage_truecaselib::truecaseDirectoryToDirectory: \"$dirFiles[$i]\" in the corpus is not a file! \tskipping...\r\n";
      } # End if
   } # End for
   @dirFiles = @tmpFiles;
   #print "Files: " . (join "\r\n", sort @dirFiles) . "\r\n";

   print "portage_truecaselib::truecaseDirectoryToDirectory: Inventory done !\r\n" unless not $verbose;

   ########################################################
   ## Generate the outputs into that directory with ######
   ## portage_truecaselibconstantes::DEFAULT_TRUECASED_FILENAME_SUFFIX
   ## appended to the input files' name ##
   ########################################################

   if(defined $outputDir)
   {  #------------- Truecasing files into temporary files and then cat them into output -----------#
      my $outTmpFile;
      print "portage_truecaselib::truecaseDirectoryToDirectory: Truecasing...\r\n" unless not $verbose;
      foreach my $inputFile (@dirFiles)
      {  print "portage_truecaselib::truecaseDirectoryToDirectory: Processing \"$inputFile\"...\r\n" unless not $verbose;
         my @path = getPathAndFilenameFor($inputFile);
         $outTmpFile = "$outputDir/$path[0]" . portage_truecaselibconstantes::DEFAULT_TRUECASED_FILENAME_SUFFIX;

         #----- Truecase ---
         print "portage_truecaselib::truecaseDirectoryToDirectory: Truecasing ...\r\n" unless not $verbose;
         advancedTruecaseFile($inputFile, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $outTmpFile, $lmOrder, $forceNISTTitlesFUFlag, $useLMOnlyFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi);
         print "portage_truecaselib::truecaseDirectoryToDirectory: Truecasing done...\r\n" unless not $verbose;

         print "portage_truecaselib::truecaseDirectoryToDirectory: Processing done...\r\n" unless not $verbose;
      } # End foreach
      print "portage_truecaselib::truecaseDirectoryToDirectory: Truecasing each file done...\r\n" unless not $verbose;

   ########## To standard output #########
   }else # No outputDir
   {  print "portage_truecaselib::truecaseDirectoryToDirectory: Directory files truecasing to standard output requested...\r\n" unless not $verbose;
      truecaseDirectoryToFile($inputDir, undef, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $lmOrder, $useLMOnlyFlag, $forceNISTTitlesFUFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose);
      print "portage_truecaselib::truecaseDirectoryToDirectory: Directory files truecasing to standard output done...\r\n" unless not $verbose;
   } # End if

   print "portage_truecaselib::truecaseDirectoryToDirectory: completed successfully!\r\n" unless not $verbose;

} # End truecaseDirectoryToDirectory




#============ truecaseDirectoryToFile ==============#

=head1 SUB

B< ======================================
 ======= truecaseDirectoryToFile ======
 ======================================>

=over 4

=item B<DESCRIPTION>

 Convert the files found in the given input directory into truecase form
 stored into the given output directory.

=item B<SYNOPSIS>

 portage_truecaselib::truecaseDirectoryToFile($inputDir, $outputFile, $LMFile,
          $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles,
          $lmOrder, $forceNISTTitlesFUFlag, $uppercaseSentenceBeginFlag,
          $useLMOnlyFlag, $cleanMarkupFlag, $verbose);

 @PARAM $inputDir the reference to the input files directory.
 @PARAM $outputFile the file into which the truecase results should be
         concatenated.
 @PARAM $LMFile the Language Models filename or path. The file should
         have the ARPA ngram-format(5) format as output by SRILM
         "ngram-count -lm"
 @PARAM $vocabMapFile the V1 to V2 vocabulary mapping file. It should
         have the format required by SRILM "disambig -map XXX"
 @PARAM $unknownsMapFile the V1 to V2 unknown words classes mapping file. It should
         have the format required by SRILM "disambig -map XXX"
 @PARAM $LMFileTitles the NGram Language Models file for titles. The file should
         have the ARPA ngram-format(5) format as output by SRILM
         "ngram-count -lm". It can be undefined if not used.
 @PARAM $vocabMapFileTitles the vocabulary V1-to-V2 mapping model for titles.
         It should have the format required by SRILM "disambig -map XXX".
 @PARAM $lmOrder the effective N-gram order used by the language models.
 @PARAM $forceNISTTitlesFUFlag if this flag is true, the first letter of all
         words in titles are uppercased.
 @PARAM $uppercaseSentenceBeginFlag if this flag is true, the begining
         of all the sentence is uppercased.
 @PARAM $useLMOnlyFlag if true, only a given NGram model ($LMFile) will be
         use; the V1-to-V2 ($vocabMapFile) will be ignored.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

 @RETURN Void. However truecase output File file is created.

=item B<SEE ALSO>

 truecaseFile()

=back

=cut

# --------------------------------------------------------------------------------#
sub truecaseDirectoryToFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::truecaseDirectoryToFile requires 14 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::truecaseDirectoryToFile requires 14 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($inputDir, $outputFile, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $lmOrder, $useLMOnlyFlag, $forceNISTTitlesFUFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi) = @_;

   if((not defined $inputDir) or (! -e $inputDir))
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::truecaseDirectoryToFile: the provided samples' directory \"$inputDir\" does not exist! \taborting...\r\n\r\n";
   } # End if

   #---- Reads all the sample files URL from the considered dir. -----#

   print "portage_truecaselib::truecaseDirectoryToFile: Inventory of all the sample files from \"$inputDir\"...\r\n" unless not $verbose;

   opendir(INPUTFILE, $inputDir) or die ("\r\n\r\n!!! ERROR portage_truecaselib::truecaseDirectoryToFile: could not open \"$inputDir\": $! \taborting...\r\n\r\n");
   my @dirFiles = grep !/(^\.\.?$)/, readdir(INPUTFILE);
   closedir(INPUTFILE);
   #build full filepaths
   my @tmpFiles = ();
   for(my $i=0; $i<@dirFiles; $i++)
   {  $dirFiles[$i] = "$inputDir/$dirFiles[$i]";
      if(not -d $dirFiles[$i])
      {  push @tmpFiles, $dirFiles[$i];
      }else
      {  warn "\r\nWarning portage_truecaselib::truecaseDirectoryToFile: \"$dirFiles[$i]\" in the corpus is not a file! \tskipping...\r\n";
      } # End if
   } # End for
   @dirFiles = @tmpFiles;
   #print "Files: " . (join "\r\n", sort @dirFiles) . "\r\n";
   print "portage_truecaselib::truecaseDirectoryToFile: Inventory done...\r\n" unless not $verbose;

   # Generate the outputs into temporary files
   # and concatenate them after into the sent in outputFile file
   @tmpFiles = ();    # Temporary files collection
   my $outTmpFile;

   #------------- Truecasing files into temporary files and then cat them into output -----------#
   print "portage_truecaselib::truecaseDirectoryToFile: Truecasing...\r\n" unless not $verbose;
   foreach my $inputFile (@dirFiles)
   {  print "portage_truecaselib::truecaseDirectoryToFile: Processing \"$inputFile\"...\r\n" unless not $verbose;
      $outTmpFile = getTemporaryFile();   # get a temporary file
      push @tmpFiles, $outTmpFile;  # to be automatically deleted when we exit or die
      
      #----- Truecase ---
      print "portage_truecaselib::truecaseDirectoryToFile: Truecasing ...\r\n" unless not $verbose;
      advancedTruecaseFile($inputFile, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $outTmpFile, $lmOrder, $forceNISTTitlesFUFlag, $useLMOnlyFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi);
      print "portage_truecaselib::truecaseDirectoryToFile: Truecasing done...\r\n" unless not $verbose;

      print "portage_truecaselib::truecaseDirectoryToFile: Processing done...r\n" unless not $verbose;
   } # End foreach
   print "portage_truecaselib::truecaseDirectoryToFile: Truecasing each file done...\r\n" unless not $verbose;


   #------ Concatenate all the tempfiles into the given file ------
   print "portage_truecaselib::truecaseDirectoryToFile: Finalizing the output...\r\n" unless not $verbose;
   if(@tmpFiles > 1)
   {  my @disambigLines;
      if(defined $outputFile)
      {  open (OUTPUTFILE, ">$outputFile") or die ("\r\n\r\n!!! ERROR portage_truecaselib::truecaseDirectoryToFile: could not open \"$outputFile\" for the truecase output! \taborting...\r\n\r\n");
         flock OUTPUTFILE, 2;  # Lock
         foreach my $file (@tmpFiles)
         {  open (INPUTFILE, $file) or warn "\r\nWarning portage_truecaselib::truecaseDirectoryToFile: could not open \"$file\" for reading... lost of data!\r\n";
            flock INPUTFILE, 2;  # Lock
            @disambigLines = <INPUTFILE>;
            close (INPUTFILE);
            print OUTPUTFILE @disambigLines;
            system('rm -f ' . $file);  # delete the temporary file
         } # End foreach
      }else    # Standard output
      {  foreach my $file (@tmpFiles)
         {  open (INPUTFILE, $file) or warn "\r\nWarning portage_truecaselib::truecaseDirectoryToFile: could not open \"$file\" for reading... lost of data!\r\n";
            flock INPUTFILE, 2;  # Lock
            @disambigLines = <INPUTFILE>;
            close (INPUTFILE);
            print @disambigLines;
            system('rm -f ' . $file);  # delete the temporary file
         } # End foreach
      } # End if
   } # End if
   print "portage_truecaselib::truecaseDirectoryToFile: Finalizing done...\r\n" unless not $verbose;

   print "portage_truecaselib::truecaseDirectoryToFile: completed successfully!\r\n" unless not $verbose;

   # delete the temporary files JUST REDUNDANCY
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system("rm -rf $file");
      } # End if
   } # End foreach

} # End truecaseDirectoryToFile



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
          $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose,
          $useSRILM, $useViterbi);

 @PARAM $inputFile the file to be converted into truecase.
 @PARAM $LMFile the NGram Language Models file. The file should
    have the ARPA ngram-format(5) format as output by SRILM
    "ngram-count -lm". It can be undefined if not used.
 @PARAM $vocabMapFile the vocabulary V1-to-V2 mapping model.
    It should have the format required by SRILM "disambig -map XXX".
 @PARAM $unknownsMapFile the V1 to V2 unknown words classes mapping file. It should
    have the format required by SRILM "disambig -map XXX"
 @PARAM $LMFileTitles the NGram Language Models file for titles. The file should
    have the ARPA ngram-format(5) format as output by SRILM
    "ngram-count -lm". It can be undefined if not used.
 @PARAM $vocabMapFileTitles the vocabulary V1-to-V2 mapping model for titles.
    It should have the format required by SRILM "disambig -map XXX".
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
 @PARAM $useSRILM if true, SRILM disambig will be used rather than canoe.
 @PARAM $useViterbi if true and using SRILM, use viterbi rather than forward-backward.

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
   my($inputFile, $LMFile, $vocabMapFile, $unknownsMapFile, $LMFileTitles, $vocabMapFileTitles, $outputFile, $lmOrder, $forceNISTTitlesFUFlag, $useLMOnlyFlag, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi) = @_;

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
      truecaseFile($tmpInputFile, $LMFile, $extendedMapFile, $lmOrder, undef, $outTmpFile, $uppercaseSentenceBeginFlag, undef, $verbose, $useSRILM, $useViterbi);
      print "portage_truecaselib::advancedTruecaseFile: Truecasing done...\r\n" unless not $verbose;

      #----5- Recover the unknown classes
      print "portage_truecaselib::advancedTruecaseFile: Recovering the unknown words from classes...\r\n" unless not $verbose;
      recoverUnkownWordsFromClasses($outTmpFile, \%unknownWordsReplacementTrack, $outTmpFile);
      print "portage_truecaselib::advancedTruecaseFile: Recovering done...\r\n" unless not $verbose;

   ############## CASE OF NO UNKNOW WORDS CLASSES RESOLUTION ###########
   }else
   {  #----1- ------- Don't resolve Unknown classes -----------#
      print "portage_truecaselib::advancedTruecaseFile: Truecasing ...\r\n" unless not $verbose;
      truecaseFile($inputFile, $LMFile, $vocabMapFile, $lmOrder, $useLMOnlyFlag, $outTmpFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi);
      print "portage_truecaselib::advancedTruecaseFile: Truecasing done...\r\n" unless not $verbose;
   } # End if

   ############## CASE OF TITLES PROCESSING ###########
   if($forceNISTTitlesFUFlag or defined $LMFileTitles or defined $vocabMapFileTitles)
   {  print "portage_truecaselib::advancedTruecaseFile: Processing titles...\r\n" unless not $verbose;
      specialNISTTitleProcessing($outTmpFile, $LMFileTitles, $vocabMapFileTitles, $forceNISTTitlesFUFlag, $lmOrder, $useLMOnlyFlag, $outTmpFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi);

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
   $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi);
 
 @PARAM $inputFile the file to be converted into truecase.
 @PARAM $LMFile the NGram Language Models file. The file should
    have the ARPA ngram-format(5) format as output by SRILM
    "ngram-count -lm". It can be undefined if not used.
 @PARAM $vocabMapFile the vocabulary V1-to-V2 mapping model.
      It should have the format required by SRILM "disambig -map XXX".
      It's requiered.
 @PARAM $lmOrder the effective N-gram order used by the language models.
 @PARAM $outputFile the file into which the truecase result should be written.
 @PARAM $uppercaseSentenceBeginFlag if this flag is true, the begining
    of all the sentence is uppercased.
 @PARAM $useLMOnlyFlag if true, only a given NGram model ($LMFile) will be
         use; the V1-to-V2 ($vocabMapFile) will be ignored.
 @PARAM $cleanMarkupFlag if true, all the markup blocs are removed from
         the files before processing.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.
 @PARAM $useSRILM if true, SRILM disambig will be used rather than canoe.
 @PARAM $useViterbi if true and using SRILM, use viterbi rather than forward-backward.

 @RETURN Void. However the truecase is generated into $outputFile
    if it's given or on the standard output.

=item B<SEE ALSO>

    truecaseDirectoryToDirectory, truecaseDirectoryToFile

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
   my($inputFile, $LMFile, $vocabMapFile, $lmOrder, $useLMOnlyFlag, $outputFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi) = @_;

   if(not defined $inputFile)
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::truecaseFile: a file to convert into truecase is required! \taborting...\r\n\r\n";
   } # End if

   $uppercaseSentenceBeginFlag = portage_truecaselibconstantes::ENSURE_UPPERCASE_SENTENCE_BEGIN unless defined $uppercaseSentenceBeginFlag;
   $cleanMarkupFlag = portage_truecaselibconstantes::REMOVE_MARKUPS_FLAG unless defined $cleanMarkupFlag;
   $lmOrder = portage_truecaselibconstantes::DEFAULT_NGRAM_ORDER unless defined $lmOrder;

   my @tmpFiles = ();     # Temporary files collection

   print "portage_truecaselib::truecaseFile: Truecasing \"$inputFile\"...\r\n" unless not $verbose;
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


   if ( $useSRILM ) {

      system( "disambig -text $cleanedInputTextFile "
            . (defined $lmOrder ? "-order $lmOrder " : '')
            . (defined $LMFile ? "-lm $LMFile " : '')
            . ((defined $vocabMapFile and (not $useLMOnlyFlag)) ? "-map $vocabMapFile " : '')
            . "-keep-unk "
            . ($useViterbi ? "" : "-fb ")
            . "| perl -n -e 's/^<s>\\s*//o; s/\\s*<\\/s>[ ]*//o;"
            . ($uppercaseSentenceBeginFlag ? " print ucfirst;'" : " print;'")
            . (defined $outputFile ? " > $outputFile" : ''));

   }
   else {
      if ( ! defined $vocabMapFile ) {
         print STDERR "Error: A map file must be provided\n";
         exit( 1 );
      }
      if ( ! defined $LMFile ) {
         print STDERR "Error: An LM file must be provided\n";
         exit( 1 );
      }

      open( MAP, "<", $vocabMapFile );
      open( TM,  ">", "canoe_tc_tmp_$$.tm" );
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

      open( INI,  ">", "canoe_tc_tmp_$$.ini" );
         print INI "[ttable-multi-prob] canoe_tc_tmp_$$.tm\n";
         print INI "[lmodel-file]       $LMFile\n";
      if ( defined $lmOrder ) {
         print INI "[lmodel-order]      $lmOrder\n";
      }
         print INI "[ttable-limit]      100\n";
         print INI "[stack]             100\n";
         print INI "[ftm]               1.0\n";
         print INI "[lm]                2.302585\n";
         print INI "[tm]                0.0\n";
         print INI "[distortion-limit]  0\n";
         print INI "[load-first]\n";
      close( INI );

      system(
          'canoe-escapes.pl -add < ' . $cleanedInputTextFile
        . "| canoe -f canoe_tc_tmp_$$.ini "
        . "| perl -n -e 's/^<s>\\s*//o; s/\\s*<\\/s>[ ]*//o;"
        .   ( $uppercaseSentenceBeginFlag ? " print ucfirst;'" : " print;'" )
        .   ( defined $outputFile ? " > $outputFile" : '' )
      );

      system( "rm -rf canoe_tc_tmp_$$.ini canoe_tc_tmp_$$.tm" );
   }

   print "portage_truecaselib::truecaseFile: Truecasing done...\r\n" unless not $verbose;

   # delete the temporary files
   if($cleanMarkupFlag)
   {  system("rm -rf $cleanedInputTextFile");
   } # End foreach

   print "portage_truecaselib::truecaseFile: completed successfully!\r\n" unless not $verbose;

   # delete the temporary files
   foreach my $file (@tmpFiles)
   {  if($file)
      {  system("rm -rf $file");
      } # End if
   } # End foreach

} # End truecaseFile





#####################################################################
##################### ERROR EVALUATION ##############################
#####################################################################

#======= computeTrueCasingErrorsBetween =======#

=head1 NAME

B< ========================================
 ==== computeTrueCasingErrorsBetween ====
 ========================================>

=head1 DESCRIPTION

 Computes word form errors between $truecasedFile and $originalFile.
 It returns a hash table of forms for each sentence (line) in
 $truecasedFile.

=head1 SYNOPSIS

%errorLogData = portage_truecaselib::computeTrueCasingErrorsBetween($truecasedFile,
                $originalFile, $evalResultFile, $errorTaggedOutputFile)

 @PARAM $truecasedFile the input file to be compared to $originalFile.
 @PARAM $originalFile the original truecase file to be used as reference.
 @PARAM $evalResultFile a file where to log out the results of the error.
 @PARAM $errorTaggedOutputFile a file where to write out truecasing
           results but with all errored words tagged with the form of
           error that occured on them.
 @PARAM $verbose if true, progress information is printed to the standard ouptut.

 @RETURN a hash table %errorLogData of error rates for.
    %errorLogData has the following format:
    {  ERRORED_WORDS_HASHTABLE => hashtable,
       TOTAL_ERROR_COUNT => x,
       FU_FORM_ERROR_COUNT => x,
       AU_FORM_ERROR_COUNT => x,
       AL_FORM_ERROR_COUNT => x,
       MC_FORM_ERROR_COUNT => x,
       UNK_FORM_ERROR_COUNT => x
    }

=head1 SEE ALSO

=cut

# --------------------------------------------------------------------------------#
sub computeTrueCasingErrorsBetween
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::computeTrueCasingErrorsBetween requires 4 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::computeTrueCasingErrorsBetween requires 4 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my ($truecasedFile, $originalFile, $evalResultFile, $errorTaggedOutputFile, $verbose) = @_;

   # print join "\n", @_;

   #######---- Args in checkup: check existance of functional files -----#######
   if((not defined $truecasedFile) or (!-e $truecasedFile))
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::computeTrueCasingErrorsBetween: provide a valid input file for the error evaluation. The file \"$truecasedFile\" is not valide! \taborting...\r\n\r\n";
   } # end if
   if((not defined $originalFile) or (!-e $originalFile))
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::computeTrueCasingErrorsBetween: provide a valid reference file (original) for the error evaluation. The file \"$originalFile\" is not valide! \taborting...\r\n\r\n";
   } # end if

   # --------------------------------------------------------------------------------#
   # Build a hash table of LM read from the trained LM file.
   # %hashNGrams is a hash (lower case sequence entry)
   #                of hashes (different surface forms)
   #                of hash (data as {PROBABILITY=>x, BACKOFF=>y})
   # --------------------------------------------------------------------------------#

   print "portage_truecaselib::computeTrueCasingErrorsBetween: Matching words from \"$truecasedFile\" to \"$originalFile\"...\r\n" unless not $verbose;

   #--- Input file ---
   open (INPUTFILE, $truecasedFile) or die ("\r\n\r\n!!! ERROR portage_truecaselib::computeTrueCasingErrorsBetween: could not open \"$truecasedFile\": $! \taborting...\r\n\r\n");
   flock INPUTFILE, 2;  # Lock
   my @lines = <INPUTFILE>;
   @lines = grep !/^\s+$/, @lines;
   close(INPUTFILE);
   #--- Reference file ---
   open (INPUTFILE, $originalFile) or die ("\r\n\r\n!!! ERROR portage_truecaselib::computeTrueCasingErrorsBetween: could not open \"$originalFile\": $! \taborting...\r\n\r\n");
   flock INPUTFILE, 2;  # Lock
   my @refLines = <INPUTFILE>;
   @refLines = grep !/^\s+$/, @refLines;
   close(INPUTFILE);

   #--- Warn if the 2 files are differents in number of lignes ----#
   if(@lines != @refLines)
   {  warn "\r\nWarning portage_truecaselib::computeTrueCasingErrorsBetween: The input file \"$truecasedFile\" has a different LIGNE NUMBER than \"$originalFile\"! The matching might be owfully wrong!!!!\tProceeding nonetheless...\r\n";
   } # End if

   if(defined $errorTaggedOutputFile)
   {  open (ERROR_TAGGED_OUTPUTFILE_REF, ">$errorTaggedOutputFile") or die ("\r\n\r\n!!! ERROR portage_truecaselib::computeTrueCasingErrorsBetween: could not open \"$errorTaggedOutputFile\": $! \taborting...\r\n\r\n");
      flock ERROR_TAGGED_OUTPUTFILE_REF, 2;  # Lock
   } # End if

   my %errorLogData;
   my %errorSummary;
   my %sentenceSurfaceForms;
   my (@words, @origWords);
   my $totalWordsCount = 0;
   my $totalErrorCount = 0;
   my $FUformErrorCount= 0;
   my $AUformErrorCount= 0;
   my $ALformErrorCount= 0;
   my $MCformErrorCount = 0;
   my $UNKformErrorCount= 0;

   for(my $i=0; $i<@lines; $i++)
   {  # Remove all trailing whitespaces
      $lines[$i] =~ s/\s{2,}/ /go;
      $refLines[$i] =~ s/\s{2,}/ /go;
      # Break into tokens
      my @words = split /\s/, $lines[$i];
      my @origWords = split /\s/, $refLines[$i];
#      print "\r\nOOWW= " . (join '|', @origWords) . "\r\n";
#      print "\r\nWW= " . (join '|', @words) . "\r\n";
      if(@words != @origWords)
      {  #problem still persists
         warn "\r\nWarning portage_truecaselib::computeTrueCasingErrorsBetween:Invalide line:TestFilename=$truecasedFile#$originalFile#\r\nline=\t$lines[$i]\r\nrefLine=\t$refLines[$i]\r\n";
         next;
      } #End if

      my $errorTaggedOutputBuf = '' unless not defined $errorTaggedOutputFile;
      for(my $j=0; $j < @words; $j++)
      {  #print "\t#$words[$j] | $origWords[$j]#\r\n";

         ####------- Trace all error forms -----####
         if($words[$j] ne $origWords[$j])
         {  if(!exists $errorSummary{$words[$j]})
            {  $errorSummary{$words[$j]}{AU} = 0;
               $errorSummary{$words[$j]}{FU} = 0;
               $errorSummary{$words[$j]}{AL} = 0;
               $errorSummary{$words[$j]}{MC} = 0;
               $errorSummary{$words[$j]}{UNK} = 0;
               $errorSummary{$words[$j]}{ERRTAG} = '';
               # print ("\terrorSummary words[$j]=" . $words[$j] . "#j=$j#\r\n") unless $words[$j] ne '';
            } # End if

            ####------- Record all error forms -----####
            if(ucfirst lc $words[$j] eq $origWords[$j])
            {  $FUformErrorCount++;
               $errorSummary{$words[$j]}{FU} += 1;
               $errorSummary{$words[$j]}{ERRTAG} .= '_FU';
               $errorTaggedOutputBuf .= ' ' . $words[$j] . '_FU' unless not defined $errorTaggedOutputFile;
            ####------- Trace all Uppercase error forms -----####
            }elsif(uc $words[$j] eq $origWords[$j])
            {  $AUformErrorCount++;
               $errorSummary{$words[$j]}{AU} += 1;
               $errorSummary{$words[$j]}{ERRTAG} .= '_AU';
               $errorTaggedOutputBuf .= ' ' . $words[$j] . '_AU' unless not defined $errorTaggedOutputFile;
            ####------- Trace all lowercase error forms -----####
            }elsif(lc $words[$j] eq $origWords[$j])
            {  $ALformErrorCount++;
               $errorSummary{$words[$j]}{AL} += 1;
               $errorSummary{$words[$j]}{ERRTAG} .= '_AL';
               $errorTaggedOutputBuf .= ' ' . $words[$j] . '_AL' unless not defined $errorTaggedOutputFile;
            ####------- Trace all Mixedcase error forms -----####
            }elsif(lc $words[$j] eq lc $origWords[$j])
            {  $MCformErrorCount++;
               $errorSummary{$words[$j]}{MC} += 1;
               $errorSummary{$words[$j]}{ERRTAG} .= '_MC';
               $errorTaggedOutputBuf .= ' ' . $words[$j] . '_MC' unless not defined $errorTaggedOutputFile;
            ####------- Trace all Unknown error forms -----####
            }else # if(lc $words[$j] ne lc $origWords[$j])
            {  $UNKformErrorCount++;
               $errorSummary{$words[$j]}{UNK} += 1;
               $errorSummary{$words[$j]}{ERRTAG} .= '_UNK';
               $errorTaggedOutputBuf .= ' ' . $words[$j] . '_UNK' unless not defined $errorTaggedOutputFile;
            } #End if

            $totalErrorCount++;
         }elsif(defined $errorTaggedOutputFile)
         {  $errorTaggedOutputBuf .= ' ' . $words[$j];
         } #End if
      } #End foreach

      if(defined $errorTaggedOutputFile)
      {  $errorTaggedOutputBuf =~ s/^ //;
         print ERROR_TAGGED_OUTPUTFILE_REF "$errorTaggedOutputBuf\r\n";
      } # End if

      $totalWordsCount += @words;

   } # End for

   #print "totalWordsCount=$totalWordsCount\ttotalErrorCount=$totalErrorCount\r\n";
   if($totalWordsCount > 0)
   {  $errorLogData{ERRORED_WORDS_HASHTABLE} = \%errorSummary;
      $errorLogData{TOTAL_ERROR_COUNT} = $totalErrorCount;
      $errorLogData{AU_FORM_ERROR_COUNT} = $AUformErrorCount;
      $errorLogData{FU_FORM_ERROR_COUNT} = $FUformErrorCount;
      $errorLogData{AL_FORM_ERROR_COUNT} = $ALformErrorCount;
      $errorLogData{MC_FORM_ERROR_COUNT} = $MCformErrorCount;
      $errorLogData{UNK_FORM_ERROR_COUNT} = $UNKformErrorCount;
      $errorLogData{TOTAL_ERROR_COUNT} = $totalErrorCount;
      $errorLogData{TOTAL_WORDS_COUNT} = $totalWordsCount;
   } #End if

   print "portage_truecaselib::computeTrueCasingErrorsBetween: Matching words done...\r\n" unless not $verbose;


   ################# Debug ######################
   if(defined $evalResultFile)
   {  print "portage_truecaselib::computeTrueCasingErrorsBetween: loggin out the results...\r\n" unless not $verbose;

      open (OUTPUTFILE, ">$evalResultFile") or die ("\r\n\r\n!!! ERROR portage_truecaselib::computeTrueCasingErrorsBetween: could not write into \"$evalResultFile\": $! \taborting...\r\n\r\n");
      flock OUTPUTFILE, 2;  # Lock

      print OUTPUTFILE "\r\n#----------- Error statistics --------------\r\n";

      print OUTPUTFILE ("File:$truecasedFile\tTOTAL_ERROR=" . $errorLogData{TOTAL_ERROR_COUNT} . "\tTOTAL_WORDS_COUNT=" . $errorLogData{TOTAL_WORDS_COUNT}) . "\r\n"
                     . "\tAU FORM ERROR=" . ($errorLogData{AU_FORM_ERROR_COUNT} / $errorLogData{TOTAL_WORDS_COUNT})
                     . "\tFU FORM ERROR=" . ($errorLogData{FU_FORM_ERROR_COUNT} / $errorLogData{TOTAL_WORDS_COUNT})
                     . "\tMC FORM ERROR=" . ($errorLogData{MC_FORM_ERROR_COUNT} / $errorLogData{TOTAL_WORDS_COUNT})
                     . "\tAL FORM ERROR=" . ($errorLogData{AL_FORM_ERROR_COUNT} / $errorLogData{TOTAL_WORDS_COUNT})
                     . "\tUNK FORM ERROR=" . ($errorLogData{UNK_FORM_ERROR_COUNT} / $errorLogData{TOTAL_WORDS_COUNT}) . "\r\n\r\n";

      print OUTPUTFILE "\r\n#----------- Errored Words --------------\r\n";
      print OUTPUTFILE "\r\n\tERRORED_WORD\tAU\tFU\tMC\tAL\tUNK\r\n";

      print OUTPUTFILE "\r\n#----------- Error Words count statistics --------------\r\n";

      my %errorSummary = %{$errorLogData{ERRORED_WORDS_HASHTABLE}};
      foreach my $erroredWord (sort keys %errorSummary)
      {
         print OUTPUTFILE "\t$erroredWord\t" . ($errorSummary{$erroredWord}{AU} / $errorLogData{TOTAL_WORDS_COUNT})
                  . "\t" . ($errorSummary{$erroredWord}{FU} / $errorLogData{TOTAL_WORDS_COUNT})
                  . "\t" . ($errorSummary{$erroredWord}{MC}  / $errorLogData{TOTAL_WORDS_COUNT})
                  . "\t" . ($errorSummary{$erroredWord}{AL} / $errorLogData{TOTAL_WORDS_COUNT})
                  . "\t" . ($errorSummary{$erroredWord}{UNK}  / $errorLogData{TOTAL_WORDS_COUNT})
                  . "\r\n";
      } # End foreach

      print OUTPUTFILE "\r\n#--------- End Errored Words ------------\r\n";

      print "portage_truecaselib::computeTrueCasingErrorsBetween: loggin done...\r\n" unless not $verbose;

   } #End if

   print "portage_truecaselib::computeTrueCasingErrorsBetween: completed successfully!\r\n" unless not $verbose;

   return %errorLogData;

} # End computeTrueCasingErrorsBetween





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
        by SRILM ngram-count -write1

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



=head1 SUB

B< ====================================
 ====== extractVocabMapFromFile =====
 ====================================>

=over 4

=item B<DESCRIPTION>

 Reads a given V1 to V2 vocabulary mapping file into a hash table.

=item B<SYNOPSIS>

 %vocabWordsData = portage_truecaselib::extractVocabMapFromFile($vocabMapFilename)

 @PARAM $vocabMapFilename the V1 to V2 vocabulary mapping file in the format
            similar to the follwing example.
       Example:
         barry barry 25 Barry 2 BARRY 1
         bas-richelieu Bas-Richelieu 1
         base base 5000 Base 727
         ...

 @RETURN %vocabWordsHash a hash of hash of the vocabulary words as follow:
        hash of lower case entry of words of hash of different
        surface forms of words with value data as FREQUENCY.
 %vocabWordsHash= { barry => {"barry" => 25, "Barry" => 2, "BARRY" => 1},
                    base => {"base" => 5000, "Base" => 727},
                    ...
                  }

=back

=cut

# --------------------------------------------------------------------------------#
sub extractVocabMapFromFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::extractVocabMapFromFile requires an input argument! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::extractVocabMapFromFile requires an input argument! \taborting...\r\n\r\n";
      } # End if
   } # End if
   my($vocabMapFilename) = @_;

   my(%vocabWordsMapHash, $line, @data);

   open (INPUTFILE, $vocabMapFilename) or die "\r\n\r\n!!! ERROR portage_truecaselib::extractVocabMapFromFile: could not open \"$vocabMapFilename\" for input: $! \taborting...\r\n\r\n";
   flock INPUTFILE, 2;  # Lock
   #my @lines = grep !/^[ \t\r\n\f]+|[ \t\r\n\f]+$/, <INPUTFILE>;
   my @lines = <INPUTFILE>;
   close(INPUTFILE);

   foreach $line (@lines)
   {  @data = split /\s+/, $line;
      chomp @data;

      if(@data > 1)
      {  for(my $i=1; $i<@data; $i += 2)
         {  $vocabWordsMapHash{lc $data[0]}{$data[$i]} = $data[$i+1];
         } # End for
         # print "\tINNN--- $data[0] # $vocabWordsMapHash{$data[0]} #\r\n";
      }elsif(@data == 1)
      {  warn "\r\nWarning portage_truecaselib::extractVocabMapFromFile: the data has a wrong format in \"$vocabMapFilename\"! \tskipping... ...\r\n";
      } # End if
   } #end foreach

   return %vocabWordsMapHash;

} # End of extractVocabMapFromFile()



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
 (count) file. The vocabulary count file should have the ARPA ngram-format(5)
 format as output by SRILM "ngram-count -lm". The mapping has the format
 required by SRILM "disambig -map XXX".

=item B<SYNOPSIS>

 portage_truecaselib::writeVocabularyMappingFileForVocabularymy($vocabularyCountFilename,
                                 $vocabularyMappingFilename, $onesAllFormProbabilityFlag)

 @PARAM $vocabularyCountFilename the vocabulary filename in the format output
       by SRILM ngram-count -write1
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
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::writeVocabularyMappingFileForVocabulary: vocabulary Count File is required and should have the format output by SRILM \"ngram-count -write1\". The file \"$vocabularyCountFilename\" is invalide! \taborting...\r\n\r\n";
   } # End if

   if(!defined $vocabularyMappingFilename)
   {  die "\r\n\r\n!!! ERROR portage_truecaselib::writeVocabularyMappingFileForVocabulary: a valide name must be provided for the vocabulary V1-to-V2 mapping information file. The file \"$vocabularyCountFilename\" is invalide! \taborting...\r\n\r\n";
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
 ==== getParentDirAndFilename ====
 =================================>

=over 4

=item B<DESCRIPTION>

 Gets the filename and the parent path of a given file.

=item B<SYNOPSIS>

 @path = portage_truecaselib::getParentDirAndFilename($file)

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
} # End getParentDirAndFilename




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
#        by SRILM ngram-count -write1
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
# @PARAM $vocabularyMappingFilename the name or path of the mapping file as required
#        by SRILM disambig -map
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
            $outputFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose,
            $useSRILM, $useViterbi);

 @PARAM $inputFile the file whose eventual titles to be converted into truecase.
 @PARAM $LMFileTitles the NGram Language Models file for titles. The file should
    have the ARPA ngram-format(5) format as output by SRILM
    "ngram-count -lm". It can be undefined if not used.
 @PARAM $vocabMapFileTitles the vocabulary V1-to-V2 mapping model for titles.
    It should have the format required by SRILM "disambig -map XXX".
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
 @PARAM $useSRILM if true, SRILM disambig will be used rather than canoe.
 @PARAM $useViterbi if true and using SRILM, use viterbi rather than forward-backward.

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
   my($inputFile, $LMFileTitles, $vocabMapFileTitles, $forceNISTTitlesFUFlag, $lmOrder, $useLMOnlyFlag, $outputFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi) = @_;

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

      truecaseFile($inTmpFile, $LMFileTitles, $vocabMapFileTitles, $lmOrder, $useLMOnlyFlag, $outTmpFile, $uppercaseSentenceBeginFlag, $cleanMarkupFlag, $verbose, $useSRILM, $useViterbi);
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





#============ prepareFileResolvingUnknowWordsToClassesWithVocabFile ==============#

sub prepareFileResolvingUnknowWordsToClassesWithVocabFile
{  if(not @_)  # if no argument is sent in
   {  die "\r\n\r\n!!! ERROR: portage_truecaselib::prepareFileResolvingUnknowWordsToClassesWithVocabFile requires 3 input arguments! \taborting...\r\n\r\n";
   }elsif($_[0] and $_[0] =~ /^portage_truecaselib/) # if we are called as a module then skip the our reference name
   {  shift;
      if(not @_)  # if no argument is sent in
      {  die "\r\n\r\n!!! ERROR: portage_truecaselib::prepareFileResolvingUnknowWordsToClassesWithVocabFile requires 3 input arguments! \taborting...\r\n\r\n";
      } # End if
   } # End if

   my($inputFile, $preparedFile, $vocabFile) = @_;

   my %knownWordsListHash = extractWordsListHashFrom($vocabFile);
   return prepareFileResolvingUnknowWordsToClasses($inputFile, $preparedFile, \%knownWordsListHash);

} # End prepareFileResolvingUnknowWordsToClassesWithVocabFile



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

 Copyright (c) 2004, 2005, Sa Majeste la Reine du Chef du Canada /
 Copyright (c) 2004, 2005, Her Majesty in Right of Canada

 This software is distributed to the GALE project participants under the terms
 and conditions specified in GALE project agreements, and remains the sole
 property of the National Research Council of Canada.

 For further information, please contact :
 Technologies langagieres interactives / Interactive Language Technologies
 Institut de technologie de l'information / Institute for Information Technology
 Conseil national de recherches Canada / National Research Council Canada
 See http://iit-iti.nrc-cnrc.gc.ca/locations-bureaux/gatineau_e.html

=back

=cut

1;  # End of module
