#!/usr/bin/env python3
# Scratch instrumentation of candidate5 / candidate7 copies (not the repo): split black-box calls by phase.
# counters: 0 base-case calls (whole block), 1 gate lessThan(max) calls, 2 base cases, 3 N6 lessThan probes,
#           4 union second-pass (box family) calls, 5 calcDistance initial evaluation calls, 6 gates YES,
#           7 union second passes
import os
I = os.environ.get("INST", os.path.expanduser("~/inst"))


def patch_fut(path):
    s = open(path).read()
    a = s.index("distance_t FrechetUnderTranslation::calcDistance2")
    b = s.index("\n}\n", a)
    body = s[a:b]
    old_gate = "if (n6_alg.lessThan(max, curve1, curve2)) {"
    assert body.count(old_gate) == 2, body.count(old_gate)
    body = body.replace(old_gate, "long long _b0 = INST_BB(); inst_cnt()[2]++; bool _g = n6_alg.lessThan(max, curve1, curve2); "
                        "inst_cnt()[1] += INST_BB() - _b0;\n\t\t\t\tif (_g) { inst_cnt()[6]++;")
    old_stop = "MEASUREMENT::stop(EXP::FUT_ARRANGEMENT2);\n\t\t\t\tcontinue;"
    assert body.count(old_stop) == 2, body.count(old_stop)
    body = body.replace(old_stop, "inst_cnt()[0] += INST_BB() - _b0;\n\t\t\t\t" + old_stop)
    s = '#include "inst_counters.h"\n' + s[:a] + body + s[b:]
    open(path, "w").write(s)


def patch_n6(path, c7):
    s = open(path).read()
    sig = "bool N6Alg::lessThan(distance_t distance, Curve const& curve1, Curve const& curve2)\n{"
    assert s.count(sig) == 1
    s = s.replace(sig, sig + "\n\tinst_cnt()[3]++;")
    a = s.index("distance_t N6Alg::calcDistance(Curve const& curve1, Curve const& curve2)\n")
    ev = "distance_t max = frechet.evaluationFixedTranslation(curve1, curve2);"
    k = s.index(ev, a)
    s = s[:k] + "long long _bi = INST_BB(); " + ev + " inst_cnt()[5] += INST_BB() - _bi;" + s[k + len(ev):]
    if c7:
        t = "\t\tr = test(tr, nullptr);"
        k = s.index(t)
        assert s.rfind("if (fix == 5)", 0, k) > s.rfind("if (fix == 1)", 0, k)
        s = s[:k] + "\t\t{ long long _bx = INST_BB(); inst_cnt()[7]++; r = test(tr, nullptr); inst_cnt()[4] += INST_BB() - _bx; }" + s[k + len(t):]
    s = '#include "inst_counters.h"\n' + s
    open(path, "w").write(s)


for arm, c7 in (("candidate5", False), ("candidate7", True)):
    patch_fut(f"{I}/{arm}/src/frechet_under_translation.cpp")
    patch_n6(f"{I}/{arm}/src/fut_n6_algorithm.cpp", c7)

p = f"{I}/pb/paper_bench.cpp"
s = open(p).read()
s = s.replace('#include <vector>', '#include <vector>\n#include "inst_counters.h"', 1)
h_old = 'csv << "file1,file2,n1,n2,value,time_ms,bbcalls,n6_arr_ms,n6_fre_ms,pre2_ms,bb2_ms,disc2_ms,arr2_ms\\n";'
assert s.count(h_old) == 1
s = s.replace(h_old, h_old.replace("arr2_ms\\n", "arr2_ms,base_calls,gate_calls,bases,probes,boxfam_calls,init_eval_calls,gate_yes,boxfam_passes\\n"))
old = '<< timerMs(EXP::FUT_DISCSELECTION2) << "," << timerMs(EXP::FUT_ARRANGEMENT2) << "\\n";'
assert s.count(old) == 1
s = s.replace(old, '<< timerMs(EXP::FUT_DISCSELECTION2) << "," << timerMs(EXP::FUT_ARRANGEMENT2);\n'
              '\t\t\tfor (int i : {0,1,2,3,4,5,6,7}) csv << "," << inst_cnt()[i];\n\t\t\tcsv << "\\n";')
old2 = "MEASUREMENT::reset();\n\t\t\tauto start = hrc::now();\n\t\t\tFrechetUnderTranslation frechet;\n\t\t\tauto val = frechet.calcDistance2(c1, c2);"
assert s.count(old2) == 1
s = s.replace(old2, old2.replace("MEASUREMENT::reset();", "MEASUREMENT::reset(); inst_reset();"))
open(p, "w").write(s)
print("patched")
