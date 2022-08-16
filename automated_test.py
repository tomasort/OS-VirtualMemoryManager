#!/usr/local/bin/python3
# -*- coding: utf-8 -*-
# Create a module

import random
import string
import os
import time

# print('Starting testing')
# for i in range(1, 11):
#     for frames in range(1, 128+1):
#         for alg in ['a', 'f', 'r', 'c', 'e', 'w']:
#             os.system('./mmu lab3_assign/inputs/in' + str(i) + ' -f' + str(frames) + ' -oOFPS -a' + str(alg) + ' lab3_assign/inputs/rfile  > out_file_mine')
#             os.system('/home/frankeh/Public/mmu lab3_assign/inputs/in' + str(i) + ' -f' + str(frames) + ' -oOFPS -a' + str(alg) + ' lab3_assign/inputs/rfile > out_file_his')
#             f1 = open('out_file_mine', 'r')
#             f2 = open('out_file_his', 'r')
#             s1 = f1.read()
#             s2 = f2.read()
#             print("Verify input in" + str(i)) 
#             # time.sleep(50)
#             assert(s1 == s2)
#             print("File " + str(i) + f" with alg {alg} and {frames} frames "  + "Completed")
#             f1.close()
#             f2.close()
print('Starting random testing')
for i in range(100):
    procs = random.randint(1, 9)
    os.system("/home/frankeh/Public/mmu_generator " + "-P" + str(procs) + " -V" + str(random.randint(1, 8)) + " -i" + str(random.randint(1, 20_000)) + " -E" + str(random.randint(0,procs)) + " -m" + " -w" + " -H" + " -r" + str(random.randint(0, 100)) + " -L" + str(random.uniform(1.0, 5.0)) + " > demo.txt")
    for alg in ['a', 'f', 'r', 'c', 'e', 'w']:
        frames = random.randint(1, 128)
        print("Verify " + str(frames) + " with alg " + str(alg)) 
        print("Running his")
        os.system('/home/frankeh/Public/mmu ' + 'demo.txt' + ' -f' + str(frames) + ' -oOFPS -a' + str(alg) + ' lab3_assign/inputs/rfile > out_file_his')
        print("Running mine")
        os.system('./mmu demo.txt' + ' -f' + str(frames) + ' -oOFPS -a' + str(alg) + ' lab3_assign/inputs/rfile  > out_file_mine')
        f1 = open('out_file_mine', 'r')
        f2 = open('out_file_his', 'r')
        s1 = f1.read()
        s2 = f2.read()
        assert s1 == s2
        print("File " + str(frames) + " with alg " + str(alg) +  " Completed")
        # print("mine: \n", f1.read())
        # print("\n\n\n")
        # print("his: \n", f2.read())
        f1.close()
        f2.close()
