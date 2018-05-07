if ( ! $?THEANO_FLAGS && ! -r ~/.theanorc ) then
   # If nvidia-smi works, we assume there is a GPU and enable NNJM training on
   # it, otherwise we fall-back on CPU.
   nvidia-smi >& /dev/null && \
      setenv THEANO_FLAGS "device=cuda,floatX=float32,mode=FAST_RUN" || \
      setenv THEANO_FLAGS "device=cpu,floatX=float32,mode=FAST_RUN"
endif


# Manual overrides. Uncommenting either of these lines will override any values
# set in your ~/.theanorc file and erase any previously defined THEANO_FLAGS
# value.

# Force training NNJMs on GPU:
#setenv THEANO_FLAGS "device=cuda,floatX=float32,mode=FAST_RUN"

# Force training NNJMs on CPU (much slower, use only if you don't have a GPU):
#setenv THEANO_FLAGS "device=cpu,floatX=float32,mode=FAST_RUN"
