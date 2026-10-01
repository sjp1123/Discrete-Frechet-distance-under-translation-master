#!/usr/bin/env python3
"""Make a scratch copy of paper_bench.cpp that also prints the translation (getTranslation) of the
LMF result as two extra CSV columns tx,ty.  Build it like paper_bench (CMakeLists.txt of paper_bench).
    python3 patch_pb_translation.py <paper_bench.cpp in> <out.cpp>"""
import sys
s = open(sys.argv[1]).read()
h = 'csv << "file1,file2,n1,n2,value,time_ms,bbcalls,n6_arr_ms,n6_fre_ms,pre2_ms,bb2_ms,disc2_ms,arr2_ms\\n";'
assert s.count(h) == 1
s = s.replace(h, h.replace('arr2_ms\\n', 'arr2_ms,tx,ty\\n'))
old = '<< timerMs(EXP::FUT_DISCSELECTION2) << "," << timerMs(EXP::FUT_ARRANGEMENT2) << "\\n";'
assert s.count(old) == 1
s = s.replace(old, '<< timerMs(EXP::FUT_DISCSELECTION2) << "," << timerMs(EXP::FUT_ARRANGEMENT2) << "," '
                   '<< frechet.getTranslation().x << "," << frechet.getTranslation().y << "\\n";')
open(sys.argv[2], "w").write(s)
