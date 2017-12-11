#!/usr/bin/env python

# ~/sandboxes/PORTAGEshared/src/nn/test_portage_nnjm  -native -s 2 -n 2 delme.bin <(echo '1 1 / 1 / 4')

from unpickle import Layer
from unpickle import Embed
from unpickle import writeModelToFile
import numpy as np
from numpy.random import random
from numpy import ones
from numpy import zeros

embedding_size = 3
source_voc_size = 5
target_voc_size = 4
layer_size = 6
output_size = 5
semb  = Embed(source_voc_size, embedding_size, 2, 1*ones((source_voc_size, embedding_size)))
temb  = Embed(target_voc_size, embedding_size, 1, 2*ones((target_voc_size, embedding_size)))
layer = Layer(3*ones((9, layer_size)), zeros((layer_size, 1)), 'Elemwise{tanh,no_inplace}.0')
out   = Layer(4*ones((layer_size, output_size)), zeros((output_size, 1)), "none")



model_name = 'nnjm'
with open(model_name + '.txt', 'w') as f:
   writeModelToFile(f, True, semb, temb, [layer], out.w.shape[1], out)

with open(model_name + '.bin', 'w') as f:
   writeModelToFile(f, False, semb, temb, [layer], out.w.shape[1], out)
