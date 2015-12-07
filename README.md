# perceptron

1.  Copy the cpu/ directory to your workspace. In case things get wrong, you'd better move your directory to somewhere else first, and then move this one to the place. Don not use copy, cp can't do things recursively.

2.  Update the files in configs/ in correponding place(Simulation.py in common/ and run.py in spec2k6/). Do not move directory here because here are the only changed files. For other not changed ones, leave them there. However, you can mv files around in case bad things happen.

3.  Command line for running is:
  ./build/ALPHA_MESI_CMP_directory/gem5.opt -d gcc/ configs/spec2k6/run.py -b gcc --cpu-type=atomic -I 1000000000 --at-instruction --take-checkpoints=1000000000 --checkpoint-dir=checkpoint
  
  This will take the checkpoint at 1 billion insts by using atomic cpu, and save the checkpoint file in checkpoint/.
  
  ./build/ALPHA_MESI_CMP_directory/gem5.opt -d gcc/ configs/spec2k6/run.py -b gcc --cpu-type=detailed -I 100000000 --at-instruction -r 1000000000 -s 1 --checkpoint-dir=checkpoint --restore-with-cpu=atomic --caches --predtype=perceptron --historylen=60
  
  This will restore the checkpoint at 1 billion insts using checkpoint in checkpoint/ and run 100 million insts using o3 cpu with 60 bits perceptron branch predictor.
  
P.S. If you find the benchmark doesn't work, go for the email sent by TA and check details. Plus, better check the correct simpoints place with TA to make sure we are using the right one. I sent an email to TA yesterday but get no response.
