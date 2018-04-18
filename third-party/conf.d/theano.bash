if [[ ! $THEANO_FLAGS && ! -r ~/.theanorc ]]; then
   if nvidia-smi >& /dev/null; then
      # There appears to be a GPU, use it
      export THEANO_FLAGS="device=cuda,floatX=float32,mode=FAST_RUN"
   else
      # No GPU, use the CPU to train NNJMs
      export THEANO_FLAGS="device=cpu,floatX=float32,mode=FAST_RUN"
   fi
fi


# Manual overrides. Uncommenting either of these lines will override any values
# set in your ~/.theanorc file and erase any previously defined THEANO_FLAGS
# value.

# Force training NNJMs on GPU:
#export THEANO_FLAGS="device=cuda,floatX=float32,mode=FAST_RUN"

# Force training NNJMs on CPU (much slower, use only if you don't have a GPU):
#export THEANO_FLAGS="device=cpu,floatX=float32,mode=FAST_RUN"
