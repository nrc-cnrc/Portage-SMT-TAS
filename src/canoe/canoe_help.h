/**
 * @author Aaron Tikuisis
 * @file canoe_help.h  Contains the help message for canoe.
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
 -ttable-multi-prob FILE1[:FILE2[:..]]  Multi-prob phrase table(s)\n\
     The phrase translation model file(s) in multi-prob text format.  A\n\
     multi-prob table has the source phrase in the first column, then the\n\
     target phrase, then some backward probabilities, followed by the same\n\
     number of forward probabilities.\n\
\n\
     For example, if you combine a phrase probability estimate P_1 and a\n\
     lexical probability estimate P_2 and other estimates up to P_n into a\n\
     multi-prob phrase table, each line would look like:\n\
     s ||| t ||| P_1(s|t) P_2(s|t) .. P_n(s|t) P_1(t|s) P_2(t|s) .. P_n(t|s)\n\
     where s is the source phrase and t is the target phrase.\n\
\n\
     A multi-prob phrase table containing 2*N prob columns requires N\n\
     backward weights and, if they are supplied, N forward weights.\n\
\n\
     A multi-prob phrase table may also contain a \"4th column\" with\n\
     adirectional features, whose weights are supplied via -atm.  Assuming\n\
     directional models P_1 and P_2 and adirectional score A_1 .. A_m,\n\
     the syntax is:\n\
     s ||| t ||| P_1(s|t) P_2(s|t) P_1(t|s) P_2(t|s) ||| A_1(s,t) .. A_m(s,t)\n\
\n\
     A multi-prob phrase table may also contain alignment information as the\n\
     last field of the 3rd column.  The format is\n\
        a=<al_1>[:<count_1>]{;<al_i>[:<count_i>]}\n\
     where <al_i> is in \"green\" format with underscores instead of spaces,\n\
     and the optional <count_i> is a float or int indicating the frequency of\n\
     <al_i>.  This information is used to calculate the unal features (-see\n\
     -unal-feature for details).  Currently, if multiple alignments are shown,\n\
     only the most frequent one is used.\n\
\n\
     One or more counts in float or int format can also be stored at the end of\n\
     the 3rd column, in the format c=<count>{,<count>}. The interpretation of\n\
     these counts is left to individual features.  Intended for future use:\n\
     no feature uses these yet.\n\
     If the same phrase pair occurs in multiple phrase tables, any count\n\
     vectors are added element-wise, eg [1] + [1,2,3] = [2,2,3].\n\
\n\
     All combinations of presence/absence of forward and backward probs (3rd\n\
     column), alignment, count (end of 3rd column) or adirectional scores (4th\n\
     column) are allowed.\n\
\n\
 -ttable-tppt FILE1[:FILE2[:..]]        Tightly Packed phrase table(s)\n\
     Phrase translation model file(s) in TPPT format (Tightly Packed Phrase\n\
     Table), indexed on the source language, containing an even number of\n\
     models, considered to be backward models followed by the same number of\n\
     forward models.\n\
     Like a multi-prob phrase table, a TPPT contining 2*N probs requires N\n\
     backward weights and, if they are supplied, N forward weights.\n\
\n\
 Phrase table notes:\n\
     At least one translation model must be specified (through -ttable-tppt or\n\
     -ttable-multi-prob).  The number of translation models must match the\n\
     number of translation model weights.\n\
\n\
 -use-ftm                               Enable forward TMs  [don't, unless -ftm is used]\n\
     By default, forward translation model are only used as decoder features\n\
     when given weights via -weight-t|-ftm.  With -use-ftm, they are always\n\
     used, with a default weight of 1.0 each if no weights are provided.\n\
\n\
 -lmodel-file FILE1[:FILE2[:..]]        Language model(s)  (required)\n\
     At least one LM is required.  The number of LM files must match the number\n\
     of LM weights.  The order of each LM will be determined by inspection, but\n\
     you can restrict the order of an LM by appending #N to its name.  E.g.,\n\
     4g.lm#3 will be interpreted as a 3-gram model even if it is a 4-gram LM.\n\
\n\
     Warning: all LM formats specify the use of base-10 log probs, but canoe\n\
     interprets them as natural log.  This known bug has minimal impact;\n\
     correcting it requires multiplying the desired -weight-l values by\n\
     log(10), which MERT corrects for implicitly.  We chose not to fix this bug\n\
     to avoid having to adjust all previously tuned sets of weights.  Note that\n\
     throughout PortageII, logs are natural by default, not base 10.\n\
\n\
 -lmodel-order LMORDER                  Global LM order limit  [0, i.e., none]\n\
     If non-zero, globally limits the order of all language models.\n\
\n\
 -weight-t|-tm W1[:W2[:..]]             Backward TM weight(s)  [1.0 for each feature]\n\
     The translation model weight(s).  If this option is used, the number of\n\
     weights must match the number of translation models specified.\n\
\n\
     If multi-prob and TPPT translation models are both specified, the weights\n\
     of the multi-prob models come first, then the weights of the TPPT models.\n\
\n\
 -weight-f|-ftm W1[:W2[:..]]            Forward TM weight(s)  [none]\n\
     The weight(s) for the forward probabilities in the translation models.\n\
     If this option is used, it has the same requirements and ordering rules as\n\
     -weight-t.  If neither this option nor -use-ftm are used, forward TM\n\
     scores are not used, except possibly for pruning.  With -use-ftm, this\n\
     option defaults to 1.0 for each feature.\n\
\n\
 -weight-a|-atm W1[:W2[:..]]            Adirectional TM weight(s)  [1.0 for each adir feat.]\n\
     The weight(s) for the adirectional scores in the translation models.\n\
     If this option is used, it must supply the same number of weights as\n\
     there are adirectional features in all the translation models.\n\
\n\
 -weight-l|-lm W1[:W2[:..]]             Language model weight(s)  [1.0 for each LM]\n\
     If this option is used, the number of weights must match the number of\n\
     language models specified.  Note: see warning under -lmodel-file for\n\
     details on the numerical interpretation of this parameter.\n\
\n\
 -weight-lev|-lev W                     Levenshtein distance weight [none]\n\
     Weight for Levenshtein distance w.r.t. the reference, which is required.\n\
\n\
 -weight-ngrams|-ng W1[:W2[:..]]        Ngram match weight(s)  [none]\n\
     Weight for n-gram precision w.r.t. the reference, which is required.\n\
     The first weight is for unigrams, the second for bigrams, etc.\n\
\n\
 -weight-d|-d W1[:W2[:..]]              Distortion model weight(s)  [1.0 for each feature]\n\
\n\
 -weight-w|-w W                         Sentence length weight  [0.0]\n\
\n\
 -weight-s|-sm W                        Segmentation model weight  [1.0 for each feature]\n\
\n\
 -weight-unal|-unal W1[:W2[:..]]        Unal feature weight(s)  [1.0 for each feature]\n\
\n\
 -weight-ibm1-fwd|-ibm1f W              Forward IBM1 feature weight  [1.0 for each feature]\n\
     The forward IBM1 feature weight (see -ibm1-fwd-file).\n\
\n\
 -weight-bilm W1[:W2[:..]]              Weights for BiLM models  [1.0 for each feature]\n\
\n\
 -random-weights|-r                     Set weights randomly per sent  [don't]\n\
     Ignore given weights, setting weights randomly for each sentence.\n\
     By default, the uniform distribution between -1 and +1 will be used for\n\
     each feature.  To choose your distributions by feature, all options of the\n\
     form weight-X where X is a decoder feature can be replaced by random-X\n\
     with values of U(min, max) (Uniform) or N(mean, sigma) (Normal).  Example:\n\
        [random-t] U(0,3):N(2,0.1):...\n\
\n\
 -seed N                                Seed for -random-weights  [0]\n\
     Set (positive integer) random seed (see -random-weights).\n\
\n\
 -stack|-s S                            Cube pruning hypothesis stack size  [10000]\n\
     The maximum number of hypotheses to keep in each cube-pruning decoder\n\
     stack, i.e., covering the same number of source words.  With the cube\n\
     pruning decoder, recombined states are counted, so a much larger value\n\
     of S is recommended than RS (see below).  Note: it is an error to specify\n\
     -stack without -cube-pruning.\n\
\n\
 -regular-stack|-rs RS                  Regular decoder hypothesis stack size  [1000]\n\
     The maximum number of hypotheses to keep in each regular decoder stack,\n\
     i.e., covering the same number of source words.  With the default decoder,\n\
     recombined states are not counted, so a much smaller value of RS is\n\
     recommended than S (see above).  Note: it is an error to specify\n\
     -regular-stack with -cube-pruning.\n\
\n\
 -beam-threshold|-b T                   Beam threshold (per stack)  [0.0001]\n\
     The hypothesis stack relative threshold: max ratio of probability between\n\
     the highest and lowest score for hypotheses kept in one stack.\n\
\n\
 -cov-limit COV_L                       Coverage pruning limit  [0, i.e., none]\n\
     The coverage pruning limit: max number of hypotheses to keep with\n\
     identical coverage (use 0 for no limit; example value: 10)\n\
     (Not compatible with cube pruning)\n\
\n\
 -cov-threshold COV_T                   Coverage pr. threshold  [0.0, i.e., none]\n\
     The coverage pruning threshold: max ratio of probability between highest\n\
     and lowest score for hypotheses with identical coverage (use 0.0 for no\n\
     threshold; example value: 0.01)\n\
     (Not compatible with cube pruning)\n\
\n\
 -diversity MIN_DIVERSITY               Min hypotheses to keep per coverage  [0]\n\
     Keep at least MIN_DIVERSITY hypotheses for each coverage in the stack.\n\
     (Not compatible with cube pruning)\n\
\n\
 -diversity-stack-increment DSI         Max hyps added to each stack by the -diversity option [RS]\n\
     Notwithstanding MIN_DIVERSITY, don't keep more than RS+DSI hypotheses in\n\
     any given stack, so the search does not become exponential.  (Use 0 for no\n\
     limit, -1 for DSI=RS, or a positive value to set DSI explicitly.)\n\
\n\
 -ttable-limit L                        Max candidates per source phrase  [0, i.e., no limit]\n\
     The number of target phrases to keep in translation table pruning; 0\n\
     means no limit.  The top L target phrases are kept for each source phrase,\n\
     according to the log-linear criterion specified with -ttable-prune-type.\n\
     Forward TM scores should be provided (via the -ttable-* options), or\n\
     phrase table pruning will be arbitrary.\n\
\n\
 -ttable-threshold T                    Phrase table pruning threshold  [0, i.e., none]\n\
     The translation table pruning threshold; 0.0 means no threshold.  If\n\
     non-0, target phrases with a log-linear pruning score (defined via\n\
     -ttable-prune-type below) less than log(T) are discarded.\n\
\n\
 -ttable-prune-type PRUNE_TYPE          Phrase table pruning criterion\n\
     Semantics of the L and T parameters above.  Pruning is done using:\n\
     'backward-weights': forward probs (if available) with backward weights\n\
     'forward-weights': forward probs with forward weights\n\
     'combined': the combination of forward and backward probs with their\n\
     respective weights.\n\
     [forward-weights if -weight-f is supplied, backward-weights otherwise]\n\
\n\
 -ttable-log-zero LZ                    Log(epsilon)  [-18]\n\
     The log probability assigned to phrase pairs that either don't occur\n\
     within some phrasetable, or that have 0 probability.\n\
\n\
 -levenshtein-limit levL                Levenshtein limit w.r.t. ref  [-1, i.e., no limit]\n\
     The maximum levenshtein distance between the current translation and the\n\
     reference, expressed as a percentage of the source sentence length.\n\
     (Use -1 for no limit.)\n\
\n\
 -forced                                Use forced decoding with ref  [don't]\n\
     Use forced decoding to find a phrase alignment between sentence pairs in\n\
     line-aligned source and reference files.  Requires -ref.  (Use -lev or -ng\n\
     to perform fuzzy phrase alignment.)  Causes the -lm, -w and -ibm1f weights\n\
     to be set to 0.\n\
\n\
 -forced-nz                             Forced dec. using all features  [don't]\n\
     Same as -forced, but does not zero the -lm, -w and -ibm1f weights.\n\
\n\
 -maxlen MAXLEN                         Skip long sentences  [0, i.e., no maxlen]\n\
     If non 0, skip sentences longer than MAXLEN.\n\
\n\
 -distortion-limit L                    Max jump between source words  [-1, i.e., no limit]\n\
     The max distortion distance between two source words; -1 means no limit; 0\n\
     means monotonic decoding.  See -dist-limit-ext for the semantics of L.\n\
\n\
 -dist-limit-ext                        Use 'Extended' distortion limit defn  [don't]\n\
     Suppose the previous source phrase spans [a,b), the current source phrase\n\
     spans [c,d), and the first uncovered source position, after adding [c,d),\n\
     is u.\n\
\n\
     By default, the distortion limit L is respected iff\n\
        |c-b| <= L AND d <= u + L\n\
     i.e., if the new jump respects the distortion limit L and the new phrase\n\
     doesn't *end* past u + L.\n\
\n\
     The Extended distortion limit relaxes the second condition to require only\n\
     that the new phrase doesn't *begin* past u + L:  With -dist-limit-ext, the\n\
     distortion limit is respected iff\n\
        |c-b| <= L AND c <= u + L\n\
\n\
     In both cases, dead-end hypotheses (partial hypotheses that can't be\n\
     completed without eventually violating the distortion limit) are pruned\n\
     as soon as they can be detected.\n\
\n\
 -dist-limit-simple                     Use the 'Simple' distortion limit defn  [don't]\n\
     Suppose the first uncovered source position, *before* adding the new\n\
     phrase [c,d), is v.  Under the Simple definition, the distortion limit L\n\
     is respected iff c <= v + L.  That means a new phrase is allowed as long\n\
     as it starts no more than L words after the first uncovered word.  Notice\n\
     that the jump length relative to the previous source phrase is not\n\
     limited.  Less restrictive than Extended, Simple never generates dead-end\n\
     hypotheses.\n\
     Only one of -dist-limit-simple and -dist-limit-ext may be specified.\n\
\n\
 -dist-limit-itg                        Enforce ITG re-ordering limits [don't]\n\
     Switches on the shift-reduce parser and enforces a constraint such that\n\
     each next phrase must be reachable from the top of the stack traveling\n\
     only over uncovered words. Eliminates 2413 by blocking 2,4->1: 4 is top\n\
     of stack, 1 is next, 2 is between 1 and 4, hence 1 is blocked.\n\
\n\
     Enforced in addition to (anded with) the other dist-limits\n\
\n\
 -shift-reduce-only-itg                 Shift-reduce will [not] use only 2-reductions\n\
     The canoe shift-reduce parser can handle arbitrarily complex re-orderings.\n\
     In the worst case scenario, all n words are reduced with a single n-reduction.\n\
     Only attempting 2-reductions means linear, rather than quadratic time, and makes\n\
     a lot of sense if we are also enforcing -dist-limit-itg.\n\
     This flag is used without -dist-limit-itg mostly to replicate historical results.\n\
\n\
 -dist-phrase-swap                      Allow phrase swaps  [don't]\n\
     Allow swapping two contiguous source phrases of any length.\n\
     Applied as an OR with the distortion limit:  meaningless if L = -1,\n\
     yields quasi-monotonic decoding if L = 0, and a targetted relaxing of\n\
     the distortion limit rule if L > 0.  Orthogonal with -dist-limit-ext.\n\
\n\
     Can also be combined with -dist-limit-simple, but the implementation is\n\
     buggy: Suppose you have L=7, u=v=0 and you add phrase spanning [10,20),\n\
     which is allowed with -dist-phrase-swap as long as there exists a phrase\n\
     spanning [0,10).  With the conservative or extended distortion limit, only\n\
     [0,10) can be added next, completing the phrase swap, but under simple\n\
     distortion any new phrase that respects c <= v + L will also be allowed.\n\
\n\
 -distortion-model MODEL[#ARGS][:MODEL2[#ARGS][:..]]  Dist. model(s)  [WordDisplacement]\n\
     The distortion model(s) and their arguments. Zero or more of:\n\
     WordDisplacement, LeftDistance, PhraseDisplacement, fwd-lex[#dir],\n\
     back-lex[#dir], fwd-hlex[#dir], back-hlex[#dir], back-fhlex[#dir],\n\
     ZeroInfo.  To get no distortion model, use 'none'.\n\
\n\
     The lexicalized distortion models (LDM), fwd-*lex and back-*lex, take\n\
     an optional direction argument, which can be m (monotone), s (swap) or\n\
     d (discontinuous). The hlex models are hierarchical LDMs, and back-hlex\n\
     triggers the use of the shift-reduce parser. back-fhlex is an experimental,\n\
     parser-free version of back-hlex.\n\
     To combine the distortion penalty with a 2-feature LDM, use:\n\
        WordDisplacement:back-lex:fwd-lex\n\
     To combine the distortion penalty with a 6-feature LDM, use:\n\
        WordDisplacement:back-lex#m:back-lex#s:back-lex#d:fwd-lex#m:fwd-lex#s:fwd-lex#d\n\
     To combine the distortion penalty with a 2-feature HLDM, use:\n\
        WordDisplacement:back-hlex:fwd-hlex\n\
     To combine the distortion penalty with a 6-feature HLDM, use:\n\
        WordDisplacement:back-hlex#m:back-hlex#s:back-hlex#d:fwd-hlex#m:fwd-hlex#s:fwd-hlex#d\n\
     To combine distortion penalty with both types of LDM use:\n\
        WordDisplacement:\n\
        back-lex#m#L:back-lex#s#L:back-lex#d#L:fwd-lex#m#L:fwd-lex#s#L:fwd-lex#d#L\n\
        back-hlex#m#H:back-hlex#s#H:back-hlex#d#H:fwd-hlex#m#H:fwd-hlex#s#H:fwd-hlex#d#H\n\
     Note that this final instance tags each LDM feature with either #L or #H.\n\
     These tags tell the feature what file to use to retrieve probabilities.\n\
     They must match tags annotating filenames in -lex-dist-model-file\n\
\n\
 -lex-dist-model-file FILELIST             Lexicalized distortion model file(s)  [none]\n\
     A list of lexicalized distortion model files in multi-prob text format.\n\
     FILE is formatted like a phrase table, but with different semantics:\n\
     src phrase ||| tgt phrase ||| pm ps pd nm ns nd\n\
     Where p=prev, n=next and m=monotonic, s=swap and d=discontinuous.\n\
     If you include multiple files, each file must be #tagged (for example,\n\
     with #L or #H) and features must exist in -distortion-model that reference\n\
     each tag.  For example: dist_file_ldm.gz#L:dist_file_hdm.gz#H\n\
\n\
 -segmentation-model MODEL[#ARGS]       Segmentation model  [none]\n\
     The segmentation model: one of none, count, bernoulli.\n\
     Some models require an argument, introduced by '#':\n\
     - bernoulli requires a numerical argument (Q parameter)\n\
\n\
 -unal-feature FEAT1[:FEAT2[:..]]       Unaligned word feature(s)  [none]\n\
     The unal features(s), each in the form UNAL1[+UNAL2[+..]], where each\n\
     UNAL is in [src|tgt]{right|left|edges|any}. Fields are:\n\
        right/left = count unaligned words on right/left edge of phrase pair\n\
        edges = count unaligned words on both edges of phrase pair\n\
        any = count unaligned words anywhere in phrase pair\n\
        src/tgt = count over source/target phrase only (default is both)\n\
     Eg: srcleft+tgtany = count unaligned words on left side of source phrase,\n\
     and anywhere in target phrase.\n\
\n\
 -ibm1-fwd-file FILE                    Forward IBM1 feature file  [none]\n\
     Use 'forward' IBM1 feature - FILE should be an IBM1 model trained for\n\
     target language given source language. [none]\n\
\n\
 -bilm-file FILE1[:FILE2[:..]]          BiLM files - bilingual language models  [none]\n\
     Bilingual language model file, following Niehues et al, WMT-2011.\n\
     Must be an LM trained on the output of align-words -o bilm, with the\n\
     source language as its lang2 and the target language as its lang1.\n\
\n\
 -bypass-marked                         Allow bypassing marked translations  [don't]\n\
     When marked translations are found in the source text, translation\n\
     options from the translation table are not excluded.\n\
\n\
 -weight-marked W                       Weight for marked translations  [1.0]\n\
     A weight to multiply marked translation probabilites by.\n\
\n\
 -oov METHOD                            How to handle OOVs  [pass]\n\
     How to handle out-of-vocabulary source words. METHOD is one of:\n\
        pass - pass them through to target translation;\n\
        write-src-marked - DON'T TRANSLATE, just write the source text\n\
           with OOVs marked like this: <oov>the-oov</oov>\n\
        write-src-deleted - DON'T TRANSLATE, just write the source text\n\
           with OOVs stripped out.\n\
\n\
 -tolerate-markup-errors                Warn on markup errors  [abort on markup errors]\n\
     When invalid markup is found, attempt to interpret it as literal text.\n\
     Minimal effort is used: parts of the invalid input will typically be lost.\n\
\n\
 -check-input-only                      Validate input markup - no translation\n\
     Just check the input for markup errors, don't read any models or decode\n\
     anything.  With this option, a non-zero exit status indicates fatal\n\
     format errors were found.\n\
\n\
 -backwards                             Translate from end to start\n\
     Forms the translation from end to start instead of start to end.\n\
     The language model should be trained on a backwards corpus.\n\
\n\
 -load-first                            Load models before input; no filtering\n\
     Loads the models before reading the source sentences, so that\n\
     translations are produced on the fly.  If this is not used, source\n\
     sentences are read first and only applicable entries are stored from\n\
     the phrase table and language model files.\n\
\n\
 -palign|-trace|-t                      Output alignment and OOV info  [don't]\n\
     Produce alignment and OOV output. If -lattice is given, this info\n\
     will also be stored in the lattice. If -nbest is given, alignment\n\
     info (but not OOV) will be written along with the nbest list.\n\
\n\
 -walign                                Output word alignment, if available  [don't]\n\
     Requires -palign to have any effect - adds the word alignment information\n\
     to the -palign output in canoe's 1-best and lattice output.\n\
\n\
 -ffvals                                Output feature function values  [don't]\n\
     Produce feature function output. If -lattice or -nbest is given,\n\
     this info will also be written to the lattice or nbest list.\n\
\n\
 -lattice LPREFIX[.gz]                  Output lattices  [don't]\n\
     Produces word graph output into files LPREFIX.SENTNUM[.gz], where SENTNUM\n\
     is a 4+ digit representation of the sentence number, starting at 0000 (or\n\
     the value of -first-sentnum).  State coverage vectors are output into\n\
     LPREFIX.SENTNUM.state[.gz].  If -trace or -ffvals is specified then this\n\
     form of output is used in the wordgraph as well.  Even if -backwards is\n\
     specified, the word graph gives forwards sentences.  If .gz is specified,\n\
     the output will be gzipped.\n\
\n\
 -nbest NPREFIX[.gz][:N]                Output N-best lists  [don't]\n\
     Produces nbest output into files NPREFIX.SENTNUM.Nbest[.gz]. If N is\n\
     not specified, 100 is used. If -ffvals is also specified, feature\n\
     function values are written to NPREFIX.SENTNUM.Nbest.ffvals[.gz].\n\
     With -trace, alignment info is written to NPREFIX.SENTNUM.Nbest.pal[.gz].\n\
     If .gz is specified, the outputs will be gzipped.\n\
\n\
 -first-sentnum INDEX                   First external sentence ID  [0000]\n\
     Indicates the first SENTNUM to use in creating the file names for\n\
     the -lattice and -nbest output.  Used by canoe-parallel.sh.\n\
\n\
 -input FILE                            Input file  [-, i.e., STDIN]\n\
     The source sentences file.  [-]\n\
\n\
 -ref   FILE                            Reference translation file  [none]\n\
     The reference target sentences file.\n\
\n\
 -append                                Don't split output files by sentence\n\
     The decoder will output its results in a single file instead of one\n\
     file per source sentences.  This will automatically be applied to\n\
     nbest, ffvals, pal, lattice and lattice_state.\n\
\n\
 -lb                                    Use load-balancing mode  [don't]\n\
     Indicates that canoe is running in load-balancing mode: source sentences\n\
     are preceded by an external id and a tab.  Used by canoe-parallel.sh\n\
\n\
 -cube-pruning                          Use cube pruning  [don't]\n\
     Use the cube pruning decoder, a la Huang+Chiang (ACL 2007), instead of\n\
     the regular stack decoder.  (Not compatible yet with coverage pruning).\n\
     Note that with regular decoding, each stack gets up to RS elements, not\n\
     counting recombined states, whereas with cube pruning recombined states\n\
     are counted.  Recomended S values are therefore around 10000-30000 with\n\
     cube pruning, rather than 100-1000 for RS.\n\
\n\
 -future-score-use-ftm                  Include forward TMs in future score  [do]\n\
     Also use forward translation model probabilities to compute the\n\
     future costs during decoding.  [do] (use -no-future-score-use-ftm to\n\
     turn this behaviour off).\n\
\n\
 -future-score-lm-heuristic HEURISTIC   LM heuristic for future score  [incremental]\n\
     Specify the LM heuristic to use for the future score.  One of:\n\
        none - h = 1.0;\n\
        unigram - h = unigram probabilities;\n\
        simple - h = 1.0 if context is partial, h = prob otherwise;\n\
        incremental - h = unigram for first word, bigram for next, etc.;\n\
\n\
 -cube-lm-heuristic CUBE-LM-HEURISTIC   LM heuristic for cube pruning  [incremental]\n\
     Specify the LM heuristic to use for cube pruning.  Same choices as\n\
     above.  Affects the order in which the cube pruning decoder considers\n\
     candidate phrases, whereas HEURISTIC (see above) is used to calculate the\n\
     global future score\n\
\n\
 -rule-classes|-ruc class1[:class2[:..]]  List of rule classes  [none]\n\
     Lists allowed rule classes in source text.\n\
\n\
 -rule-weights|-ruw W1[:W2[:..]]        Weight for each rule class  [1.0 for each class]\n\
\n\
 -rule-log-zero|-rulz E1[:E2[:..]]      Log(epsilon) for each rule class [LZ for each class]\n\
     Log value of epsilon for each rule class.  These values are used when a\n\
     rule knows about the source phrase but the target phrase is not the one\n\
     specified in the rule.  [LZ, if -ttable-log-zero is given, or -18]\n\
\n\
 -final-cleanup                         Delete models before exiting  [don't]\n\
     Indicates to clear the bmg when canoe is done [false].\n\
     For speed, we normally don't delete the bmg at the end of canoe, but\n\
     for some debugging deleting the bmg might be appropriate.\n\
\n\
 -bind PID                              Exit when process PID stops running\n\
     Binds this instance of canoe to the existence of PID running: when PID\n\
     disappears, canoe will exit automatically with exit status 45.\n\
\n\
 -verbose|-v V                          Verbosity level  [1]\n\
     The verbosity level (1 to 4).  Verbose output is written to std error.\n\
\n\
 -timing                                Show per-sentence timing  [don't unless V >= 2]\n\
\n\
 -options                               Show the brief help message\n\
     Produce a shorter help message with one line per option\n\
\n\
 NOTES:\n\
\n\
   - All boolean options (i.e., all the switches that don't take an argument)\n\
     can be reversed: -no-OPT reverses -OPT.  Options set in the CONFIG file\n\
     can be overridden on the command line with this syntax.\n\
\n\
   - If you have a hard time reading this help message in your terminal, try\n\
     bash: canoe -h 2>&1 | less\n\
     tcsh: canoe -h |& less\n\
";
