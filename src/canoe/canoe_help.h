/**
 * @author Aaron Tikuisis
 * @file canoe_help.h  Contains the help message for canoe.
 *
 * $Id$ *
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Institut de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

/// Help message for canoe program.
static const char *HELP =
"Usage: %s -f config [options] < marked_up_src\n\
\n\
Translate text, reading from standard input and writing to standard output.\n\
Translation is controlled by the options listed below, which may be specified\n\
in the file <config> or as command-line switches (the latter take precedence).\n\
\n\
The <config> file contains a space-separated list of options, with each option\n\
followed by an argument. Options are surrounded by square brackets:\n\
'[option] arg' in <config> is the same as '-option arg' on the command line.\n\
Arguments in <config> may be zero or more strings separated by whitespace or by\n\
colons. The special sequence '--' is used to indicate an empty string. Any part\n\
of a line that begins with a hash mark (#) is interpreted as a comment.\n\
\n\
The input source text must be in marked-up format: \\, < and > are special\n\
characters and must be escaped with \\ to be interpreted literally.  See the\n\
user manual for a full description of the markup language.\n\
\n\
Options (in command-line format):\n\
\n\
 -ttable-file-s2t FILE1[:FILE2[:..]]\n\
 -ttable-file FILE1[:FILE2[:..]]\n\
 -ttable-file-f2n FILE1[:FILE2[:..]]\n\
        The phrase translation model file(s) in text format containing backward\n\
        probabilities.  That is, the left column of each table should contain\n\
        source phrases and the probabilities should be p(source|target).\n\
\n\
 -ttable-file-t2s FILE1[:FILE2[:..]]\n\
 -ttable-file-n2f FILE1[:FILE2[:..]]\n\
        The phrase translation model file(s) in text format containing forward\n\
        probabilities.  That is, the left column of each table should contain\n\
        target phrases and the probabilities should be p(target|source).  This\n\
        file is used only for translation table pruning, and it is highly\n\
        recommended if ttable-pruning is used.  If this option is used, the\n\
        number of forward translation model files must match the number of\n\
        backward translation model files.\n\
\n\
 -ttable-multi-prob FILE1[:FILE2[:..]]\n\
        The phrase translation model file(s) in multi-prob text format.  A\n\
        multi-prob table combined the information of multiple forward/backward\n\
        pairs.  Each table must contain an even number of probability columns,\n\
        separated by spaces, starting with all the backward probabilities,\n\
        followed, in the same order, by all the forward probabilities.\n\
\n\
        For example, if you combine a phrase probability estimate P_p and a\n\
        lexical probability estimate P_l into a multi prob phrase table, each\n\
        line would look like:\n\
        src ||| tgt ||| P_p(src|tgt) P_l(src|tgt) P_p(tgt|src) P_l(tgt|src)\n\
\n\
        A multi-prob phrase table containing 2*N prob columns requires N\n\
        backward weigths and, if they are supplied, N forward weights.\n\
\n\
        At least one translation model is required, either using this option or\n\
        using the -ttable-file option pair.\n\
\n\
 -lmodel-file FILE1[:FILE2[:..]]\n\
        The language model file(s).  At least one file must be specified\n\
        (either in the configuration file or on the command line).  The number\n\
        of language model files specified must match the number of language\n\
        model weights.  Note that the file list is delimited by ':'.\n\
        The order of each language model will be determined by inspection,\n\
        but you can restrict the order of a model by adding #N to its name.\n\
        E.g., if 4g.lm is a 4-gram model, specifying 4g.lm#3 will treat it as\n\
        a 3-gram model.\n\
\n\
 -lmodel-order LMORDER\n\
        If non-zero, globally limits the order of all language models. [0]\n\
\n\
 -weight-t W1[:W2[:..]]\n\
 -tm W1[:W2[:..]]\n\
        The translation model weight(s).  If this option is used, the number of\n\
        weights must match the number of translation models specified.  [1.0]\n\
\n\
        If a mix of text and multi-prob translation models are specified, the\n\
        weights of the single-prob text translation models must come first,\n\
        then the weights of the multi-prob models.\n\
\n\
 -weight-f W1[:W2[:..]]\n\
 -ftm W1[:W2[:..]]\n\
        The weight(s) for the forward probabilities in the translation models.\n\
        If this option is used, it has the same requirements as -weight-t.\n\
        [0.0]\n\
\n\
 -weight-l W1[:W2[:..]]\n\
 -lm W1[:W2[:..]]\n\
        The language model weight(s).  If this option is used, the number of\n\
        weights must match the number of language models specified.  [1.0]\n\
\n\
 -weight-d W\n\
 -d W\n\
        The distortion probability weight.  [1.0]\n\
\n\
 -weight-w W\n\
 -w W\n\
        The sentence length weight.  [0.0]\n\
\n\
 -weight-s W\n\
 -sm W\n\
        The segmentation model weight.  [0.0]\n\
\n\
 -weight-ibm1-fwd W\n\
 -ibm1f W\n\
        The forward IBM1 feature weight (see -ibm1-fwd-file). [0.0]\n\
\n\
 -random-weights\n\
 -r\n\
        Ignore given weights, set weights randomly for each sentence instead\n\
        [don't]\n\
\n\
 -seed N\n\
        Set (positive integer) random seed (see -random-weights).  [0]\n\
\n\
 -stack S\n\
 -s S\n\
        The hypothesis stack size.  [100]\n\
\n\
 -beam-threshold T\n\
 -b T\n\
        The hypothesis stack relative threshold [0.0001]\n\
\n\
 -cov-limit COV_L\n\
        The coverage pruning limit (max number of states to keep with identical\n\
        coverage) (use 0 for no limit) (example value: 10) [0, i.e., no limit]\n\
\n\
 -cov-threshold COV_T\n\
        The coverage pruning threshold (max ratio of probability between top\n\
        and lowest state with identical coverage) (use 0.0 for no threshold)\n\
        (example value: 0.1) [0.0, i.e., no threshold]\n\
\n\
 -ttable-limit L\n\
        The translation table limit, or 0 for no limit.  If used, it is\n\
        recommended that a ttable-file-t2s is specified, or quality may\n\
        suffer severely.  [0]\n\
\n\
 -ttable-threshold T\n\
        The translation table pruning threshold.  [0]\n\
\n\
 -ttable-prune-type PRUNE_TYPE\n\
        Semantics of the L and T parameters above.  Pruning is done using:\n\
        'backward-weights': forward probs (if available) with backward weights\n\
        'forward-weights': forward probs with forward weights\n\
        'combined': the combination of forward and backward probs with their\n\
        respective weights.\n\
        [forward-weights if -weight-f is supplied, backward-weights otherwise]\n\
\n\
 -ttable-log-zero LZ\n\
        The log probability assigned to phrase pairs that either don't occur\n\
        within some phrasetable, or that have 0 probability. [-18]\n\
\n\
 -distortion-limit L\n\
        The maximum distortion distance between two source words, or -1 for\n\
        no limit.  Use 0 to do monotonic decoding.  [-1]\n\
\n\
 -dist-limit-ext\n\
        'Extended' distortion limit:  By default, a new source phrase can't end\n\
        more than L positions past the first non-covered word (1NCW) remaining\n\
        after its addition.  With -dist-limit-ext, a source phrase can end past\n\
        1NCW + L if it starts at or before 1NCW + L and a sequence of jumps\n\
        each <= L can be used to finish covering the source sentence. [don't]\n\
\n\
 -dist-phrase-swap\n\
        Allow swapping two contiguous source phrases of any length. [don't]\n\
        Applied as an OR with the distortion limit:  meaningless if L = -1,\n\
        yields quasi-monotonic decoding if L = 0, and a targetted relaxing of\n\
        the distortion limit rule if L > 0.  Orthogonal with -dist-limit-ext.\n\
\n\
 -distortion-model model[:model2[:..]]\n\
        The distortion model(s). Zero or more of of:\n\
        WordDisplacement, PhraseDisplacement, ZeroInfo.\n\
        To get no distortion model, use 'none'.  [WordDisplacement]\n\
\n\
 -segmentation-model model[#args]\n\
        The segmentaion model: one of none, count, bernoulli. [none]\n\
        Some models require an argument, introduced by '#':\n\
        - bernoulli requires a numerical argument (Q parameter)\n\
\n\
 -ibm1-fwd-file file\n\
        Use 'forward' IBM1 feature - file should be an IBM1 model trained for\n\
        target language given source language. [none]\n\
\n\
 -bypass-marked\n\
        When marked translations are found in the source text, translation\n\
        options from the translation table are not excluded.\n\
\n\
 -weight-marked W\n\
        A weight to multiply marked translation probabilites by.  [1.0]\n\
\n\
 -oov method\n\
        How to handle out-of-vocabulary source words. Method is one of:\n\
           pass - pass them through to target translation; \n\
           write-src-marked - DON'T TRANSLATE, just write the source text\n\
              with OOVs marked like this: <oov>the-oov</oov>\n\
           write-src-deleted - DON'T TRANSLATE, just write the source text\n\
              with OOVs stripped out.\n\
        [pass]\n\
\n\
 -tolerate-markup-errors\n\
        When invalid markup is found, attempt to interpret it as literal text.\n\
        This does a minimal effort only, so parts of the invalid input will\n\
        typically be lost.  [abort on invalid markup]\n\
\n\
 -check-input-only\n\
        Just check the input for markup errors, don't read any models or decode\n\
        anything.  With this option, a non-zero exit status indicates fatal\n\
        format errors were found.\n\
\n\
 -backwards\n\
        Forms the translation from end to start instead of start to end.\n\
        The language model should be trained on a backwards corpus.\n\
\n\
 -load-first\n\
        Loads the models before reading the source sentences, so that\n\
        translations are produced on the fly.  If this is not used, source\n\
        sentences are read first and only applicable entries are stored from\n\
        the phrase table and language model files.\n\
\n\
 -palign\n\
 -trace\n\
 -t\n\
        Produce alignment and OOV output. If -lattice is given, this info\n\
        will also be stored in the lattice. If -nbest is given, alignment\n\
        info (but not OOV) will be written along with the nbest list.\n\
\n\
 -ffvals\n\
        Produce feature function output. If -lattice or -nbest is given,\n\
        this info will also be written to the lattice or nbest list.\n\
\n\
 -lattice LPREFIX[.gz]\n\
        Produces word graph output into files LPREFIX.SENTNUM[.gz], where\n\
        SENTNUM is a 4+ digit representation of the sentence number, starting\n\
        at 0000 (or the value of -first-sentnum).  State coverage vectors are\n\
        output into LPREFIX.SENTNUM.state[.gz].  If -trace or -ffvals is\n\
        specified then this form of output is used in the wordgraph as well.\n\
        Even if -backwards is specified, the word graph gives forwards\n\
        sentences.  If .gz is specified, the output will be gzipped.\n\
\n\
 -nbest NPREFIX[.gz][:N]\n\
        Produces nbest output into files NPREFIX.SENTNUM.Nbest[.gz]. If N is\n\
        not specified, 100 is used. If -ffvals is also specified, feature\n\
        function values are written to NPREFIX.SENTNUM.Nbest.ffvals[.gz].  With\n\
        -trace, alignment info is written to NPREFIX.SENTNUM.Nbest.pal[.gz].\n\
        If .gz is specified, the outputs will be gzipped.\n\
\n\
 -first-sentnum INDEX\n\
        Indicates the first SENTNUM to use in creating the file names for\n\
        the -lattice and -nbest output.  [0000]\n\
\n\
 -input FILE\n\
        The source sentences file.  [-]\n\
\n\
 -append\n\
        The decoder will output its results in a single file instead of one\n\
        file per source sentences.  This will automatically be applied to\n\
        nbest, ffvals, pal, lattice and lattice_state\n\
\n\
 -lb\n\
        Indicates that canoe is running in load-balancing mode thus the source\n\
        sentences will be prepended with a source sentence id.\n\
\n\
 -future-score-lm-heuristic FUT-LM-HEURISTIC\n\
        Specify the LM heuristic to use for the future score.  One of:\n\
           none - h = 1.0;\n\
           unigram - h = unigram probabilities;\n\
           simple - h = 1.0 if context is partial, h = prob otherwise;\n\
           incremental - h = unigram for first word, bigram for next, etc.;\n\
        [incremental]\n\
\n\
 -verbose V\n\
 -v V\n\
        The verbosity level (1, 2, 3, or 4).  All verbose output is written to\n\
        standard error.\n\
\n\
 -options\n\
        Produce a shorter help message only listing the option names\n\
";
