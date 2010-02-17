/**
 * @author Aaron Tikuisis
 * @file canoe_help.h  Contains the help message for canoe.
 *
 * $Id$ *
 *
 * Canoe Decoder
 *
 * Technologies langagieres interactives / Interactive Language Technologies
 * Inst. de technologie de l'information / Institute for Information Technology
 * Conseil national de recherches Canada / National Research Council Canada
 * Copyright 2004, Sa Majeste la Reine du Chef du Canada /
 * Copyright 2004, Her Majesty in Right of Canada
 */

/// Help message for canoe program.
static const char *HELP =
"Usage: %s -f CONFIG [options] < marked_up_src\n\
\n\
Translate text, reading from standard input and writing to standard output.\n\
Translation is controlled by the options listed below, which may be specified\n\
in the CONFIG file or as command-line switches (the latter take precedence).\n\
\n\
The CONFIG file contains a space-separated list of options, with each option\n\
followed by an argument. Options are surrounded by square brackets:\n\
'[option] arg' in CONFIG is the same as '-option arg' on the command line.\n\
Arguments in CONFIG may be zero or more strings separated by whitespace or by\n\
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
     target phrases and the probabilities should be p(target|source).\n\
     Used for translation table pruning if enabled, and as a feature if\n\
     -weight-f is specified.  If this option is used, the number of forward\n\
     translation model files must match the number of backward translation\n\
     model files.\n\
\n\
 -ttable-multi-prob FILE1[:FILE2[:..]]\n\
     The phrase translation model file(s) in multi-prob text format.  A\n\
     multi-prob table combines the information of multiple forward/backward\n\
     ttable pairs, replacing one or more -ttable-file-s2t/-t2s arguments\n\
     pairs.  Each table must contain an even number of probability columns,\n\
     separated by spaces, starting with all the backward probabilities,\n\
     followed, in the same order, by all the forward probabilities.\n\
\n\
     For example, if you combine a phrase probability estimate P_1 and a\n\
     lexical probability estimate P_2 and other estimates up to P_n into a\n\
     multi prob phrase table, each line would look like:\n\
     s ||| t ||| P_1(s|t) P_2(s|t) .. P_n(s|t) P_1(t|s) P_2(t|s) .. P_n(t|s)\n\
     where s is the source phrase and t is the target phrase.\n\
\n\
     A multi-prob phrase table containing 2*N prob columns requires N\n\
     backward weigths and, if they are supplied, N forward weights.\n\
\n\
     A multi-prob phrase table may also contain a \"4th column\" with\n\
     adirectional features, whose weights are supplied via -atm.  Assuming\n\
     directional models P_1 and P_2 and adirectional score A_1 .. A_m,\n\
     the syntax is:\n\
     s ||| t ||| P_1(s|t) P_2(s|t) P_1(t|s) P_2(t|s) ||| A_1(s,t) .. A_m(s,t)\n\
\n\
 -ttable-tppt FILE1[FILE2[:..]]\n\
     Phrase translation model file(s) in TPPT format (Tightly Packed Phrase\n\
     Table), indexed on the source language, containing an even number of\n\
     models, considered to be backward models followed by the same number of\n\
     forward models.\n\
     Like a multi-prob phrase table, a TPPT contining 2*N probs requires N\n\
     backward weights and, if they are supplied, N forward weights.\n\
\n\
 Phrase table notes:\n\
     At least one translation model must be specified (through the\n\
     -ttable-tppt or -ttable-file* options).\n\
     The number of translation models must match the number of translation\n\
     model weights.\n\
\n\
 -lmodel-file FILE1[:FILE2[:..]]\n\
     The language model file(s).  At least one file must be specified\n\
     (either in the CONFIG file or on the command line).  The number of\n\
     language model files specified must match the number of language\n\
     model weights.  Note that the file list is delimited by ':'.\n\
     The order of each language model will be determined by inspection,\n\
     but you can restrict the order of a model by adding #N to its name.\n\
     E.g., if 4g.lm is a 4-gram model, specifying 4g.lm#3 will treat it as\n\
     a 3-gram model.\n\
\n\
     Warning: all LM formats specify the use of base-10 log probs, but canoe\n\
     interprets them as natural log.  This known bug has minimal impact;\n\
     correcting it requires multiplying the desired -weight-l values by\n\
     log(10), which cow/rat/rescore_train do implicitly.  We chose not to\n\
     fix it to avoid having to adjust all previously tuned sets of weights.\n\
     Note that throughout Portage, logs are natural by default, not base 10.\n\
\n\
 -lmodel-order LMORDER\n\
     If non-zero, globally limits the order of all language models. [0]\n\
\n\
 -weight-t W1[:W2[:..]]\n\
 -tm W1[:W2[:..]]\n\
     The translation model weight(s).  If this option is used, the number of\n\
     weights must match the number of translation models specified.  [1.0]\n\
\n\
     If a mix of text, multi-prob and TPPT translation models are\n\
     specified, the weights of the single-prob text translation models come\n\
     first, then the weights of the multi-prob models, finally the weights of\n\
     the TPPT models.\n\
\n\
 -weight-f W1[:W2[:..]]\n\
 -ftm W1[:W2[:..]]\n\
     The weight(s) for the forward probabilities in the translation models.\n\
     If this option is used, it has the same requirements as -weight-t.\n\
     [none]\n\
\n\
 -weight-a W1[:W2[:..]]\n\
 -atm W1[:W2[:..]]\n\
     The weight(s) for the adirectional scores in the translation models.\n\
     If this option is used, it must supply the same number of weights as\n\
     there are adirectional features in all the translation models.\n\
     [none]\n\
\n\
 -weight-l W1[:W2[:..]]\n\
 -lm W1[:W2[:..]]\n\
     The language model weight(s).  If this option is used, the number of\n\
     weights must match the number of language models specified.  [1.0]\n\
     Note: see warning under -lmodel-file for details on the numerical\n\
     interpretation of this parameter.\n\
\n\
 -weight-lev W\n\
 -lev W\n\
     Weight for Levenshtein distance when the reference is used in canoe.\n\
\n\
 -weight-ngrams W1[:W2[:..]]\n\
 -ng W1[:W2[:..]]\n\
     Weight for n-gram precision when the reference is used in canoe.\n\
\n\
 -weight-d W1[:W2[:..]]\n\
 -d W1[:W2[:..]]\n\
     The distortion model weight(s).  [1.0]\n\
\n\
 -weight-w W\n\
 -w W\n\
     The sentence length weight.  [0.0]\n\
\n\
 -weight-s W\n\
 -sm W\n\
     The segmentation model weight.  [none]\n\
\n\
 -weight-ibm1-fwd W\n\
 -ibm1f W\n\
     The forward IBM1 feature weight (see -ibm1-fwd-file). [none]\n\
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
     The number of target phrases to keep in translation table pruning; 0\n\
     means no limit.  If used, forward probs should be provided (via the\n\
     -ttable-* options); ttable pruning is arbitrary otherwise. [0]\n\
\n\
 -ttable-threshold T\n\
     The translation table pruning threshold; 0.0 means no threshold.  If\n\
     non-0, target phrases with a score (defined via -ttable-prune-type\n\
     below) less than log(T) are discarded.  Also has arbitrary effects if\n\
     forward probs are not available.\n\
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
 -levenshtein-limit levL\n\
     The maximum levenshtien distance between current translation, or -1 for\n\
     no limit. The system will return an error if no translation can be found\n\
     that respects levL. [-1]\n\
\n\
 -distortion-limit L\n\
     The maximum distortion distance between two source words, or -1 for\n\
     no limit.  Use 0 to do monotonic decoding.  [-1]\n\
     See -dist-limit-ext for further details on the semantics of L.\n\
\n\
 -dist-limit-ext\n\
     Use the 'Extended' distortion limit definition.\n\
\n\
     Suppose the previous source phrase spans [a,b), the current source phrase\n\
     spans [c,d), and the first non-covered source position is NC1.\n\
\n\
     By default, the distortion limit is respected iff\n\
        |c-b| <= L AND d <= NC1 + L\n\
     i.e., if the new jump respects the distortion limit and the new phrase\n\
     doesn't *end* past NC1 + L.\n\
\n\
     The extended distortion limit changes the second condition to require that\n\
     the new phrase doesn't *begin* past NC1 + L:  With -dist-limit-ext, the\n\
     distortion limit is respected iff\n\
        |c-b| <= L AND c <= NC1 + L\n\
\n\
     In both cases, dead-end hypotheses (partial hypotheses that can't be\n\
     completed without eventually violating the distortion limit) are pruned\n\
     as soon as they can be detected.\n\
\n\
 -dist-phrase-swap\n\
     Allow swapping two contiguous source phrases of any length. [don't]\n\
     Applied as an OR with the distortion limit:  meaningless if L = -1,\n\
     yields quasi-monotonic decoding if L = 0, and a targetted relaxing of\n\
     the distortion limit rule if L > 0.  Orthogonal with -dist-limit-ext.\n\
\n\
 -distortion-model model[#args][:model2[#args][:..]]\n\
     The distortion model(s) and their arguments. Zero or more of:\n\
     WordDisplacement, PhraseDisplacement,\n\
     fwd-lex[#dir], back-lex[#dir], ZeroInfo.\n\
     To get no distortion model, use 'none'.  [WordDisplacement]\n\
\n\
     The lexicalized distortion models (LDM), fwd-lex and back-lex, take an\n\
     optional direction argument, which can be m (monotone), s (swap) or\n\
     d(discontinuous).\n\
     To combine the distortion penalty with a 2-feature LDM, use:\n\
        WordDisplacement:back-lex:fwd-lex\n\
     To combine the distortion penalty with a 6-feature LDM, use:\n\
        WordDisplacement:back-lex#m:back-lex#s:back-lex#d:fwd-lex#m:fwd-lex#s:fwd-lex#d\n\
\n\
 -lex-dist-model-file FILE1[:FILE2[:..]]\n\
     The lexicalized distortion model file(s) in multi-prob text format.\n\
     The parameter FILE is formatted like a phrase table, but with different\n\
     semantics:\n\
     src phrase ||| tgt phrase ||| pm ps pd nm ns nd\n\
     Where p=prev, n=next and m=monotonic, s=swap and d=discontinuous.\n\
\n\
 -segmentation-model model[#args]\n\
     The segmentation model: one of none, count, bernoulli. [none]\n\
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
        pass - pass them through to target translation;\n\
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
 -ref   FILE\n\
     The reference target sentences file.\n\
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
 -cube-pruning\n\
     Run the cube pruning decoder, a la Huang+Chiang (ACL 2007), instead of\n\
     the regular stack decoder.  (Not compatible yet with coverage pruning).\n\
     Note that with regular decoding, each stack gets up to S elements, not\n\
     counting recombined states, whereas with cube pruning recombined states\n\
     are counted.  Recomemded S values are therefore around 10000-30000 with\n\
     cube pruning, rather than 100-300.\n\
\n\
 -future-score-lm-heuristic FUT-LM-HEURISTIC\n\
     Specify the LM heuristic to use for the future score.  One of:\n\
        none - h = 1.0;\n\
        unigram - h = unigram probabilities;\n\
        simple - h = 1.0 if context is partial, h = prob otherwise;\n\
        incremental - h = unigram for first word, bigram for next, etc.;\n\
     [incremental]\n\
\n\
 -future-score-use-ftm\n\
     Also use forward translation model probabilities to compute the\n\
     future costs during decoding.  [do] (use -no-future-score-use-ftm to\n\
     turn this behaviour off).\n\
\n\
 -cube-lm-heuristic CUBE-LM-HEURISTIC\n\
     Specify the LM heuristic to use for cube pruning.  Same choices as\n\
     above.  Affects the order in which the cube pruning decoder considers\n\
     candidate phrases, whereas FUT-LM-HEURISTIC is used to calculate the\n\
     global future score [incremental]\n\
\n\
 -rule-classes class1[:class2[:..]]\n\
 -ruc class1[:class2[:..]]\n\
     Lists allowed rule classes in source text.\n\
\n\
 -rule-weights W1[:W2[:..]]\n\
 -ruw W1[:W2[:..]]\n\
     Weights associated with each rule classes.\n\
\n\
 -rule-log-zero E1[:E2[:..]]\n\
 -rulz E1[:E2[:..]]\n\
     Log value of epsilon for each rule classes.  These values are used\n\
     when a rule knows about the source phrase but the target phrase is not\n\
     the one specified in the rule.\n\
\n\
 -final-cleanup\n\
     Indicates to clear the bmg when canoe is done [false].\n\
     For speed, we normally don't delete the bmg at the end of canoe, but\n\
     for some debugging deleting the bmg might be appropriate.\n\
\n\
 -bind PID\n\
     Binds this instance of canoe to the existence of PID running: when PID\n\
     disappears, canoe will exit automatically with exit status 45.\n\
\n\
 -verbose V\n\
 -v V\n\
     The verbosity level (1, 2, 3, or 4).  All verbose output is written to\n\
     standard error.\n\
\n\
 -options\n\
     Produce a shorter help message only listing the option names\n\
\n\
 NOTES:\n\
\n\
   - All options of the form weight-X where X is a decoder feature can be\n\
     replaced by random-X with a value of U(min, max) or N(mean, sigma).  This\n\
     allows canoe to use Uniform or Normal distribution to determine the\n\
     weights of the decoder feature for each source sentence.  Intended for use\n\
     with -random-weights.  If no distribution is specified, U(-1.0, 1.0) is\n\
     used as a default value.  Example:\n\
        [random-t] U(0,3):N(2,0.1):...\n\
\n\
   - All boolean options (i.e., all the switches that don't take an argument)\n\
     can be reversed: -no-OPT reverses -OPT.  Options set in the CONFIG file\n\
     can be overridden on the command line with this syntax.\n\
\n\
   - If you have a hard time reading this help message in your terminal, try\n\
     bash: canoe -h 2>&1 | less\n\
     tcsh: canoe -h |& less\n\
";
